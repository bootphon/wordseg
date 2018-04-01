// slice_sampler.hpp is an MCMC slice sampler

#ifndef SLICE_SAMPLER_HPP
#define SLICE_SAMPLER_HPP

#include <cmath>
#include <limits>


namespace sampler
{
    // slice_sampler_rfc_type{} returns the value of a user-specified
    // function if the argument is within range, or - infinity otherwise
    template <class Scalar, class Fonction>
    class slice_sampler_rfc_type
    {
    private:
        Scalar min_x, max_x;
        const Fonction& f;
        unsigned int max_nfeval, nfeval;

    public:
        slice_sampler_rfc_type(Scalar min_x, Scalar max_x, const Fonction& f, unsigned int max_nfeval)
            : min_x(min_x), max_x(max_x), f(f), max_nfeval(max_nfeval), nfeval(0)
            {}

        Scalar operator() (Scalar x)
            {
                if (min_x < x && x < max_x)
                {
                    assert(++nfeval <= max_nfeval);

                    Scalar fx = f(x);

                    assert(finite(fx));
                    return fx;
                }
                else
                {
                    return -std::numeric_limits<Scalar>::infinity();
                }
            }
    };


    // Univariate "range doubling" slice sampler described in Neal (2003)
    // "Slice Sampling", The Annals of Statistics 31(3), 705-767.
    template <class Scalar, class LogF, class Uniform01>
    Scalar slice_sampler1d(const LogF& logF0,               //!< log of function to sample
                           Scalar x,                             //!< starting point
                           Uniform01& u01,                  //!< uniform [0,1) random number generator
                           Scalar min_x = -std::numeric_limits<Scalar>::infinity(),  //!< minimum value of support
                           Scalar max_x = std::numeric_limits<Scalar>::infinity(),   //!< maximum value of support
                           Scalar w = 0.0,                       //!< guess at initial width
                           unsigned nsamples=1,             //!< number of samples to draw
                           unsigned max_nfeval=200)         //!< max number of function evaluations
    {
        slice_sampler_rfc_type<Scalar, LogF> logF(min_x, max_x, logF0, max_nfeval);

        assert(finite(x));

        // set w to a default width
        if (w <= 0.0)
        {
            if (min_x > -std::numeric_limits<double>::infinity()
                and max_x < std::numeric_limits<double>::infinity())
                w = (max_x - min_x)/4;
            else
                w = std::max(((x < 0.0) ? -x : x)/4, 0.1);
        }
        assert(finite(w));

        Scalar logFx = logF(x);
        for (unsigned int sample = 0; sample < nsamples; ++sample)
        {
            Scalar logY = logFx + log(u01()+1e-100);     //! slice logFx at this value
            assert(finite(logY));

            Scalar xl = x - w*u01();                     //! lower bound on slice interval
            Scalar logFxl = logF(xl);
            Scalar xr = xl + w;                          //! upper bound on slice interval
            Scalar logFxr = logF(xr);

            while (logY < logFxl || logY < logFxr)  // doubling procedure
                if (u01() < 0.5)
                    logFxl = logF(xl -= xr - xl);
                else
                    logFxr = logF(xr += xr - xl);

            Scalar xl1 = xl;
            Scalar xr1 = xr;

            // shrinking procedure
            while (true)
            {
                Scalar x1 = xl1 + u01()*(xr1 - xl1);
                if (logY < logF(x1))
                {
                    // acceptance procedure
                    Scalar xl2 = xl;
                    Scalar xr2 = xr;
                    bool d = false;
                    while (xr2 - xl2 > 1.1*w)
                    {
                        Scalar xm = (xl2 + xr2)/2;
                        if ((x < xm && x1 >= xm) || (x >= xm && x1 < xm))
                            d = true;
                        if (x1 < xm)
                            xr2 = xm;
                        else
                            xl2 = xm;

                        if (d && logY >= logF(xl2) && logY >= logF(xr2))
                            goto unacceptable;
                    }

                    x = x1;
                    goto acceptable;
                }
                goto acceptable;
            unacceptable:
                if (x1 < x)
                    // rest of shrinking procedure
                    xl1 = x1;
                else
                    xr1 = x1;
            }
        acceptable:
            // update width estimate
            w = (4*w + (xr1 - xl1))/5;
        }

        return x;
    }


