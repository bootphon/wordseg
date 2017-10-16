# coding: utf-8

"""Test of the 'wordseg-prep' command and it's underlying functions"""

import pytest
from wordseg import utils
from wordseg.separator import Separator
from wordseg.prepare import check_utterance, prepare, _pairwise


# correctly formatted phonological forms
good_utterances = [
    'a ;eword',
    'a ;esyll ;eword',
    'ˌʌ s ;eword t ə ;eword p l eɪ ;eword ð ə ;eword s oʊ l oʊ z ;eword']

# badly formatted phonological forms
bad_utterances = [
    '',
    ' ',
    '\n\n',
    'hello',
    'ah ;esyll ah',
    'ah ;esyll ah ;esyll',  # missing ;eword
    'ah ;esyll ah ;eword',  # missing ;esyll
    'ah ah ; eword',
    ';eword',
    ';eword a b ;esyll ;eword',
    ';esyll a b ;esyll ;eword',
    'a. ;eword',
    'a! ;eword'
    ' a b ;esyll ;eword',
]


def test_paiwise():
    assert list(_pairwise([])) == []
    assert list(_pairwise([1])) == []
    assert list(_pairwise([1, 2])) == [(1, 2)]
    assert list(_pairwise([1, 2, 3])) == [(1, 2), (2, 3)]


@pytest.mark.parametrize('utt', bad_utterances)
def test_bad_utterances(utt):
    with pytest.raises(ValueError):
        check_utterance(utt, separator=Separator())


@pytest.mark.parametrize('utt', good_utterances)
def test_good_utterances(utt):
    assert check_utterance(utt, separator=Separator())


@pytest.mark.parametrize('level', ['phone', 'syllable'])
def test_prepare_text(level):
    p = {'raw': ['hh ax l ;esyll ow ;esyll ;eword w er l d ;esyll ;eword'],
         'phone': ['hh ax l ow w er l d'],
         'syllable': ['hhaxl ow werld']}

    def f(u):
        return list(prepare(p['raw'], separator=Separator(), unit=u))

    assert f(level) == p[level]


def test_prepare_bad_types():
    # give dict or list of int as input, must fail
    with pytest.raises(AttributeError):
        list(prepare({1: 1, 2: 2}))

    with pytest.raises(AttributeError):
        list(prepare([1, 2], separator=Separator()))


def test_prepare_tolerant():
    utterances = good_utterances + bad_utterances

    # tolerant=False
    with pytest.raises(ValueError):
        list(prepare(utterances, tolerant=False))

    # tolerant=True
    prepared = list(prepare(utterances, tolerant=True))
    assert len(prepared) == len(good_utterances)
    assert prepared == list(prepare(good_utterances))
