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

"""Test of the wordseg_eval module"""


import pytest
from wordseg.evaluate import read_data, evaluate

from . import gold

def test_read_data():
    assert read_data([]) == ([], [])
    assert read_data(['a']) == (['a'], [{(0, 1)}])
    assert read_data(
        ['a b', 'c']) == (['ab', 'c'], [{(0, 1), (1, 2)}, {(0, 1)}])


def test_evaluate(gold):
    for v in evaluate(gold, gold).values():
        assert v == 1.0
