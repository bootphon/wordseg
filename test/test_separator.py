# Copyright 2017 Mathieu Bernard, Elin Larsen
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

"""Test of the Separator class"""

import pytest

from wordseg import Separator


def test_bad_separators():
    bad_separators = [
        [' ', ' ', ' '],
        ['p', ' ', ' '],
        ['same', 'same', 'w'],
    ]

    for sep in bad_separators:
        with pytest.raises(ValueError):
            Separator(sep[0], sep[1], sep[2])


def test_good_separators():
    good_separators = [
        ['p', 's', 'w'],
        [' ', 's', None],
        [' ', None, None],
        [' ', None, 'w'],
        [None, ' ', 'w'],
        [None, None, 'w'],
    ]

    for sep in good_separators:
        s = Separator(sep[0], sep[1], sep[2])
        assert s.phone == sep[0]
        assert s.syllable == sep[1]
        assert s.word == sep[2]


def test_remove():
    s = Separator(phone='p', syllable='s', word='w')
    assert s.remove('abc') == 'abc'
    assert s.remove('wss p') == ' '


def test_remove_re():
    s = Separator('ab', None, None)
    assert s.remove('ab') == ''
    assert s.remove('aa') == 'aa'
    assert s.remove('[ab]') == '[]'

    s = Separator('[ab]', None, None)
    assert s.remove('ab') == ''
    assert s.remove('aa') == ''
    assert s.remove('[ab]') == '[]'

    s = Separator('\[ab\]', None, None)
    assert s.remove('ab') == 'ab'
    assert s.remove('abc') == 'abc'
    assert s.remove('[ab]') == ''


def test_iterate():
    sep = Separator()
    gold = [' ', ';esyll', ';eword']
    for i, sep in enumerate(Separator().iterate()):
        assert sep == gold[i]
