"""Test of the wordseg.algos.baseline module"""

import pytest

from wordseg.algos.baseline import segment


@pytest.mark.parametrize('p', (1.01, -1, 'a', True, 1))
def test_proba_bad(p):
    with pytest.raises(ValueError):
        list(segment('a b c', probability=p))


def test_hello():
    assert list(segment(['h e l l o'], probability=0.0)) == ['hello']
    assert list(segment(['h e l l o'], probability=1.0)) == ['h e l l o ']
