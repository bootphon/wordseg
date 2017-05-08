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

"""Test of the 'wordseg-prep' command and it's underlying functions"""

import pytest
from wordseg import utils, Separator
from wordseg.prepare import check_utterance, prepare


def test_strip():
    assert utils.strip('  ') == ''
    assert utils.strip('\na\ta   \t\naa a\n  ') == 'a a aa a'
    assert utils.strip('a  a \n') == 'a a'


bad_utterances = [
        '',
        ' ',
        '\n\n',
        'ah ;esyll ah',
        'ah ;esyll ah ;esyll',  # missing ;eword
        'ah ;esyll ah ;eword',  # missing ;esyll
        'ah ah ; eword',
        ';eword',
        ';eword a b ;esyll ;eword',
        ';esyll a b ;esyll ;eword',
        ' a b ;esyll ;eword',
        'a. ;eword',
        'a! ;eword',
]


@pytest.mark.parametrize('utt', bad_utterances)
def test_bad_utterances(utt):
    with pytest.raises(ValueError):
        check_utterance(utt, separator=Separator())


good_utterances = [
    'a ;eword',
    'a ;esyll ;eword']


@pytest.mark.parametrize('utt', good_utterances)
def test_good_utterances(utt):
    assert check_utterance(utt, separator=Separator())


@pytest.mark.parametrize('level', ['phoneme', 'syllable'])
def test_prepare_text(level):
    p = {'raw': ['hh ax l ;esyll ow ;esyll ;eword w er l d ;esyll ;eword'],
         'phoneme': ['hh ax l ow w er l d'],
         'syllable': ['hhaxl ow werld']}

    def f(u):
        return list(prepare(p['raw'], separator=Separator(), unit=u))

    assert f(level) == p[level]


def test_prepare_bad_types():
    # give dict or list of int as input, must fail
    with pytest.raises(ValueError):
        list(prepare({1: 1, 2: 2}, separator=Separator()))

    with pytest.raises(ValueError):
        list(prepare([1, 2], separator=Separator()))


def test_prepare_tolerant():
    utterances = good_utterances + bad_utterances

    # tolerant=False
    with pytest.raises(ValueError):
        list(prepare(utterances, separator=Separator(), tolerant=False))

    # tolerant=True
    prepared = list(prepare(utterances, separator=Separator(), tolerant=True))
    assert len(prepared) == len(good_utterances)
    assert prepared == list(prepare(good_utterances, separator=Separator()))
