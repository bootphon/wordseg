#include "estimator/parameters.hh"


estimator::parameters::parameters(
    bool do_mbdp,
    double pstop,
    double hypersampling_ratio,
    double init_pboundary,
    const std::string& forget_method,
    double aeos,
    double a1,
    double b1,
    double a2,
    double b2,
    double pya_beta_a,
    double pya_beta_b,
    double pyb_gamma_c,
    double pyb_gamma_s,
    std::size_t trace_every,
    std::size_t token_memory,
    std::size_t type_memory)
    : m_do_mbdp(do_mbdp),
      m_pstop(pstop),
      m_hypersampling_ratio(hypersampling_ratio),
      m_init_pboundary(init_pboundary),
      m_forget_method(forget_method),
      m_aeos(aeos),
      m_a1(a1),
      m_b1(b1),
      m_a2(a2),
      m_b2(b2),
      m_pya_beta_a(pya_beta_a),
      m_pya_beta_b(pya_beta_b),
      m_pyb_gamma_c(pyb_gamma_c),
      m_pyb_gamma_s(pyb_gamma_s),
      m_trace_every(trace_every),
      m_token_memory(token_memory),
      m_type_memory(type_memory)
{}


estimator::parameters::~parameters()
{}


bool estimator::parameters::do_mbdp() const
{
    return m_do_mbdp;
}

double estimator::parameters::pstop() const
{
    return m_pstop;
}

double estimator::parameters::hypersampling_ratio() const
{
    return m_hypersampling_ratio;
}

double estimator::parameters::init_pboundary() const
{
    return m_init_pboundary;
}

std::string estimator::parameters::forget_method() const
{
    return m_forget_method;
}

double estimator::parameters::aeos() const
{
    return m_aeos;
}

double estimator::parameters::a1() const
{
    return m_a1;
}

double estimator::parameters::b1() const
{
    return m_b1;
}

double estimator::parameters::a2() const
{
    return m_a2;
}

double estimator::parameters::b2() const
{
    return m_b2;
}

double estimator::parameters::pya_beta_a() const
{
    return m_pya_beta_a;
}

double estimator::parameters::pya_beta_b() const
{
    return m_pya_beta_b;
}

double estimator::parameters::pyb_gamma_c() const
{
    return m_pyb_gamma_c;
}

double estimator::parameters::pyb_gamma_s() const
{
    return m_pyb_gamma_s;
}

std::size_t estimator::parameters::trace_every() const
{
    return m_trace_every;
}

std::size_t estimator::parameters::token_memory() const
{
    return m_token_memory;
}

std::size_t estimator::parameters::type_memory() const
{
    return m_type_memory;
}
