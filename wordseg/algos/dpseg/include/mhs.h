#ifndef MHS_H
#define MHS_H

//! \file
//!
//! mhs.h defines the Pitman-Yor adaptor class
//!
//! A sampler needs to implement the following API (where F is a floating-point type
//! and U is an unsigned int type)
//!
//! typedef V argument_type               -- type of objects over which labels are defined
//!
//! typedef S state_type                  -- type of hidden state information
//!
//! F operator()(const V& v) const        -- returns proposal prob of generating a v,
//!                                          summing over all possible states
//!
//! F insert(const V& v)                  -- inserts v
//!
//! F erase(const V& v)                   -- erases v
//!
//! F insert(const V& v, S& s)            -- inserts (v,s), returns exact prob of (v,s)
//!
//! F erase(const V& v, S& s)             -- removes (v,s), returns exact prob of insert(v,s)
//!

#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <functional>
#include <unordered_map>
#include <utility>


#include "random-mt19937ar.h"
#include "util.h"

typedef int I;           //!< 32-bit signed integers
typedef unsigned int U;  //!< 32-bit unsigned integers
typedef double F;        //!< floating-point numbers
extern U debug_level;

// Note that in this class, both the base distribution and the
// parameters are actually reference variables, which means they must
// exist *outside* the class.  For base distrib, this is so that
// hierarchical models can share/modify a single base distrib.  For
// parms, this is because constants are all stored in a single class,
// and we will modify the values in that class (which is accessible to
// more parts of the code).
template <typename Base>
class PYAdaptor
{
public:
    typedef typename Base::argument_type argument_type;
    typedef F result_type;

    //public functions are defined after defining private table class
    //below.

protected:
    Base& base;           //!< base distribution
    uniform01_type<F>& u01;  //!< shared random number generator
    F a;                  //!< Pitman-Yor b parameter
    F b;                  //!< Pitman-Yor b parameter
    U m;                  //!< number of occupied tables
    U n;                  //!< number of customers in restaurant

    typedef argument_type V;
    typedef std::map<U,U> U_U;

    struct T
    {
        U n;      //!< total number of customers at tables with this label
        U m;      //!< number of tables with this label
        U_U n_m;  //!< number of customers at table -> number of tables

        T() : n(), m() {}

        //! insert_old() inserts a customer at a random old table
        //! using PY sampling distribution
        //
        void insert_old(F r, F a) {
            // when r is not positive, we have reached our table
            for (U_U::iterator it = n_m.begin(); it != n_m.end(); ++it)
            {
                if ((r -= it->second * (it->first - a)) <= 0)
                {
                    U n1 = it->first+1;  // new table size
                    if (--it->second == 0)
                        n_m.erase(it);
                    ++n_m[n1];           // add customer to new table
                    break;
                }
            }

            assert(r <= 0);          // check that we actually updated a table
            ++n;                     // increment no of customers with this label
        }

        //! insert_new() inserts a customer at a new table
        void insert_new()
            {
                ++n;
                ++m;
                ++n_m[1];
            }

        //! empty() is true if there are no customers left with this
        //! label
        bool empty() const
            {
                assert(m <= n);
                return n == 0;
            }

        //! erase() removes a customer from a randomly chosen table,
        //!  returns number of customers left at table
        U erase(I r)
            {
                --n;
                for (U_U::iterator it = n_m.begin(); it != n_m.end(); ++it)
                {
                    if ((r -= it->first * it->second) <= 0)
                    {
                        U n1 = it->first-1;  //!< new table size
                        if (--it->second == 0)
                            n_m.erase(it);
                        if (n1 == 0)
                            --m;
                        else
                            ++n_m[n1];
                        return n1;
                    }
                }

                // shouldn't ever get here
                assert(r <= 0);
                return 0;
            }

        //! sanity_check() checks that all of our numbers add up
        bool sanity_check() const
            {
                assert(m > 0);
                assert(n > 0);
                assert(m <= n);

                U mm = 0, nn = 0;
                for(const auto& item: n_m)
                {
                    assert(n > 0);   // shouldn't have any empty tables
                    assert(m > 0);

                    mm += item.second;
                    nn += item.first * item.second;
                }

                bool sane_n = (n == nn);
                bool sane_m = (m == mm);
                assert(sane_n);
                assert(sane_m);
                return sane_n && sane_m;
            }

        std::wostream& print(std::wostream& os) const
            {
                return os << "(n=" << n << ", m=" << m << ", n_m=" << n_m << ")";
            }
    };

