#pragma once

#include <quadmath.h>

#include <iostream>

inline std::istream& operator>> (std::istream& is, __float128& x) {
  double x0;
  is >> x0;
  x = x0;
  return is;
}

inline std::ostream& operator<< (std::ostream& os, __float128 x) {
  return os << double(x);
}
