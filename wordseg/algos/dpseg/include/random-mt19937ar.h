/*
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

#ifndef RANDOM_MT19937AR_H
#define RANDOM_MT19937AR_H


#include <random>


// random number generator based on mersenne twister from the stdlib.
// T must be a floating point number type.
template<typename T=double>
class uniform01_type
{
private:
    std::mt19937 m_generator;
    std::uniform_real_distribution<T> m_distribution;

public:
    using result_type = T;

    uniform01_type(const T& min = 0, const T& max = 1)
        : m_generator(), m_distribution(min, max)
        {}

    ~uniform01_type()
        {}

    // Return a random number in [min, max)
    T operator()()
        {
            return m_distribution(m_generator);
        }

    void seed(const std::mt19937::result_type& new_seed)
        {
            m_generator.seed(new_seed);
        }
};


#endif // RANDOM_MT19937AR_H
