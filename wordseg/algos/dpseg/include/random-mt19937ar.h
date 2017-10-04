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
