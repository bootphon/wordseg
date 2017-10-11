"""Test of the wordseg.algos.ag module"""

import os
import pytest

from wordseg.algos import ag
from . import prep


test_arguments = (
    '-E -P -R -1 -n 50 -r 32565 -a 0.0001 -b 10000.0 '
    '-e 1.0 -f 1.0 -g 100.0 -h 0.01 -x 10')


@pytest.yield_fixture(scope='session')
def grammar():
    grammar = os.path.join(
        os.path.dirname(ag.get_grammar_files()[0]),
        'Coll3syllfnc_enFestival.lt')
    assert os.path.isfile(grammar)
    return grammar


def test_grammar_files():
    assert len(ag.get_grammar_files()) != 0


def test_ag_single_run(prep, grammar):
    raw_parses = ag._run_ag_single(prep, grammar, args=test_arguments)
    trees = [tree for tree in ag._yield_trees(raw_parses)]

    assert len(trees) == 6
    for tree in trees:
        assert len(tree) == len(prep)

    print(type(trees[0]), type(trees[0][0]))
    print(trees[0][0])

    words = ag._tree2words(tree[0], score_category_re=r'Word\b')
    print(words)
