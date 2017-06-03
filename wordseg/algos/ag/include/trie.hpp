// trie.h
//
// (c) Mark Johnson, 19th June 2000
// modified (c) Mark Johnson, 21st Feb 2003, added trie::size()
// modified (c) Mark Johnson, 19th June 2005, added write/read invariance
// modified (c) Mark Johnson, 26th November 2005, added trie::erase()
// modified (c) Mark Johnson, 5th January 2006
//
// A trie package for C++

#ifndef TRIE_H
#define TRIE_H

#include "custom-allocator.h"  // must be first

#include <map>
#include <iostream>
#include <vector>

//! A trie maps from a sequence of keys to data.  
//
template <class key_type_, class data_type_>
class trie {

public:

  typedef unsigned int size_type;
  typedef key_type_ key_type;
  typedef data_type_ data_type;
  typedef std::map<key_type,trie> key_trie_type;
  typedef trie*       iterator;
  typedef const trie* const_iterator;
  typedef trie*       pointer;
  typedef trie&       reference;

  data_type     data;
  key_trie_type key_trie;

  trie() : data(), key_trie() { }

  iterator end() const { return 0; }   // NULL pointer

  //! size() returns the number of non-default values in the trie.
  //
  size_type size() const {
    size_type s = (data == data_type() ? 0 : 1);
    for (typename key_trie_type::const_iterator i = key_trie.begin();
	 i != key_trie.end(); ++i)
      s += i->second.size();
    return s;
  }  // trie::size()

  //! clear() removes all the elements from a trie
  //
  void clear() {
    key_trie.clear();
    data = data_type();
  }

  //! find1() returns the trie obtained by looking up the first element
  //! of key.
  //
  iterator find1(const key_type& key) {
    typename key_trie_type::iterator i = key_trie.find(key);
    if (i == key_trie.end())
      return end();
    else 
      return &i->second;
  }  // trie::find1()

  const_iterator find1(const key_type& key) const {
    typename key_trie_type::const_iterator i = key_trie.find(key);
    if (i == key_trie.end())
      return end();
    else 
      return &i->second;
  }  // trie::find1()

  //! find() returns the trie obtained by looking up the sequence of
  //! elements from start to finish.
  //
  template <class It>
  iterator find(It start, It finish) {
    if (start == finish) 
      return this;
    else {
      typename key_trie_type::iterator i = key_trie.find(*start);
      if (i == key_trie.end())
	return end();
      else 
	return i->second.find(++start, finish);
    }
  }  // trie::find()

  //! find() returns the trie obtained by looking up the sequence of
  //! elements from keys.begin() to keys.end().
  //
  template <class It>
  iterator find(It keys) {
    return find(keys.begin(), keys.end());
  }  // trie::find()

  template <class It>
  const_iterator find(It start, It finish) const {
    if (start == finish) 
      return this;
    else {
      typename key_trie_type::const_iterator i = key_trie.find(*start);
      if (i == key_trie.end())
	return end();
      else 
	return i->second.find(++start, finish);
    }
  }  // trie::find()

  template <class It>
  const_iterator find(It keys) const {
    return find(keys.begin(), keys.end());
  }  // trie::find()

private:

  template <class It>
  std::pair<iterator,bool> insert_(It start, It finish, const data_type& d) {
    if (start == finish) {
      data = d;
      return std::make_pair(this, true);
    }
    else {
      const key_type& key = *start;
      return key_trie[key].insert_(++start, finish, d);
    }
  }  // trie::insert_()

public:

  //! insert() inserts d as the value of start-finish if no such value exists,
  //! and returns a pair consisting of an iterator and a flag indicating whether
  //! the value d was in fact inserted.
  //
  template <class It>
  std::pair<iterator,bool> insert(It start, It finish, const data_type& d) {
    if (start == finish)
      return std::make_pair(this, false);
    else {
      typename key_trie_type::iterator i = key_trie.find(*start);
      if (i == key_trie.end()) {  // key not found in trie
	const key_type& key = *start;
	return key_trie[key].insert_(++start, finish, d);
      }
      else
	return i->second.insert(++start, finish, d);
    }
  }  // trie::insert()

  template <class It>
  std::pair<iterator,bool> insert(It keys, const data_type& d) {
    return insert(keys.begin(), keys.end(), d);
  }  // trie::insert()

