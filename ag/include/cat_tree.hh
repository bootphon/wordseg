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


#ifndef _CAT_TREE_HH
#define _CAT_TREE_HH

#include "tree.hpp"
#include "symbol.hh"


// a tree with nodes containing a category label only
class cat_tree : public tree<symbol, cat_tree>
{
    cat_tree(symbol label = symbol());
};


#endif  // _LABEL_TREE_HH
