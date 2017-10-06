// py-cky.h
//
// (c) Mark Johnson, 27th January 2006, last modified 7th Jan 2014

#ifndef PY_CKY_H
#define PY_CKY_H

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
// #include <ext/hash_map>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <utility>
#include <vector>
#include <tr1/unordered_map>

#include "earley.h"
#include "gammadist.h"
#include "mt19937ar.h"
#include "slice-sampler.h"
#include "sym.h"
#include "xtree.h"
#include "trie.h"
#include "utility.h"

extern int debug;

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

#ifndef QUADPREC
  typedef double F;
#else
  #include "quadmath.h"
  typedef __float128 F;
  inline __float128 power(__float128 x, __float128 y) { return y == 1 ? x : pow(double(x), double(y)); }
#endif

// inline long double power(long double x, long double y) { return powl(x, y); }

typedef symbol S;
typedef std::vector<S> Ss;

typedef std::map<S,F> S_F;
// typedef tr1::unordered_map<S,F> S_F;

typedef std::pair<S,Ss> SSs;
typedef std::map<SSs,F> SSs_F;

//! readline_symbols() reads all of the symbols on the current
//! line into syms
//
std::istream& readline_symbols(std::istream& is, Ss& syms) {
  syms.clear();
  std::string line;
  if (std::getline(is, line)) {
    std::istringstream iss(line);
    std::string s;
    while (iss >> s)
      syms.push_back(s);
  }
  return is;
}  // readline_symbols()


//! A default_value_type{} object is used to read an object from a stream,
//! assigning a default value if the read fails.  Users should not need to
//! construct such objects, but should use the default_value() function instead.
//
template <typename object_type, typename default_type>
struct default_value_type {
  object_type& object;
  const default_type defaultvalue;
  default_value_type(object_type& object, const default_type defaultvalue)
    : object(object), defaultvalue(defaultvalue) { }
};

//! default_value() is used to read an object from a stream, assigning a
//! default value if the read fails.  It returns a default_value_type{}
//! object, which does the actual reading.
//
template <typename object_type, typename default_type>
default_value_type<object_type,default_type>
default_value(object_type& object, const default_type defaultvalue=default_type()) {
  return default_value_type<object_type,default_type>(object, defaultvalue);
}

//! This operator>>() reads default_value_type{} from an input stream.
//
template <typename object_type, typename default_type>
std::istream& operator>> (std::istream& is, 
			  default_value_type<object_type, default_type> dv) {
  if (is) {
    if (is >> dv.object)
      ;
    else {
      is.clear(is.rdstate() & ~std::ios::failbit);  // clear failbit
      dv.object = dv.defaultvalue;
    }
  }
  return is;
}

// inline F random1() { return rand()/(RAND_MAX+1.0); }
inline F random1() { return mt_genrand_res53(); }


//! A pycfg_type is a CKY parser for a py-cfg
//
struct pycfg_type {

  pycfg_type() 
    : estimate_theta_flag(false), predictive_parse_filter(false), 
      default_weight(1), default_pya(1e-1), default_pyb(1e3),
      pya_beta_a(0), pya_beta_b(0), pyb_gamma_s(0), pyb_gamma_c(0) { }

  typedef unsigned int U;
  typedef std::pair<U,U> UU;

  typedef std::map<S,U> S_U;

  typedef std::map<S,UU> S_UU;

  typedef tr1::unordered_map<S,S_F> S_S_F;

  typedef trie<S, S_F> St_S_F;
  typedef St_S_F::const_iterator Stit;

  typedef catcounttree_type tree;

  typedef std::set<tree*> sT;

  typedef trie<S,sT> St_sT;

  typedef std::vector<tree*> Ts;

  typedef std::map<S,Ts> S_Ts;

  //! If estimate_theta_flag is true, then we estimate the generator 
  //! rule weights using a Dirichlet prior
  //
  bool estimate_theta_flag;

  //! If predictive_parse_filter is true, then first do a deterministic
  //! Earley parse of each sentence and use this to filter the nondeterministic
  //! CKY parses
  //
  bool predictive_parse_filter;

  //! predictive_parse_filter_grammar is the grammar used by the Earley parser
  //
  earley::grammar predictive_parse_filter_grammar;

  //! start is the start symbol of the grammar
  //
  S start;

  //! rhs_parent_weight maps the right-hand sides of rules
  //! to rule parent and rule weight 
  //
  St_S_F rhs_parent_weight;

  //! unarychild_parent_weight maps unary children to a vector
  //! of parent-weight pairs
  //
  S_S_F unarychild_parent_weight;

  //! parent_weight maps parents to the sum of their rule weights
  //
  S_F parent_weight;

