#ifndef _GIBBS_HH
#define _GIBBS_HH

#include <iostream>
#include <vector>

#include "pstream.hh"
#include "pycky.hh"


using Postreamps = std::vector<pstream::ostream*>;


F gibbs_estimate(
    pycfg_type& g, const std::vector<std::vector<symbol> >& trains,
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
    Postreamps& grammarcmds);


#endif  // _GIBBS_HH
