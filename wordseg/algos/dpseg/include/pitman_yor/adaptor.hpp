#ifndef _PY_ADAPTER_HPP
#define _PY_ADAPTER_HPP

// py/adapter.hpp defines the Pitman-Yor adaptor class
//
// A sampler needs to implement the following API (where F is a
// floating-point type and U is an unsigned int type)
//
// typedef V argument_type               -- type of objects over which labels are defined
// typedef S state_type                  -- type of hidden state information
// F operator()(const V& v) const        -- returns proposal prob of generating a v,
//                                          summing over all possible states
// F insert(const V& v)                  -- inserts v
// F erase(const V& v)                   -- erases v
// F insert(const V& v, S& s)            -- inserts (v,s), returns exact prob of (v,s)
// F erase(const V& v, S& s)             -- removes (v,s), returns exact prob of insert(v,s)


#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <functional>
#include <unordered_map>
#include <utility>

#include "random-mt19937ar.hpp"
#include "pitman_yor/restaurant.hh"
#include "util.hpp"


extern std::size_t debug_level;


namespace pitman_yor
{
    // Note that in this class, both the base distribution and the
    // parameters are actually reference variables, which means they
    // must exist *outside* the class. For base distrib, this is so
    // that hierarchical models can share/modify a single base
    // distrib. For parms, this is because constants are all stored in
    // a single class, and we will modify the values in that class
    // (which is accessible to more parts of the code).
    template <typename Base>
    class adaptor
    {
    public:
        typedef typename Base::argument_type argument_type;
        typedef double result_type;

        //public functions are defined after defining private table class
        //below.

    protected:
        Base& base;           //!< base distribution
        uniform01_type& u01;  //!< shared random number generator
        double a;                  //!< Pitman-Yor b parameter
        double b;                  //!< Pitman-Yor b parameter
        std::size_t m;                  //!< number of occupied tables
        std::size_t n;                  //!< number of customers in restaurant

        typedef argument_type V;
        typedef std::map<std::size_t,std::size_t> U_U;
        typedef std::unordered_map<V, restaurant> V_T;

        V_T label_tables;

    public:
        adaptor(Base& base, uniform01_type& u01, double a, double b)
            : base(base), u01(u01), a(a), b(b), m(), n()
            {}

        // note that copies of the adaptor will have a reference to the
        // same base distribution (useful for hierarchical models)

        /*
        // sg: need to figure out and test these if I do a particle
        // filter; static refs are annoying.
        PYAdaptor(const PYAdaptor& pya):
        base(pya.base), u01(pya.u01), a(pya.a), b(pya.b),
        m(pya.m), n(pya.n), label_tables(pya.label_tables)
        {}

        // make a copy with a copy of the base distribution (needed for
        // particle filtering)
        PYAdaptor(const PYAdaptor& pya, Base& base0):
        base(base0), u01(pya.u01), a(pya.a), b(pya.b),
        m(pya.m), n(pya.n), label_tables(pya.label_tables)
        {}
        */
    private:
        adaptor& operator=(const adaptor& pya)
            {}

//       PYAdaptor& operator=(const PYAdaptor& pya) {
// 	base = pya.base; u01 = pya.u01;
// 	a = pya.a; b = pya.b;
// 	m = pya.m; n = pya.n;
// 	label_tables = pya.label_tables;
// 	return *this;}

    public:
        double& pya()
            {
                return a;
            }

        double& pyb()
            {
                return b;
            }

        std::size_t ntypes() const
            {
                return label_tables.size();
            }

        std::size_t ntables() const
            {
                return m;
            }

        std::size_t ntokens() const
            {
                return n;
            }

        std::size_t ntokens(const V& v) const
            {
                typename V_T::const_iterator tit = label_tables.find(v);
                return (tit == label_tables.end()) ? 0 : tit->second.get_n();
            }

        const Base& base_dist() const
            {
                return base;
            }

        Base& base_dist()
            {
                return base;
            }

        //! operator() returns the approximate probability for inserting
        //! v, with context
        double operator() (const V& v) const
            {
                if (debug_level >= 1000000)
                    TRACE1(v);
                const auto tit = label_tables.find(v);
                double p_old = (tit == label_tables.end()) ? 0 : (tit->second.get_n() - tit->second.get_m()*a) / (n + b);
                double p_new = base(v) * (m*a + b) / (n + b);

                assert(p_new > 0);

                double sum_p = p_old + p_new;
                return sum_p;
            }

