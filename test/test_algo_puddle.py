"""Test of the wordseg.algos.puddle module"""

import codecs
import os
import pytest

from wordseg.separator import Separator
from wordseg.prepare import gold, prepare
from wordseg.evaluate import evaluate
from wordseg.algos import puddle as puddle

from . import prep, datadir


@pytest.mark.parametrize(
    'window, nfolds, njobs',
    [(w, f, j) for w in (1, 5) for f in (1, 5) for j in (3, 10)])
def test_puddle(prep, window, nfolds, njobs):
    out = list(puddle.segment(prep, window=window, nfolds=nfolds, njobs=njobs))
    s = Separator().remove

    assert len(out) == len(prep)
    for n, (a, b) in enumerate(zip(out, prep)):
        assert s(a) == s(b), 'line {}: "{}" != "{}"'.format(n+1, s(a), s(b))


def test_replicate(datadir):
    sep = Separator()

    _tags = [utt for utt in codecs.open(
        os.path.join(datadir, 'tagged.txt'), 'r', encoding='utf8')
            if utt][:100]  # 100 first lines only
    _prepared = prepare(_tags, separator=sep)
    _gold = gold(_tags, separator=sep)

    segmented = puddle.segment(_prepared, nfolds=1)
    score = evaluate(segmented, _gold)

    # we obtained that score from the dibs version in CDSWordSeg
    # (using wordseg.prepare and wordseg.evaluate in both cases)
    expected = {
        'type_fscore': 0.06369,
        'type_precision': 0.1075,
        'type_recall': 0.04525,
        'token_fscore': 0.06295,
        'token_precision': 0.2056,
        'token_recall': 0.03716,
        'boundary_fscore': 0.02806,
        'boundary_precision': 1.0,
        'boundary_recall': 0.01423}

    assert score == pytest.approx(expected, rel=1e-3)
