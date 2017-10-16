#ifndef _PYCKY_HH
#define _PYCKY_HH

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <utility>
#include <vector>
#include <unordered_map>

#include "quadmath.hh"
#include "earley.hh"
#include "gammadist.hh"
#include "mt19937ar.hh"
#include "slice-sampler.hh"
#include "symbol.hh"
#include "trie.hpp"
#include "utility.hh"

#include "pycfg_type.hh"


extern int debug;


typedef symbol S;

// readline_symbols() reads all of the symbols on the current line
// into syms
std::istream& readline_symbols(std::istream& is, Ss& syms);


// A default_value_type{} object is used to read an object from a
// stream, assigning a default value if the read fails.  Users should
// not need to construct such objects, but should use the
// default_value() function instead.
template <typename object_type, typename default_type>
struct default_value_type
{
    object_type& object;
    const default_type default_value;

    default_value_type(object_type& object, const default_type default_value)
        : object(object),
          default_value(default_value)
        {}
};

// Reads an object from a stream, assigning a default value if the
// read fails. It returns a default_value_type{} object, which does
// the actual reading.
template <typename object_type, typename default_type>
default_value_type<object_type,default_type>
default_value(object_type& object, const default_type default_value=default_type())
{
    return default_value_type<object_type,default_type>(
        object, default_value);
}

// Reads default_value_type{} from an input stream.
template <typename object_type, typename default_type>
std::istream& operator>> (
    std::istream& is, default_value_type<object_type, default_type> dv)
{
    if (is)
    {
        if (is >> dv.object)
            ;
        else
        {
            is.clear(is.rdstate() & ~std::ios::failbit);  // clear failbit
            dv.object = dv.default_value;
        }
    }

    return is;
}


inline F random1()
{
    return mt_genrand_res53();
}



static const F unaryclosetolerance = 1e-7;

class pycky {

public:

    const pycfg_type& g;
    F anneal;         // annealing factor (1 = no annealing)

    pycky(const pycfg_type& g, F anneal=1) : g(g), anneal(anneal) { }

    typedef pycfg_type::tree tree;
    typedef pycfg_type::size_t U;
    typedef pycfg_type::S_S_F S_S_F;
    typedef pycfg_type::St_S_F St_S_F;
    typedef pycfg_type::Stit Stit;

    typedef std::vector<S_F> S_Fs;
    // typedef ext::hash_map<Stit,F> Stit_F;
    typedef std::unordered_map<Stit,F> Stit_F;
    typedef std::vector<Stit_F> Stit_Fs;

    typedef pycfg_type::sT sT;

    typedef pycfg_type::St_sT St_sT;
    typedef St_sT::const_iterator StsTit;
    typedef std::vector<StsTit> StsTits;

    //! index() returns the location of cell in cells[]
    //
    static U index(U i, U j) { return j*(j-1)/2+i; }

    //! ncells() returns the number of cells required for sentence of length n
    //
    static U ncells(U n) { return n*(n+1)/2; }

    Ss terminals;
    S_Fs inactives;
    Stit_Fs actives;
    StsTits pytits;

    typedef std::set<S> sS;
    typedef std::vector<sS> sSs;
    sSs predicteds;

