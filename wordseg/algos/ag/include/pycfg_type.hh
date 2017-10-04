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

#ifndef _PYCFG_TYPE_HH
#define _PYCFG_TYPE_HH

#include <math.h>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "quadmath.hh"
#include "symbol.hh"
#include "earley.hh"
#include "catcount_tree.hh"
#include "trie.hpp"



//! Suppose there are n samples occupying m tables.
//! Then the probability that the n+1 sample occupies
//! table 1 <= k <= m is:
//!
//!  P(x_{n+1} = k) = (n_k - a)/(n + b)
//!
//! and the probability that the n+1 sample occupies
//! the new table m+1 is:
//!
//!  P(x_{n+1} = m+1) = (m*a + b)/(n + b)
//!
//! The probability of a configuration in which a
//! restaurant contains n customers at m tables,
//! with n_k customers at table k is:
//!
//!
//!  a^{-m} G(m+b/a)  G(b)                 G(n_k-a)
//!         -------- ------  \prod_{k=1}^m --------
//!          G(b/a)  G(n+b)                 G(1-a)
//!
//! where G is the Gamma function.

inline float power(float x, float y) { return y == 1 ? x : powf(x, y); }
inline double power(double x, double y) { return y == 1 ? x : pow(x, y); }


//typedef symbol S;
typedef std::vector<symbol> Ss;
typedef std::map<symbol,F> S_F;
typedef std::pair<symbol,Ss> SSs;
typedef std::map<SSs,F> SSs_F;



// A pycfg_type is a CKY parser for a py-cfg
class pycfg_type
{
public:
    using size_t = std::size_t;

    template<class value_type>
    using symbol_map = std::map<symbol, value_type>;

    typedef std::unordered_map<symbol, S_F> S_S_F;
    typedef trie<symbol, symbol_map<F> > St_S_F;
    typedef St_S_F::const_iterator Stit;
    typedef catcount_tree tree;
    typedef std::set<tree*> sT;
    typedef trie<symbol,sT> St_sT;
    typedef std::vector<tree*> Ts;
    typedef std::map<symbol,Ts> S_Ts;

    pycfg_type();

    virtual ~pycfg_type();

    // If estimate_theta_flag is true, then we estimate the generator
    // rule weights using a Dirichlet prior
    bool estimate_theta_flag;

    // If predictive_parse_filter is true, then first do a deterministic
    // Earley parse of each sentence and use this to filter the nondeterministic
    // CKY parses
    bool predictive_parse_filter;

    // predictive_parse_filter_grammar is the grammar used by the Earley parser
    earley::grammar predictive_parse_filter_grammar;

    // start is the start symbol of the grammar
    symbol start;

    // rhs_parent_weight maps the right-hand sides of rules
    // to rule parent and rule weight
    St_S_F rhs_parent_weight;

    // unarychild_parent_weight maps unary children to a vector
    // of parent-weight pairs
    S_S_F unarychild_parent_weight;

    // parent_weight maps parents to the sum of their rule weights
    symbol_map<F> parent_weight;

    // default_weight is the default weight for rules with no explicit
    // weight.  Used when grammar is read in.
    F default_weight;

    // rule_priorweight is the prior weight of rule
    SSs_F rule_priorweight;

    // parent_priorweight is the prior weight the parent
    S_F parent_priorweight;

    // terms_pytrees maps terminal strings to their PY trees
    St_sT terms_pytrees;

    // parent_pyn maps parents to the number of times they have been expanded
    symbol_map<std::size_t> parent_pyn;

    // parent_pym maps parents to the number of distinct PY tables for parent
    symbol_map<std::size_t> parent_pym;

    F default_pya;   // default value for pya
    F default_pyb;   // default value for pyb

    F pya_beta_a;    // alpha parameter of Beta prior on pya
    F pya_beta_b;    // beta parameter of Beta prior on pya

    F pyb_gamma_s;   // s parameter of Gamma prior on pyb
    F pyb_gamma_c;   // c parameter of Gamma prior on pyb

    symbol_map<F> parent_pya;  // pya value for parent
    symbol_map<F> parent_pyb;  // pyb value for parent

    // returns the value of pya for this parent
    F get_pya(symbol parent) const;

    // set_pya() sets the value of pya for this parent, returning
    // the old value for pya
    F set_pya(symbol parent, F pya);

    // get_pyb() returns the value of pyb for this parent
    F get_pyb(symbol parent) const;

