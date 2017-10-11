// py-cfg.cc
//
// py-cfg runs a Pitman-Yor process for each nonterminal
// to estimate an Adaptor Grammar

const char usage[] =
"py-cfg version of 5th March, 2014\n"
"\n"
"py-cfg [-d debug]\n"
"       [-A parses-file] [-C] [-D] [-E] [-F trace-file] [-G grammar-file]\n"
"       [-H] [-I] [-P] [-R nr]\n"
"       [-r rand-init] [-n niterations] [-N nanal-its]\n"
"       [-a a] [-b b] [-w weight]\n"
"       [-e pya-beta-a] [-f pya-beta-b] [-g pyb-gamma-s] [-h pyb-gamma-c]\n"
"       [-s train_frac] -S\n"
"       [-T anneal-temp-start] [-t anneal-temp-stop] [-m anneal-its]\n"
"       [-Z ztemp] [-z zits]\n"
"       [-x eval-every] [-X eval-cmd] [-Y eval-cmd]\n"
"       [-u test1.yld] [-U eval-cmd]\n"
"       [-v test1.yld] [-V eval-cmd]\n"
"       grammar.lt < train.yld\n"
"\n"
" -d debug        -- debug level\n"
" -A parses-file  -- print analyses of training data at termination\n"
" -N nanal-its    -- print analyses during last nanal-its iterations\n"
" -C              -- print compact trees omitting uncached categories\n"
" -D              -- delay grammar initialization until all sentences are parsed\n"
" -E              -- estimate rule prob (theta) using Dirichlet prior\n"
" -F trace-file   -- file to write trace output to\n"
" -G grammar-file -- print grammar at termination\n"
" -H              -- skip Hastings correction of tree probabilities\n"
" -I              -- parse sentences in order (default is random order)\n"
" -P              -- use a predictive Earley parse to filter useless categories\n"
" -R nr           -- resample PY cache strings during first nr iterations (-1 = forever)\n"
" -n niterations  -- number of iterations\n"
" -r rand-init    -- initializer for random number generator (integer)\n"
" -a a            -- default PY a parameter\n"
" -b b            -- default PY b parameter\n"
" -e pya-beta-a   -- if positive, parameter of Beta prior on pya; if negative, number of iterations to anneal pya\n"
" -f pya-beta-b   -- if positive, parameter of Beta prior on pya\n"
" -g pyb-gamma-s  -- if non-zero, parameter of Gamma prior on pyb\n"
" -h pyb-gamma-c  -- parameter of Gamma prior on pyb\n"
" -w weight       -- default value of theta (or Dirichlet prior) in generator\n"
" -s train_frac   -- train only on train_frac percentage of training sentences (ignore remainder)\n"
" -S              -- randomise training fraction of sentences (default: training fraction is at front)\n"
" -T tstart       -- start with this annealing temperature\n"
" -t tstop        -- stop annealing at this annealing temperature\n"
" -m anneal-its   -- anneal for this many iterations\n"
" -Z ztemp        -- temperature used just before stopping\n"
" -z zits         -- perform zits iterations at temperature ztemp at end of run\n"
" -X eval-cmd     -- pipe each run's parses into this command (empty line separates runs)\n"
" -Y eval-cmd     -- pipe each run's grammar-rules into this command (empty line separates runs)\n"
" -x eval-every   -- pipe trees into the eval-cmd every eval-every iterations\n"
" -u test1.yld    -- test strings to be parsed (but not trained on) every eval-every iterations\n"
" -U eval-cmd     -- parses of test1.yld are piped into this command\n"
" -v test2.yld    -- test strings to be parsed (but not trained on) every eval-every iterations\n"
" -V eval-cmd     -- parses of test2.yld are piped into this command\n"
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

#include "custom-allocator.h"

#include <cmath>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <time.h>
#include <unistd.h>
#include <vector>

#include "pstream.h"
#include "py-cky.h"
#include "sym.h"
#include "xtree.h"

typedef unsigned int U;
typedef std::vector<Ss> Sss;