    typedef std::unordered_map<V,T> V_T;
    V_T label_tables;

public:
    PYAdaptor(Base& base, uniform01_type<F>& u01, F a, F b)
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
    PYAdaptor& operator=(const PYAdaptor& pya)
        {}

//       PYAdaptor& operator=(const PYAdaptor& pya) {
// 	base = pya.base; u01 = pya.u01;
// 	a = pya.a; b = pya.b;
// 	m = pya.m; n = pya.n;
// 	label_tables = pya.label_tables;
// 	return *this;}

public:
    F& pya()
        {
            return a;
        }

    F& pyb()
        {
            return b;
        }

    U ntypes() const
        {
            return label_tables.size();
        }

    U ntables() const
        {
            return m;
        }

    U ntokens() const
        {
            return n;
        }

    U ntokens(const V& v) const
        {
            typename V_T::const_iterator tit = label_tables.find(v);
            return (tit == label_tables.end()) ? 0 : tit->second.n;
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
    F operator() (const V& v) const
        {
            if (debug_level >= 1000000) TRACE1(v);
            typename V_T::const_iterator tit = label_tables.find(v);
            F p_old = (tit == label_tables.end()) ? 0 : (tit->second.n - tit->second.m*a) / (n + b);
            F p_new = base(v) * (m*a + b) / (n + b);

            assert(p_new > 0);

            F sum_p = p_old + p_new;
            return sum_p;
        }

    //! insert() adds a customer to a table, and returns its
    // (predictive) probability
    F insert(const V& v)
        {
            typename V_T::iterator tit = label_tables.find(v);

            // note: ignores (n - b) factor
            F p_old = (tit == label_tables.end()) ? 0 : (tit->second.n - tit->second.m*a);
            F p_new = base(v) * (m*a + b);
            F p = p_old + p_new;  //sgwater: unnormalized prob

            assert(p > 0);
            F r = p*u01();
            if (r <= p_old && tit != label_tables.end())
            {
                // insert at an old table
                assert(tit != label_tables.end());
                tit->second.insert_old(r, a);
            }
            else
            {
                // insert customer at a new table
                T& t = (tit == label_tables.end()) ? label_tables[v] : tit->second;
                t.insert_new();
                ++m;    // one more table
                base.insert(v);
            }

            p /= (n+b); // normalize
            ++n;    // one more customer
            return p;
        }

    //! erase() removes a customer at random from a restaurant
    U erase(const V& v)
        {
            typename V_T::iterator tit = label_tables.find(v);
            assert(tit != label_tables.end());  // we should have tables with this label
            I r = (I) tit->second.n*u01();
            --n;  // one less customer

            if (tit->second.erase(r) == 0)
            {
                --m;
                base.erase(v);
                if (tit->second.empty())
                    label_tables.erase(tit);
            }

            return n;
        }

    // Removes a token chosen uniformly at random
    void erase_token_uniform()
        {
            F r = ntokens()*u01();
            for(const auto& item: label_tables)
            {
                r -= item.second.n;
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
            I r = (I)  ntypes()*u01();
            typename V_T::iterator iter = label_tables.begin();
            for (I j = 0; j <r; j++)
            {
                iter++;
                assert(iter != label_tables.end());
            }

            U tokens = ntokens(iter->first);
            if (debug_level >= 15000) TRACE3(r, iter->first, tokens);

            for (U j = 0; j <tokens; j++)
            {
                erase(iter->first);
            }
        }

    // Removes all tokens and tables associated with some word type
    // chosen inverse proportional to # of tokens. TODO not efficient.
    void erase_type_proportional()
        {
            //find max table
            F max = 0;
            for(const auto& item: label_tables)
            {
                if (item.second.n > max) max = item.second.n;
            }

            //compute partition func
            F tot = 0;
            for(const auto& item: label_tables)
            {
                tot += max / item.second.n;
            }

            // I r = (I)  ntypes()*(ntokens()-1)*u01();
            F r = tot * u01();
            for(const auto& item: label_tables)
            {
                // r -= ntokens() - it->second.n;
                r -= max / item.second.n;
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
    F logprob() const
        {
            F logp = 0;
            for(const auto& it0: label_tables)
            {
                for(const auto& it1: it0.second.n_m)
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
            U i=0;
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

            U nn = 0, mm = 0;
            bool sane_tables = true;
            for(const auto& item: label_tables)
            {
                nn += item.second.n;
                mm += item.second.m;
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
std::wostream& operator<< (std::wostream& os, const PYAdaptor<Base>& py) {
    return py.print(os);
}

#endif // MHS_H