  //! default_weight is the default weight for rules with no explicit
  //! weight.  Used when grammar is read in.
  //
  F default_weight;

  //! rule_priorweight is the prior weight of rule
  //
  SSs_F rule_priorweight;

  //! parent_priorweight is the prior weight the parent
  //
  S_F parent_priorweight;

  //! terms_pytrees maps terminal strings to their PY trees
  //
  St_sT terms_pytrees;

  //! parent_pyn maps parents to the number of times they have been expanded
  //
  S_U parent_pyn;

  //! parent_pym maps parents to the number of distinct PY tables for parent
  //
  S_U parent_pym;

  F default_pya;   //!< default value for pya
  F default_pyb;   //!< default value for pyb

  F pya_beta_a;    //!< alpha parameter of Beta prior on pya
  F pya_beta_b;    //!< beta parameter of Beta prior on pya

  F pyb_gamma_s;   //!< s parameter of Gamma prior on pyb
  F pyb_gamma_c;   //!< c parameter of Gamma prior on pyb

  S_F parent_pya;  //!< pya value for parent
  S_F parent_pyb;  //!< pyb value for parent

  //! get_pya() returns the value of pya for this parent
  //
  F get_pya(S parent) const { 
    S_F::const_iterator it = parent_pya.find(parent);
    return (it == parent_pya.end()) ? default_pya : it->second;
  }  // pycfg_type::get_pya()

  //! set_pya() sets the value of pya for this parent, returning
  //! the old value for pya
  //
  F set_pya(S parent, F pya) {
    F old_pya = default_pya;
    S_F::iterator it = parent_pya.find(parent);
    if (it != parent_pya.end())
      old_pya = it->second;
    if (pya != default_pya)
      parent_pya[parent] = pya;
    else // pya == default_pya
      if (it != parent_pya.end())
	parent_pya.erase(it);
    return old_pya;
  }  // pycfg_type::set_pya()

  //! get_pyb() returns the value of pyb for this parent
  //
  F get_pyb(S parent) const { 
    S_F::const_iterator it = parent_pyb.find(parent);
    return (it == parent_pyb.end()) ? default_pyb : it->second;
  }  // pycfg_type::get_pyb()

  //! sum_pym() returns the sum of the pym for all parents
  //
  U sum_pym() const {
    U sum = 0;
    cforeach (S_U, it, parent_pym)
      sum += it->second;
    return sum;
  }  // pycfg_type::sum_pym()

  //! terms_pytrees_size() returns the number of trees in terms_pytrees.
  //
  U terms_pytrees_size() const {
    U size = 0;
    terms_pytrees.for_each(terms_pytrees_size_helper(size));
    return size;
  }  // pycfg_type::terms_pytrees_size()

  struct terms_pytrees_size_helper {
    U& size;
    terms_pytrees_size_helper(U& size) : size(size) { }

    template <typename Words, typename TreePtrs>
    void operator() (const Words& words, const TreePtrs& tps) {
      size += tps.size();
    }  // pycfg_type::terms_pytrees_size_helper::operator()    

  };  // pycfg_type::terms_pytrees_size_helper{}

  //! rule_weight() returns the weight of rule parent --> rhs
  //
  template <typename rhs_type>
  F rule_weight(S parent, const rhs_type& rhs) const {
    assert(!rhs.empty());
    if (rhs.size() == 1) {
      S_S_F::const_iterator it = unarychild_parent_weight.find(rhs[0]);
      if (it == unarychild_parent_weight.end())
	return 0;
      else
	return dfind(it->second, parent);
    }
    else {  // rhs.size() > 1
      Stit it = rhs_parent_weight.find(rhs);
      if (it == rhs_parent_weight.end())
	return 0;
      else
	return dfind(it->data, parent);
    }
  }  // pycfg_type::rule_weight()

  //! rule_prob() returns the probability of rule parent --> rhs
  //
  template <typename rhs_type>
  F rule_prob(S parent, const rhs_type& rhs) const {
    assert(!rhs.empty());
    F parentweight = afind(parent_weight, parent);
    F ruleweight = rule_weight(parent, rhs);
    assert(ruleweight > 0);
    assert(parentweight > 0);
    return ruleweight/parentweight;
  }  // pycfg_type::rule_prob()

