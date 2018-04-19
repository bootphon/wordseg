"""Test of the folding module"""

import pytest
from wordseg import folding

from . import tags


def test_permute():
    l = list(range(5))
    l = folding._permute(l)
    assert l == [4, 0, 1, 2, 3]

    l = folding._permute(l)
    assert l == [3, 4, 0, 1, 2]


def test_flatten():
    assert folding._flatten([[0], [1], [2, 3]]) == [0, 1, 2, 3]

def test_cumsum():
    c = folding._cumsum
    assert c([]) == []
    assert c([2, 3]) == [2, 5]
    assert c([1, 2, 3]) == [1, 3, 6]
    assert c([2, 2, 3]) == [2, 4, 7]


def test_boundaries():
    with pytest.raises(ValueError):
        folding.boundaries([], 1)

    with pytest.raises(ValueError):
        folding.boundaries(['a'], 0)

    with pytest.raises(ValueError):
        folding.boundaries(['a'], 2)

    assert folding.boundaries(['a'], 1) == [0]
    assert folding.boundaries(['a', 'b'], 1) == [0]
    assert folding.boundaries(['a', 'b'], 2) == [0, 1]
    assert folding.boundaries(['a', 'b', 'c'], 2) == [0, 1]
    assert folding.boundaries(['a', 'b', 'c', 'd'], 2) == [0, 2]


def test_fold():
    # a single fold
    assert folding.fold([1, 2, 3], 1) == \
        ([[1, 2, 3]], [0])

    # each group have 1 element
    assert folding.fold([1, 2, 3], 3) == \
        ([[1, 2, 3], [3, 1, 2], [2, 3, 1]], [2, 2, 2])

    # here the last group is [3, 4]
    assert folding.fold([1, 2, 3, 4], 3) == \
        ([[1, 2, 3, 4], [3, 4, 1, 2], [2, 3, 4, 1]], [2, 3, 3])


@pytest.mark.parametrize('nfolds', [1, 2, 3])
def test_unfold_basic(nfolds):
    folds, index = folding.fold([1, 2, 3], nfolds)
    assert folding.unfold(folds, index) == [1, 2, 3]


@pytest.mark.parametrize('nfolds', [1] + list(range(2, 12, 2)))
def test_fold_unfold_nfolds(nfolds, tags):
    folds, index = folding.fold(tags, nfolds)
    assert folding.unfold(folds, index) == tags