    // sum_pym() returns the sum of the pym for all parents
    std::size_t sum_pym() const;

    // terms_pytrees_size() returns the number of trees in terms_pytrees
    std::size_t terms_pytrees_size() const;

    struct terms_pytrees_size_helper
    {
        std::size_t& size;
        terms_pytrees_size_helper(std::size_t& size) : size(size) {}

        template <typename Words, typename TreePtrs>
        void operator() (const Words& words, const TreePtrs& tps)
            {
                size += tps.size();
            }
    };

    // rule_weight() returns the weight of rule parent --> rhs
    template <typename rhs_type>
    F rule_weight(symbol parent, const rhs_type& rhs) const
        {
            assert(!rhs.empty());
            if (rhs.size() == 1)
            {
                S_S_F::const_iterator it = unarychild_parent_weight.find(rhs[0]);
                if (it == unarychild_parent_weight.end())
                    return 0;
                else
                    return dfind(it->second, parent);
            }
            else
            {   // rhs.size() > 1
                Stit it = rhs_parent_weight.find(rhs);
                if (it == rhs_parent_weight.end())
                    return 0;
                else
                    return dfind(it->data, parent);
            }
        }

    // rule_prob() returns the probability of rule parent --> rhs
    template <typename rhs_type>
    F rule_prob(symbol parent, const rhs_type& rhs) const
        {
            assert(!rhs.empty());
            F parentweight = afind(parent_weight, parent);
            F ruleweight = rule_weight(parent, rhs);
            assert(ruleweight > 0);
            assert(parentweight > 0);
            return ruleweight/parentweight;
        }

    // tree_prob() returns the probability of the tree under the
    // current model
    F tree_prob(const tree* tp) const;

    // incrrule() increments the weight of the rule parent --> rhs,
    // returning the probability of this rule under the old grammar.
    template <typename rhs_type>
    F incrrule(symbol parent, const rhs_type& rhs, F weight = 1)
        {
            assert(!rhs.empty());
            assert(weight >= 0);
            F& parentweight = parent_weight[parent];
            F parentweight0 = parentweight;
            F rhsweight0;
            parentweight += weight;

            if (rhs.size() == 1)
            {
                F& rhsweight = unarychild_parent_weight[rhs[0]][parent];
                rhsweight0 = rhsweight;
                rhsweight += weight;
            }
            else
            {   // rhs.size() > 1
                F& rhsweight = rhs_parent_weight[rhs][parent];
                rhsweight0 = rhsweight;
                rhsweight += weight;
            }

            assert(parentweight0 >= 0);
            assert(rhsweight0 >= 0);

            return rhsweight0/parentweight0;
        }

    // decrrule() decrements the weight of rule parent --> rhs,
    // returning the probability of this rule under the new grammar,
    // and deletes the rule if it has weight 0.
    template <typename rhs_type>
    F decrrule(symbol parent, const rhs_type& rhs, F weight = 1)
        {
            assert(weight >= 0);
            assert(!rhs.empty());

            F rhsweight;
            F parentweight = (parent_weight[parent] -= weight);
            assert(parentweight >= 0);
            if (parentweight == 0)
                parent_weight.erase(parent);

            if (rhs.size() == 1)
            {
                auto& parent1_weight = unarychild_parent_weight[rhs[0]];
                rhsweight = (parent1_weight[parent] -= weight);
                assert(rhsweight >= 0);
                if (rhsweight == 0)
                {
                    parent1_weight.erase(parent);
                    if (parent1_weight.empty())
                        unarychild_parent_weight.erase(rhs[0]);
                }
            }
            else
            {  // non-unary rule
                auto& parent1_weight = rhs_parent_weight[rhs];
                rhsweight = (parent1_weight[parent] -= weight);
                if (rhsweight == 0)
                {
                    parent1_weight.erase(parent);
                    if (parent1_weight.empty())
                        rhs_parent_weight.erase(rhs);
                }
            }

            return rhsweight / parentweight;
        }

    // incrtree() increments the cache for tp, increments
    // the rules if the cache count is appropriate, and returns
    // the probability of this tree under the original model.
    F incrtree(tree* tp, std::size_t weight = 1);

    // decrtree() decrements the cache for tp, decrements
    // the rules if the cache count is appropriate, and returns
    // the probability of this tree under the new model.
    F decrtree(tree* tp, std::size_t weight = 1);

    // read() reads a grammar from an input stream (implements >> )
    std::istream& read(std::istream& is);

