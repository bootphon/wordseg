"""Test of the wordseg_eval module"""

from wordseg.evaluate import read_data, evaluate
from wordseg.algos import tp
from . import prep, gold


def test_read_data():
    assert read_data([]) == ([], [])
    assert read_data(['a']) == (['a'], [{(0, 1)}])
    assert read_data(
        ['a b', 'c']) == (['ab', 'c'], [{(0, 1), (1, 2)}, {(0, 1)}])


def test_evaluate_silly(gold):
    for v in evaluate(gold, gold).values():
        assert v == 1.0


def test_evaluate_segmented(prep, gold):
    s = tp.segment(prep)
    e = evaluate(s, gold)

    # make sure we have different scores for type and token
    assert e['type_fscore'] != e['token_fscore']
    assert e['type_precision'] != e['token_precision']
    assert e['type_recall'] != e['token_recall']