  //! operator[]() returns a reference to the value associated with 
  //! keys.begin() to keys.end(), creating such a value if necessary.
  //
  template <class It>
  data_type& operator[] (It keys) {
    return insert(keys, data_type()).first->data;
  }  // trie::operator[]

  //! erase() deletes the value associated with start-end.
  //
  template <class It>
  bool erase(It start, It finish) {
    if (start == finish) {
      data = data_type();
      return key_trie.empty();
    }
    else {
      typename key_trie_type::iterator it(key_trie.find(*start));
      if (it != key_trie.end()) {
	if (it->second.erase(++start, finish)) 
	  key_trie.erase(it->first);
      }
      return key_trie.empty() && data == data_type();
    }
  }  // trie::erase()

  template <class It>
  bool erase(It keys) {
    return erase(keys.begin(), keys.end());
  }  // trie::erase()

  //! foreach() does a top-down traversal of the trie, calling
  //! p(keys, data) at each non-default value of data
  //
  template <typename Proc>
  void for_each(Proc p) {
    std::vector<key_type> keys;
    for_each_helper(p, keys);
  }  // trie::for_each()

  template <typename Proc, typename Keys>
  void for_each_helper(Proc& p, Keys& keys) {
    if (data != data_type())
      p(keys, data);
    for (typename key_trie_type::iterator i = key_trie.begin(); 
	 i != key_trie.end(); ++i) {
      keys.push_back(i->first);
      i->second.for_each_helper(p, keys);
      keys.pop_back();
    }
  }  // trie::for_each_helper()

  //! foreach() does a top-down traversal of the trie, calling
  //! p(keys, data) at each non-default value of data
  //
  template <typename Proc>
  void for_each(Proc p) const {
    std::vector<key_type> keys;
    for_each_helper(p, keys);
  }  // trie::for_each()

  template <typename Proc, typename Keys>
  void for_each_helper(Proc& p, Keys& keys) const {
    if (data != data_type())
      p(keys, data);
    for (typename key_trie_type::const_iterator i = key_trie.begin(); 
	 i != key_trie.end(); ++i) {
      keys.push_back(i->first);
      i->second.for_each_helper(p, keys);
      keys.pop_back();
    }
  }  // trie::for_each_helper()

};  // trie{}

template <class key_type, class data_type>
std::ostream& operator<< (std::ostream& os, const trie<key_type,data_type>& t) {
  typedef const trie<key_type,data_type> trie_type;
  typedef typename trie_type::key_trie_type key_trie_type;
  typedef typename key_trie_type::const_iterator const_iterator;
  os << '(' << t.data;
  for (const_iterator i = t.key_trie.begin();
       i != t.key_trie.end(); ++i)
    os << ' ' << i->first << ' ' << i->second;
  return os << ')';
}  // operator<<
  
template <class key_type, class data_type>
std::istream& operator>> (std::istream& is, trie<key_type,data_type>& t) {
  typedef trie<key_type,data_type> trie_type;
  typedef typename trie_type::key_trie_type key_trie_type;
  typedef typename key_trie_type::iterator key_trie_iterator;
  typedef typename key_trie_type::value_type key_trie_value_type;
  char c;
  if (is >> c) {
    if (c == '(') {
      is >> t.data;
      t.key_trie.clear();
      key_type key;
      while (is >> key) {
	key_trie_iterator it = t.key_trie.insert(key_trie_value_type(key, trie_type())).first;
	assert(it != t.key_trie.end());
	is >> it->second;  // read value into trie
      }
      is.clear(is.rdstate() & ~std::ios::failbit);
      if (is >> c && c == ')')
	return is;
      else
	is.setstate(std::ios::badbit);
    }
    else
      is.putback(c);
  }
  is.setstate(std::ios::failbit);
  return is;
}

template <class key_type_>
class btrie : public trie<key_type_,bool> {

  typedef typename trie<key_type_,bool>::key_trie_type key_trie_type;

public:

  template <class It>
  It longest_prefix(It start, It finish) const {
    It last_match = start;
    const trie<key_type_,bool>* current = this;
    while (start != finish) {
      typename key_trie_type::const_iterator kti = current->key_trie.find(*start++);
      if (kti == current->key_trie.end())
	break;
      current = &kti->second;
      if (current->data == true)
	last_match = start;
    }
    return last_match;
  }  // btrie::longest_prefix()

};  // btrie{}

#endif // TRIE_H