    // write() writes a grammar (implements << )
    std::ostream& write(std::ostream& os) const;

    std::ostream& write_rules(std::ostream& os, symbol parent) const;

    // write_rule{} writes a single rule
    struct write_rule
    {
        std::ostream& os;
        symbol parent;

        write_rule(std::ostream& os, symbol parent)
            : os(os), parent(parent) {}

        template <typename Keys, typename Value>
        void operator() (const Keys& rhs, const Value& parentweights)
            {
                for (const auto& pwit: parentweights)
                    if (pwit.first == parent)
                    {
                        os << pwit.second << '\t' << parent << " -->";
                        for (const auto& rhsit: rhs)
                            os << ' ' << rhsit;
                        os << std::endl;
                    }
            }
    };

    // write_pycache{} writes the cache entries for a category
    struct write_pycache {
        std::ostream& os;
        symbol parent;

        write_pycache(std::ostream& os, symbol parent)
            : os(os), parent(parent) { }

        template <typename Words, typename TreePtrs>
        void operator() (const Words& words, const TreePtrs& tps)
            {
                for (const auto& tpit: tps)
                    if (tpit->label() == parent)
                        os << tpit << std::endl;
            }
    };

    // logPcorpus() returns the log probability of the corpus trees
    F logPcorpus() const;

    struct logPcache
    {
        const pycfg_type& g;
        F& logP;

        logPcache(const pycfg_type& g, F& logP)
            : g(g), logP(logP) { }

        template <typename Words, typename TreePtrs>
        void operator() (const Words& words, const TreePtrs& tps)
            {
                for (const auto& it: tps)
                {
                    F pya = g.get_pya(it->label());
                    logP += std::lgamma(it->count() - pya) - std::lgamma(static_cast<double>(1 - pya));
                }
            }
    };

    // logPrior() returns the prior probability of the PY a and b values
    F logPrior() const;

    // pya_logPrior() calculates the Beta prior on pya.
    static F pya_logPrior(F pya, F pya_beta_a, F pya_beta_b);

    // pyb_logPrior() calculates the prior probability of pyb
    // wrt the Gamma prior on pyb.
    static F pyb_logPrior(F pyb, F pyb_gamma_c, F pyb_gamma_s);

    // resample_pyb_type{} is a function object that returns the part
    // of log prob that depends on pyb.  This includes the Gamma prior
    // on pyb, but doesn't include e.g. the rule probabilities (as
    // these are a constant factor)
    struct resample_pyb_type;

    // resample_pya_type{} calculates the part of the log prob that
    // depends on pya.  This includes the Beta prior on pya, but
    // doesn't include e.g. the rule probabilities (as these are a
    // constant factor)
    struct resample_pya_type;

    // resample_pyb() samples new values for pyb for each adapted
    // nonterminal.
    void resample_pyb();

    // resample_pya() samples new values for pya for each adapted
    // nonterminal
    void resample_pya(const S_Ts& parent_trees);

    // resample_pyab_parent_trees_helper{} constructs parent_trees
    // from terms_pytrees
    struct resample_pyab_parent_trees_helper
    {
        S_Ts& parent_trees;
        resample_pyab_parent_trees_helper(S_Ts& parent_trees)
            : parent_trees(parent_trees) { }

        template <typename Words, typename TreePtrs>
        void operator() (const Words& words, const TreePtrs& tps)
            {
                for (const auto& it: tps)
                    parent_trees[it->label()].push_back(it);
            }
    };

    // resample_pyab() resamples both pya and pyb for each adapted
    // nonterminal
    void resample_pyab();

    // write_adaptor_parameters() writes out adaptor parameters to file
    std::ostream& write_adaptor_parameters(std::ostream& os) const;

    // initialize_predictive_parse_filter() initializes the predictive
    // parse filter by building the grammar that the Earley parser
    // requires
    void initialize_predictive_parse_filter();
};


// operator>> (pycfg_type&) reads a pycfg_type g, setting g.start
// to the parent of the first rule read.
std::istream& operator>> (std::istream& is, pycfg_type& g);

std::ostream& operator<< (std::ostream& os, const pycfg_type& g);


namespace std
{
    // namespace tr1
    // {
        template <> struct hash<pycfg_type::Stit>
            : public std::unary_function<pycfg_type::Stit, std::size_t>
        {
            size_t operator()(const pycfg_type::Stit t) const
                {
                    return size_t(&(*t));
                }
        };
    }
// }


#endif  // _PYCFG_TYPE_HH
