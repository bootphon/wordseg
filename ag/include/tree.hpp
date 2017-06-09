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


#ifndef _TREE_HPP
#define _TREE_HPP

#include <iostream>
#include <vector>


/*
  tree is a generic tree type.

  It consists of a generic label together with a sequence of children
  pointers.

  Uses the "Barton and Nackman trick" as described in
  http://osl.iu.edu/~tveldhui/papers/techniques/techniques01.html#l14
*/
template <class LabelType, class ChildrenType>
class tree
{
public:
    using label_type = LabelType;
    using children_type = ChildrenType;
    using children_list_type = std::vector<children_type*>;
    using tree_type = tree<label_type, children_type>;

    tree(label_type label = label_type())
        : m_label(label)
        {}

    // deletes the top node and all of its children
    virtual ~tree()
        {
            for (auto child: m_children)
                delete child;
            delete this;
        }

    // returns the label of this node
    const label_type& label() const
        {
            return m_label;
        }

    children_list_type& children()
        {
            return m_children;
        }

    const children_list_type& children() const
        {
            return m_children;
        }

    // equality operator
    bool operator== (const tree_type& other) const
        {
            return m_label == other.m_label
                and specialize().equal_children(other.specialize());
        }

    // swaps the contents of two tree nodes
    void swap(tree_type& other)
        {
            std::swap(m_children, other.m_children);
            std::swap(m_label, other.m_label);
        }

    // add a child in the children list
    void add_child(children_type* child)
        {
            m_children.push_back(child);
        }


    // returns the terminal yield of the tree
    template <typename terminals_type>
    void terminals(terminals_type& terms) const
        {
            if (m_children.empty())
                terms.push_back(specialize().label());
            else
                for (const auto& it: m_children)
                    it->terminals(terms);
        }

    std::ostream& write_tree(std::ostream& os) const
        {
            if (m_children.empty())
                return specialize().write_label(os);
            else
            {
                os << '(';
                specialize().write_label(os);
                for (const auto& it: m_children)
                    it->specialize().write_tree(os << ' ');
                return os << ')';
            }
        }

    std::ostream& write_label(std::ostream& os) const
        {
            return os << m_label;
        }

    // converts a tree pointer to the more specific treeptr type
    children_type& specialize()
        {
            return static_cast<children_type&>(*this);
        }

    const children_type& specialize() const
        {
            return static_cast<const children_type&>(*this);
        }

    // converts a tree pointer to the more general treeptr type
    tree_type& generalize()
        {
            return static_cast<tree_type&>(*this);
        }

    const tree_type& generalize() const
        {
            return static_cast<tree_type&>(*this);
        }

protected:
    bool equal_children(const tree_type& other) const
        {
            auto it0 = m_children.cbegin();
            auto it1 = other.m_children.cbegin();
            for ( ; it0 != m_children.cend(); ++it0, ++it1)
            {
                if (it1 == other.m_children.cend())
                    return false;
                if (!(**it0 == **it1))
                    return false;
            }
            return it1 == other.m_children.cend();
        }

private:
    label_type m_label;

    children_list_type m_children;
};


template<class LabelType, class ChildrenType>
std::ostream& operator<< (std::ostream& os, const tree<LabelType, ChildrenType>& xt)
{
    return xt.specialize().write_tree(os);
}


template<class LabelType, class ChildrenType>
std::ostream& operator<< (std::ostream& os, const tree<LabelType, ChildrenType>* xtp)
{
    return xtp->specialize().write_tree(os);
}


#endif  // _TREE_HPP