  //! tree_prob() returns the probability of the tree under the current
  //! model
  //
  F tree_prob(const tree* tp) const {
    if (tp->children.empty()) 
      return 1;
    F pya = get_pya(tp->cat);
    if (pya == 1) { // no cache
      F prob = 1;
      Ss children;
      cforeach(tree::ptrs_type, it, tp->children) {
	children.push_back((*it)->cat);
	prob *= tree_prob(*it);
      }
      prob *= rule_prob(tp->cat, children);
      return prob;
    }
    F pyb = get_pyb(tp->cat);
    U pym = dfind(parent_pym, tp->cat);
    U pyn = dfind(parent_pyn, tp->cat);
    if (tp->count > 0) { // existing node
      assert(tp->count <= pyn);
      assert(pym > 0);
      F prob = (tp->count - pya)/(pyn + pyb);
      assert(finite(prob)); assert(prob > 0); assert(prob <= 1);
      return prob;
    }
    // new node
    F prob = (pym * pya + pyb)/(pyn + pyb);
    assert(finite(prob)); assert(prob > 0); assert(prob <= 1);
    Ss children;
    cforeach(tree::ptrs_type, it, tp->children) {
      children.push_back((*it)->cat);
      prob *= tree_prob(*it);
    }
    prob *= rule_prob(tp->cat, children);
    if (prob < 0)
      std::cerr << "## pycfg_type::tree_prob(" << *tp << ") = " 
		<< prob << std::endl;
    assert(finite(prob)); assert(prob <= 1); assert(prob >= 0);
    // assert(prob > 0); 
    return prob;
  }  // pycfg_type::tree_prob()

  //! incrrule() increments the weight of the rule parent --> rhs,
  //! returning the probability of this rule under the old grammar.
  //
  template <typename rhs_type>
  F incrrule(S parent, const rhs_type& rhs, F weight = 1) {
    assert(!rhs.empty());
    assert(weight >= 0);
    F& parentweight = parent_weight[parent];
    F parentweight0 = parentweight;
    F rhsweight0;
    parentweight += weight;
    if (rhs.size() == 1) {
      F& rhsweight = unarychild_parent_weight[rhs[0]][parent];
      rhsweight0 = rhsweight;
      rhsweight += weight;
    }
    else {  // rhs.size() > 1
      F& rhsweight = rhs_parent_weight[rhs][parent];
      rhsweight0 = rhsweight;
      rhsweight += weight;
    }
    assert(parentweight0 >= 0);
    assert(rhsweight0 >= 0);
    return rhsweight0/parentweight0;
  }  // incrrule()

  //! decrrule() decrements the weight of rule parent --> rhs,
  //! returning the probability of this rule under the new grammar,
  //! and deletes the rule if it has weight 0.
  //
  template <typename rhs_type>
  F decrrule(S parent, const rhs_type& rhs, F weight = 1) {
    assert(weight >= 0);
    assert(!rhs.empty());
    F rhsweight;
    F parentweight = (parent_weight[parent] -= weight);
    assert(parentweight >= 0);
    if (parentweight == 0)
      parent_weight.erase(parent);
    if (rhs.size() == 1) {
      S_F& parent1_weight = unarychild_parent_weight[rhs[0]];
      rhsweight = (parent1_weight[parent] -= weight);
      assert(rhsweight >= 0);
      if (rhsweight == 0) {
	parent1_weight.erase(parent);
	if (parent1_weight.empty())
	  unarychild_parent_weight.erase(rhs[0]);
      }
    }
    else {  // non-unary rule
      S_F& parent1_weight = rhs_parent_weight[rhs];
      rhsweight = (parent1_weight[parent] -= weight);
      if (rhsweight == 0) {
	parent1_weight.erase(parent);
	if (parent1_weight.empty())
	  rhs_parent_weight.erase(rhs);
      }
    }
    return rhsweight/parentweight;
  }  // pycfg_type::decrrule()

  //! incrtree() increments the cache for tp, increments
  //! the rules if the cache count is appropriate, and returns
  //! the probability of this tree under the original model.
  //
  F incrtree(tree* tp, U weight = 1) {
    if (tp->children.empty())  
      return 1;  // terminal node
    assert(weight >= 0);
    F pya = get_pya(tp->cat);    // PY cache statistics
    F pyb = get_pyb(tp->cat);
    if (pya == 1) { // don't table this category
      F prob = 1;
      {
	Ss children;
	cforeach (tree::ptrs_type, it, tp->children)
	  children.push_back((*it)->cat);
	prob *= incrrule(tp->cat, children, estimate_theta_flag*weight);
      }
      cforeach (tree::ptrs_type, it, tp->children)
	prob *= incrtree(*it, weight);
      return prob;
    }
    else if (tp->count > 0) {  // old PY table entry
      U& pyn = parent_pyn[tp->cat];
      F prob = (tp->count - pya)/(pyn + pyb);
      assert(finite(prob)); assert(prob > 0); assert(prob <= 1);
      tp->count += weight;              // increment entry count
      pyn += weight;                    // increment PY count
      return prob;
    } 
    else { // new PY table entry
      {
	Ss terms;
	tp->terminals(terms);
	bool inserted ATTRIBUTE_UNUSED = terms_pytrees[terms].insert(tp).second;
	assert(inserted);
      }
      U& pym = parent_pym[tp->cat];
      U& pyn = parent_pyn[tp->cat];
      F prob = (pym*pya + pyb)/(pyn + pyb);  // select new table
      assert(finite(prob)); assert(prob > 0); assert(prob <= 1);
      tp->count += weight;              // increment count
      pym += 1;                         // one more PY table entry
      pyn += weight;                    // increment PY count
      {
	Ss children;
	cforeach (tree::ptrs_type, it, tp->children)
	  children.push_back((*it)->cat);
	prob *= incrrule(tp->cat, children, estimate_theta_flag*weight);
      }
      cforeach (tree::ptrs_type, it, tp->children)
	prob *= incrtree(*it, weight);
      return prob;
    }
  }  // pycfg_type::incrtree()

