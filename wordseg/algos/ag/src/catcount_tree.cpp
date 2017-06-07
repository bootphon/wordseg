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

#include "catcount_tree.hh"



bool catcount_tree::m_compact_trees = false;


catcount_tree::catcount_tree(symbol label, count_type count)
    : tree<symbol, catcount_tree>(label), m_count(count)
{}


catcount_tree::count_type catcount_tree::count() const
{
    return m_count;
}


void catcount_tree::increment(const count_type& delta)
{
    m_count += delta;
}


void catcount_tree::decrement(const count_type& delta)
{
    m_count -= delta;
}


bool catcount_tree::get_compact_trees()
{
    return m_compact_trees;
}


void catcount_tree::set_compact_trees(bool flag)
{
    m_compact_trees = flag;
}


bool catcount_tree::operator== (const catcount_tree& other) const
{
    return label() == other.label()
        and count() == other.count()
        and equal_children(other);
}


void catcount_tree::selective_delete_helper(std::set<catcount_tree*>& to_delete)
{
    if (m_count == 0)
    {
        to_delete.insert(this);
        for (auto child: children())
            child->selective_delete_helper(to_delete);
    }
}


void catcount_tree::selective_delete()
{
    std::set<catcount_tree*> to_delete;
    selective_delete_helper(to_delete);

    // assert((m_count != 0) and to_delete.empty());

    for(auto* child: to_delete)
        delete child;
}

void catcount_tree::swap(catcount_tree& other)
{
    std::swap(m_count, other.m_count);
    generalize().swap(other.generalize());
}


std::ostream& catcount_tree::write_label(std::ostream& os) const
{
    return (m_compact_trees || count() == 0)  ? (os << label()) : (os << label() << '#' << count());
}

std::ostream& catcount_tree::write_tree(std::ostream& os) const
{
    if (children().empty())
        return write_label(os);

    else if (m_compact_trees and m_count == 0)
    {
        for (auto it = children().cbegin(); it != children().cend(); ++it)
        {
            if (it != children().cbegin())
                os << ' ';
            (*it)->specialize().write_tree(os);
        }
        return os;
    }
    else
    {
        os << '(';
        specialize().write_label(os);
        for (auto it = children().cbegin(); it != children().cend(); ++it)
            (*it)->specialize().write_tree(os << ' ');
        return os << ')';
    }
}
