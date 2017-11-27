"""Test of the wordseg.statistics module"""

import pytest

from wordseg.statistics import CorpusStatistics
from wordseg.separator import Separator
from . import tags


utts = ['i mean the cops are just looking for people that look younger',
        'ten people call so she\'s like it\'s easy she\'s like i get paid to']


def test_top_frequency():
    stats = CorpusStatistics(utts, separator=Separator(
        phone=None, syllable=None, word=' '))

    top_freq = stats.top_frequency_tokens('word', n=4)
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


def test_descibe_corpus1():
    stats = CorpusStatistics(utts, separator=Separator(
        phone=None, syllable=None, word=' ')).describe_corpus()

    assert stats == pytest.approx({
        'nutts': 2, 'nswu': 0, 'nwtok': 26, 'nwtyp': 22,
        'nhapax': 18, 'mattr': 0.9125000000000003})


def test_descibe_corpus2(tags):
    stats = CorpusStatistics(tags, separator=Separator(
        phone=' ', syllable=';esyll', word=';eword')).describe_corpus()

    assert stats == pytest.approx({
        'nutts': 13, 'nswu': 4, 'nwtok': 34, 'nwtyp': 24,
        'nhapax': 19, 'mattr': 0.7166666666666667,
        'awl': 3.5588235294117645, 'nse': 0.06298494117721846})


def test_entropy(tags):
    stats = CorpusStatistics(utts, separator=Separator(
        phone=None, syllable=None, word=' '))
    with pytest.raises(KeyError):
        stats.normalized_segmentation_entropy()

    stats = CorpusStatistics(tags, Separator())
    assert stats.normalized_segmentation_entropy() \
        == pytest.approx(0.06298494117721846)
