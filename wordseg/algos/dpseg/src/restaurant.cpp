#include "pitman_yor/restaurant.hh"

#include <algorithm>
#include <sstream>
#include <vector>
#include <cassert>


pitman_yor::restaurant::restaurant()
    : m_ncustomers(0), m_ntables(0), m_occupation_map()
{}


pitman_yor::restaurant::restaurant(const restaurant& other)
    : m_ncustomers(other.m_ncustomers),
      m_ntables(other.m_ntables),
      m_occupation_map(other.m_occupation_map)
{}


pitman_yor::restaurant::~restaurant()
{}


void pitman_yor::restaurant::insert_old(double r, const double& a)
{
    // when r is not positive, we have reached our table
    for (auto it = m_occupation_map.begin(); it != m_occupation_map.end(); ++it)
    {
        // decrement r
        r -= it->second * (it->first - a);

        if (r <= 0)
        {
            // increment total amount of customers
            m_ncustomers += 1;

            // increment new occupation for the choosen table
            std::size_t occupation = it->first + 1;

            // register the choosen table with that occupation
            m_occupation_map[occupation] += 1;

            // unregister the choosen table from the previous occupation
            it->second -= 1;

            // clean up empty tables
            clean();

            return;
        }
    }

    // check that we actually updated a table
    assert(r <= 0);
}


void pitman_yor::restaurant::insert_new()
{
    m_ncustomers++;
    m_ntables++;
    m_occupation_map[1]++;  // a new table with a single customer
}


std::size_t pitman_yor::restaurant::erase(int r)
{
    for (auto it = m_occupation_map.begin(); it != m_occupation_map.end(); ++it)
    {
        // decrement r
        r -= it->first * it->second;

        if (r <= 0)
        {
            // decrement total amount of customers
            m_ncustomers -= 1;

            // increment new occupation for the choosen table
            std::size_t n1 = it->first - 1;

            // register the choosen table with that occupation, or
            // suppress a table if there is no more occupants
            if (n1 == 0)
                m_ntables -= 1;
            else
                m_occupation_map[n1] += 1;

            // unregister the choosen table from the previous occupation
            it->second -= 1;

            // clean up empty tables
            clean();

            return n1;
        }
    }

    // shouldn't ever get here
    assert(r <= 0);
    return 0;
}


const std::map<std::size_t, std::size_t>& pitman_yor::restaurant::get_n_m() const
{
    return m_occupation_map;
}


const std::size_t& pitman_yor::restaurant::get_n() const
{
    return m_ncustomers;
}


const std::size_t& pitman_yor::restaurant::get_m() const
{
    return m_ntables;
}


bool pitman_yor::restaurant::is_empty() const
{
    assert(m_ntables <= m_ncustomers);
    return m_ncustomers == 0;
}


void pitman_yor::restaurant::clean()
{
    // std::wcout << "restaurant::clean " << std::endl;
    std::vector<std::size_t> to_erase;
    for (const auto& elem: m_occupation_map)
    {
        if (elem.second == 0)
        {
            to_erase.push_back(elem.first);
        }
    }

    if (to_erase.size() > 0)
    {
        for (auto& key: to_erase)
        {
            // std::wcout << "restaurant::clean erase " << key << ", " << m_occupation_map[key] << std::endl;
            m_occupation_map.erase(key);
        }
    }
    // std::wcout << "restaurant::clean done" << std::endl;
}

bool pitman_yor::restaurant::sanity_check() const
{
    std::vector<bool> sane;
    sane.push_back(m_ntables > 0);
    sane.push_back(m_ncustomers > 0);
    sane.push_back(m_ntables <= m_ncustomers);

    std::size_t mm = 0, nn = 0;
    for(const auto& item: m_occupation_map)
    {
        mm += item.second;
        nn += item.first * item.second;
    }

    sane.push_back(m_ncustomers == nn);
    sane.push_back(m_ntables == mm);

    return std::all_of(sane.begin(), sane.end(), [](bool b){return b;});
}


std::wostream& pitman_yor::restaurant::print(std::wostream& os) const
{
    std::wostringstream occupation;
    for (const auto& elem: m_occupation_map)
    {
        occupation << "(" << elem.first << " " << elem.second << ")";
    }

    return os << "(n=" << m_ncustomers << ", m=" << m_ntables << ", n_m=" << occupation.str() << ")";
}


std::wostream& operator<<(std::wostream& os, const pitman_yor::restaurant& r)
{
    r.print(os);
    return os;
}
