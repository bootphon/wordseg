"""Test of the wordseg.algos.tp module"""

import pytest
from wordseg.separator import Separator
from wordseg.algos import tp as tp

from . import prep


@pytest.mark.parametrize('threshold', ['relative', 'absolute'])
def test_tp(prep, threshold):
    out = list(tp.segment(prep, threshold=threshold))
    s = Separator().remove

    assert len(out) == len(prep)
    for n, (a, b) in enumerate(zip(out, prep)):
        assert s(a) == s(b), 'line {}: "{}" != "{}"'.format(n+1, s(a), s(b))
