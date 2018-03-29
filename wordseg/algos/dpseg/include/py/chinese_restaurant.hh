#ifndef _PY_CHINESE_RESTAURANT_H
#define _PY_CHINESE_RESTAURANT_H

#include <iostream>
#include <map>
#include "typedefs.hh"


namespace py
{
    class chinese_restaurant
    {
        // total number of customers at tables with this label
        U n;

        // number of tables with this label
        U m;

        // number of customers at table -> number of tables
        std::map<U, U> n_m;

    public:
        chinese_restaurant();
        chinese_restaurant(const chinese_restaurant& other);
        ~chinese_restaurant();

        // inserts a customer at a random old table using PY sampling
        // distribution
        void insert_old(F r, const F& a);

        // inserts a customer at a new table
        void insert_new();

        // removes a customer from a randomly chosen table, returns number
        // of customers left at table
        U erase(I r);

        // getters
        const std::map<U, U>& get_n_m() const;
        const U& get_n() const;
        const U& get_m() const;

        // true if there are no customers left with this label
        bool is_empty() const;

        // sanity_check() checks that all of our numbers add up
        bool sanity_check() const;

        // print description to an output stream
        std::wostream& print(std::wostream& os) const;
    };
}


#endif  // _PY_CHINESE_RESTAURANT_H
