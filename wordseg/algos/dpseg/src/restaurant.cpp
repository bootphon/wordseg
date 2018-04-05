#include "pitman_yor/restaurant.hh"
#include "util.hpp"
#include <cassert>


pitman_yor::restaurant::restaurant()
    : n(), m(), n_m()
{}


pitman_yor::restaurant::restaurant(const restaurant& other)
    : n(other.n), m(other.m), n_m(other.n_m)
{}


pitman_yor::restaurant::~restaurant()
{}


void pitman_yor::restaurant::insert_old(double r, const double& a)
{
    // when r is not positive, we have reached our table
    for (auto it = n_m.begin(); it != n_m.end(); ++it)
    {
        // decrement r
        r -= it->second * (it->first - a);

        if (r <= 0)
        {
            // new table size
            std::size_t n1 = it->first+1;

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


void pitman_yor::restaurant::insert_new()
{
    ++n;
    ++m;
    ++n_m[1];
}


std::size_t pitman_yor::restaurant::erase(int r)
{
    --n;
    for (auto it = n_m.begin(); it != n_m.end(); ++it)
    {
        if ((r -= it->first * it->second) <= 0)
        {
            // new table size
            std::size_t n1 = it->first - 1;
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


const std::map<std::size_t, std::size_t>& pitman_yor::restaurant::get_n_m() const
{
    return n_m;
}


const std::size_t& pitman_yor::restaurant::get_n() const
{
    return n;
}


const std::size_t& pitman_yor::restaurant::get_m() const
{
    return m;
}


bool pitman_yor::restaurant::is_empty() const
{
    assert(m <= n);
    return n == 0;
}


bool pitman_yor::restaurant::sanity_check() const
{
    std::vector<bool> sane;
    sane.push_back(m > 0);
    sane.push_back(n > 0);
    sane.push_back(m <= n);

    std::size_t mm = 0, nn = 0;
    for(const auto& item: n_m)
    {
        mm += item.second;
        nn += item.first * item.second;
    }

    sane.push_back(n == nn);
    sane.push_back(m == mm);

    return std::all_of(sane.begin(), sane.end(), [](bool b){return b;});
}


std::wostream& pitman_yor::restaurant::print(std::wostream& os) const
{
    return os << "(n=" << n << ", m=" << m << ", n_m=" << n_m << ")";
}
