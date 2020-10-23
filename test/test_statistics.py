# coding: utf-8

"""Test of the wordseg.statistics module"""

import pytest

from wordseg.statistics import CorpusStatistics
from wordseg.separator import Separator


UTTS = ['i mean the cops are just looking for people that look younger',
        'ten people call so she\'s like it\'s easy she\'s like i get paid to']


def test_most_common():
    stats = CorpusStatistics(UTTS, separator=Separator(
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
    stats = CorpusStatistics(UTTS, separator=Separator(
        phone=None, syllable=None, word=' ')).describe_all()

    assert stats['corpus'] == pytest.approx({
        'nutts': 2,
        'nutts_single_word': 0,
        'mattr': 0.9125000000000003})

    assert stats['words'] == pytest.approx({
        'tokens': 26,
        'types': 22,
        'hapaxes': 18})


def test_descibe2(tags):
    stats = CorpusStatistics(tags, separator=Separator(
        phone=' ', syllable=';esyll', word=';eword')).describe_all()

    assert stats['corpus'] == pytest.approx({
        'entropy': 0.06298494117721846,
        'mattr': 0.7166666666666667,
        'nutts': 13,
        'nutts_single_word': 4})

    assert stats['phones'] == pytest.approx({
        'tokens': 121,
        'types': 28,
        'hapaxes': 5})

    assert stats['syllables'] == pytest.approx({
        'tokens': 49,
        'types': 31,
        'hapaxes': 24})

    assert stats['words'] == pytest.approx({
        'tokens': 34,
        'types': 24,
        'hapaxes': 19})

def test_describe3(tags):
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
    stats = CorpusStatistics(UTTS, separator=Separator(
        phone=None, syllable=None, word=' '))
    with pytest.raises(KeyError):
        stats.normalized_segmentation_entropy()

    stats = CorpusStatistics(tags, Separator())
    assert stats.normalized_segmentation_entropy() \
        == pytest.approx(0.06298494117721846)
