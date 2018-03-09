# coding: utf-8

"""Test of the wordseg_eval module"""

import pytest

from wordseg.evaluate import read_data, evaluate
from wordseg.separator import Separator


def test_read_data():
    assert read_data([]) == ([], [], [])
    assert read_data(['a']) == (['a'], [{(0, 1)}], ['a'])
    assert read_data(['a b', 'c']) == (
        ['ab', 'c'], [{(0, 1), (1, 2)}, {(0, 1)}], ['a', 'b', 'c'])


def test_gold_on_gold():
    gold = ['the dog bites the dog']
    for v in evaluate(gold, gold).values():
        assert v == 1.0


def test_ipa():
    separator = Separator(phone=None, syllable=None, word=' ')
    text = ['juːviː mɔː kʊkɪz ']
    gold = ['juː viː mɔː kʊkɪz']
    evaluate(text, gold, separator=separator)


# auxiliary function
def _test_basic(text, gold, expected):
    assert evaluate(text, gold) == pytest.approx(expected)


def test_basic_1():
    text = ['the dog bites thedog']
    gold = ['the dog bites the dog']
    expected = {
        'type_fscore': 0.8571428571428571,
        'type_precision': 0.75,
        'type_recall': 1.0,
        'token_fscore': 0.6666666666666666,
        'token_precision': 0.75,
        'token_recall': 0.6,
        'boundary_all_fscore': 0.9090909090909091,
        'boundary_all_precision': 1.0,
        'boundary_all_recall': 0.8333333333333334,
        'boundary_noedge_fscore': 0.8571428571428571,
        'boundary_noedge_precision': 1.0,
        'boundary_noedge_recall': 0.75}
    _test_basic(text, gold, expected)


def test_basic_2():
    text = ['thedog bites thedog']
    gold = ['the dog bites the dog']
    expected = {
        'type_fscore': 0.4,
        'type_precision': 0.5,
        'type_recall': 0.3333333333333333,
        'token_fscore': 0.25,
        'token_precision': 0.3333333333333333,
        'token_recall': 0.2,
        'boundary_all_fscore': 0.8,
        'boundary_all_precision': 1.0,
        'boundary_all_recall': 0.6666666666666666,
        'boundary_noedge_fscore': 0.6666666666666666,
        'boundary_noedge_precision': 1.0,
        'boundary_noedge_recall': 0.5}
    _test_basic(text, gold, expected)


def test_basic_3():
    text = ['thedogbitest hedog']
    gold = ['the dog bites the dog']
    expected = {
        'type_fscore': 0,
        'type_precision': 0,
        'type_recall': 0,
        'token_fscore': 0,
        'token_precision': 0,
        'token_recall': 0,
        'boundary_all_fscore': 0.4444444444444444,
        'boundary_all_precision': 0.6666666666666666,
        'boundary_all_recall': 0.3333333333333333,
        'boundary_noedge_fscore': 0,
        'boundary_noedge_precision': 0,
        'boundary_noedge_recall': 0}
    _test_basic(text, gold, expected)


def test_basic_4():
    text = ['th e dog bit es the d og']
    gold = ['the dog bites the dog']
    expected = {
        'type_fscore': 0.36363636363636365,
        'type_precision': 0.25,
        'type_recall': 0.6666666666666666,
        'token_fscore': 0.3076923076923077,
        'token_precision': 0.25,
        'token_recall': 0.4,
        'boundary_all_fscore': 0.8,
        'boundary_all_precision': 0.6666666666666666,
        'boundary_all_recall': 1.0,
        'boundary_noedge_fscore': 0.7272727272727273,
        'boundary_noedge_precision': 0.5714285714285714,
        'boundary_noedge_recall': 1.0}
    _test_basic(text, gold, expected)


def test_basic_5():
    text = ['the band age of the band age']
    gold = ['the bandage of the band age']
    expected = {
        'type_fscore': 0.8888888888888888,
        'type_precision': 1.0,
        'type_recall': 0.8,
        'token_fscore': 0.7692307692307693,
        'token_precision': 0.7142857142857143,
        'token_recall': 0.8333333333333334,
        'boundary_all_fscore': 0.9333333333333333,
        'boundary_all_precision': 0.875,
        'boundary_all_recall': 1.0,
        'boundary_noedge_fscore': 0.9090909090909091,
        'boundary_noedge_precision': 0.8333333333333334,
        'boundary_noedge_recall': 1.0}
    _test_basic(text, gold, expected)


def test_boundary_1():
    text = ['hello']
    gold = ['hello']

    score = {k: v for k, v in evaluate(text, gold).items() if 'boundary' in k}
    expected = {
        'boundary_all_precision': 1.0,
        'boundary_all_recall': 1.0,
        'boundary_all_fscore': 1.0,
        'boundary_noedge_precision': None,
        'boundary_noedge_recall': None,
        'boundary_noedge_fscore': None}

    for k, v in expected.items():
        assert score[k] == v, k


def test_boundary_2():
    text = ['he llo']
    gold = ['hello']

    score = evaluate(text, gold)
    expected = {
        'boundary_all_precision': 2.0 / 3.0,
        'boundary_all_recall': 1.0,
        'boundary_all_fscore': 4.0 / 5.0,
        'boundary_noedge_precision': 0,
        'boundary_noedge_recall': None,
        'boundary_noedge_fscore': 0}

    for k, v in expected.items():
        assert score[k] == v, k


def test_boundary_3():
    text = ['hell o']
    gold = ['h ello']

    score = evaluate(text, gold)
    expected = {
        'boundary_all_precision': 2.0 / 3.0,
        'boundary_all_recall': 2.0 / 3.0,
        'boundary_all_fscore': 2.0 / 3.0,
        'boundary_noedge_precision': 0,
        'boundary_noedge_recall': 0,
        'boundary_noedge_fscore': 0}

    for k, v in expected.items():
        assert score[k] == v, k
