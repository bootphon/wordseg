#ifndef _PY_RESTAURANT_H
#define _PY_RESTAURANT_H

#include <iostream>
#include <map>


namespace pitman_yor
{
    class restaurant
    {
        // total number of customers at tables with this label
        std::size_t n;

        // number of tables with this label
        std::size_t m;

        // number of customers at table -> number of tables
        std::map<std::size_t, std::size_t> n_m;

    public:
        restaurant();
        restaurant(const restaurant& other);
        ~restaurant();

        // inserts a customer at a random old table using PY sampling
        // distribution
        void insert_old(double r, const double& a);

        // inserts a customer at a new table
        void insert_new();

        // removes a customer from a randomly chosen table, returns number
        // of customers left at table
        std::size_t erase(int r);

        // getters
        const std::map<std::size_t, std::size_t>& get_n_m() const;
        const std::size_t& get_n() const;
        const std::size_t& get_m() const;

        // true if there are no customers left with this label
        bool is_empty() const;

        // sanity_check() checks that all of our numbers add up
        bool sanity_check() const;

        // print description to an output stream
        std::wostream& print(std::wostream& os) const;
    };
}


#endif  // _PY_RESTAURANT_H
