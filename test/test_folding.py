# Copyright 2017 Mathieu Bernard
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

"""Test of the folding module"""

import os
import pytest
from wordseg import folding


def load_science_text():
    science_file = os.path.join(
        os.path.dirname(__file__), 'data', 'science.txt')
    text = open(science_file, 'r').readlines()
    return [line.strip() for line in text if len(line.strip())]


def test_permute():
    l = list(range(5))
    l = folding.permute(l)
    assert l == [4, 0, 1, 2, 3]

    l = folding.permute(l)
    assert l == [3, 4, 0, 1, 2]


def test_flatten():
    assert folding.flatten([[0], [1], [2, 3]]) == [0, 1, 2, 3]


def test_boundaries():
    with pytest.raises(ValueError):
        folding.fold_boundaries([], 1)

    with pytest.raises(ValueError):
        folding.fold_boundaries(['a'], 0)

    with pytest.raises(ValueError):
        folding.fold_boundaries(['a'], 2)

    assert folding.fold_boundaries(['a'], 1) == [0]
    assert folding.fold_boundaries(['a', 'b'], 1) == [0]
    assert folding.fold_boundaries(['a', 'b'], 2) == [0, 1]
    assert folding.fold_boundaries(['a', 'b', 'c'], 2) == [0, 1]
    assert folding.fold_boundaries(['a', 'b', 'c', 'd'], 2) == [0, 2]


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


@pytest.mark.parametrize('nfolds', [1] + list(range(2, 16, 2)))
def test_fold_unfold(nfolds):
    text = load_science_text()
    folds, index = folding.fold(text, nfolds)
    assert folding.unfold(folds, index) == text
