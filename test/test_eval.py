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
    text = ['juː;ewordviː;eword mɔː;eword kʊkɪz;eword']
    gold = ['juː viː mɔː kʊkɪz']
    evaluate(text, gold, separator=Separator(word=';eword'))


# auxiliary function
def _test_basic(text, gold, expected):
    assert evaluate(text, gold) == pytest.approx(expected)


def test_basic_1():
    gold = ['the dog bites the dog']
    text = ['the dog bites thedog']
    expected = {
        'type_fscore': 0.8571428571428571,
        'type_precision': 0.75,
        'type_recall': 1.0,
        'token_fscore': 0.6666666666666666,
        'token_precision': 0.75,
        'token_recall': 0.6,
        'boundary_fscore': 0.8571428571428571,
        'boundary_precision': 1.0,
        'boundary_recall': 0.75}
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
        'boundary_fscore': 0.6666666666666666,
        'boundary_precision': 1.0,
        'boundary_recall': 0.5}
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
        'boundary_fscore': 0,
        'boundary_precision': 0,
        'boundary_recall': 0}
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
        'boundary_fscore': 0.7272727272727273,
        'boundary_precision': 0.5714285714285714,
        'boundary_recall': 1.0}
    _test_basic(text, gold, expected)


def test_basic_5():
    gold = ['the bandage of the band age']
    text = ['the band age of the band age']
    expected = {
        'type_fscore': 0.8888888888888888,
        'type_precision': 1.0,
        'type_recall': 0.8,
        'token_fscore': 0.7692307692307693,
        'token_precision': 0.7142857142857143,
        'token_recall': 0.8333333333333334,
        'boundary_fscore': 0.9090909090909091,
        'boundary_precision': 0.8333333333333334,
        'boundary_recall': 1.0}
    _test_basic(text, gold, expected)
