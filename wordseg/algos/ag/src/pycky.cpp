#include "pycky.hh"


std::istream& readline_symbols(std::istream& is, Ss& syms) {
    syms.clear();
    std::string line;
    if (std::getline(is, line)) {
        std::istringstream iss(line);
        std::string s;
        while (iss >> s)
            syms.push_back(s);
    }
    return is;
}

pycfg_type::pycfg_type()
    : estimate_theta_flag(false), predictive_parse_filter(false),
      default_weight(1), default_pya(1e-1), default_pyb(1e3),
      pya_beta_a(0), pya_beta_b(0), pyb_gamma_s(0), pyb_gamma_c(0)
{}

F pycfg_type::get_pya(S parent) const
{
    S_F::const_iterator it = parent_pya.find(parent);
    return (it == parent_pya.end()) ? default_pya : it->second;
}