  //! decrtree() decrements the cache for tp, decrements
  //! the rules if the cache count is appropriate, and returns
  //! the probability of this tree under the new model.
  //
  F decrtree(tree* tp, U weight = 1) {
    if (tp->children.empty())  
      return 1;  // terminal node
    F pya = get_pya(tp->cat);    // PY cache statistics
    if (pya == 1) {  // don't table this category
      F prob = 1;
      {
	Ss children;
	cforeach (tree::ptrs_type, it, tp->children)
	  children.push_back((*it)->cat);
	F ruleprob = decrrule(tp->cat, children, estimate_theta_flag*weight);
	assert(ruleprob > 0);
	prob *= ruleprob;
      }
      cforeach (tree::ptrs_type, it, tp->children) 
	prob *= decrtree(*it, weight);
      return prob;
    }
    assert(weight <= tp->count);
    tp->count -= weight;
    assert(afind(parent_pyn, tp->cat) >= weight);
    const U pyn = (parent_pyn[tp->cat] -= weight);
    F pyb = get_pyb(tp->cat);
    if (tp->count > 0) {  // old PY table entry
      assert(pyn > 0);
      F prob = (tp->count - pya)/(pyn + pyb);
      assert(finite(prob)); assert(prob > 0); assert(prob <= 1);
      return prob;
    } 
    else { // tp->count == 0, remove PY table entry
      {
	Ss terms;
	tp->terminals(terms);
	sT& pytrees = terms_pytrees[terms];
	sT::size_type nerased ATTRIBUTE_UNUSED = pytrees.erase(tp);
	assert(nerased == 1);
	if (pytrees.empty()) 
	  terms_pytrees.erase(terms);
      }
      // Bug: when pym or pyn goes to zero and the parent is erased, 
      // and then the reference to pym or pyn becomes a dangling reference
      // U& pym = parent_pym[tp->cat];
      // pym -= 1;                         // reduce cache count
      assert(parent_pym.count(tp->cat) > 0);
      const U pym = --parent_pym[tp->cat];
      if (pym == 0) 
	parent_pym.erase(tp->cat);
      if (pyn == 0)
	parent_pyn.erase(tp->cat);
      F prob = (pym*pya + pyb)/(pyn + pyb);  // select new table
      assert(finite(prob)); assert(prob > 0); assert(prob <= 1);
      {
	Ss children;
	cforeach (tree::ptrs_type, it, tp->children)
	  children.push_back((*it)->cat);
	prob *= decrrule(tp->cat, children, estimate_theta_flag*weight);
      }
      assert(prob > 0);
      cforeach (tree::ptrs_type, it, tp->children)
	prob *= decrtree(*it, weight);
      // assert(prob > 0);
      return prob;
    }
  }  // pycfg_type::decrtree()

  //! read() reads a grammar from an input stream (implements >> )
  //
  std::istream& read(std::istream& is) {
    start = symbol::undefined();
    F weight = default_weight;
    F pya = default_pya;
    F pyb = default_pyb;
    S parent;
    while (is >> default_value(weight, default_weight) 
	      >> default_value(pya, default_pya)
	      >> default_value(pyb, default_pyb)
	      >> parent >> " -->") {
      if (weight<=0)
	weight=default_weight;
      if (start.is_undefined())
	start = parent;
      Ss rhs;
      readline_symbols(is, rhs);
      if (debug >= 100000)
	std::cerr << "# " << weight << '\t' << parent << " --> " << rhs << std::endl;
      if (pya < 0 || pya > 1) 
	std::cerr << "Error while reading grammar rule " << parent << " --> " << rhs
		  << ": pya = " << pya << " is out of bounds 0 <= pya <= 1." << abort;
      if (pyb <= 0)
	std::cerr << "Error while reading grammar rule " << parent << " --> " << rhs
		  << ": pyb = " << pyb << " is out of bounds 0 < pyb." << abort;
      incrrule(parent, rhs, weight);
      if (pya != default_pya)
	parent_pya[parent] = pya;
      if (pyb != default_pyb)
	parent_pyb[parent] = pyb;
      rule_priorweight[SSs(parent,rhs)] += weight;
      parent_priorweight[parent] += weight;
    }
    return is;
  }  // pycfg_type::read()

