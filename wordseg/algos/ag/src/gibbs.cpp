#include "gibbs.hh"

#include "logging.hh"

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
