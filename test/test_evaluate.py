# coding: utf-8

"""Test of the wordseg_eval module"""

import numpy as np
import pytest

from wordseg.evaluate import read_data, evaluate, compute_class_labels


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
    text = ['juːviː mɔː kʊkɪz ']
    gold = ['juː viː mɔː kʊkɪz']
    evaluate(text, gold)


# auxiliary function
def _test_basic(text, gold, units, expected):
    pred = evaluate(text, gold, units=units)
    assert pred == pytest.approx(expected)


def test_basic_1():
    units = ['th e d o g b i te s th e d o g']
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
        'boundary_noedge_recall': 0.75,
        'adjusted_rand_index': 0.7845303867403315}
    _test_basic(text, gold, units, expected)


def test_basic_2():
    units = ['t h e d o g b i t e s t h e d o g']
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
        'boundary_noedge_recall': 0.5,
        'adjusted_rand_index': 0.6330935251798561}
    _test_basic(text, gold, units, expected)


def test_basic_3():
    units = ['t h e d o g b i t e s t h e d o g']
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
        'boundary_noedge_recall': 0,
        'adjusted_rand_index': 0.20993589743589744}
    _test_basic(text, gold, units, expected)


def test_basic_4():
    units = ['t h e d o g b i t e s t h e d o g']
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
        'boundary_noedge_recall': 1.0,
        'adjusted_rand_index': 0.66796875}
    _test_basic(text, gold, units, expected)


def test_basic_5():
    units = ['th e b a n d a ge o f th e b a n d a ge']
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
        'boundary_noedge_recall': 1.0,
        'adjusted_rand_index': 0.7804878048780488}
    _test_basic(text, gold, units, expected)


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


@pytest.mark.parametrize('words, units, expected', [
    (['hello world', 'python'], ['h el lo wo r ld', 'py th on'],
     [0, 0, 0, 1, 1, 1, 2, 2, 2]),
    ([], [], [])])
def test_compute_class_labels_good(words, units, expected):
    assert np.array_equal(
        compute_class_labels(words, units),
        np.asarray(expected))


@pytest.mark.parametrize('words, units', [
    ([], ['a']),
    (['a'], []),
    (['hell'], ['hello']),
    (['one', 'two'], ['one']),
    (['one', 'two'], ['one', 'twos'])])
def test_compute_class_labels_bad(words, units):
    with pytest.raises(ValueError):
        compute_class_labels(words, units)