    //! inside() constructs the inside table, and returns the probability
    //! of the start symbol rewriting to the terminals.
    //
    template <typename terminals_type>
    F inside(const terminals_type& terminals0, S start) {

        terminals = terminals0;

        if (debug >= 10000)
            std::cerr << "# cky::inside() terminals = " << terminals << std::endl;

        U n = terminals.size();

        if (g.predictive_parse_filter) {
            earley(g.predictive_parse_filter_grammar, start, terminals, predicteds);
            if (!predicteds[index(0,n)].count(start))
                std::cerr << "## " << HERE << " Error: earley parse failed, terminals = "
                          << terminals << std::endl << exit_failure;
        }

        inactives.clear();
        inactives.resize(ncells(n));
        actives.clear();
        actives.resize(ncells(n));
        pytits.clear();
        pytits.resize(ncells(n));

#ifdef _OPENMP
#pragma omp parallel for
#endif
        for (U i = 0; i < n; ++i) {   // terminals
            pytits[index(i, i+1)] = g.terms_pytrees.find1(terminals[i]);  // PY cache
            inactives[index(i,i+1)][terminals[i]] = 1;
            StsTit& pytit = pytits[index(i,i+1)];
            if (pytit != g.terms_pytrees.end())
                add_pycache(pytit->data, inactives[index(i,i+1)]);
            inside_unaryclose(inactives[index(i,i+1)], actives[index(i,i+1)],
                              g.predictive_parse_filter ? &predicteds[index(i,i+1)] : NULL);

            if (debug >= 20000)
                std::cerr << "# cky::inside() inactives[" << i << "," << i+1 << "] = "
                          << inactives[index(i,i+1)] << std::endl;
            if (debug >= 20100)
                std::cerr << "# cky::inside() actives[" << i << "," << i+1 << "] = "
                          << actives[index(i,i+1)] << std::endl;

            if (debug >= 20100) {
                std::cerr << "# cky::inside() pytits[" << i << "," << i+1 << "] = ";
                if (pytits[index(i, i+1)] == g.terms_pytrees.end())
                    std::cerr << "()" << std::endl;
                else
                    std::cerr << pytits[index(i, i+1)]->data << std::endl;
            }
        }

        for (U gap = 2; gap <= n; ++gap) // non-terminals
#ifdef _OPENMP
#pragma omp parallel for
#endif
            for (U left = 0; left <= n-gap; ++left) {
                U right = left + gap;
                sS* predictedparents = g.predictive_parse_filter ?
                    &predicteds[index(left,right)] : NULL;
                const StsTit& pytit0 = pytits[index(left, right-1)];
                StsTit& pytit = pytits[index(left, right)];
                if (pytit0 == g.terms_pytrees.end())
                    pytit = g.terms_pytrees.end();
                else
                    pytit = pytit0->find1(terminals[right-1]);
                S_F& parentinactives = inactives[index(left,right)];
                Stit_F& parentactives = actives[index(left,right)];
                for (U mid = left+1; mid < right; ++mid) {
                    const S_F& rightinactives = inactives[index(mid,right)];
                    if (rightinactives.empty())
                        continue;
                    Stit_F& leftactives = actives[index(left,mid)];
                    cforeach (Stit_F, itleft, leftactives) {
                        const Stit leftactive = itleft->first;
                        const F leftprob = itleft->second;
                        cforeach (S_F, itright, rightinactives) {
                            S rightinactive = itright->first;
                            const F rightprob = itright->second;
                            const Stit parentactive = leftactive->find1(rightinactive);
                            if (parentactive != leftactive->end()) {
                                F leftrightprob = leftprob * rightprob;
                                cforeach (S_F, itparent, parentactive->data) {
                                    S parent = itparent->first;
                                    if (g.predictive_parse_filter
                                        && !predictedparents->count(parent))
                                        continue;
                                    parentinactives[parent] += leftrightprob
                                        * std::pow(itparent->second/afind(g.parent_weight, parent), anneal);
                                }
                                if (!parentactive->key_trie.empty())
                                    parentactives[parentactive] += leftrightprob;
                            }
                        }
                    }
                }
                // PY correction
                foreach (S_F, it, parentinactives) {
                    F pya = g.get_pya(it->first);    // PY cache statistics
                    if (pya == 1.0)
                        continue;
                    F pyb = g.get_pyb(it->first);
                    U pym = dfind(g.parent_pym, it->first);
                    U pyn = dfind(g.parent_pyn, it->first);
                    it->second *= std::pow( (pym*pya + pyb)/(pyn + pyb), anneal);
                }
                if (pytit != g.terms_pytrees.end())
                    add_pycache(pytit->data, parentinactives);
                inside_unaryclose(parentinactives, parentactives, predictedparents);
                if (debug >= 20000)
                    std::cerr << "# cky::inside() inactives[" << left << "," << right
                              << "] = " << parentinactives << std::endl;
                if (debug >= 20100)
                    std::cerr << "# cky::inside() actives[" << left << "," << right << "] = "
                              << parentactives << std::endl;
                if (debug >= 20100) {
                    std::cerr << "# cky::inside() pytits[" << left << "," << right << "] = ";
                    if (pytits[index(left, right)] == g.terms_pytrees.end())
                        std::cerr << "()" << std::endl;
                    else
                        std::cerr << pytits[index(left, right)]->data << std::endl;
                }
            }
        return dfind(inactives[index(0,n)], start);
    }  // pycky::inside()

    template <typename terminals_type>
    F inside(const terminals_type& terminals) {
        return inside(terminals, g.start);
    }

    void add_pycache(const sT& tps, S_F& inactives) const {
        cforeach (sT, it, tps) {
            symbol cat = (*it)->label();
            F pya = g.get_pya(cat);    // PY cache statistics
            if (pya == 1.0)
                continue;
            F pyb = g.get_pyb(cat);
            U pyn = dfind(g.parent_pyn, cat);
            inactives[cat] += std::pow( ((*it)->count() - pya)/(pyn + pyb), anneal);
        }
    }  // pycky::add_cache()