    // returns the log probability density of x under a
    // Beta(alpha,beta) distribution.
    template<class Scalar>
    inline Scalar lbetadist(Scalar x, Scalar alpha, Scalar beta)
    {
        assert(x > 0);
        assert(x < 1);

        assert(alpha > 0);
        assert(beta > 0);

        return (alpha - 1) * std::log(x) + (beta - 1) * std::log(1 - x)
            + std::lgamma(alpha + beta) - std::lgamma(alpha) - std::lgamma(beta);
    }


    // returns the log probability density of x under a
    // Gamma(alpha,beta) distribution.
    template<class Scalar>
    inline Scalar lgammadist(Scalar x, Scalar alpha, Scalar beta)
    {
        assert(alpha > 0);
        assert(beta > 0);

        return (alpha - 1) * std::log(x) - alpha * std::log(beta) - x / beta - std::lgamma(alpha);
    }



    // calculates the Beta prior on pya.
    template<class Scalar>
    static Scalar pya_logPrior(Scalar pya, Scalar pya_beta_a, Scalar pya_beta_b)
    {
        return lbetadist(pya, pya_beta_a, pya_beta_b);
    }


    // calculates the prior probability of pyb wrt the Gamma prior on
    // pyb.
    template<class Scalar>
    static Scalar pyb_logPrior(Scalar pyb, Scalar pyb_gamma_c, Scalar pyb_gamma_s)
    {
        return lgammadist(pyb, pyb_gamma_c, pyb_gamma_s);
    }


    // Function object that returns the part of log prob that depends
    // on pyb. This includes the Gamma prior on pyb, but doesn't
    // include e.g. the rule probabilities (as these are a constant
    // factor)
    template<class Unigrams, class Scalar>
    class resample_pyb_type
    {
    private:
        Unigrams& lex;
        Scalar pyb_gamma_c, pyb_gamma_s;

    public:
        resample_pyb_type(Unigrams& lex, Scalar pyb_gamma_c, Scalar pyb_gamma_s)
            : lex(lex), pyb_gamma_c(pyb_gamma_c), pyb_gamma_s(pyb_gamma_s)
            {}

        // returns the part of the log posterior probability that
        // depends on pyb
        Scalar operator() (Scalar pyb) const
            {
                // prior for pyb
                Scalar logPrior = 0;
                logPrior += pyb_logPrior(lex.pyb(), pyb_gamma_c, pyb_gamma_s);

                Scalar logProb = lex.logprob();
                // TRACE2(logPrior, logProb);

                return logProb+logPrior;
            }
    };


    // calculates the part of the log prob that depends on pya.  This
    // includes the Beta prior on pya, but doesn't include e.g. the
    // rule probabilities (as these are a constant factor)
    template<class Unigrams, class Scalar>
    class resample_pya_type
    {
    private:
        Unigrams& lex;
        Scalar pya_beta_a, pya_beta_b;

    public:
        resample_pya_type(Unigrams& lex, Scalar pya_beta_a, Scalar pya_beta_b)
            : lex(lex), pya_beta_a(pya_beta_a), pya_beta_b(pya_beta_b)
            {}

        // operator() returns the part of the log posterior probability that
        // depends on pya
        Scalar operator() (Scalar pya) const
            {
                Scalar logPrior = pya_logPrior(lex.pya(), pya_beta_a, pya_beta_b);
                Scalar logProb = lex.logprob();
                // TRACE2(logPrior, logProb);
                return logPrior + logProb;
            }
    };
}

#endif  // SLICE_SAMPLER_HPP
