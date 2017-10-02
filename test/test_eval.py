"""Test of the wordseg_eval module"""


import pytest
from wordseg.evaluate import read_data, evaluate

from . import gold

def test_read_data():
    assert read_data([]) == ([], [])
    assert read_data(['a']) == (['a'], [{(0, 1)}])
    assert read_data(
        ['a b', 'c']) == (['ab', 'c'], [{(0, 1), (1, 2)}, {(0, 1)}])


def test_evaluate(gold):
    for v in evaluate(gold, gold).values():
        assert v == 1.0
