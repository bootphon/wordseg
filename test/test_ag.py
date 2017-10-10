"""Test of the wordseg.algos.ag module"""

import os
import pytest

from wordseg.algos import ag
from . import prep


def test_grammar_files():
    assert len(ag.get_grammar_files()) != 0


def test_ag_base(prep):
    grammar = os.path.join(
        os.path.dirname(ag.get_grammar_files()[0]),
        'Colloc0_enFestival.lt')
    assert os.path.isfile(grammar)

    ag.segment(prep, grammar)