    void inside_unaryclose(S_F& inactives, Stit_F& actives, const sS* predictedparents) const {
        F delta = 1;
        S_F delta_prob1 = inactives;
        S_F delta_prob0;
        while (delta > unaryclosetolerance) {
            delta = 0;
            delta_prob0 = delta_prob1;
            // delta_prob0.swap(delta_prob1);
            delta_prob1.clear();
            cforeach (S_F, it0, delta_prob0) {
                S child = it0->first;
                S_S_F::const_iterator it = g.unarychild_parent_weight.find(child);
                if (it != g.unarychild_parent_weight.end()) {
                    const S_F& parent_weight = it->second;
                    cforeach (S_F, it1, parent_weight) {
                        S parent = it1->first;
                        if (g.predictive_parse_filter
                            && !predictedparents->count(parent))
                            continue;
                        F prob = it0->second;
                        F pya = g.get_pya(parent);
                        if (pya == 1)
                            prob *= std::pow(it1->second/afind(g.parent_weight, parent),
                                          anneal);
                        else {
                            F pyb = g.get_pyb(parent);
                            U pym = dfind(g.parent_pym, parent);
                            U pyn = dfind(g.parent_pyn, parent);
                            prob *= std::pow(it1->second/afind(g.parent_weight, parent)
                                          * (pym*pya + pyb)/(pyn + pyb),
                                          anneal);
                        }
                        delta_prob1[parent] += prob;
                        delta = std::max(delta, prob/(inactives[parent] += prob));
                    }
                }
            }
        }
        cforeach (S_F, it0, inactives) {
            Stit it1 = g.rhs_parent_weight.find1(it0->first);
            if (it1 != g.rhs_parent_weight.end())
                actives[it1] += it0->second;
        }
    } // pycky::inside_unaryclose()


    //! random_tree() returns a random parse tree for terminals
    //
    tree* random_tree(S s) {
        U n = terminals.size();
        return random_inactive(s, afind(inactives[index(0, n)], s), 0, n);
    }  // pycky::random_tree

    tree* random_tree() { return random_tree(g.start); }

    //! random_inactive() returns a random expansion for an inactive edge
    //
    tree* random_inactive(const S parent, F parentprob,
                          const U left, const U right) const {

        if (left+1 == right && parent == terminals[left])
            return new tree(parent);

        F probthreshold = parentprob * random1();
        F probsofar = 0;
        F pya = g.get_pya(parent);
        F rulefactor = 1;

        if (pya != 1) {

            // get tree from cache

            F pyb = g.get_pyb(parent);
            U pyn = dfind(g.parent_pyn, parent);
            const StsTit& pytit = pytits[index(left, right)];
            if (pytit != g.terms_pytrees.end())
                cforeach (sT, it, pytit->data) {
                    if ((*it)->label() != parent)
                        continue;
                    probsofar += std::pow( ((*it)->count() - pya)/(pyn + pyb), anneal);
                    if (probsofar >= probthreshold)
                        return *it;
                }
            U pym = dfind(g.parent_pym, parent);
            rulefactor = (pym*pya + pyb)/(pyn + pyb);
        }

        // tree won't come from cache, so cons up new node

        tree* tp = new tree(parent);
        rulefactor /=  afind(g.parent_weight, parent);
        const S_F& parentinactives = inactives[index(left, right)];

        // try unary rules

        cforeach (S_F, it0, parentinactives) {
            S child = it0->first;
            F childprob = it0->second;
            S_S_F::const_iterator it1 = g.unarychild_parent_weight.find(child);
            if (it1 != g.unarychild_parent_weight.end()) {
                const S_F& parent1_weight = it1->second;
                probsofar += childprob
                    * std::pow(dfind(parent1_weight, parent)*rulefactor, anneal);
                if (probsofar >= probthreshold) {
                    tp->add_child(random_inactive(child, childprob, left, right));
                    return tp;
                }
            }
        }

        // try binary rules
        for (U mid = left+1; mid < right; ++mid) {
            const Stit_F& leftactives = actives[index(left,mid)];
            const S_F& rightinactives = inactives[index(mid,right)];
            cforeach (Stit_F, itleft, leftactives) {
                const Stit leftactive = itleft->first;
                const F leftprob = itleft->second;
                cforeach (S_F, itright, rightinactives) {
                    S rightinactive = itright->first;
                    const F rightprob = itright->second;
                    const Stit parentactive = leftactive->find1(rightinactive);
                    if (parentactive != leftactive->end()) {
                        S_F::const_iterator it = parentactive->data.find(parent);
                        if (it != parentactive->data.end()) {
                            probsofar += leftprob * rightprob
                                * std::pow(it->second*rulefactor, anneal);
                            if (probsofar >= probthreshold) {
                                random_active(leftactive, leftprob, left, mid, tp->children());
                                tp->add_child(random_inactive(rightinactive, rightprob, mid, right));
                                return tp;
                            }
                        }
                    }
                }
            }
        }

        std::cerr << "\n## Error in pycky::random_inactive(), parent = " << parent
                  << ", left = " << left << ", right = " << right
                  << ", probsofar = " << probsofar
                  << " still below probthreshold = " << probthreshold
                  << std::endl;
        return tp;
    }  // pycky::random_inactive()

