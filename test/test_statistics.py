# coding: utf-8

"""Test of the wordseg.statistics module"""

import pytest

from wordseg.statistics import CorpusStatistics
from wordseg.separator import Separator
from . import tags, gold


utts = ['i mean the cops are just looking for people that look younger',
        'ten people call so she\'s like it\'s easy she\'s like i get paid to']


def test_most_common():
    stats = CorpusStatistics(utts, separator=Separator(
        phone=None, syllable=None, word=' '))

    top_freq = stats.most_common_tokens('word', n=4)
    assert dict(top_freq) == {'i': 2, 'people': 2, 'she\'s': 2, 'like': 2}


def test_unigram():
    sep = Separator(phone=None, syllable=None, word=' ')
    text = ['hello world', 'hello you']
    stats = CorpusStatistics(text, separator=sep)
    assert stats.unigram['word'] == pytest.approx(
        {'hello': .50, 'world': 0.25, 'you': 0.25})

    sep = Separator(phone='_', syllable=None, word=' ')
    text = ['he_llo_ wo_rl_d_', 'he_llo_ you_']
    stats = CorpusStatistics(text, separator=sep)
    assert stats.unigram['word'] == pytest.approx(
        {'hello': .50, 'world': 0.25, 'you': 0.25})
    assert stats.unigram['phone'] == \
        {'d': 0.125, 'he': 0.25, 'llo': 0.25, 'rl': 0.125,
         'wo': 0.125, 'you': 0.125}


def test_descibe1():
    stats = CorpusStatistics(utts, separator=Separator(
        phone=None, syllable=None, word=' ')).describe_all()

    assert stats['corpus'] == pytest.approx({
        'nutts': 2,
        'nutts_single_word': 0,
        'nword_tokens': 26,
        'nword_types': 22,
        'nword_hapax': 18,
        'mattr': 0.9125000000000003})

    assert stats['words'] == pytest.approx({
        'token/types': 1.1818181818181819,
        'tokens': 26,
        'tokens/utt': 13.0,
        'types': 22,
        'uniques': 18})


def test_descibe2(tags):
    stats = CorpusStatistics(tags, separator=Separator(
        phone=' ', syllable=';esyll', word=';eword')).describe_all()

    assert stats['corpus'] == pytest.approx({
        'entropy': 0.06298494117721846,
        'mattr': 0.7166666666666667,
        'nutts': 13,
        'nutts_single_word': 4,
        'nword_hapax': 19,
        'nword_tokens': 34,
        'nword_types': 24})

    assert stats['phones'] == pytest.approx({
        'token/types': 4.321428571428571,
        'tokens': 121,
        'tokens/syllable': 2.4693877551020407,
        'tokens/utt': 9.307692307692308,
        'tokens/word': 3.5588235294117645,
        'types': 28,
        'uniques': 5})

    assert stats['syllables'] == pytest.approx({
        'token/types': 1.5806451612903225,
        'tokens': 49,
        'tokens/utt': 3.769230769230769,
        'tokens/word': 1.4411764705882353,
        'types': 31,
        'uniques': 24})

    assert stats['words'] == pytest.approx({
        'token/types': 1.4166666666666667,
        'tokens': 34,
        'tokens/utt': 2.6153846153846154,
        'types': 24,
        'uniques': 19})

def test_describe3(tags, gold):
    stats_tags = CorpusStatistics(
        tags,
        separator=Separator(phone=' ', syllable=';esyll', word=';eword')
    ).describe_tokens('word')

    stats_gold = CorpusStatistics(
        tags,
        separator=Separator()
    ).describe_tokens('word')

    assert pytest.approx(stats_tags) == stats_gold

def test_entropy(tags):
    stats = CorpusStatistics(utts, separator=Separator(
        phone=None, syllable=None, word=' '))
    with pytest.raises(KeyError):
        stats.normalized_segmentation_entropy()

    stats = CorpusStatistics(tags, Separator())
    assert stats.normalized_segmentation_entropy() \
        == pytest.approx(0.06298494117721846)
