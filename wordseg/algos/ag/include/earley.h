// earley.h -- A predictive CFG parser using the Earley algorithm
//
// (c) Mark Johnson, 28th October 2009, last modified 14th August 2011
//
// This is used to filter the possible categories and locations
// to speed the bottom-up probabilistic CKY parser that follows.

#pragma once

#include <cassert>
#include <vector>
#include <tr1/unordered_map>
#include <tr1/unordered_set>

#include "sym.h"
#include "utility.h"

extern int debug;

//! earley{} implements the Earley predictive parsing algorithm
//!  for CFGs.  It is used to filter the possible categories and
//!  their locations in order to speed the bottom-up probabilistic
//!  CKY parser that follows.
//
struct earley {
  
  typedef unsigned int U;
  typedef std::set<U> sU;

  typedef symbol S;
  typedef std::vector<S> Ss;
  typedef std::pair<S,Ss> R;
  typedef const R* Rp;
  typedef std::vector<Rp> Rps;
  typedef tr1::unordered_map<S,Rps> S_Rps;
  typedef std::set<S> sS;
  typedef tr1::unordered_map<S,sS> S_sS;
  typedef const sS* sSp;
  typedef std::vector<sSp> sSps;
  typedef std::vector<sS> U_sS;

  //! grammar{} holds the grammar in a format that is useful for faster parsing
  //
  struct grammar {
    S_Rps parent_ruleps;         //!< parent -> rules expanding parent
    S_sS terminal_preterminals;  //!< terminal -> set of preterminals

    void add_rule(const R& rule, bool preterminalchild) {
      if (preterminalchild) {
	assert(rule.second.size() == 1);
	terminal_preterminals[rule.second.front()].insert(rule.first);
      }
      else 
	parent_ruleps[rule.first].push_back(&rule);
    }  // earley::grammar::add_rule()

  };  //  earley::grammar{}

  struct A {      //!< Active edge
    U index;      //!< number of children found so far
    U left;       //!< location of left edge in string
    Rp rulep;     //!< pointer to rule being expanded
    A(U index, U left, Rp rulep) : index(index), left(left), rulep(rulep) { }

    bool operator= (const A& a) const { 
      return index == a.index && left == a.left && rulep == a.rulep; 
    }

    bool operator< (const A& a) const {
      return index < a.index 
	|| (index == a.index 
	    && (left < a.left
		|| (left == a.left && rulep < a.rulep)));
    }
  };

  typedef std::set<A> sA;
  typedef std::pair<sA,sU> sAsU;
  typedef tr1::unordered_map<S,sAsU> S_sAsU;
  typedef std::vector<S_sAsU> U_S_sAsU;

  const grammar& g;         //!< grammar
  const Ss& terminals;      //!< terminals
  sSps preterminals;        //!< preterminals
  U_S_sAsU ichart;          //!< inside chart
  U_sS& completes;          //!< complete edges

  static U index(U i, U j) { return j*(j-1)/2+i; }
  static U ncells(U n) { return n*(n+1)/2; }
  
  void inside(U left, U right, Rp rp, U i) {
    // TRACE4(left, right, *rp, i);
    if (i == rp->second.size()) 
      icomplete(left, right, rp->first);
    else if (right < terminals.size()) {
      S child = rp->second[i];
      std::pair<S_sAsU::iterator,bool> citb 
	= ichart[right].insert(S_sAsU::value_type(child,sAsU()));
      sA& as = citb.first->second.first;
      sU& cs = citb.first->second.second;
      // TRACE2(citb.second, as.count(A(i,left,rp)));
      if (!as.insert(A(i,left,rp)).second)
	return;  // this active edge is already in chart
      if (terminals[right] == child) 
	inside(left, right+1, rp, i+1);
      if (citb.second) { // new search
	const sSp preterminals_right = preterminals[right];
	if (preterminals_right != NULL && preterminals_right->count(child)) {
	  completes[index(right,right+1)].insert(child);
	  cs.insert(right+1);
	  inside(left, right+1, rp, i+1);
	}
	inside(right, child);
      }
      else {  // already searched for
	cforeach (sU, cit, cs) 
	  inside(left, *cit, rp, i+1);
      }
    }
  }  // earley::inside()

  void icomplete(U left, U right, S cat) {
    // TRACE3(left, right, cat);
    sAsU& ascs = ichart[left][cat];
    sA& as = ascs.first;
    sU& cs = ascs.second;
    if (!cs.insert(right).second)
      return;
    cforeach (sA, it, as) {
      assert(it->rulep->second[it->index] == cat);
      inside(it->left, right, it->rulep, it->index+1);
    }
    completes[index(left,right)].insert(cat);
  }

  void inside(U left, S cat) {
    // TRACE2(left, cat);
    S_Rps::const_iterator prit = g.parent_ruleps.find(cat);
    if (prit != g.parent_ruleps.end()) {
      const Rps& ruleps = prit->second;
      cforeach (Rps, it, ruleps)
	inside(left, left, *it, 0);
    }
  }

  bool complete(U left, U right, S cat) const {
    U i = index(left, right);
    assert(i < completes.size());
    return completes[i].count(cat);
  }

  earley(const grammar& g, S start, const Ss& terminals, U_sS& completes) 
    : g(g), terminals(terminals), preterminals(terminals.size()), 
      ichart(terminals.size()), completes(completes)
  {
    completes.clear();
    completes.resize(ncells(terminals.size()));
    // TRACE1(terminals);
    for (U i = 0; i < terminals.size(); ++i) {
      S_sS::const_iterator it = g.terminal_preterminals.find(terminals[i]);
      if (it == g.terminal_preterminals.end())
	preterminals[i] = NULL;
      else
	preterminals[i] = &it->second;
    }

    if (debug >= 50000) 
      std::cerr << "\n# earley terminals = " << terminals << std::endl;

    if (debug >= 70000) {
      std::cerr << "# earley preterminals:";
      for (U i = 0; i < preterminals.size(); ++i)
        if (preterminals[i])
	  std::cerr << " " << *preterminals[i];
	else
	  std::cerr << " ()";
      std::cerr << std::endl;
    }

    { // here is where we actually parse

      const sSp preterminals0 = preterminals[0];
      if (preterminals0 != NULL && preterminals0->count(start)) {
	completes[index(0,1)].insert(start);
	std::pair<S_sAsU::iterator,bool> citb 
	  = ichart[0].insert(S_sAsU::value_type(start,sAsU()));
	assert(citb.second);
	sU& cs = citb.first->second.second;
	cs.insert(1);
      }
      inside(0, start);
    }

    if (debug >= 50000)
      for (U left = 0; left < terminals.size(); ++left)
	for (U right = left+1; right <= terminals.size(); ++right) 
	  if (!completes[index(left,right)].empty()) {
	    std::cerr << "# earley: left = " << left << ", right = " << right 
		      << ", completes =";
	    cforeach (sS, it, completes[index(left,right)])
	      std::cerr << ' ' << *it;
	    std::cerr << std::endl;
	  }

    if (debug >= 70000)
      for (U right = 0; right < ichart.size(); ++right) 
	cforeach (S_sAsU, it0, ichart[right]) {
	  S cat = it0->first;
	  const sA& as = it0->second.first;
	  const sU& cs = it0->second.second;
	  std::cerr << "# earley incomplete: right = " << right << ", cs = " << cs;
	  cforeach (sA, it1, as) 
	    std::cerr << "; cat = " << cat
		      << "; left = " << it1-> left
		      << ", index = " << it1->index
		      << ", rule = " << *(it1->rulep);
	  std::cerr << std::endl;
	}
  }

};  // earley{}
