#ifndef _QUADMATH_HH
#define _QUADMATH_HH

#ifndef QUADPREC

typedef double F;

#else

#include <boost/multiprecision/float128.hpp>
 #include <math.h>
#include <iostream>

typedef boost::multiprecision::float128 F;

inline F power(F x, F y)
{
    return y == 1 ? x : std::pow(double(x), double(y));
}


// inline std::istream& operator>> (std::istream& is, __float128& x)
// {
//     double x0;
//     is >> x0;
//     x = x0;
//     return is;
// }


// inline std::ostream& operator<< (std::ostream& os, const __float128& x)
// {
//     return os << double(x);
// }

#endif

#endif  // _QUADMATH_HH
