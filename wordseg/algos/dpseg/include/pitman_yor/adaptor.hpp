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


#include <algorithm>
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

        adaptor(Base& base, uniform01_type& u01, double pya, double pyb)
            : m_base(base), u01(u01), m_pya(pya), m_pyb(pyb), m_ntables(0), m_ntokens(0)
            {}

    public:
        double& pya()
            {
                return m_pya;
            }

        double& pyb()
            {
                return m_pyb;
            }

        std::size_t ntypes() const
            {
                return m_label_tables.size();
            }

        std::size_t ntables() const
            {
                return m_ntables;
            }

        std::size_t ntokens() const
            {
                return m_ntokens;
            }

        std::size_t ntokens(const argument_type& v) const
            {
                auto tit = m_label_tables.find(v);
                if (tit == m_label_tables.end())
                {
                    return 0;
                }
                else
                {
                    tit->second.get_n();
                }
            }

        const Base& base_dist() const
            {
                return m_base;
            }

        Base& base_dist()
            {
                return m_base;
            }

        //! operator() returns the approximate probability for inserting
        //! v, with context
        double operator() (const argument_type& v) const
            {
                if (debug_level >= 1000000)
                    TRACE1(v);

                const auto tit = m_label_tables.find(v);
                double p_old = (tit == m_label_tables.end()) ? 0 : (tit->second.get_n() - tit->second.get_m() * m_pya) / (m_ntokens + m_pyb);
                double p_new = m_base(v) * (m_ntables * m_pya + m_pyb) / (m_ntokens + m_pyb);

                assert(p_new > 0);

                double sum_p = p_old + p_new;
                return sum_p;
            }

        // adds a customer to a table, and returns its (predictive)
        // probability
        double insert(const argument_type& v)
            {
                // std::wcout << "adaptor::insert" << std::endl;
                auto tit = m_label_tables.find(v);

                // note: ignores (n - b) factor
                double p_old = 0.0;
                if (tit != m_label_tables.end())
                {
                    p_old = tit->second.get_n() - tit->second.get_m() * m_pya;
                }

                double p_new = m_base(v) * (m_ntables * m_pya + m_pyb);
                double p = p_old + p_new;  // sgwater: unnormalized prob

                assert(p > 0);
                double r = p * u01();
                if (r <= p_old && tit != m_label_tables.end())
                {
                    // insert at an old table
                    assert(tit != m_label_tables.end());
                    tit->second.insert_old(r, m_pya);
                }
                else
                {
                    // insert customer at a new table
                    restaurant& t = (tit == m_label_tables.end()) ? m_label_tables[v] : tit->second;
                    t.insert_new();
                    ++m_ntables;    // one more table
                    m_base.insert(v);
                }

                p /= (m_ntokens + m_pyb); // normalize
                ++m_ntokens;    // one more customer

                // std::wcout << "adaptor::insert done" << std::endl;
                return p;
            }

        // removes a customer at random from a restaurant
        std::size_t erase(const argument_type& v)
            {
                // std::wcout << "adaptor::erase" << std::endl;

                auto tit = m_label_tables.find(v);
                assert(tit != label_tables.end());  // we should have tables with this label

                int r = (int) tit->second.get_n() * u01();

                --m_ntokens;  // one less customer
                if (tit->second.erase(r) == 0)
                {
                    --m_ntables;
                    m_base.erase(v);
                    if (tit->second.is_empty())
                        m_label_tables.erase(tit);
                }

                // std::wcout << "adaptor::erase done" << std::endl;
                return m_ntokens;
            }

        // removes a token chosen uniformly at random
        void erase_token_uniform()
            {
                double r = ntokens()*u01();
                for(const auto& item: m_label_tables)
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
                auto iter = m_label_tables.begin();
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
                for(const auto& item: m_label_tables)
                {
                    if (item.second.get_n() > max) max = item.second.get_n();
                }

                //compute partition func
                double tot = 0;
                for(const auto& item: m_label_tables)
                {
                    tot += max / item.second.get_n();
                }

                // I r = (I)  ntypes()*(ntokens()-1)*u01();
                double r = tot * u01();
                for(const auto& item: m_label_tables)
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
                assert(m_ntables <= m_ntokens);
                return m_ntokens == 0;
            }

        //! clear() zeros out this adaptor. You'll need to clear base()
        //! yourself
        void clear()
            {
                m_ntables = 0;
                m_ntokens = 0;
                m_label_tables.clear();
            }

        //! logprob() returns the log probability of the table assignment in
        //! the adaptor.  You'll need to compute the base probability
        //! yourself
        double logprob() const
            {
                double logp = 0;
                for(const auto& it0: m_label_tables)
                {
                    for(const auto& it1: it0.second.get_n_m())
                    {
                        logp += it1.second * (lgamma(it1.first - m_pya) - lgamma(1 - m_pya));
                    }
                }

                if (m_pya > 0)
                    logp += m_ntables * log(m_pya) + lgamma(m_ntables + m_pyb / m_pya) - lgamma(m_pyb / m_pya);
                else
                    logp += m_ntables * log(m_pyb);

                logp -= lgamma(m_ntokens + m_pyb) - lgamma(m_pyb);
                return logp;
            }

        //! prints the PY adaptor
        std::wostream& print(std::wostream& os) const
            {
                os << "ntypes=" << ntypes() << ", ";
                os << "n=" << m_ntokens << ", m=" << m_ntables << ", label_tables=";

                char sep = '(';
                std::size_t i=0;
                for(const auto& item: m_label_tables)
                {
                    os << sep << i << ": " << item.first << '=';
                    item.second.print(os);
                    sep = ',';
                    i++;
                }

                return os << "))";
            }

        // ensures that all of the numbers in the adaptor add up
        bool sanity_check() const
            {
                std::vector<bool> sane;
                sane.push_back(m_pyb >= 0);
                sane.push_back(m_pya <= 1 && m_pya >= 0);
                sane.push_back(m_ntables <= m_ntokens);

                std::size_t nn = 0, mm = 0;
                for(const auto& item: m_label_tables)
                {
                    nn += item.second.get_n();
                    mm += item.second.get_m();
                    sane.push_back(item.second.sanity_check());
                }

                sane.push_back(m_ntokens == nn);
                sane.push_back(m_ntables == mm);

                return std::all_of(sane.begin(), sane.end(), [](bool b){return b;});
            }

    protected:
        Base& m_base;               // base distribution
        uniform01_type& u01;        // shared random number generator
        double m_pya;               // Pitman-Yor b parameter
        double m_pyb;               // Pitman-Yor b parameter
        std::size_t m_ntables;      // number of occupied tables
        std::size_t m_ntokens;       // number of customers in restaurant
        std::unordered_map<argument_type, restaurant> m_label_tables;

    private:
        // copy is not allowed
        adaptor& operator=(const adaptor& other) {}
        //adaptor(const adaptor& other) {}
    };


    template <typename Base>
    std::wostream& operator<< (std::wostream& os, const adaptor<Base>& py)
    {
        return py.print(os);
    }
}

#endif // _PY_ADAPTER_HPP
