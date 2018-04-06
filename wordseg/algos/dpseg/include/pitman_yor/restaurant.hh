#ifndef _PY_RESTAURANT_H
#define _PY_RESTAURANT_H

#include <iostream>
#include <map>


namespace pitman_yor
{
    class restaurant
    {
    public:
        restaurant();
        restaurant(const restaurant& other);
        ~restaurant();

        // getters
        const std::map<std::size_t, std::size_t>& get_n_m() const;
        const std::size_t& get_n() const;
        const std::size_t& get_m() const;

        // inserts a customer at a random old table using pitman-yor
        // sampling distribution
        void insert_old(double r, const double& a);

        // inserts a customer at a new table
        void insert_new();

        // removes a customer from a randomly chosen table, returns number
        // of customers left at table
        std::size_t erase(int r);

        // true if there are no customers left with this label
        bool is_empty() const;

        // sanity_check() checks that all of our numbers add up
        bool sanity_check() const;

        // print description to an output stream
        std::wostream& print(std::wostream& os) const;

        friend std::wostream& operator<<(std::wostream& os, const restaurant& r);

    private:
        // total number of customers at tables with this label
        std::size_t m_ncustomers;

        // number of tables with this label
        std::size_t m_ntables;

        // number of customers at table -> number of tables
        std::map<std::size_t, std::size_t> m_occupation_map;

        // cleanup the occupation map, erase entries when count is 0
        void clean();
    };
}


#endif  // _PY_RESTAURANT_H
