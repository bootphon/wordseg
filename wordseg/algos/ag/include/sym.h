// sym.h -- A symbol table package
//
// (c) Mark Johnson, 10th March 2001, modified 18th July 2001
// (c) Mark Johnson, 12th December 2001 (fix static initialization order bug)
// (c) Mark Johnson, 1st January 2002 (enlarge initial table size)
// (c) Mark Johnson, 4th May 2002 (write/read invariance, i.e., << and >> are inverses)
// (c) Mark Johnson, 16th July 2002 (g++ 3.1 namespace compatibility)
//
// A symbol contains a pointer to a string.  These strings are guaranteed to be
// unique, i.e., if symbols s1 and s2 contain different string pointers then
// the strings they point to are different.  This means that symbol copying, 
// equality, ordering and hashing are very cheap (they involve only the
// pointer and not the string's contents).
//
// A symbol whose string pointer is NULL is an undefined string.  Undefined
// symbols can be compared and hashed just like other symbols, but it is an
// error to obtain the string pointed to by an undefined symbol.
//
// Symbols possess write/read invariance, i.e., you can write a symbol
// to a stream and read it from the same stream.  (Note that the relative
// ordering of symbols is not preserved, since the underlying string objects
// may be allocated in different locations).  
//
//   A sequence of alphanumeric characters or escaped characters, where '\'
//   is the escape character.  Note that '_', '.', '-' and '+' are considered 
//   alphanumeric characters (thus ints and floats can be symbols).
//
//   A ' (quote) character, followed by a sequence of non-quote characters
//   or an escaped \' character, and terminated by an unescaped ' (quote)
//   character.  Thus the empty symbol can be written ''.
//
//   %UNDEFINED% denotes the undefined symbol.
//
// NOTE: You _must_ write a whitespace or non-alphanumeric character other
// than '\', '_', '+', '-', and '.' immediately after each symbol.  
// The symbol reader will stop reading at this character and then put it back on 
// the input stream, so your code is responsible for consuming this character.
//
//
// Constructors:
//
//  sym()                This returns an undefined symbol
//  sym(const string&)
//
// Conversions and member functions:
//
//  string(symbol)
//  symbol.string_reference()
//  symbol.string_pointer()
//  symbol.c_str()
//  symbol.is_defined()
//
//  std::hash(symbol)
//
//  The comparison operators ==, !=, <, <=, >, >=.
//    Note: the comparison ordering is NOT alphabetic, but is based on 
//    the location in memory of each symbol.  Thus the relative ordering
//    of the same symbol strings may differ on different runs.
//
// Static (global) functions:
//
//  symbol::size()       The number of symbols defined
//  symbol::already_defined(const string&)
//  
//  The input and output operators >> and <<

#ifndef SYM_H
#define SYM_H

#include "custom-allocator.h"  // must be first

#include <cassert>
// #include <ext/hash_set>
#include <iostream>
#include <string>
#include <utility>
#include <tr1/unordered_set>

namespace tr1 = std::tr1;

class symbol {

  typedef std::string* stringptr;
  const std::string* sp;
  symbol(const std::string* sp_) : sp(sp_) { }

  typedef tr1::unordered_set<std::string> Table;
  static Table& table();

public:
  
  symbol() : sp(NULL) { }       // constructs an undefined symbol
  symbol(const std::string& s);
  symbol(const char* cp);

  bool is_defined() const { return sp != NULL; }
  bool is_undefined() const { return sp == NULL; }

  operator std::string() const { assert(is_defined()); return *sp; }
  const std::string& string_reference() const { assert(is_defined()); return *sp; }
  const std::string* string_pointer() const { return sp; }
  const char* c_str() const { assert(is_defined()); return sp->c_str(); }

  static symbol undefined() { return symbol(stringptr(NULL)); }
  static size_t size() { return table().size(); }

  bool operator== (const symbol s) const { return sp == s.sp; }
  bool operator!= (const symbol s) const { return sp != s.sp; }
  bool operator< (const symbol s) const { return sp < s.sp; }
  bool operator<= (const symbol s) const { return sp <= s.sp; }
  bool operator> (const symbol s) const { return sp > s.sp; }
  bool operator>= (const symbol s) const { return sp >= s.sp; }
};

std::istream& operator>> (std::istream& is, symbol& s);
std::ostream& operator<< (std::ostream& os, symbol s);

namespace std { namespace tr1 {
    template <> struct hash<symbol> 
      : public std::unary_function<symbol, std::size_t>  {
      size_t operator()(symbol s) const
      {
	return size_t(s.string_pointer());
      }
    };
  } }

#endif  // sym.h