  //! write() writes a grammar (implements << )
  //
  std::ostream& write(std::ostream& os) const {
    assert(start.is_defined());
    write_rules(os, start);
    cforeach (S_F, it, parent_weight)
      if (it->first != start) 
	write_rules(os, it->first);
    return os;
  }  // pycfg_type::write()

  std::ostream& write_rules(std::ostream& os, S parent) const {
    rhs_parent_weight.for_each(write_rule(os, parent));
    cforeach (S_S_F, it0, unarychild_parent_weight) {
      S child = it0->first;
      const S_F& parent_weight = it0->second;
      cforeach (S_F, it1, parent_weight)
	if (it1->first == parent)
	  os << it1->second << '\t' << parent 
	     << " --> " << child << std::endl;
    }
    bool old_compact_trees_flag = catcounttree_type::compact_trees;  // save old flag
    catcounttree_type::compact_trees = false;  // turn off compact_trees
    terms_pytrees.for_each(write_pycache(os, parent));
    catcounttree_type::compact_trees = old_compact_trees_flag;
    return os;
  }  // pycfg_type::write_rules()

  //! write_rule{} writes a single rule
  //
  struct write_rule {
    std::ostream& os;
    S parent;

    write_rule(std::ostream& os, symbol parent) : os(os), parent(parent) { }

    template <typename Keys, typename Value>
    void operator() (const Keys& rhs, const Value& parentweights) {
      cforeach (typename Value, pwit, parentweights) 
	if (pwit->first == parent) {
	  os << pwit->second << '\t' << parent << " -->";
	  cforeach (typename Keys, rhsit, rhs)
	    os << ' ' << *rhsit;
	  os << std::endl;
	}
    }  // pycfg_type::write_rule::operator()

  };  // pycfg_type::write_rule{}
  
  //! write_pycache{} writes the cache entries for a category
  //
  struct write_pycache {
    std::ostream& os;
    S parent;
    
    write_pycache(std::ostream& os, S parent) : os(os), parent(parent) { }

    template <typename Words, typename TreePtrs>
    void operator() (const Words& words, const TreePtrs& tps) {
      cforeach (typename TreePtrs, tpit, tps) 
	if ((*tpit)->cat == parent)
	  os << (*tpit) << std::endl;
    }  // pycfg_type::write_pycache::operator()
  };  // pycfg_type::write_pycache{}

  //! logPcorpus() returns the log probability of the corpus trees
  //
  F logPcorpus() const {
    F logP = 0;
    // grammar part
    cforeach (SSs_F, it, rule_priorweight) {
      S parent = it->first.first;
      const Ss& rhs = it->first.second;
      F priorweight = it->second;
      F weight = rule_weight(parent, rhs);
      logP += lgamma(weight) - lgamma(priorweight);
    }
    if (debug >= 5000)
      TRACE1(logP);
    cforeach (S_F, it, parent_priorweight) {
      S parent = it->first;
      F priorweight = it->second;
      F weight =dfind(parent_weight, parent);
      logP += lgamma(priorweight) - lgamma(weight);
    }
    if (debug >= 5000)
      TRACE1(logP);
    assert(logP <= 0);
    // PY adaptor part
    cforeach (S_U, it, parent_pyn) {
      S parent = it->first;
      U pyn = it->second;
      U pym = afind(parent_pym, parent);
      F pya = get_pya(parent);
      F pyb = get_pyb(parent);
      logP += lgamma(pyb) - lgamma(pyn+pyb);
      for (U i = 0; i < pym; ++i)
	logP += log(i*pya + pyb);
    }
    if (debug >= 5000)
      TRACE1(logP);
    terms_pytrees.for_each(logPcache(*this, logP));
    if (debug >= 5000)
      TRACE1(logP);
    assert(logP <= 0);
    return logP;
  }  // pycfg_type::logPcorpus()

  struct logPcache {
    const pycfg_type& g;
    F& logP;

    logPcache(const pycfg_type& g, F& logP) : g(g), logP(logP) { }

