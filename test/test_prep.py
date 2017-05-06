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
    assert utils.strip('a  b \n') == 'a b'


bad_utterances = [
        '',
        ' ',
        '\n\n',
        'ah ;esyll ah',
        'ah ;esyll ah ;esyll',
        'ah ah ; eword',
        ';eword',
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


def test_prepare_text():
    p = {'raw': ['hh ax l ;esyll ow ;esyll ;eword w er l d ;esyll ;eword'],
         'phn': ['hh ax l ow w er l d'],
         'syl': ['hhaxl ow werld']}

    def f(u):
        return list(prepare(p['raw'], separator=Separator(), unit=u))

    assert f('phoneme') == p['phn']
    assert f('syllable') == p['syl']
