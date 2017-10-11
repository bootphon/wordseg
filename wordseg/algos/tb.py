"""tb.py reads, searches and displays trees from Penn Treebank (PTB) format
treebank files.

(c) Mark Johnson, 30th August, 2005, modified 23rd March 2008

Trees are represented in Python as nested list structures in the following
format:

  Terminal nodes are represented by strings.

  Nonterminal nodes are represented by lists.  The first element of
  the list is the node's label (a string), and the remaining elements
  of the list are lists representing the node's children.

This module also defines two regular expressions.

nonterm_re matches Penn treebank nonterminal labels, and parses them into
their various parts.

empty_re matches empty elements (terminals), and parses them into their
various parts.
"""

import re

from wordseg.algos import lx


_header_re = re.compile(r"(\*x\*.*\*x\*[ \t]*\n)*\s*")
_openpar_re = re.compile(r"\s*\(\s*([^ \t\n\r\f\v()]*)\s*")
_closepar_re = re.compile(r"\s*\)\s*")
_terminal_re = re.compile(r"\s*((?:[^ \\\t\n\r\f\v()]|\\.)+)\s*")

# This is such a complicated regular expression that I use the special
# "verbose" form of regular expressions, which lets me index and document it
#
nonterm_re = re.compile(r"""
^(?P<CAT>[A-Z0-9$|^]+)                                  # category comes first
 (?:                                                    # huge disjunct of optional annotations
     (?: - (?P<FORMFUN>ADV|NOM)                         # stuff beginning with -
          |(?P<GROLE>DTV|LGS|PRD|PUT|SBJ|TPC|VOC)
          |(?P<ADV>BNF|DIR|EXT|LOC|MNR|PRP|TMP)
          |(?P<MISC>CLR|CLF|HLN|TTL)
          |(?P<TPC>TPC)
          |(?P<DYS>UNF|ETC|IMP)
          |(?P<INDEX>\d+)
     )
   |(?:=(?P<EQINDEX>\d+))                               # stuff beginning with =
 )*                                                     # Kleene star
$""", re.VERBOSE)


empty_re = re.compile(r"^(?P<CAT>[A-Z0-9\?\*]+)(?:-(?P<INDEX>\d+))")



def read_file(filename):

    """Return a list of the trees in the PTB file filename."""

    filecontents = file(filename, "rU").read()
    pos = _header_re.match(filecontents).end()
    trees = []
    _string_trees(trees, filecontents, pos)
    return trees

def string_trees(s):

    """Returns a list of the trees in PTB-format string s"""

    trees = []
    _string_trees(trees, s)
    return trees

def _string_trees(trees, s, pos=0):

    """Reads a sequence of trees in string s[pos:].
    Appends the trees to the argument trees.
    Returns the ending position of those trees in s."""

    while pos < len(s):
        closepar_mo = _closepar_re.match(s, pos)
        if closepar_mo:
            return closepar_mo.end()
        openpar_mo = _openpar_re.match(s, pos)
        if openpar_mo:
            tree = [openpar_mo.group(1)]
            trees.append(tree)
            pos = _string_trees(tree, s, openpar_mo.end())
        else:
            terminal_mo = _terminal_re.match(s, pos)
            trees.append(terminal_mo.group(1))
            pos = terminal_mo.end()
    return pos


def trees(treebankdir, treebank_re):

    """trees is a generator that iterates through
    the trees in a treebank.  treebankdir is the name
    of the directory in which the treebank files are
    located, and treebank_re is a regular expression
    that all treebank files must match."""

    for filename in lx.findfiles(treebankdir, treebank_re):
        trees = read_file(filename)
        for root in trees:
            tree = root[1]
            yield tree

def subtrees(treebankdir, treebank_re):

    """subtrees is a generator that iterates through all nonterminal
    subtrees of the trees in a treebank."""

    def node_visitor(node):
        yield node
        for child in node[1:]:
            if isinstance(child, list):
                for descendant in node_visitor(child):
                    yield descendant

    for tree in trees(treebankdir, treebank_re):
        for node in node_visitor(tree):
            yield node


