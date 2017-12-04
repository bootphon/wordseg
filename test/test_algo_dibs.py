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
    model = dibs.Summary(tags, separator=sep)

    out = list(dibs.segment(
        prep, model, type=type, threshold=threshold, pwb=pwb))

    s = Separator().remove
    assert len(out) == len(prep)
    for n, (a, b) in enumerate(zip(out, prep)):
        assert s(a) == s(b), 'line {}: "{}" != "{}"'.format(n+1, s(a), s(b))


def test_replicate_cdswordseg(datadir):
    sep = Separator()

    _tags = [utt for utt in codecs.open(
        os.path.join(datadir, 'phonologic.txt'), 'r', encoding='utf8')
            if utt]
    _prepared = prepare(_tags, separator=sep)
    _gold = gold(_tags, separator=sep)
    _train = _tags[:200]

    model = dibs.Summary(_train)
    segmented = dibs.segment(_prepared, model)
    score = evaluate(segmented, _gold)

    # we obtained that score from the dibs version in CDSWordSeg
    # (using wordseg.prepare and wordseg.evaluate in both cases)
    assert score == pytest.approx({
        'type_fscore': 0.1983,
        'type_precision': 0.1577,
        'type_recall': 0.2672,
        'token_fscore': 0.2728,
        'token_precision': 0.3135,
        'token_recall': 0.2415,
        'boundary_fscore': 0.5478,
        'boundary_precision': 0.6486,
        'boundary_recall': 0.4741}, rel=1e-3)
