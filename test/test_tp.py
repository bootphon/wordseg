"""Test of the wordseg.algos.tp module"""

import pytest
from wordseg.separator import Separator
from wordseg.algos import tp as tp

from . import prep


@pytest.mark.parametrize(
    'threshold, probability',
    [(t, p) for t in ('relative', 'absolute') for p in ('forward', 'backward')])
def test_tp(prep, threshold, probability):
    """Check input and output are the same, once the separators are removed"""
    out = list(tp.segment(prep, threshold=threshold, probability=probability))
    s = Separator().remove

    assert len(out) == len(prep)
    for n, (a, b) in enumerate(zip(out, prep)):
        assert s(a) == s(b), 'line {}: "{}" != "{}"'.format(n+1, s(a), s(b))


def test_hello_world():
    text = ['hh ax l ow w er l d']

    assert list(tp.segment(text, threshold='absolute', probability='forward')) \
        == ['hhaxl owwerl d']

    assert list(tp.segment(text, threshold='relative', probability='forward')) \
        == ['hhaxl owwerld']

    assert list(tp.segment(text, threshold='absolute', probability='backward')) \
        == ['hhax lowwer ld']

    assert list(tp.segment(text, threshold='relative', probability='backward')) \
        == ['hhax lowwer ld']
