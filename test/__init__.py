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

import pytest
import wordseg


_tags = '''
hh ih r ;esyll ;eword
dh eh r ;esyll ;eword w iy ;esyll ;eword g ow ;esyll ;eword
s ow ;esyll ;eword ax m p ;esyll ey sh ;esyll ax n t ;esyll ;eword
w ah t ;esyll ;eword d ih d ;esyll ;eword y uw ;esyll ;eword iy t ;esyll ;eword ah p ;esyll ;eword
uw p s ;esyll ;eword
l ih t ;esyll ax l ;esyll ;eword b ih t ;esyll ;eword m ao r ;esyll ;eword
l ih t ;esyll ax l ;esyll ;eword b ih t ;esyll ;eword m ao r ;esyll ;eword
l ih t ;esyll ax l ;esyll ;eword b ih t ;esyll ;eword m ao r ;esyll ;eword
uw p ;esyll s iy ;esyll ;eword
uw p ;esyll s iy ;esyll ;eword b ih t ;esyll iy ;esyll ;eword b ih t ;esyll iy ;esyll ;eword b ih t ;esyll iy ;esyll ;eword b ey b ;esyll iy ;esyll ;eword g er l ;esyll ;eword
y uw hh ;esyll aa ;esyll ;eword s p ae g hh ;esyll eh t ;esyll iy ;esyll ow z ;esyll ;eword
w aa n t ;esyll ;eword s ax m ;esyll ;eword m ao r ;esyll ;eword
n ow ;esyll ;eword
'''


@pytest.yield_fixture(scope='session')
def tags():
    """Return a list of `nlines` utterances loaded from tags.txt"""
    return _tags.strip().split('\n')


@pytest.yield_fixture(scope='session')
def prep():
    return list(wordseg.prepare(tags(), wordseg.Separator()))


@pytest.yield_fixture(scope='session')
def gold():
    return list(wordseg.gold(tags(), wordseg.Separator()))