    template <typename Words, typename TreePtrs>
    void operator() (const Words& words, const TreePtrs& tps) {
      cforeach (typename TreePtrs, it, tps) {
	S parent = (*it)->cat;
	U count = (*it)->count;
	F pya = g.get_pya(parent);
	logP += lgamma(count-pya) - lgamma(1-pya);
      }
    }  // pycfg_type::logPcache::operator()
  };  // pycfg_type::logPcache{}

  //! logPrior() returns the prior probability of the PY a and b values
  //
  F logPrior() const {
    F sumLogP = 0;
    if (pyb_gamma_s > 0 && pyb_gamma_c > 0)
      cforeach (S_U, it, parent_pyn) {
	S parent = it->first;
	F pya = get_pya(parent);
	assert(pya >= 0);
	assert(pya <= 1);
	F pyb = get_pyb(parent);
	assert(pyb >= 0);
	if (pya_beta_a > 0 && pya_beta_b > 0 && pya > 0) {
	  F logP = pya_logPrior(pya, pya_beta_a, pya_beta_b);
	  if (debug >= 2000)
	    TRACE5(parent, logP, pya, pya_beta_a, pya_beta_b);
	  sumLogP += logP;
	}
	F logP = pyb_logPrior(pyb, pyb_gamma_c, pyb_gamma_s);
	if (debug >= 2000)
	  TRACE5(parent, logP, pyb, pyb_gamma_c, pyb_gamma_s);
	sumLogP += logP;
      }
    return sumLogP;
  }  // pycfg_type::logPrior()

  //! pya_logPrior() calculates the Beta prior on pya.
  //
  static F pya_logPrior(F pya, F pya_beta_a, F pya_beta_b) {
    F prior = lbetadist(pya, pya_beta_a, pya_beta_b);     //!< prior for pya
    return prior;
  }  // pycfg_type::pya_logPrior()

  //! pyb_logPrior() calculates the prior probability of pyb 
  //! wrt the Gamma prior on pyb.
  //
  static F pyb_logPrior(F pyb, F pyb_gamma_c, F pyb_gamma_s) {
    F prior = lgammadist(pyb, pyb_gamma_c, pyb_gamma_s);  // prior for pyb
    return prior;
  }  // pcfg_type::pyb_logPrior()

  //////////////////////////////////////////////////////////////////////
  //                                                                  //
  //                      Resample pyb                                //
  //                                                                  //
  //////////////////////////////////////////////////////////////////////
  
  //! resample_pyb_type{} is a function object that returns the part of log prob that depends on pyb.
  //! This includes the Gamma prior on pyb, but doesn't include e.g. the rule probabilities
  //! (as these are a constant factor)
  //
  struct resample_pyb_type {
    typedef double F;

    U pyn, pym;
    F pya, pyb_gamma_c, pyb_gamma_s, min_pyb;
    resample_pyb_type(U pyn, U pym, F pya, F pyb_gamma_c, F pyb_gamma_s, F min_pyb) 
      : pyn(pyn), pym(pym), pya(pya), pyb_gamma_c(pyb_gamma_c), pyb_gamma_s(pyb_gamma_s), min_pyb(min_pyb)
    { }

    //! operator() returns the part of the log posterior probability that depends on pyb
    //
    F operator() (F pyb0) const {
      F pyb = pyb0+min_pyb;
      // TRACE5(pya, pyb, pyb_gamma_c, pyb_gamma_c, pyb_gamma_s);
      assert(pyb > 0);
      F logPrior = pyb_logPrior(pyb, pyb_gamma_c, pyb_gamma_s);  //!< prior for pyb
      F logProb = 0;
      logProb += (pya == 0 ? pym*log(pyb) : pym*log(pya) + lgamma(pym + pyb/pya) - lgamma(pyb/pya));
      logProb += lgamma(pyb) - lgamma(pyn+pyb);
      // TRACE2(logProb, logPrior);
      return logProb+logPrior;
    }
  };  // pcfg_type::resample_pyb_type{}

  //! resample_pyb() samples new values for pyb for each adapted nonterminal.
  //
  void resample_pyb() {
    U niterations = 20;   //!< number of resampling iterations
    F min_pyb = 1e-20;    //!< minimum value for pyb
    cforeach (S_U, it, parent_pyn) {
      S parent = it->first;
      U pyn = it->second;
      U pym = afind(parent_pym, parent);
      F pya = get_pya(parent);
      F pyb = get_pyb(parent);
      // TRACE5(parent, pym, pyn, pya, pyb);
      resample_pyb_type pyb_logP(pyn, pym, pya, pyb_gamma_c, pyb_gamma_s, min_pyb);
      // pyb = slice_sampler1d(pyb_logP, pyb, random1, 0.0, std::numeric_limits<double>::infinity(), 0.0, niterations, 100*niterations);
      F pyb0 = slice_sampler1dp(pyb_logP, pyb, random1, 1, niterations);
      parent_pyb[parent] = pyb0 + min_pyb;
      // parent_bap[parent].first += naccepted;
      // parent_bap[parent].second += nproposed;
    }
  }  // pcfg_type::resample_pyb()

