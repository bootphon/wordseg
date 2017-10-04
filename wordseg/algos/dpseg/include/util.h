#ifndef UTIL_H
#define UTIL_H

#include <algorithm>
#include <iostream>
#include <map>
#include <utility>
#include <vector>


// Predeclaration of the following operator<< functions because
// compilation failed on OSX with xbuild-7.3
template <typename X, typename Y>
std::wostream& operator<< (std::wostream&, const std::pair<X,Y>&);

template <typename Key, typename Value>
std::wostream& operator<< (std::wostream&, const std::map<Key,Value>&);

template <typename Value>
std::wostream& operator<< (std::wostream&, const std::vector<Value>&);


template <typename X, typename Y>
std::wostream& operator<< (std::wostream& os, const std::pair<X,Y>& xy)
{
    return os << '(' << xy.first << ' ' << xy.second << ')';
}


template <typename Key, typename Value>
std::wostream& operator<< (std::wostream& os, const std::map<Key,Value>& k_v)
{
    os << L'(';
    const wchar_t* sep = L"";
    for(const auto& item: k_v)
    {
        os << sep << L'(' << item.first << L' ' << item.second << L')';
        sep = L" ";
    }
    return os << L')';
}


template <typename Value>
std::wostream& operator<< (std::wostream& os, const std::vector<Value>& vs)
{
    os << L'(';
    const wchar_t* sep = L"";
    for(const auto& item: vs)
    {
        os << sep << item;
        sep = L" ";
    }
    return os << L')';
}



#define HERE   __FILE__ << ":" << __LINE__ << " in " << __func__

#ifndef __STRING
#define __STRING(x) #x
#endif

#define TRACE(expr) std::wcerr << HERE << ", " << __STRING(expr) << " = " << (expr) << std::endl

#define TRACE1(expr) TRACE(expr)

#define TRACE2(expr1, expr2)						     \
  std::wcerr << HERE                                                         \
            << ", " << __STRING(expr1) << " = " << (expr1)                   \
            << ", " << __STRING(expr2) << " = " << (expr2) << std::endl

#define TRACE3(expr1, expr2, expr3)					     \
  std::wcerr << HERE                                                         \
            << ", " << __STRING(expr1) << " = " << (expr1)                   \
            << ", " << __STRING(expr2) << " = " << (expr2)                   \
            << ", " << __STRING(expr3) << " = " << (expr3) << std::endl

#define TRACE4(expr1, expr2, expr3, expr4)				     \
  std::wcerr << HERE                                                         \
            << ", " << __STRING(expr1) << " = " << (expr1)                   \
            << ", " << __STRING(expr2) << " = " << (expr2)                   \
            << ", " << __STRING(expr3) << " = " << (expr3)                   \
            << ", " << __STRING(expr4) << " = " << (expr4) << std::endl

#define TRACE5(expr1, expr2, expr3, expr4, expr5)			     \
  std::wcerr << HERE                                                         \
            << ", " << __STRING(expr1) << " = " << (expr1)                   \
            << ", " << __STRING(expr2) << " = " << (expr2)                   \
            << ", " << __STRING(expr3) << " = " << (expr3)                   \
            << ", " << __STRING(expr4) << " = " << (expr4)                   \
            << ", " << __STRING(expr5) << " = " << (expr5) << std::endl

#define TRACE6(expr1, expr2, expr3, expr4, expr5, expr6)		     \
  std::wcerr << HERE                                                         \
            << ", " << __STRING(expr1) << " = " << (expr1)                   \
            << ", " << __STRING(expr2) << " = " << (expr2)                   \
            << ", " << __STRING(expr3) << " = " << (expr3)                   \
            << ", " << __STRING(expr4) << " = " << (expr4)                   \
            << ", " << __STRING(expr5) << " = " << (expr5)                   \
            << ", " << __STRING(expr6) << " = " << (expr6) << std::endl

#define TRACE7(expr1, expr2, expr3, expr4, expr5, expr6, expr7)		     \
  std::wcerr << HERE                                                         \
            << ", " << __STRING(expr1) << " = " << (expr1)                   \
            << ", " << __STRING(expr2) << " = " << (expr2)                   \
            << ", " << __STRING(expr3) << " = " << (expr3)                   \
            << ", " << __STRING(expr4) << " = " << (expr4)                   \
            << ", " << __STRING(expr5) << " = " << (expr5)                   \
            << ", " << __STRING(expr6) << " = " << (expr6)                   \
            << ", " << __STRING(expr7) << " = " << (expr7) << std::endl


#endif // UTIL_H
