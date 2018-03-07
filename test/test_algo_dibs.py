"""Test of the wordseg.algos.dibs module"""

import codecs
import os
import pytest

from wordseg.separator import Separator
from wordseg.prepare import gold, prepare
from wordseg.evaluate import evaluate
from wordseg.algos import dibs

from . import tags, prep, datadir


# make sure we have the expected result for a variety of parameters
@pytest.mark.parametrize(
    'type, threshold, pwb', [
        (t, T, p)
        for t in ('baseline', 'phrasal', 'lexical')
        for T in (0, 0.5, 1)
        for p in (None, 0.2)])
def test_basic(prep, tags, type, threshold, pwb):
    sep = Separator()
    model = dibs.CorpusSummary(tags, separator=sep)

    out = list(dibs.segment(
        prep, model, type=type, threshold=threshold, pwb=pwb))

    s = Separator().remove
    assert len(out) == len(prep)
    for n, (a, b) in enumerate(zip(out, prep)):
        assert s(a) == s(b), 'line {}: "{}" != "{}"'.format(n+1, s(a), s(b))


def test_phone_sep():
    text = ['hh_ih_r_;eword ',
            'dh_eh_r_;eword w_iy_;eword g_ow_;eword ']
    sep = Separator(phone='_', syllable=None, word=';eword ')
    model = dibs.CorpusSummary(text, separator=sep)
    assert model.summary == {'nlines': 2, 'nwords': 4, 'nphones': 10}


def test_bad_train(prep):
    # cannot have a train text without word separators
    with pytest.raises(ValueError):
        dibs.CorpusSummary(prep)

def test_replicate_cdswordseg(datadir):
    sep = Separator()

    _tags = [utt for utt in codecs.open(
        os.path.join(datadir, 'tagged.txt'), 'r', encoding='utf8')
            if utt]
    _prepared = prepare(_tags, separator=sep)
    _gold = gold(_tags, separator=sep)
    _train = _tags[:200]

    model = dibs.CorpusSummary(_train)
    segmented = dibs.segment(_prepared, model)
    score = evaluate(segmented, _gold)

    # we obtained that score from the dibs version in CDSWordSeg
    # (using wordseg.prepare and wordseg.evaluate in both cases). You
    # can replicate this result in CDSWordseg using
    # ".../CDSwordSeg/algoComp/segment.py test/data/tagged.txt -a dibs"
    expected = {
        'type_fscore': 0.2359,
        'type_precision': 0.2084,
        'type_recall': 0.2719,
        'token_fscore': 0.239,
        'token_precision': 0.3243,
        'token_recall': 0.1892,
        'boundary_fscore': 0.4804,
        'boundary_precision': 0.7161,
        'boundary_recall': 0.3614}

    assert score == pytest.approx(expected, rel=1e-3)