    void random_active(const Stit parent, F parentprob, const U left, const U right,
                       tree::children_list_type& siblings) const {
        F probthreshold = random1() * parentprob;
        F probsofar = 0;

        // unary rule

        const S_F& parentinactives = inactives[index(left, right)];
        cforeach (S_F, it, parentinactives)
            if (g.rhs_parent_weight.find1(it->first) == parent) {
                probsofar += it->second;
                if (probsofar >= probthreshold) {
                    siblings.push_back(random_inactive(it->first, it->second, left, right));
                    return;
                }
                break;  // only one unary child can possibly generate this parent
            }

        // binary rules

        for (U mid = left + 1; mid < right; ++mid) {
            const Stit_F& leftactives = actives[index(left,mid)];
            const S_F& rightinactives = inactives[index(mid,right)];
            cforeach (Stit_F, itleft, leftactives) {
                const Stit leftactive = itleft->first;
                const F leftprob = itleft->second;
                cforeach (S_F, itright, rightinactives) {
                    S rightinactive = itright->first;
                    const F rightprob = itright->second;
                    if (parent == leftactive->find1(rightinactive)) {
                        probsofar += leftprob * rightprob;
                        if (probsofar >= probthreshold) {
                            random_active(leftactive, leftprob, left, mid, siblings);
                            siblings.push_back(random_inactive(rightinactive, rightprob, mid, right));
                            return;
                        }
                    }
                }
            }
        }

        std::cerr << "## Error in pycky::random_active(), parent = " << parent
                  << ", left = " << left << ", right = " << right
                  << ", probsofar = " << probsofar << ", probthreshold = " << probthreshold
                  << std::endl;
        return;
    }  // pycky::random_active()

}; // pycky{}

struct resample_pycache_helper {
    typedef catcount_tree tree;

    pycfg_type& g;
    pycky& p;

    resample_pycache_helper(pycfg_type& g, pycky& p) : g(g), p(p) { }

    template <typename Words, typename TreePtrs>
    void operator() (const Words& words, TreePtrs& tps) {

        foreach (typename TreePtrs, tit, tps) {
            tree* tp0 = *tit;
            Ss words;
            tp0->terminals(words);
            S start = tp0->label();
            F old_pya = g.set_pya(start, 1.0);
            F pi0 = g.decrtree(tp0);
            if (pi0 < 0)
                std::cerr << "## pi0 = " << pi0 << ", tp0 = " << tp0 << std::endl;
            assert(pi0 >= 0);
            F r0 = g.tree_prob(tp0);
            assert(r0 >= 0);

            F tprob = p.inside(words, start);   // parse string
            if (tprob <= 0)
                std::cerr << "## Error in resample_pycache(): words = " << words << ", tprob = " << tprob
                          << ", tp0 = " << tp0 << std::endl
                          << "## g = " << g << std::endl;
            assert(tprob >= 0);
            tree* tp1 = p.random_tree(start);
            F r1 = g.tree_prob(tp1);
            assert(r1 >= 0);

            if (tp0->generalize() == tp1->generalize()) {  // ignore top count
                g.incrtree(tp0);
                tp1->selective_delete();
            }
            else {  // *tp1 != *tp0, do acceptance rejection
                F pi1 = g.incrtree(tp1);
                F pi1r0 = pi1 * r0;
                F pi0r1 = pi0 * r1;
                F accept = (pi0r1 > 0) ? std::pow(pi1r0/pi0r1, p.anneal) : 2.0; // accept if there has been an underflow
                if (random1() <= accept) {
                    tp0->generalize().swap(tp1->generalize());  // don't swap top counts
                    tp1->selective_delete();
                }
                else {  // don't accept
                    g.decrtree(tp1);
                    g.incrtree(tp0);
                    tp1->selective_delete();
                }
            }

            g.set_pya(tp0->label(), old_pya);
        }
    }
};


// resamples the strings associated with each cache
inline void resample_pycache(pycfg_type& g, pycky& p) {
    resample_pycache_helper h(g, p);
    p.g.terms_pytrees.for_each(h);
}

#endif // PYCKY_H
