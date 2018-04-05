#ifndef _ESTIMATOR_PARAMETERS_HH
#define _ESTIMATOR_PARAMETERS_HH

#include <string>


namespace estimator
{
    class parameters
    {
    public:
        parameters(
            bool do_mbdp,                       // if true, compute using Brent model
            double pstop,                       // character (monkey) model parameter
            double hypersampling_ratio,         // the standard deviation for new hyperparm proposals
            double init_pboundary,              // initial prob of boundary
            const std::string& forget_method,   // used in sampler::online_unigram::forget_items
            double aeos,                        // Beta parameter for end-of-sentence
            double a1,                          // unigram PY parameter
            double b1,                          // unigram PY parameter
            double a2,                          // bigram PY parameters
            double b2,                          // bigram PY parameters
            double pya_beta_a,                  // beta prior on pya
            double pya_beta_b,                  // beta prior on pya
            double pyb_gamma_c,                 // gamma prior on pyb
            double pyb_gamma_s,                 // gamma prior on pyb
            std::size_t trace_every,            // frequency with which tracing should be performed
            std::size_t token_memory,
            std::size_t type_memory);

        ~parameters();

        bool do_mbdp() const;
        double pstop() const;
        double hypersampling_ratio() const;
        double init_pboundary() const;
        std::string forget_method() const;
        double aeos() const;
        double a1() const;
        double b1() const;
        double a2() const;
        double b2() const;
        double pya_beta_a() const;
        double pya_beta_b() const;
        double pyb_gamma_c() const;
        double pyb_gamma_s() const;
        std::size_t trace_every() const;
        std::size_t token_memory() const;
        std::size_t type_memory() const;

    private:
        bool m_do_mbdp;
        double m_pstop;
        double m_hypersampling_ratio;
        double m_init_pboundary;
        std::string m_forget_method;
        double m_aeos;
        double m_a1;
        double m_b1;
        double m_a2;
        double m_b2;
        double m_pya_beta_a;
        double m_pya_beta_b;
        double m_pyb_gamma_c;
        double m_pyb_gamma_s;
        std::size_t m_trace_every;
        std::size_t m_token_memory;
        std::size_t m_type_memory;
    };
}

#endif  // _ESTIMATOR_PARAMETERS_HH