        //! insert() adds a customer to a table, and returns its
        // (predictive) probability
        double insert(const V& v)
            {
                typename V_T::iterator tit = label_tables.find(v);

                // note: ignores (n - b) factor
                double p_old = (tit == label_tables.end()) ? 0 : (tit->second.get_n() - tit->second.get_m()*a);
                double p_new = base(v) * (m*a + b);
                double p = p_old + p_new;  //sgwater: unnormalized prob

                assert(p > 0);
                double r = p*u01();
                if (r <= p_old && tit != label_tables.end())
                {
                    // insert at an old table
                    assert(tit != label_tables.end());
                    tit->second.insert_old(r, a);
                }
                else
                {
                    // insert customer at a new table
                    restaurant& t = (tit == label_tables.end()) ? label_tables[v] : tit->second;
                    t.insert_new();
                    ++m;    // one more table
                    base.insert(v);
                }

                p /= (n+b); // normalize
                ++n;    // one more customer
                return p;
            }

        //! erase() removes a customer at random from a restaurant
        std::size_t erase(const V& v)
            {
                typename V_T::iterator tit = label_tables.find(v);
                assert(tit != label_tables.end());  // we should have tables with this label

                int r = (int) tit->second.get_n() * u01();
                --n;  // one less customer

                if (tit->second.erase(r) == 0)
                {
                    --m;
                    base.erase(v);
                    if (tit->second.is_empty())
                        label_tables.erase(tit);
                }

                return n;
            }

        // Removes a token chosen uniformly at random
        void erase_token_uniform()
            {
                double r = ntokens()*u01();
                for(const auto& item: label_tables)
                {
                    r -= item.second.get_n();
                    if (r < 0)
                    {
                        erase(item.first);
                        return;
                    }
                }
                assert(0);
            }

        // Removes all tokens and tables associated with some word type
        // chosen uniformly at random.  (is there a more efficient
        // implementation?)
        void erase_type_uniform()
            {
                int r = (int)  ntypes()*u01();
                typename V_T::iterator iter = label_tables.begin();
                for (int j = 0; j <r; j++)
                {
                    iter++;
                    assert(iter != label_tables.end());
                }

                auto tokens = ntokens(iter->first);
                if (debug_level >= 15000) TRACE3(r, iter->first, tokens);

                for (std::size_t j = 0; j <tokens; j++)
                {
                    erase(iter->first);
                }
            }

        // Removes all tokens and tables associated with some word type
        // chosen inverse proportional to # of tokens. TODO not efficient.
        void erase_type_proportional()
            {
                //find max table
                double max = 0;
                for(const auto& item: label_tables)
                {
                    if (item.second.get_n() > max) max = item.second.get_n();
                }

                //compute partition func
                double tot = 0;
                for(const auto& item: label_tables)
                {
                    tot += max / item.second.get_n();
                }

                // I r = (I)  ntypes()*(ntokens()-1)*u01();
                double r = tot * u01();
                for(const auto& item: label_tables)
                {
                    // r -= ntokens() - it->second.n;
                    r -= max / item.second.get_n();
                    if (r < 0)
                    {
                        erase(item.first);
                        return;
                    }
                }

                assert(0); //shouldn't get here
            }

        //! empty() returns true if there are no customers in restaurant
        bool empty() const
            {
                assert(m <= n);
                return n == 0;
            }

        //! clear() zeros out this adaptor. You'll need to clear base()
        //! yourself
        void clear()
            {
                m = n = 0;
                label_tables.clear();
            }

        //! logprob() returns the log probability of the table assignment in
        //! the adaptor.  You'll need to compute the base probability
        //! yourself
        double logprob() const
            {
                double logp = 0;
                for(const auto& it0: label_tables)
                {
                    for(const auto& it1: it0.second.get_n_m())
                    {
                        logp += it1.second * (lgamma(it1.first - a) - lgamma(1 - a));
                    }
                }

                if (a > 0)
                    logp += m*log(a) + lgamma(m + b/a) - lgamma(b/a);
                else
                    logp += m*log(b);

                logp -= lgamma(n + b) - lgamma(b);
                return logp;
            }

        //! prints the PY adaptor
        std::wostream& print(std::wostream& os) const
            {
                os << "ntypes=" << ntypes() << ", ";
                os << "n=" << n << ", m=" << m << ", label_tables=";

                char sep = '(';
                std::size_t i=0;
                for(const auto& item: label_tables)
                {
                    os << sep << i << ": " << item.first << '=';
                    item.second.print(os);
                    sep = ',';
                    i++;
                }

                return os << "))";
            }

        //! sanity_check() ensures that all of the numbers in the adaptor
        //! add up
        bool sanity_check() const
            {
                assert(b >= 0);
                assert(a <= 1 && a >= 0);
                assert(m <= n);

                std::size_t nn = 0, mm = 0;
                bool sane_tables = true;
                for(const auto& item: label_tables)
                {
                    nn += item.second.get_n();
                    mm += item.second.get_m();
                    sane_tables = (sane_tables && item.second.sanity_check());
                }

                bool sane_n = (n == nn);
                bool sane_m = (m == mm);
                assert(sane_n);
                assert(sane_m);
                return sane_n && sane_m;
            }
    };


    template <typename Base>
    std::wostream& operator<< (std::wostream& os, const adaptor<Base>& py) {
        return py.print(os);
    }
}

#endif // _PY_ADAPTER_HPP
