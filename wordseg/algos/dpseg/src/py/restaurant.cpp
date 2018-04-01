#include "py/restaurant.hh"
#include "util.hpp"
#include <cassert>


py::restaurant::restaurant()
    : n(), m(), n_m()
{}


py::restaurant::restaurant(const restaurant& other)
    : n(other.n), m(other.m), n_m(other.n_m)
{}


py::restaurant::~restaurant()
{}


void py::restaurant::insert_old(double r, const double& a)
{
    // when r is not positive, we have reached our table
    for (auto it = n_m.begin(); it != n_m.end(); ++it)
    {
        // decrement r
        r -= it->second * (it->first - a);

        if (r <= 0)
        {
            // new table size
            uint n1 = it->first+1;

            if (--it->second == 0)
                n_m.erase(it);

            // add customer to new table
            ++n_m[n1];

            break;
        }
    }

    // check that we actually updated a table
    assert(r <= 0);

    // increment no of customers with this label
    ++n;
}


void py::restaurant::insert_new()
{
    ++n;
    ++m;
    ++n_m[1];
}


uint py::restaurant::erase(int r)
{
    --n;
    for (auto it = n_m.begin(); it != n_m.end(); ++it)
    {
        if ((r -= it->first * it->second) <= 0)
        {
            // new table size
            uint n1 = it->first - 1;
            if (--it->second == 0)
                n_m.erase(it);
            if (n1 == 0)
                --m;
            else
                ++n_m[n1];

            return n1;
        }
    }

    // shouldn't ever get here
    assert(r <= 0);
    return 0;
}


const std::map<uint, uint>& py::restaurant::get_n_m() const
{
    return n_m;
}


const uint& py::restaurant::get_n() const
{
    return n;
}


const uint& py::restaurant::get_m() const
{
    return m;
}


bool py::restaurant::is_empty() const
{
    assert(m <= n);
    return n == 0;
}


bool py::restaurant::sanity_check() const
{
    assert(m > 0);
    assert(n > 0);
    assert(m <= n);

    uint mm = 0, nn = 0;
    for(const auto& item: n_m)
    {
        mm += item.second;
        nn += item.first * item.second;
    }

    bool sane_n = (n == nn);
    bool sane_m = (m == mm);

    assert(sane_n);
    assert(sane_m);
    return sane_n && sane_m;
}


std::wostream& py::restaurant::print(std::wostream& os) const
{
    return os << "(n=" << n << ", m=" << m << ", n_m=" << n_m << ")";
}