  //////////////////////////////////////////////////////////////////////
  //                                                                  //
  //                   Resample pya and pyb                           //
  //                                                                  //
  //////////////////////////////////////////////////////////////////////

  //! resample_pya_type{} calculates the part of the log prob that depends on pya.
  //! This includes the Beta prior on pya, but doesn't include e.g. the rule probabilities
  //! (as these are a constant factor)
  //
  struct resample_pya_type {
    U pyn, pym;
    F pyb, pya_beta_a, pya_beta_b;
    const Ts& trees;
    
    resample_pya_type(U pyn, U pym, F pyb, F pya_beta_a, F pya_beta_b, const Ts& trees) 
      : pyn(pyn), pym(pym), pyb(pyb), pya_beta_a(pya_beta_a), pya_beta_b(pya_beta_b), trees(trees)
    { }

    //! operator() returns the part of the log posterior probability that depends on pya
    //
    F operator() (F pya) const {
      F logPrior = pya_logPrior(pya, pya_beta_a, pya_beta_b);     //!< prior for pya
      F logProb = 0;
      F lgamma1a = lgamma(1-pya);
      cforeach (Ts, it, trees) {
	U count = (*it)->count;
	logProb += lgamma(count-pya) - lgamma1a;
      }
      logProb += (pya == 0 ? pym*log(pyb) : pym*log(pya) + lgamma(pym + pyb/pya) - lgamma(pyb/pya));
      return logPrior + logProb;
    }   // pycfg_type::resample_pya_type::operator()

  };  // pycfg_type::resample_pya_type{}
  
  //! resample_pya() samples new values for pya for each adapted nonterminal
  //
  void resample_pya(const S_Ts& parent_trees) {
    U niterations = 20;   //!< number of resampling iterations
    // std::cerr << "\n## Initial parent_pya = " << parent_pya << ", parent_pyb = " << parent_pyb << std::endl;
    cforeach (S_U, it, parent_pyn) {
      S parent = it->first;
      F pya = get_pya(parent);
      if (pya == 0)   // if this nonterminal has pya == 0, then don't resample
	continue;
      F pyb = get_pyb(parent);
      U pyn = it->second;
      U pym = afind(parent_pym, parent);
      const Ts& trees = afind(parent_trees, parent);
      resample_pya_type pya_logP(pyn, pym, pyb, pya_beta_a, pya_beta_b, trees);
      pya = slice_sampler1d(pya_logP, pya, random1, std::numeric_limits<double>::min(), 1.0, 0.0, niterations);
      parent_pya[parent] = pya;
    }
  }  // pycfg_type::resample_pya()

  //! resample_pyab_parent_trees_helper{} constructs parent_trees from terms_pytrees.
  //
  struct resample_pyab_parent_trees_helper {
    S_Ts& parent_trees;
    resample_pyab_parent_trees_helper(S_Ts& parent_trees) : parent_trees(parent_trees) { }

    template <typename Words, typename TreePtrs>
    void operator() (const Words& words, const TreePtrs& tps) {
      cforeach (typename TreePtrs, it, tps) {
	S parent = (*it)->cat;
	parent_trees[parent].push_back(*it);
      }
    }  // pycfg_type::resample_pyab_parent_trees_helper::operator()
  };  // pycfg_type::resample_pyab_parent_trees_helper{}

  //! resample_pyab() resamples both pya and pyb for each adapted nonterminal.
  //
  void resample_pyab() {
    const U niterations = 5;  //!< number of alternating samples of pya and pyb
    S_Ts parent_trees;
    terms_pytrees.for_each(resample_pyab_parent_trees_helper(parent_trees));
    for (U i=0; i<niterations; ++i) {
      resample_pyb();
      resample_pya(parent_trees);
    }
    resample_pyb();
  }  // pycfg_type::resample_pyab()

  //! write_adaptor_parameters() writes out adaptor parameters to a file
  //
  std::ostream& write_adaptor_parameters(std::ostream& os) const {
    cforeach (S_F, it, parent_priorweight) {
      S parent = it->first;
      F pya = get_pya(parent);
      if (pya == 1)
	continue;
      U pym = dfind(parent_pym, parent);
      U pyn = dfind(parent_pyn, parent);
      F pyb = get_pyb(parent);
      os << ' ' << parent << ' ' << pym << ' ' << pyn << ' ' << pya << ' ' << pyb;
    }
    return os;
  }  // pycfg_type::write_adaptor_parameters()

