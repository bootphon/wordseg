"""Defines input data and fixtures common to all wordseg tests"""

import os
import pytest
import wordseg.prepare
import wordseg.separator


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
def datadir():
    return os.path.abspath(os.path.join(os.path.dirname(__file__), 'data'))

@pytest.yield_fixture(scope='session')
def tags():
    return _tags.strip().split('\n')


@pytest.yield_fixture(scope='session')
def prep():
    return list(wordseg.prepare.prepare(tags(), wordseg.separator.Separator()))


@pytest.yield_fixture(scope='session')
def gold():
    return list(wordseg.prepare.gold(tags(), wordseg.separator.Separator()))
