"""Test of the wordseg.algos.puddle module"""

import pytest
from wordseg.separator import Separator
from wordseg.algos import puddle as puddle

from . import prep


@pytest.mark.parametrize(
    'window, nfolds, njobs',
    [(w, f, j) for w in (1, 5) for f in (1, 5) for j in (3, 10)])
def test_puddle(prep, window, nfolds, njobs):
    out = list(puddle.segment(prep, window=window, nfolds=nfolds, njobs=njobs))
    s = Separator().remove

    assert len(out) == len(prep)
    for n, (a, b) in enumerate(zip(out, prep)):
        assert s(a) == s(b), 'line {}: "{}" != "{}"'.format(n+1, s(a), s(b))
