/*
  Copyright 2009-2014 Mark Johnson
  Copyright 2017 Mathieu Bernard

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _LABELCOUNT_TREE_HH
#define _LABELCOUNT_TREE_HH


#include <algorithm>
#include <iostream>
#include <set>

#include "tree.hpp"
#include "symbol.hh"


// a tree with nodes containing a category label and an integer count
class catcount_tree : public tree<symbol, catcount_tree>
{
    using count_type = std::size_t;

public:
    catcount_tree(symbol cat=symbol(), count_type count=0);

    // return the count of this tree
    count_type count() const;

    // increment the tree count
    void increment(const count_type& delta);

    // decrement the tree count
    void decrement(const count_type& delta);

    // get the compact_tree flag
    static bool get_compact_trees();

    // get the compact_tree flag
    static void set_compact_trees(bool flag);

    // equality operator
    bool operator== (const catcount_tree& other) const;

    // deletes all nodes from the top of the tree that have a zero count
    void selective_delete();

    // swaps the contents of two catcounttrees
    void swap(catcount_tree& other);

    std::ostream& write_label(std::ostream& os) const;

    std::ostream& write_tree(std::ostream& os) const;

private:
    static bool m_compact_trees;

    count_type m_count;

    void selective_delete_helper(std::set<catcount_tree*>& to_delete);

};


#endif  // _LABELCOUNT_TREE_HH
