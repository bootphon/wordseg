#ifndef _SAMPLER_PARAMETERS_HH
#define _SAMPLER_PARAMETERS_HH


#include <string>

namespace sampler
{
    class parameters
    {
    public:
        // if true, compute using Brent model
        bool do_mbdp;

        // character (monkey) model parameter
        double pstop;

        double hypersampling_ratio; // the standard deviation for new hyperparm proposals
        double init_pboundary;      // initial prob of boundary

        // used in sampler::online_unigram::forget_items
        std::string forget_method;

        double aeos;                //!< Beta parameter for end-of-sentence
        double a1;                  //!< unigram PY parameters
        double b1;
        double a2;                  //!< bigram PY parameters
        double b2;

        double pya_beta_a;          // beta prior on pya
        double pya_beta_b;          // beta prior on pya
        double pyb_gamma_c;         // gamma prior on pyb
        double pyb_gamma_s;         // gamma prior on pyb

        std::size_t trace_every;         //!< Frequency with which tracing should be performed
        std::size_t token_memory;
        std::size_t type_memory;

    };
}

#endif  // _SAMPLER_PARAMETERS_HH
