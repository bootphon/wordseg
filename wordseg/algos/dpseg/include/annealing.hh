#ifndef _ANNEALING_HPP
#define _ANNEALING_HPP

#include <utility>  // for std::size_t


// Computes the annealing temperature at a given iteration
//
// If anneal_a is zero, we use the annealing schedule from Sharon
// Goldwater's ACL06, where anneal_iterations are broken into 9 equal
// sized bins, where the ith bin has temperature 10/(bin+1).  If
// anneal_a is non-zero, we use a sigmoid based annealing function.
class annealing
{
public:
    annealing(
        const std::size_t& max_iterations,
        const double& start_temperature,
        const double& stop_temperature,
        const double& anneal_a,
        const double& anneal_b);

    virtual ~annealing();

    // returns the annealing temperature to be used at each iteration.
    double temperature(const std::size_t& iteration) const;

protected:
    // number of iterations to read the stop temperature
    std::size_t m_max_iterations;

    // start and stop temperature for annealing
    double m_start_temperature, m_stop_temperature;

    // a and b paramaters in sigmoid function
    double m_a, m_b;

    // s0 and s1 are derived from a and b
    double m_s0, m_s1;
};


#endif  // _ANNEALING_HPP
