/*
  Copyright 2009-2014 Mark Johnson
  Copyright 2017 Mathieu Bernard

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

// pycfg runs a Pitman-Yor process for each nonterminal to estimate an
// Adaptor Grammar

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <time.h>
#include <unistd.h>
#include <vector>

#include <boost/program_options.hpp>

#include "logging.hh"
#include "pstream.h"
#include "pycky.h"
#include "sym.h"
#include "xtree.h"


static const std::string usage =
    "pycfg [-d debug]\n"
    "      [-A parses-file] [-C] [-D] [-E] [-G grammar-file]\n"
    "      [-H] [-I] [-P] [-R nr]\n"
    "      [-r rand-init] [-n niterations] [-N nanal-its]\n"
    "      [-a a] [-b b] [-w weight]\n"
    "      [-epya-beta-a] [-f pya-beta-b] [-g pyb-gamma-s] [-i pyb-gamma-c]\n"
    "      [-s train_frac] -S\n"
    "      [-T anneal-temp-start] [-t anneal-temp-stop] [-m anneal-its]\n"
    "      [-Z ztemp] [-z zits]\n"
    "      [-x eval-every] [-X eval-cmd] [-Y eval-cmd]\n"
    "      [-u test1.yld] [-U eval-cmd]\n"
    "      [-v test1.yld] [-V eval-cmd]\n"
    "      grammar.lt < train.yld\n"
    "\n"
    "The grammar consists of a sequence of rules, one per line, in the\n"
    "following format:\n"
    "\n"
    "   [theta [a [b]]] Parent --> Child1 Child2 ...\n"
    "\n"
    "where theta is the rule's probability (or, with the -E flag, the Dirichlet prior\n"
    "            parameter associated with this rule) in the generator, and\n"
    "      a, b (0<=a<=1, 0<b) are the parameters of the Pitman-Yor adaptor process.\n"
    "\n"
    "If a==1 then the Parent is not adapted.\n"
    "\n"
    "If a==0 then the Parent is sampled with a Chinese Restaurant process\n"
    "           (rather than the more general Pitman-Yor process).\n"
    "\n"
    "If theta==0 then we use the default value for the rule prior (given by the -w flag).\n"
    "\n"
    "The start category for the grammar is the Parent category of the\n"
    "first rule.\n"
    "\n"
    "If you specify the -C flag, these trees are printed in \"compact\" format,\n"
    "i.e., only cached categories are printed.\n"
    "\n"
    "If you don't specify the -C flag, cached nodes are suffixed by a \n"
    "'#' followed by a number, which is the number of customers at this\n"
    "table.\n"
    "\n"
    "The -A parses-file causes it to print out analyses of the training data\n"
    "for the last few iterations (the number of iterations is specified by the\n"
    "-N flag).\n"
    "\n"
    "The -X eval-cmd causes the program to run eval-cmd as a subprocess\n"
    "and pipe the current sample trees into it (this is useful for monitoring\n"
    "convergence).  Note that the eval-cmd is only run _once_; all the\n"
    "sampled parses of all the training data are piped into it.\n"
    "Trees belonging to different iterations are separated by blank lines.\n"
    "\n"
    "The -u and -v flags specify test-sets which are parsed using the current PCFG\n"
    "approximation every eval-every iterations, but they are not trained on.  These\n"
    "parses are piped into the commands specified by the -U and -V parameters respectively.\n"
    "Just as for the -X eval-cmd, these commands are only run _once_.\n"
    "\n"
    "The program can now estimate the Pitman-Yor hyperparameters a and b for each\n"
    "adapted nonterminal.  To specify a uniform Beta prior on the a parameter, set\n"
    "\n"
    "   -e 1 -f 1\n"
    "\n"
    "and to specify a vague Gamma prior on the b parameter, set\n"
    "\n"
    "   -g 10 -h 0.1\n"
    "or\n"
    "   -g 100 -h 0.01\n"
    "\n"
    "If you want to estimate the values for a and b hyperparameters, their\n"
    "initial values must be greater than zero.  The -a flag may be useful here.\n"
    "\n"
    "If a nonterminal has an a value of 1, this means that the nonterminal\n"
    "is not adapted.\n"
    "\n";


typedef pstream::ostream* Postreamp;
typedef std::vector<Postreamp> Postreamps;

int debug = 0;

struct S_F_incrementer {
    const F increment;
    S_F_incrementer(F increment) : increment(increment) { }

    template <typename arg_type>
    void operator() (const arg_type& arg, S_F& parent_weights) const
    {
        for (auto& w: parent_weights)
            w.second += increment;
    }
};


struct RandomNumberGenerator : public std::unary_function<unsigned,unsigned> {
    unsigned operator() (unsigned nmax) {
        return mt_genrand_int32() % nmax;
    }
};

F gibbs_estimate(pycfg_type& g, const std::vector<std::vector<symbol> >& trains,
		 F train_frac, bool train_frac_randomise,
		 Postreamps& evalcmds, unsigned eval_every,
		 unsigned niterations,
		 F anneal_start, F anneal_stop, unsigned anneal_its,
		 F z_temp, unsigned z_its,
		 bool hastings_correction, bool random_order,
		 bool delayed_initialization,
		 unsigned resample_pycache_nits,
		 unsigned nparses_iterations,
		 std::ostream* finalparses_stream_ptr,
		 std::ostream* grammar_stream_ptr,
		 const std::vector<std::vector<symbol> >& test1s, Postreamps& test1cmds,
		 const std::vector<std::vector<symbol> >& test2s, Postreamps& test2cmds,
		 Postreamps& grammarcmds) {

    typedef pycky::tree tree;
    typedef std::vector<tree*> tps_type;
    typedef std::vector<bool> Bs;
    typedef std::vector<unsigned> Us;

    unsigned n = trains.size();
    unsigned nn = unsigned(n * train_frac);
    unsigned nwords = 0;
    // F sum_log2prob = 0;
    tps_type tps(n, NULL);
    pycky p(g, anneal_start);

    if (g.pya_beta_a < -1 && g.pya_beta_b < 0)
        g.default_pya = 0.999;

    // decide which training sentences to use
    Bs train_flag(n, false);   // if true, train on this sentence
    if (train_frac == 1)
        for (unsigned i = 0; i < n; ++i)
            train_flag[i] = true;
    else {
        for (unsigned i = 0; i < nn; ++i)
            train_flag[i] = true;
        if (train_frac_randomise) {
            RandomNumberGenerator rng;
            std::random_shuffle(train_flag.begin(), train_flag.end(), rng);
        }
    }

    {
        Us try_later; // indices of sentences that are so long that the parser underflows

        // initialize tps with (random) trees

        for (unsigned i = 0; i < n; ++i) {
            if (!train_flag[i])
                continue;

            if (debug >= 1000)
                std::cerr << "# trains[" << i << "] = " << trains[i];

            nwords += trains[i].size();

            F tprob = p.inside(trains[i]);

            if (debug >= 1000)
                std::cerr << ", tprob = " << tprob;

            if (tprob <= 0) {
                if (debug >= 1000)
                    std::cerr << ", parse failed, will retry later " << std::endl;
                try_later.push_back(i);  // underflowed now; we'll try this later
                continue;
            }

            tps[i] = p.random_tree();

            if (debug >= 1000)
                std::cerr << ", tps[" << i << "] = " << tps[i] << std::endl;

            if (!delayed_initialization)
                g.incrtree(tps[i]);        // incremental initialisation
        }

        if (delayed_initialization)    // collect statistics from the random trees
            for (unsigned i = 0; i < n; ++i)
                if (tps[i] != NULL)
                    g.incrtree(tps[i]);

        cforeach (Us, it, try_later) {

            if (debug >= 1000)
                std::cerr << "# reparsing trains[" << *it << "] = " << trains[*it];

            F tprob = p.inside(trains[*it]);

            if (debug >= 1000)
                std::cerr << ", tprob = " << tprob;

            if (tprob <= 0)
                std::cerr << "\n## " << HERE << " Error in py-cfg::gibbs_estimate(), tprob = " << tprob
                          << ", trains[" << *it << "] = " << trains[*it] << " failed to parse." << std::endl
                          << exit_failure;

            tps[*it] = p.random_tree();

            if (debug >= 1000)
                std::cerr << ", tps[" << *it << "] = " << tps[*it] << std::endl;

            g.incrtree(tps[*it]);
        }
    }

    LOG(trace) << nwords << " tokens in " << nn << " sentences";
    LOG(trace) << "It\tTemp\tTime\t-logP\t-logPcorpus\t-logPrior\ttables\tsame\t"
        "changed\treject\tdefault_pya\t(parent pym pyn pya pyb)*";

    Us index(n);  // order in which parses should be resampled
    for (unsigned i = 0; i < n; ++i)
        index[i] = i;

    unsigned unchanged = 0, rejected = 0;

    for (unsigned iteration = 0; iteration < niterations; ++iteration) {

        if (random_order)
            std::random_shuffle(index.begin(), index.end());

        if (iteration + z_its > niterations)
            p.anneal = 1.0/z_temp;
        else if (iteration == 0 && anneal_its > 0)
            p.anneal = anneal_start;
        else if (iteration < anneal_its)
            p.anneal = anneal_start*power(anneal_stop/anneal_start,F(iteration)/F(anneal_its-1));
        else
            p.anneal = anneal_stop;

        assert(finite(p.anneal));

        if (debug >= 100) {
            std::cerr << "# Iteration " << iteration << ", "
                      << g.sum_pym() << " tables, "
                      << "-logPcorpus = " << -g.logPcorpus() << ", "
                      << "-logPrior = " << -g.logPrior() << ", "
                      << unchanged << '/' << n << " analyses unchanged";
            if (hastings_correction)
                std::cerr << ", " << rejected << '/' << n-unchanged
                          << " rejected";
            if (p.anneal != 1)
                std::cerr << ", temp = " << 1.0/p.anneal;
            std::cerr << '.' << std::endl;
        }

        if (logging::get_level() <= logging::severity::trace and iteration % eval_every == 0)
        {
            F nlogPcorpus = -g.logPcorpus();
            F nlogPrior = -g.logPrior();
            LOG(trace) << iteration << '\t'            // iteration
                       << 1.0/p.anneal << '\t'         // temperature
                       << runtime() << '\t'            // cpu time used
                       << nlogPcorpus+nlogPrior << '\t' // - log P(corpus,parameters)
                       << nlogPcorpus << '\t'          // -log P(corpus|parameters)
                       << nlogPrior << '\t'            // -log P(parameters)
                       << g.sum_pym() << '\t'          // # of tables
                       << unchanged << '\t'            // # unchanged parses
                       << n-unchanged << '\t'          // # changed parses
                       << rejected << '\t'             // # parses rejected
                       << g.default_pya;               // default pya parameter
            if (g.pyb_gamma_s > 0 && g.pyb_gamma_c > 0 && debug >= 10)
            {
                std::stringstream os;
                g.write_adaptor_parameters(os);
                LOG(trace) << os.str();
            }
        }

        if (iteration % eval_every == 0) {  // do we print a trace at this iteration?
            for (unsigned i = 0; i < n; ++i) {
                if (train_flag[i])
                    foreach (Postreamps, ecit, evalcmds) { // print this parse
                        pstream::ostream& ec = **ecit;
                        ec << tps[i] << std::endl;
                    }
                else {  // not trained on; sample a parse and print it
                    p.inside(trains[i]);
                    tree* tp = p.random_tree();
                    g.incrtree(tp, 1);
                    foreach (Postreamps, ecit, evalcmds) {
                        pstream::ostream& ec = **ecit;
                        ec << tp << std::endl;
                    }
                    g.decrtree(tp, 1);
                    tp->selective_delete();
                }
            }
            foreach (Postreamps, ecit, evalcmds) {  // print end of line
                pstream::ostream& ec = **ecit;
                ec << std::endl;
            }

            foreach (Postreamps, gcit, grammarcmds) { // trace the grammar counts at current stateq
                pstream::ostream& gc = **gcit;
                gc << g;
                gc << std::endl;
            }

            cforeach (std::vector<std::vector<symbol> >, it, test1s) {  // parse test1s
                p.inside(*it);
                tree* tp = p.random_tree();
                g.incrtree(tp, 1);
                foreach (Postreamps, tcit, test1cmds) {
                    pstream::ostream& tc = **tcit;
                    tc << tp << std::endl;
                }
                g.decrtree(tp, 1);
                tp->selective_delete();
            }
            foreach (Postreamps, tcit, test1cmds) {
                pstream::ostream& tc = **tcit;
                tc << std::endl;
            }

            cforeach (std::vector<std::vector<symbol> >, it, test2s) {  // parse test2s
                p.inside(*it);
                tree* tp = p.random_tree();
                g.incrtree(tp, 1);
                foreach (Postreamps, tcit, test2cmds) {
                    pstream::ostream& tc = **tcit;
                    tc << tp << std::endl;
                }
                g.decrtree(tp, 1);
                tp->selective_delete();
            }
            foreach (Postreamps, tcit, test2cmds) {
                pstream::ostream& tc = **tcit;
                tc << std::endl;
            }
        }  // end of trace

        if (debug >= 500)
            assert(g.sum_pym() == g.terms_pytrees_size());

        if (debug >= 10000)
            std::cerr << g;

        // sum_log2prob = 0;
        unchanged = 0;
        rejected = 0;

        for (unsigned i0 = 0; i0 < n; ++i0) {

            unsigned i = index[i0];

            if (!train_flag[i])  // skip this sentence if we don't train on it
                continue;

            if (debug >= 1000)
                std::cerr << "# trains[" << i << "] = " << trains[i];

            tree* tp0 = tps[i];                // get the old parse for sentence to resample
            assert(tp0);

            F pi0 = g.decrtree(tp0);           // remove the old parse's fragments from the CRPs
            if (pi0 <= 0)
                std::cerr << "## " << HERE
                          << " Underflow in gibbs_estimate() while computing pi0 = decrtree(tp0):"
                          << " pi0 = " << pi0
                          << ", iteration = " << iteration
                          << ", trains[" << i << "] = " << trains[i]
                    //      << std::endl << "## tp0 = " << tp0
                          << std::endl;

            F r0 = g.tree_prob(tp0);            // compute old tree's prob under proposal grammar
            if (r0 <= 0)
                std::cerr << "## " << HERE
                          << " Underflow in gibbs_estimate() while computing r0 = tree_prob(tp0):"
                          << " r0 = " << r0
                          << ", iteration = " << iteration
                          << ", trains[" << i << "] = " << trains[i]
                    //      << std::endl << "## tp0 = " << tp0
                          << std::endl;

            F tprob = p.inside(trains[i]);       // compute inside CKY table for proposal grammar
            if (tprob <= 0)
                std::cerr << "## " << HERE
                          << " Underflow in gibbs_estimate() while computing tprob = inside(trains[i]):"
                          << " tprob = " << tprob
                          << ", iteration = " << iteration
                          << ", trains[" << i << "] = " << trains[i]
                    //      << std::endl << "## g = " << g
                          << std::endl;
            assert(tprob > 0);

            if (debug >= 1000)
                std::cerr << ", tprob = " << tprob;
            // sum_log2prob += log2(tprob);

            tree* tp1 = p.random_tree();         // sample proposal parse from proposal grammar CKY table
            F r1 = g.tree_prob(tp1);
            // assert(r1 > 0);

            if (*tp0 == *tp1) {                  // don't do anything if proposal parse is same as old parse
                if (debug >= 1000)
                    std::cerr << ", tp0 == tp1" << std::flush;
                ++unchanged;
                g.incrtree(tp1, 1);
                tps[i] = tp1;
                tp0->selective_delete();
            }
            else {
                F pi1 = g.incrtree(tp1, 1);        // insert proposal parse into CRPs, compute proposal's true probability
                // assert(pi1 > 0);

                if (debug >= 1000)
                    std::cerr << ", r0 = " << r0 << ", pi0 = " << pi0
                              << ", r1 = " << r1 << ", pi1 = " << pi1 << std::flush;

                if (hastings_correction) {         // perform accept-reject step
                    F accept = (pi1 * r0) / (pi0 * r1); // acceptance probability
                    if (p.anneal != 1)
                        accept = power(accept, p.anneal);
                    if (!finite(accept))  // accept if there has been an underflow
                        accept = 2.0;
                    if (debug >= 1000)
                        std::cerr << ", accept = " << accept << std::flush;
                    if (random1() <= accept) {      // do we accept the proposal parse?
                        if (debug >= 1000)            //  yes
                            std::cerr << ", accepted" << std::flush;
                        tps[i] = tp1;                 //  insert proposal parse into set of parses
                        tp0->selective_delete();      //  release storage associated with old parse
                    }
                    else {                          // reject proposal parse
                        if (debug >= 1000)
                            std::cerr << ", rejected" << std::flush;
                        g.decrtree(tp1, 1);           // remove proposal parse from CRPs
                        g.incrtree(tp0, 1);           // reinsert old parse into CRPs
                        tp1->selective_delete();      // release storage associated with proposal parse
                        ++rejected;
                    }
                }
                else {                            // no hastings correction
                    tps[i] = tp1;                   // save proposal parse
                    tp0->selective_delete();        // delete old parse
                }
            }

            if (debug >= 1000)
                std::cerr << ", tps[" << i << "] = " << tps[i] << std::endl;
        }

        if (iteration < resample_pycache_nits)
            resample_pycache(g, p);

        if (iteration > 1 && g.pyb_gamma_s > 0 && g.pyb_gamma_c > 0) {
            if (g.pya_beta_a > 0 && g.pya_beta_b > 0)
                g.resample_pyab();
            else
                g.resample_pyb();
        }

        if (g.pya_beta_a < -1 && g.pya_beta_b < 0)
            g.default_pya = std::min(0.999, std::max(0.0, 1.0 - pow(iteration/(-g.pya_beta_a),-g.pya_beta_b)));

        if (finalparses_stream_ptr && iteration + nparses_iterations >= niterations) {
            for (unsigned i = 0; i < n; ++i)
                if (train_flag[i])
                    (*finalparses_stream_ptr) << tps[i] << std::endl;
                else {
                    p.inside(trains[i]);
                    tree* tp = p.random_tree();
                    g.incrtree(tp, 1);
                    (*finalparses_stream_ptr) << tp << std::endl;
                    g.decrtree(tp, 1);
                    tp->selective_delete();
                }
            (*finalparses_stream_ptr) << std::endl;
        }

    }

    if (logging::get_level() <= logging::severity::trace)
    {
        F nlogPcorpus = -g.logPcorpus();
        F nlogPrior = -g.logPrior();
        LOG(trace) << niterations << '\t'          // iteration
                   << 1.0/p.anneal << '\t'         // temperature
                   << runtime() << '\t'            // cpu time used
                   << nlogPcorpus+nlogPrior << '\t' // - log P(corpus,parameters)
                   << nlogPcorpus << '\t'          // -log P(corpus|parameters)
                   << nlogPrior << '\t'            // -log P(parameters)
                   << g.sum_pym() << '\t'          // # of tables
                   << unchanged << '\t'            // # unchanged parses
                   << nn-unchanged << '\t'         // # changed parses
                   << rejected << '\t'             // # parses rejected
                   << g.default_pya;               // default pya value
        if (g.pyb_gamma_s > 0 && g.pyb_gamma_c > 0 && debug >= 10)
        {
            std::stringstream os;
            g.write_adaptor_parameters(os);
            LOG(trace) << os.str();
        }
    }

    for (unsigned i = 0; i < n; ++i) {
        if (train_flag[i])
            foreach (Postreamps, ecit, evalcmds) {
                pstream::ostream& ec = **ecit;
                ec << tps[i] << std::endl;
            }
        else {
            p.inside(trains[i]);
            tree* tp = p.random_tree();
            g.incrtree(tp, 1);
            foreach (Postreamps, ecit, evalcmds) {
                pstream::ostream& ec = **ecit;
                ec << tp << std::endl;
            }
            g.decrtree(tp, 1);
            tp->selective_delete();
        }
    }
    foreach (Postreamps, ecit, evalcmds) {
        pstream::ostream& ec = **ecit;
        ec << std::endl;
    }

    cforeach (std::vector<std::vector<symbol> >, it, test1s) {  // final parse of test1s
        p.inside(*it);
        tree* tp = p.random_tree();
        g.incrtree(tp, 1);
        foreach (Postreamps, tcit, test1cmds) {
            pstream::ostream& tc = **tcit;
            tc << tp << std::endl;
        }
        g.decrtree(tp, 1);
        tp->selective_delete();
    }
    foreach (Postreamps, tcit, test1cmds) {
        pstream::ostream& tc = **tcit;
        tc << std::endl;
    }

    cforeach (std::vector<std::vector<symbol> >, it, test2s) {  // final parse for test2s
        p.inside(*it);
        tree* tp = p.random_tree();
        g.incrtree(tp, 1);
        foreach (Postreamps, tcit, test2cmds) {
            pstream::ostream& tc = **tcit;
            tc << tp << std::endl;
        }
        g.decrtree(tp, 1);
        tp->selective_delete();
    }
    foreach (Postreamps, tcit, test2cmds) {
        pstream::ostream& tc = **tcit;
        tc << std::endl;
    }

    F logPcorpus = g.logPcorpus();

    if (debug >= 10) {
        std::cerr << "# " << niterations << " iterations, "
                  << g.sum_pym() << " tables, "
                  << " log P(trees) = " << logPcorpus << ", "
                  << -logPcorpus/(log(2)*nwords+1e-100) << " bits/token, "
                  << unchanged << '/' << n << " unchanged";
        if (hastings_correction)
            std::cerr << ", " << rejected << '/' << n-unchanged
                      << " rejected";
        std::cerr << '.' << std::endl;
    }

    if (debug >= 10000)
        std::cerr << "# g.terms_pytrees = " << g.terms_pytrees << std::endl;

    if (grammar_stream_ptr)
        (*grammar_stream_ptr) << g;

    bool estimate_theta_flag = g.estimate_theta_flag;
    g.estimate_theta_flag = false;
    for (unsigned i = 0; i < n; ++i)
        if (train_flag[i]) {
            g.decrtree(tps[i], 1);
            tps[i]->selective_delete();
        }
    g.estimate_theta_flag = estimate_theta_flag;

    return logPcorpus;
}  // gibbs_estimate()



int main(int argc, char** argv)
{
    pycfg_type g;

    bool hastings_correction = true;
    bool random_order = true;
    bool delayed_initialization = false;
    bool predictive_parse_filter = false;

    unsigned niterations = 100;

    F anneal_start = 1;
    F anneal_stop = 1;
    unsigned anneal_its = 100;
    unsigned resample_pycache_nits = 0;
    F z_temp = 1;
    unsigned z_its = 0;
    unsigned long rand_init = 0;

    std::string parses_filename = "", grammar_filename = "";
    std::string test1_filename = "", test2_filename = "";
    std::vector<std::string> evalcmdstrs, test1cmdstrs, test2cmdstrs, grammarcmdstrs;
    Postreamps evalcmds, test1cmds, test2cmds, grammarcmds;
    unsigned eval_every = 1;

    unsigned nparses_iterations = 1;
    F train_frac = 1.0;
    bool train_frac_randomise = false;

    // define the command line arguments
    namespace po = boost::program_options;
    po::positional_options_description gram_desc;
    gram_desc.add("grammar-file", 1);

    po::options_description desc("program options");
    desc.add_options()
        ("help,h",
         "produce help message")

        ("config-file,c", po::value<std::string>(),
         "read options from this file")

        ("log-level,d", po::value<std::string>()->default_value("warning")->implicit_value("info"),
         "log level, choose in {fatal, error, warning, info, debug, trace}")

        ("print-analysis,A", po::value<std::string>(&parses_filename),
         "print analyses of training data at termination to a file")

        ("print-last-nanalysis,N", po::value<unsigned>(&nparses_iterations),
         "print analyses during last nanal-its iterations")

        ("print-compact-trees,C", po::value<bool>(&catcounttree_type::compact_trees)->implicit_value(true),
         "print compact trees omitting uncached categories")

        ("delay-init,D", po::value<bool>(&delayed_initialization)->implicit_value(true),
         "delay grammar initialization until all sentences are parsed")

        ("dirichlet-prior,E", po::value<bool>(&g.estimate_theta_flag)->implicit_value(true),
         "estimate rule prob (theta) using Dirichlet prior")

        ("grammar-file,G", po::value<std::string>(&grammar_filename),
         "print grammar at termination to a file")

        ("skip-hastings,H", po::value<bool>(&hastings_correction)->implicit_value(false),
         "skip Hastings correction of tree probabilities")

        ("ordered-parse,I", po::value<bool>(&random_order)->implicit_value(false),
         "parse sentences in order (default is random order)")

        ("predictive-parse-filter,P", po::value<bool>(&predictive_parse_filter)->implicit_value(true),
         "use a predictive Earley parse to filter useless categories")

        ("resample-pycache-niter,R", po::value<unsigned>(&resample_pycache_nits),
         "resample PY cache strings during first n iterations (-1 = forever)")

        ("niterations,n", po::value<unsigned>(&niterations),
         "number of iterations")

        ("random-seed,r", po::value<unsigned long>(&rand_init),
         "initializer for random number generator (integer)")

        ("py-a,a", po::value<F>(&g.default_pya),
         "default PY a parameter")

        ("py-b,b", po::value<F>(&g.default_pyb),
         "default PY b parameter")

        ("pya-beta-a,e", po::value<F>(&g.pya_beta_a),
         "if positive, parameter of Beta prior on pya; if negative, number of iterations to anneal pya")

        ("pya-beta-b,f", po::value<F>(&g.pya_beta_b),
         "if positive, parameter of Beta prior on pya")

        ("pyb-gamma-s,g", po::value<F>(&g.pyb_gamma_s),
         "if non-zero, parameter of Gamma prior on pyb")

        // i was h on MJ version (but conflict with -help)
        ("pyb-gamma-c,i", po::value<F>(&g.pyb_gamma_c),
         "parameter of Gamma prior on pyb")

        ("weight,w", po::value<F>(&g.default_weight),
         "default value of theta (or Dirichlet prior) in generator")

        ("train-sentences,s", po::value<F>(&train_frac),
         "train only on train_frac percentage of training sentences (ignore remainder)")

        ("random-training-fraction,S", po::value<bool>(&train_frac_randomise),
         "randomise training fraction of sentences (default: training fraction is at front)")

        ("temp-start,T", po::value<F>(&anneal_start),
         "start annealing with this temperature")

        ("temp-stop,t", po::value<F>(&anneal_stop),
         "stop annealing with this temperature")

        ("anneal-iterations,m", po::value<unsigned>(&anneal_its),
         "anneal for this many iterations")

        ("ztemp,Z", po::value<F>(&z_temp),
         "temperature used just before stopping")

        ("zits,z", po::value<unsigned>(&z_its),
         "perform zits iterations at temperature ztemp at end of run")

        ("eval-parses-cmd,X", po::value<std::vector<std::string> >(&evalcmdstrs),
         "pipe each run's parses into this command (empty line separates runs)")

        ("eval-grammar-cmd,Y", po::value<std::vector<std::string> >(&grammarcmdstrs),
         "pipe each run's grammar-rules into this command (empty line separates runs)")

        ("eval-every,x", po::value<unsigned>(&eval_every),
         "pipe trees into the eval-parses-cmd every eval-every iterations")

        ("test1-file,u", po::value<std::string>(&test1_filename),
         "test strings to be parsed (but not trained on) every eval-every iterations")

        ("test1-eval,unsigned", po::value<std::vector<std::string> >(&test1cmdstrs),
         "parses of test1-file are piped into this command")

        ("test2-file,v", po::value<std::string>(&test2_filename),
         "test strings to be parsed (but not trained on) every eval-every iterations")

        ("test2-eval,V", po::value<std::vector<std::string> >(&test2cmdstrs),
         "parses of test2-file are piped into this command")
        ;

    // parse the command line arguments and store them in vm
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(gram_desc).run(), vm);
    po::notify(vm);

    // TODO better if we trigger that directly in the argument parser...
    anneal_start = 1.0 / anneal_start;
    anneal_stop = 1.0 / anneal_stop;
    for (const auto& cmd : evalcmdstrs)
        evalcmds.push_back(new pstream::ostream(cmd));
    for (const auto& cmd : grammarcmdstrs)
        grammarcmds.push_back(new pstream::ostream(cmd));
    for (const auto& cmd : test1cmdstrs)
        test1cmds.push_back(new pstream::ostream(cmd));
    for (const auto& cmd : test2cmdstrs)
        test2cmds.push_back(new pstream::ostream(cmd));

    // if --help given, display the help message and exit
    if (vm.count("help"))
    {
        std::cerr << usage << std::endl << desc << std::endl;
        exit(0);
    }

    // init the logger at the requested severity level
    logging::init(vm["log-level"].as<std::string>());

    // if --config-file given, parse the argument specified in that file
    if (vm.count("config-file") > 0)
    {
        const std::string config = vm["config-file"].as<std::string>();
        LOG(info) << "loading configuration from '" << config << "'";

        std::ifstream is(config.c_str());
        if (not is)
        {
            LOG(fatal) << "cannot open configuration file '" << config << "', exiting";
            exit(1);
        }

        po::store(po::parse_config_file(is, desc), vm);
        po::notify(vm);
    }

    // read the grammar file
    if (vm.count("grammar-file") == 0)
    {
        LOG(fatal) << "grammar-file not specified, exiting";
        exit(1);
    }
    const std::string grammar_file = vm["grammar-file"].as<std::string>();
    LOG(info) << "loading grammar from '" << grammar_file << "'";
    std::ifstream is(grammar_file.c_str());
    if (not is)
    {
        LOG(fatal) << "cannot open grammar file '" << grammar_file << "', exiting";
        exit(1);
    }
    is >> g;

    // log the eval commands if there are ones
    if (evalcmds.size())
    {
        std::stringstream eval_cmds;
        for (const auto& cmd : evalcmdstrs)
            eval_cmds << cmd << ", ";
        std::string eval_cmds_str = eval_cmds.str();

        // remove trailing ", "
        eval_cmds_str.erase(eval_cmds_str.size() - 2);
        LOG(info) << "eval commands = " << eval_cmds_str;
    }

    // read train symbols from stdin
    std::vector<std::vector<symbol> > trains;
    std::vector<symbol> terminals;
    while (readline_symbols(std::cin, terminals))
        if (terminals.empty())
            LOG(error) << "training data sentence " << trains.size() + 1 << " is empty";
        else
            trains.push_back(terminals);
    LOG(info) << "read " << trains.size() << " lines for training";

    if (predictive_parse_filter)
        g.initialize_predictive_parse_filter();

    std::vector<std::vector<symbol> > test1s;
    if (!test1_filename.empty())
    {
        std::ifstream in1(test1_filename.c_str());
        Ss terminals;
        while (readline_symbols(in1, terminals))
            if (terminals.empty())
                std::cerr << "## Error in " << test1_filename << ": sentence "
                          << test1s.size()+1 << " is empty"
                          << std::endl;
            else
                test1s.push_back(terminals);

        if (debug >= 1000)
            std::cerr << "# test1s.size() = " << test1s.size() << std::endl;
    }

    std::vector<std::vector<symbol> > test2s;
    if (!test2_filename.empty()) {
        std::ifstream in2(test2_filename.c_str());
        Ss terminals;
        while (readline_symbols(in2, terminals))
            if (terminals.empty())
                std::cerr << "## Error in " << test2_filename << ": sentence "
                          << test2s.size()+1 << " is empty"
                          << std::endl;
            else
                test2s.push_back(terminals);

        if (debug >= 1000)
            std::cerr << "# test2s.size() = " << test2s.size() << std::endl;
    }

    if (rand_init == 0)
        rand_init = time(NULL);

    mt_init_genrand(rand_init);
    LOG(trace) << "D = " << delayed_initialization
               << ", E = " << g.estimate_theta_flag
               << ", I = " << random_order
               << ", P = " << predictive_parse_filter
               << ", R = " << resample_pycache_nits
               << ", n = " << niterations
               << ", N = " << nparses_iterations
               << ", P = " << predictive_parse_filter
               << ", w = " << g.default_weight
               << ", a = " << g.default_pya
               << ", b = " << g.default_pyb
               << ", e = " << g.pya_beta_a
               << ", f = " << g.pya_beta_b
               << ", g = " << g.pyb_gamma_s
               << ", h = " << g.pyb_gamma_c
               << ", r = " << rand_init
               << ", s = " << train_frac
               << ", S = " << train_frac_randomise
               << ", x = " << eval_every
               << ", m = " << anneal_its
               << ", Z = " << z_temp
               << ", z = " << z_its
               << ", T = " << 1.0/anneal_start
               << ", t = " << anneal_stop;

    if (train_frac < 0 || train_frac > 1)
    {
        LOG(fatal) << "-s train_frac must be between 0 and 1 but it is " << train_frac <<", exiting";
        exit(1);
    }

    std::ostream* finalparses_stream_ptr = NULL;
    if (!parses_filename.empty())
        finalparses_stream_ptr = new std::ofstream(parses_filename.c_str());

    std::ostream* grammar_stream_ptr = NULL;
    if (!grammar_filename.empty())
        grammar_stream_ptr = new std::ofstream(grammar_filename.c_str());

    LOG(info) << "initial grammar = " << g;
    pycky parser(g);

    gibbs_estimate(g, trains, train_frac, train_frac_randomise, evalcmds, eval_every,
                   niterations, anneal_start, anneal_stop, anneal_its, z_temp, z_its,
                   hastings_correction, random_order, delayed_initialization,
                   resample_pycache_nits, nparses_iterations,
                   finalparses_stream_ptr, grammar_stream_ptr,
                   test1s, test1cmds, test2s, test2cmds, grammarcmds);

    if (finalparses_stream_ptr)
        delete finalparses_stream_ptr;

    if (grammar_stream_ptr)
        delete grammar_stream_ptr;

    foreach (Postreamps, it, grammarcmds)
        delete *it;

    foreach (Postreamps, it, evalcmds)
        delete *it;

    foreach (Postreamps, it, test1cmds)
        delete *it;

    foreach (Postreamps, it, test2cmds)
        delete *it;
}