  //! initialize_predictive_parse_filter() initializes the predictive
  //! parse filter by building the grammar that the Earley parser requires
  //
  void initialize_predictive_parse_filter() {
    predictive_parse_filter = true;
    cforeach (SSs_F, it, rule_priorweight) {
      const SSs& rule = it->first;
      const Ss& children = rule.second;
      assert(!children.empty());
      S child1 = children.front();
      predictive_parse_filter_grammar.add_rule(it->first, 
					       children.size() == 1 
					       && !parent_priorweight.count(child1));
    }
  }  // pycfg_type::initialize_predictive_parse_filter();

};  // pycfg_type{}

//! operator>> (pycfg_type&) reads a pycfg_type g, setting g.start
//! to the parent of the first rule read.
//
std::istream& operator>> (std::istream& is, pycfg_type& g) {
  return g.read(is);
}  // operator>> (pycfg_type&)


std::ostream& operator<< (std::ostream& os, const pycfg_type& g) {
  return g.write(os);
}  // operator<< (pycfg_type&)

namespace std { namespace tr1 {
    template <> struct hash<pycfg_type::Stit> 
      : public std::unary_function<pycfg_type::Stit, std::size_t> {
      size_t operator()(const pycfg_type::Stit t) const
      {
	return size_t(&(*t));
      }  // ext::hash<pycfg_type::Stit>::operator()
    };  // ext::hash<pycfg_type::Stit>{}
  }  } // namespace std::tr1
  
static const F unaryclosetolerance = 1e-7;

class pycky {

public:

  const pycfg_type& g;
  F anneal;         // annealing factor (1 = no annealing)
  
  pycky(const pycfg_type& g, F anneal=1) : g(g), anneal(anneal) { }

  typedef pycfg_type::tree tree;
  typedef pycfg_type::U U;
  typedef pycfg_type::S_S_F S_S_F;
  typedef pycfg_type::St_S_F St_S_F;
  typedef pycfg_type::Stit Stit;

  typedef std::vector<S_F> S_Fs;
  // typedef ext::hash_map<Stit,F> Stit_F;
  typedef tr1::unordered_map<Stit,F> Stit_F;
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
		    * power(itparent->second/afind(g.parent_weight, parent), anneal);
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
	  it->second *= power( (pym*pya + pyb)/(pyn + pyb), anneal);
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
      symbol cat = (*it)->cat;
      F pya = g.get_pya(cat);    // PY cache statistics
      if (pya == 1.0)
	continue;
      F pyb = g.get_pyb(cat);
      U pyn = dfind(g.parent_pyn, cat);
      inactives[cat] += power( ((*it)->count - pya)/(pyn + pyb), anneal);
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
	      prob *= power(it1->second/afind(g.parent_weight, parent), 
			    anneal);
	    else {
	      F pyb = g.get_pyb(parent);
	      U pym = dfind(g.parent_pym, parent);
	      U pyn = dfind(g.parent_pyn, parent);
	      prob *= power(it1->second/afind(g.parent_weight, parent)
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
	  if ((*it)->cat != parent)
	    continue;
	  probsofar += power( ((*it)->count - pya)/(pyn + pyb), anneal);
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
	  * power(dfind(parent1_weight, parent)*rulefactor, anneal);
	if (probsofar >= probthreshold) {
	  tp->children.push_back(random_inactive(child, childprob, left, right));
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
		* power(it->second*rulefactor, anneal);
	      if (probsofar >= probthreshold) {
		random_active(leftactive, leftprob, left, mid, tp->children);
		tp->children.push_back(random_inactive(rightinactive, rightprob, mid, right));
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
		     tree::ptrs_type& siblings) const {
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
  typedef catcounttree_type tree;

  pycfg_type& g;
  pycky& p;

  resample_pycache_helper(pycfg_type& g, pycky& p) : g(g), p(p) { }

  template <typename Words, typename TreePtrs>
  void operator() (const Words& words, TreePtrs& tps) {

    foreach (typename TreePtrs, tit, tps) {
      tree* tp0 = *tit;
      Ss words;
      tp0->terminals(words);
      S start = tp0->category();
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
	F accept = (pi0r1 > 0) ? power(pi1r0/pi0r1, p.anneal) : 2.0; // accept if there has been an underflow
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
      g.set_pya(tp0->category(), old_pya);
    }
  }  // resample_pycache_helper::operator()

};  // resample_pycache_helper{}

//! resample_pycache() resamples the strings associated with each cache
//
inline void resample_pycache(pycfg_type& g, pycky& p) {
  resample_pycache_helper h(g, p);
  p.g.terms_pytrees.for_each(h);
}  // resample_pycache()

#endif // PY_CKY_H
