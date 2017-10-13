"""Test of the wordseg.statistics module"""

from wordseg import statistics as stats


utts = ['i mean the cops are just looking for people that look younger',
        'ten people call so she\'s like it\'s easy she\'s like i get paid to']


def test_top_frequency():
    top_freq = stats.top_frequency_tokens(utts, n=4)
    assert dict(top_freq) == {'i': 2, 'people': 2, 'she\'s': 2, 'like': 2}


def test_stat_corpus():
    df = stats.stat_corpus(utts)
    assert df.loc['stat', 'number_tokens'] == 26
    assert df.loc['stat', 'number_types'] == 21
    assert df.loc['stat', 'number_utterances'] == 2
