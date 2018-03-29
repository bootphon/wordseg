#include "chinese_restaurant.hh"
#include "util.hpp"
#include <cassert>

ChineseRestaurant::ChineseRestaurant()
    : n(), m(), n_m()
{}


ChineseRestaurant::ChineseRestaurant(const ChineseRestaurant& other)
    : n(other.n), m(other.m), n_m(other.n_m)
{}


ChineseRestaurant::~ChineseRestaurant()
{}


void ChineseRestaurant::insert_old(F r, const F& a)
{
    // when r is not positive, we have reached our table
    for (auto it = n_m.begin(); it != n_m.end(); ++it)
    {
        // decrement r
        r -= it->second * (it->first - a);

        if (r <= 0)
        {
            // new table size
            U n1 = it->first+1;

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


void ChineseRestaurant::insert_new()
{
    ++n;
    ++m;
    ++n_m[1];
}


U ChineseRestaurant::erase(I r)
{
    --n;
    for (auto it = n_m.begin(); it != n_m.end(); ++it)
    {
        if ((r -= it->first * it->second) <= 0)
        {
            // new table size
            U n1 = it->first-1;
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


const std::map<U, U>& ChineseRestaurant::get_n_m() const
{
    return n_m;
}

U ChineseRestaurant::get_n() const
{
    return n;
}


U ChineseRestaurant::get_m() const
{
    return m;
}


bool ChineseRestaurant::is_empty() const
{
    assert(m <= n);
    return n == 0;
}


bool ChineseRestaurant::sanity_check() const
{
    assert(m > 0);
    assert(n > 0);
    assert(m <= n);

    U mm = 0, nn = 0;
    for(const auto& item: n_m)
    {
        // assert(n > 0);   // shouldn't have any empty tables
        // assert(m > 0);

        mm += item.second;
        nn += item.first * item.second;
    }

}


std::wostream& ChineseRestaurant::print(std::wostream& os) const
{
    return os << "(n=" << n << ", m=" << m << ", n_m=" << n_m << ")";
}