def find_tree(treebankdir, treebank_re, interesting_tree_p=lambda tree: True):

    """find_tree displays trees for which interesting_tree_p(tree) is True.

    treebankdir and treebank_re specify the treebank files to be searched.
    interesting_tree_p is a function which is called on each tree in turn.
    If interesting_tree_p does not return False, the tree is displayed
    using drawtree."""

    for filename in lx.findfiles(treebankdir, treebank_re):
        trees = read_file(filename)
        treeno = 0
        for root in trees:
            treeno += 1
            tree = root[1]
            if interesting_tree_p(tree):
                drawtree.tb_tk(tree)
                inp = raw_input(filename+":"+str(treeno)+" find next tree? [Y/n/p] ").strip()
                if len(inp) != 0:
                    if inp[0] == "P" or inp[0] == "p":
                        psfilename = raw_input("Saving postscript image of tree; enter filename for image file? ").strip()
                        if psfilename != "":
                            drawtree.tktb_postscript(psfilename)
                    elif inp[0] != "Y" and inp[0] != "y":
                        return

def find_node(treebankdir, treebank_re, interesting_subtree_p):

    """find_node displays trees which contain interesting subtrees.
    An interesting subtree t is one for which interesting_subtree_p(t)
    is true.

    treebankdir and treebank_re specify which treebank files are to
    be searched.

    interesting_subtree_p is a function which is called on each
    nonterminal subtree of the tree.  If interesting_subtree_p does
    not return False, the tree is displayed using drawtree.  If
    interesting_subtree_p is a string, then that string specifies the
    color that the topmost node of the subtree will be drawn in."""

    def interesting_tree_p(subtree):
        if interesting_subtree_p(subtree):
            interesting = True
            subtree[0] = (subtree[0], 'red')
        else:
            interesting = False
        for child in subtree[1:]:
            if isinstance(child, list):
                interesting = interesting_tree_p(child) or interesting
        return interesting

    find_tree(treebankdir, treebank_re, interesting_tree_p)



def is_terminal(subtree):

    """True if this subtree consists of a single terminal node
    (i.e., a word or an empty node)."""

    return not isinstance(subtree, list)


def is_preterminal(subtree):

    """True if the treebank subtree is rooted in a preterminal node
    (i.e., is an empty node or dominates a word)."""

    return isinstance(subtree, list) and len(subtree) > 1 and reduce(lambda x,y: x and is_terminal(y), subtree[1:], True)


def is_phrasal(subtree):

    """True if this treebank subtree is not a terminal or a preterminal node."""

    return isinstance(subtree, list) and \
           (len(subtree) == 1 or isinstance(subtree[1], list))

def tree_children(tree):

    """Returns a list of the child subtrees of tree."""

    if isinstance(tree, list):
        return tree[1:]
    else:
        return []

def tree_label(tree):

    """Returns the label on the root node of tree."""

    if isinstance(tree, list):
        return tree[0]
    else:
        return tree


def tree_category(tree):

    """Returns the category of the root node of tree."""

    if isinstance(tree, list):
        label = tree[0]
        nonterm_mo = nonterm_re.match(label)
        if nonterm_mo:
            return nonterm_mo.group('CAT')
        else:
            return label

def tree_subtrees(tree):

    """Returns a list of the subtrees of tree."""

    if isinstance(tree, list):
        return tree[1:]
    else:
        return []


def terminals(tree):

    """Returns a list of the terminal strings in tree"""

    def _terminals(node, terms):
        if isinstance(node, list):
            for child in node[1:]:
                _terminals(child, terms)
        else:
            terms.append(node)

    terms = []
    _terminals(tree, terms)
    return terms

def preterminals(tree, sofar=None):

    """Returns a list of the preterminal nodes in the tree"""

    if sofar == None:
        sofar = []
    if tb.is_preterminal(tree):
        sofar.append(tree)
    else:
        for child in tb.tree_children(tree):
            sofar = preterminals(child, sofar)
    return sofar

def any(S):
    for x in S:
        if x:
            return True
    return False

def all(S):
    for x in S:
        if not x:
            return False
    return True
