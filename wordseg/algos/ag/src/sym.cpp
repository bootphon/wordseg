// sym.cc
//
// (c) Mark Johnson, 10th March 2001
// (c) Mark Johnson, 12th December 2001 (fix static initialization order bug)
// (c) Mark Johnson, 4th May 2002 (write/read invariance)
// (c) Mark Johnson, 16th July 2002 (g++ 3.1 namespace compatibility)
// (c) Mark Johnson, 15th August 2002 (added test code)
// (c) Mark Johnson, 20th August 2002 (fixed EOF bug)
// (c) Mark Johnson, some time in 2004 (minimized amount of quoting)
// (c) Mark Johnson, 9th April 2010 (modified so unicode prints correctly)

// #define SYM_CC_MAIN   // uncomment this to include the main() test program below

#include "sym.h"
#include <cctype>

#define ESCAPE     '\\'
#define OPENQUOTE  '\''
#define CLOSEQUOTE '\''
#define UNDEFINED  "%UNDEFINED%"           // UNDEFINED must start with punctuation

// define these as local static variables to avoid static initialization order bugs
//
symbol::Table& symbol::table() 
{
  static Table table_(65536);   // default table size
  return table_;
}

symbol::symbol(const std::string& s) : sp(&*(table().insert(s).first)) { };

symbol::symbol(const char* cp) { 
  if (cp) {
    std::string s(cp); 
    sp = &*(table().insert(s).first);
  }
  else
    sp = NULL;
};


// Read/write code

// inline static bool dont_escape(char c) { 
//  return isalnum(c) || c == '_' || c == '.' || c == '-' || c == '+'; 
// }

inline static bool dont_escape(char c) { 
  return (!isspace(c)) && c != ESCAPE && c != OPENQUOTE && c != CLOSEQUOTE 
         && c != '%' && c != '(' && c != ')';
}

inline static char escaped_char(char c) {
  switch (c) {
  case 'a': return('\a');
  case 'b': return('\b');
  case 'f': return('\f');
  case 'n': return('\n');
  case 'r': return('\r');
  case 't': return('\t');
  case 'v': return('\v');
  default: return c;
  }
  return c;
}

std::istream& operator>> (std::istream& is, symbol& s)
{
  std::string str;
  char c;
  if (!(is >> c)) return is;           // If read fails, return error
  if (dont_escape(c) || c == ESCAPE) { // Recognize a normal symbol
    do {
      if (c == ESCAPE) {
	if (!is.get(c)) return is;     //  Read next character; return if read fails.
	str.push_back(escaped_char(c));//  Push escaped char onto string.
      }
      else
	str.push_back(c);
    }
    while (is.get(c) && (dont_escape(c) || c == ESCAPE));
    if (!is.fail())                    //  Did we read one too many chars?
      is.putback(c);                   //   Yes.  Put it back.
    else if (is.eof())                 //  Are we at eof?
      is.clear(is.rdstate() & ~std::ios::failbit & ~std::ios::eofbit);
    s = symbol(str);                   //  Load string into symbol
  }
  else if (c == OPENQUOTE) {           // Recognize a quoted string
    if (!is.get(c)) return is;         //  Read next character; return if read fails
    while (c != CLOSEQUOTE) {
      if (c == ESCAPE) {               //  Is this character the escape character?
	if (!is.get(c)) return is;     //   Yes.  Get quoted character.
	str.push_back(escaped_char(c));//   Push character onto string.
      }
      else
	str.push_back(c);              //   Push back ordinary character.
      if (!is.get(c)) return is;       //  Read next character.
    }
    s = symbol(str);                   //  Load string into symbol
  }
  else if (c == UNDEFINED[0]) {
    for (const char* cp = &UNDEFINED[1]; *cp; ++cp)
      if (!is.get(c) || c != *cp) {
	is.clear(std::ios::failbit);   //  We didn't get the whole UNDEFINED symbol
	return is;
      }
    s = symbol::undefined();           //  Set s to undefined
  }
  else {                               // c doesn't begin a symbol
    is.putback(c);                     // put it back onto the stream
    is.clear(std::ios::failbit);       // set the fail bit
  }
  return is;
}


std::ostream& operator<< (std::ostream& os, const symbol s)
{
  if (s.is_undefined())
    os << UNDEFINED;
  else {
    const std::string& str = s.string_reference();
    if (str.empty())
      os << OPENQUOTE << CLOSEQUOTE;
    else
      for (std::string::const_iterator si = str.begin(); si != str.end(); ++si) {
	if (!dont_escape(*si))
	  os.put(ESCAPE);
	os.put(*si);
      }
  }
  return os;
}

#ifdef SYM_CC_MAIN

// The rest of this file contains a program that tests the write/read properties of symbols

#include <sstream>
#include <vector>
#include "utility.h"

int main(int argc, char** argv) {
  const size_t ns = 1000000;

  const char *syms[] = { "Hello world", "1", "2.0e-5", "this", "is", "a", "test", 
		   "'", "", "\"", "\\", " ", "-", "'-'", "**", "&", "`", "`'",
		   "(", ")", "()", ")(", "][" };
  const size_t nsyms = sizeof(syms)/sizeof(syms[0]);
  typedef std::set<symbol> sS;
  typedef std::vector<symbol> Ss;

  sS s;
  Ss vs;
  for (size_t i = 0; i < nsyms; ++i) {
    s.insert(symbol(syms[i]));
    vs.push_back(symbol(syms[i]));
  }
  s.insert(symbol::undefined());
  vs.push_back(symbol::undefined());
  
  std::ostringstream os;
  os << s;
  std::ostringstream ovs;
  ovs << vs;
  // std::cout << vs << std::endl;

  // create a lot of symbols
  //
  Ss ss;
  for (size_t i = 0; i < ns; ++i) {
    std::ostringstream os1;
    os1 << i;
    ss.push_back(symbol(os1.str()));  // ss will resize several times
  }  
  
  std::istringstream is(os.str());
  sS s1;
  is >> s1;
  if (s1 != s) {
    std::cerr << "Oops: these two sets are different!\n" << s << '\n' << s1 << std::endl;
    exit(EXIT_FAILURE);
  }
 
  std::istringstream ivs(ovs.str());
  Ss vs1;
  ivs >> vs1;
  if (vs1 != vs) {
    std::cerr << "Oops: these two vectors are different!\n" << vs << '\n' << vs1 << std::endl;
    exit(EXIT_FAILURE);
  }
  
  for (size_t i = 0; i < ns; ++i) {
    std::ostringstream os1;
    os1 << i;
    if (ss[i] != symbol(os1.str())) {
      std::cerr << "Oops: os1.str() = " << os1.str() 
		<< ", ss[" << i << "] = " << ss[i] << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  std::cerr << "All tests passed." << std::endl;
}

#endif // MAIN
