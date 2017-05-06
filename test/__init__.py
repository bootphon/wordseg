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

import os
import pytest
import wordseg


@pytest.yield_fixture(scope='session')
def tags(nlines=10):
    """Return a list of `nlines` utterances loaded from tags.txt"""
    _file = os.path.join(os.path.dirname(__file__), 'data', 'tags.txt')
    text = open(_file, 'r').readlines()[:nlines]
    return [line.strip() for line in text if len(line.strip())]


@pytest.yield_fixture(scope='session')
def prep(nlines=10):
    return list(wordseg.prepare(tags(nlines=nlines), wordseg.Separator()))


@pytest.yield_fixture(scope='session')
def gold(nlines=10):
    return list(wordseg.gold(tags(nlines=nlines), wordseg.Separator()))