typedef pstream::ostream* Postreamp;
typedef std::vector<Postreamp> Postreamps;

int debug = 0;

struct S_F_incrementer {
  const F increment;
  S_F_incrementer(F increment) : increment(increment) { }

  template <typename arg_type>
  void operator() (const arg_type& arg, S_F& parent_weights) const
  {
    foreach (S_F, it, parent_weights)
      it->second += increment;
  }
};

struct RandomNumberGenerator : public std::unary_function<U,U> {
  U operator() (U nmax) {
    return mt_genrand_int32() % nmax;
  }
};

F gibbs_estimate(pycfg_type& g, const Sss& trains,
		 F train_frac, bool train_frac_randomise,
		 Postreamps& evalcmds, U eval_every,
		 U niterations,
		 F anneal_start, F anneal_stop, U anneal_its,
		 F z_temp, U z_its,
		 bool hastings_correction, bool random_order,
		 bool delayed_initialization,
		 U resample_pycache_nits,
		 U nparses_iterations,
		 std::ostream* finalparses_stream_ptr,
		 std::ostream* grammar_stream_ptr,
		 std::ostream* trace_stream_ptr,
		 const Sss& test1s, Postreamps& test1cmds,
		 const Sss& test2s, Postreamps& test2cmds,
		 Postreamps& grammarcmds) {

  typedef pycky::tree tree;
  typedef std::vector<tree*> tps_type;
  typedef std::vector<bool> Bs;
  typedef std::vector<U> Us;

  U n = trains.size();
  U nn = U(n * train_frac);
  U nwords = 0;
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

  if (trace_stream_ptr)
    *trace_stream_ptr << "# " << nwords << " tokens in "
		      << nn << " sentences" << std::endl
		      << "#\n"
		      << "# It\tTemp\tTime\t-logP\t-logPcorpus\t-logPrior\ttables\tsame\tchanged\treject\tdefault_pya\t(parent pym pyn pya pyb)*" << std::endl;

  Us index(n);  // order in which parses should be resampled
  for (unsigned i = 0; i < n; ++i)
    index[i] = i;

  U unchanged = 0, rejected = 0;

  for (U iteration = 0; iteration < niterations; ++iteration) {

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

    if (trace_stream_ptr && iteration % eval_every == 0) {
      F nlogPcorpus = -g.logPcorpus();
      F nlogPrior = -g.logPrior();
      *trace_stream_ptr << iteration << '\t'            // iteration
			<< 1.0/p.anneal << '\t'         // temperature
			<< runtime() << '\t'            // cpu time used
			<< nlogPcorpus+nlogPrior << '\t' // - log P(corpus,parameters)
	                << nlogPcorpus << '\t'          // -log P(corpus|parameters)
			<< nlogPrior << '\t'            // -log P(parameters)
			<< g.sum_pym() << '\t'          // # of tables
			<< unchanged << '\t'            // # unchanged parses
			<< n-unchanged << '\t'          // # changed parses
			<< rejected << '\t'             // # parses rejected
			<< g.default_pya << std::flush; // default pya parameter
      if (g.pyb_gamma_s > 0 && g.pyb_gamma_c > 0 && debug >= 10)
	g.write_adaptor_parameters(*trace_stream_ptr);
      *trace_stream_ptr << std::endl;
    }

    if (iteration % eval_every == 0) {  // do we print a trace at this iteration?
      for (U i = 0; i < n; ++i) {
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

      cforeach (Sss, it, test1s) {  // parse test1s
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

      cforeach (Sss, it, test2s) {  // parse test2s
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

    for (U i0 = 0; i0 < n; ++i0) {

      U i = index[i0];

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
      for (U i = 0; i < n; ++i)
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

  if (trace_stream_ptr) {
    F nlogPcorpus = -g.logPcorpus();
    F nlogPrior = -g.logPrior();
    *trace_stream_ptr << niterations << '\t'          // iteration
		      << 1.0/p.anneal << '\t'         // temperature
		      << runtime() << '\t'            // cpu time used
		      << nlogPcorpus+nlogPrior << '\t' // - log P(corpus,parameters)
		      << nlogPcorpus << '\t'          // -log P(corpus|parameters)
		      << nlogPrior << '\t'            // -log P(parameters)
		      << g.sum_pym() << '\t'          // # of tables
		      << unchanged << '\t'            // # unchanged parses
		      << nn-unchanged << '\t'         // # changed parses
		      << rejected << '\t'             // # parses rejected
		      << g.default_pya << std::flush; // default pya value
    if (g.pyb_gamma_s > 0 && g.pyb_gamma_c > 0 && debug >= 10)
      g.write_adaptor_parameters(*trace_stream_ptr);
    *trace_stream_ptr << std::endl;
  }

  for (U i = 0; i < n; ++i) {
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

  cforeach (Sss, it, test1s) {  // final parse of test1s
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

  cforeach (Sss, it, test2s) {  // final parse for test2s
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
  for (U i = 0; i < n; ++i)
    if (train_flag[i]) {
      g.decrtree(tps[i], 1);
      tps[i]->selective_delete();
    }
  g.estimate_theta_flag = estimate_theta_flag;

  return logPcorpus;
}  // gibbs_estimate()



int main(int argc, char** argv) {

  typedef std::string Str;
  typedef std::vector<Str> Strs;

  pycfg_type g;
  bool hastings_correction = true;
  bool random_order = true;
  bool delayed_initialization = false;
  bool predictive_parse_filter = false;
  U niterations = 100;
  F anneal_start = 1;
  F anneal_stop = 1;
  U anneal_its = 100;
  int resample_pycache_nits = 0;
  F z_temp = 1;
  U z_its = 0;
  unsigned long rand_init = 0;
  Str parses_filename = "", grammar_filename = "", trace_filename = "", test1_filename = "", test2_filename = "";
  Strs evalcmdstrs, test1cmdstrs, test2cmdstrs, grammarcmdstrs;
  Postreamps evalcmds, test1cmds, test2cmds, grammarcmds;
  U eval_every = 1;
  U nparses_iterations = 1;
  F train_frac = 1.0;
  bool train_frac_randomise = false;

  int chr;
  while ((chr = getopt(argc, argv, "A:CDEF:G:H:I:N:PR:ST:U:V:X:Y:Z:a:b:d:e:f:g:h:m:n:r:s:t:u:v:w:x:z:"))
	 != -1)
    switch (chr) {
    case 'A':
      parses_filename = optarg;
      break;
    case 'C':
      catcounttree_type::compact_trees = true;
      break;
    case 'D':
      delayed_initialization = true;
      break;
    case 'E':
      g.estimate_theta_flag = true;
      break;
    case 'F':
      trace_filename = optarg;
      break;
    case 'G':
      grammar_filename = optarg;
      break;
    case 'H':
      hastings_correction = false;
      break;
    case 'I':
      random_order = false;
      break;
    case 'N':
      nparses_iterations = atoi(optarg);
      break;
    case 'P':
      predictive_parse_filter = true;
      break;
    case 'R':
      resample_pycache_nits = atoi(optarg);
      break;
    case 'S':
      train_frac_randomise = true;
      break;
    case 'T':
      anneal_start = 1.0/atof(optarg);
      break;
    case 'U':
      test1cmdstrs.push_back(std::string(optarg));
      test1cmds.push_back(new pstream::ostream(optarg));
      break;
    case 'V':
      test2cmdstrs.push_back(std::string(optarg));
      test2cmds.push_back(new pstream::ostream(optarg));
      break;
    case 'X':
      evalcmdstrs.push_back(std::string(optarg));
      evalcmds.push_back(new pstream::ostream(optarg));
      break;
    case 'Y':
      grammarcmdstrs.push_back(std::string(optarg));
      grammarcmds.push_back(new pstream::ostream(optarg));
    case 'Z':
      z_temp = atof(optarg);
      break;
    case 'a':
      g.default_pya = atof(optarg);
      break;
    case 'b':
      g.default_pyb = atof(optarg);
      break;
    case 'd':
      debug = atoi(optarg);
      break;
    case 'e':
      g.pya_beta_a = atof(optarg);
      break;
    case 'f':
      g.pya_beta_b = atof(optarg);
      break;
    case 'g':
      g.pyb_gamma_s = atof(optarg);
      break;
    case 'h':
      g.pyb_gamma_c = atof(optarg);
      break;
    case 'm':
      anneal_its = atoi(optarg);
      break;
    case 'n':
      niterations = atoi(optarg);
      break;
    case 'r':
      rand_init = strtoul(optarg, NULL, 10);
      break;
    case 's':
      train_frac = atof(optarg);
      break;
    case 't':
      anneal_stop = 1.0/atof(optarg);
      break;
    case 'u':
      test1_filename = optarg;
      break;
    case 'v':
      test2_filename = optarg;
      break;
    case 'w':
      g.default_weight = atof(optarg);
      break;
    case 'x':
      eval_every = atoi(optarg);
      break;
    case 'z':
      z_its = atoi(optarg);
      break;
    default:
      std::cerr << "# Error in " << argv[0]
		<< ": can't interpret argument -" << char(chr) << std::endl;
      std::cerr << usage << abort;
    }

  if (argc - optind != 1) {
    std::cerr << "# Error in " << argv[0]
	      << ", argc = " << argc << ", optind = " << optind << std::endl;

    std::cerr << "Command line arguments:\ni\targv[i]" << std::endl;
    for (int i = 0; i < argc; ++i)
      std::cerr << i << '\t' << argv[i] << std::endl;

    std::cerr << usage << abort;
  }

  if (debug >= 1000)
    std::cerr << "# eval_cmds = " << evalcmdstrs << std::endl;

  Sss trains;
  {
    Ss terminals;
    while (readline_symbols(std::cin, terminals))
      if (terminals.empty())
	std::cerr << "## Error in " << argv[0] << ": training data sentence "
		  << trains.size()+1 << " is empty"
		  << std::endl;
      else
	trains.push_back(terminals);
  }

  {
    std::ifstream is(argv[optind]);
    if (!is)
      std::cerr << "# Error in " << argv[0]
		<< ", can't open grammar file " << argv[optind] << abort;
    is >> g;

    if (predictive_parse_filter)
      g.initialize_predictive_parse_filter();
  }

  Sss test1s;
  if (!test1_filename.empty()) {
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

  Sss test2s;
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

  std::ostream* trace_stream_ptr = NULL;
  if (!trace_filename.empty())
    trace_stream_ptr = new std::ofstream(trace_filename.c_str());

  std::stringstream parameters;
  parameters << "# D = " << delayed_initialization
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
  // std::cerr << parameters.str() << std::endl;

  if (trace_stream_ptr)
      *trace_stream_ptr << parameters.str() << std::endl;

  if (train_frac < 0 || train_frac > 1)
    std::cerr << "## Error in py-cfg: -s train_frac must be between 0 and 1\n"
	      << abort;

  std::ostream* finalparses_stream_ptr = NULL;
  if (!parses_filename.empty())
    finalparses_stream_ptr = new std::ofstream(parses_filename.c_str());

  std::ostream* grammar_stream_ptr = NULL;
  if (!grammar_filename.empty())
    grammar_stream_ptr = new std::ofstream(grammar_filename.c_str());

  if (debug >= 1000)
    std::cerr << "# py-cfg Initial grammar = \n" << g << std::endl;

  pycky parser(g);

  gibbs_estimate(g, trains, train_frac, train_frac_randomise, evalcmds, eval_every,
		 niterations, anneal_start, anneal_stop, anneal_its, z_temp, z_its,
		 hastings_correction, random_order, delayed_initialization,
		 static_cast<U>(resample_pycache_nits), nparses_iterations,
		 finalparses_stream_ptr, grammar_stream_ptr, trace_stream_ptr,
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

  if (trace_stream_ptr)
    delete trace_stream_ptr;
}
