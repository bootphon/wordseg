#include "annealing.hh"

#include <cassert>
#include <cmath>


annealing::annealing(
    const std::size_t& max_iterations,
    const double& start_temperature,
    const double& stop_temperature,
    const double& anneal_a,
    const double& anneal_b)
    : m_max_iterations(max_iterations),
      m_start_temperature(start_temperature),
      m_stop_temperature(stop_temperature),
      m_a(anneal_a),
      m_b(anneal_b)
{
    m_s0 = 1 / (1 + std::exp(m_a * (0 - m_b)));
    m_s1 = 1 / (1 + std::exp(m_a * (1 - m_b)));
}


annealing::~annealing()
{}


double annealing::temperature(const std::size_t& iteration) const
{
    if (iteration >= m_max_iterations)
    {
        return m_stop_temperature;
    }

    if (m_a == 0)
    {
        std::size_t bin = (9 * iteration) / m_max_iterations + 1;
        return (10.0 / bin - 1) * (m_start_temperature - m_stop_temperature) / 9.0
            + m_stop_temperature;
    }

    else
    {
        double x = static_cast<double>(iteration) / static_cast<double>(m_max_iterations);
        double s = 1 / (1 + std::exp(m_a * (x - m_b)));
        double temp = (m_start_temperature - m_stop_temperature) * (s - m_s1) / (m_s0 - m_s1) + m_stop_temperature;

        assert(std::isfinite(temp));
        return temp;
    }
}
