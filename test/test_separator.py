# coding: utf-8

"""Test of the Separator class"""

import pytest
import re

from wordseg.separator import Separator


def test_iterate():
    sep = Separator()
    gold = [' ', ';esyll', ';eword']
    for i, sep in enumerate(Separator().iterate()):
        assert sep == gold[i]


def test_bad_separators():
    bad_separators = [
        [' ', ' ', ' '],
        ['p', ' ', ' '],
        ['same', 'same', 'w']]

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


def test_strip():
    s = Separator(phone='p', syllable='s', word='w')
    assert s.strip('') == ''
    assert s.strip('..') == '..'
    assert s.strip('p.p') == '.'
    assert s.strip('p .p') == '.'
    assert s.strip('psw p') == ''
    assert s.strip('ps w p') == ''
    assert s.strip(' pp ') == ''

    assert s.strip('sw p', level='phone') == 'sw'
    assert s.strip('sw p', level='syllable') == 'w p'
    assert s.strip('sw p', level='word') == 'sw p'


def test_remove():
    s = Separator(phone='p', syllable='s', word='w')
    assert s.remove('abc') == 'abc'
    assert s.remove('wss p') == ' '

    s = Separator(phone='_', word=';eword ')
    t = 'j_uː_;eword n_oʊ_;eword dʒ_ʌ_s_t_;eword s_t_uː_p_ɪ_d_ɪ_ɾ_i_;eword '
    assert s.remove(t) == 'juːnoʊdʒʌststuːpɪdɪɾi'


def test_remove_level():
    s = Separator(phone='p', syllable='s', word='w')
    assert s.remove('..p.s.p.w') == '.....'
    assert s.remove('..p.s.p.w', level='phone') == '...s..w'
    assert s.remove('..p.s.p.w', level='syllable') == '..p..p.w'
    assert s.remove('..p.s.p.w', level='word') == '..p.s.p.'

    s = Separator(phone=';', syllable='_', word=' ')
    assert s.remove('ab c', level='phone') == 'ab c'


def test_remove_bad():
    s = Separator(phone='p', syllable='s', word='w')
    with pytest.raises(ValueError) as err:
        s.remove('', level='bad')
        assert 'this is bad' in err


def test_remove_re():
    s = Separator('ab', None, None)
    assert s.remove('ab') == ''
    assert s.remove('aa') == 'aa'
    assert s.remove('[ab]') == '[]'

    s = Separator('[ab]', None, None)
    assert s.remove('ab') == ''
    assert s.remove('aa') == ''
    assert s.remove('[ab]') == '[]'

    with pytest.raises(ValueError):
        Separator('\[ab\]', None, None)

    with pytest.raises(ValueError):
        Separator(re.escape('[ab]'), None, None)


def test_split():
    s = Separator(phone='p', syllable='s', word='w')
    assert s.split('..p.s.p.', 'word', remove=False) == ['..p.s.p.']
    assert s.split('..p.s.p.w', 'word', remove=False) == ['..p.s.p.', '']
    assert s.split('aapasapaw', 'word', remove=False) == ['aapasapa', '']
    assert s.split('..p.s.p.w', 'word', remove=True) == ['.....', '']
    assert s.split('..p.s.p.w..p.s.', 'word', remove=True) == ['.....', '....']
    assert s.split('..p.s.p.w..p.s.w', 'word', remove=True) \
        == ['.....', '....', '']


def test_tokenize():
    s = Separator(phone=None, syllable=' ', word=';eword')
    t = 'j uː ;eword n oʊ ;eword dʒ ʌ s t ;eword'
    assert list(s.tokenize(t, 'word')) \
        == ['j uː', 'n oʊ', 'dʒ ʌ s t']
    assert list(s.tokenize(t, 'syllable')) \
        == ['j', 'uː', 'n', 'oʊ', 'dʒ', 'ʌ', 's', 't']

    s = Separator(phone=' ', word='_')
    t = 'j uː _ n oʊ _ dʒ ʌ s t _'
    assert list(s.tokenize(t, 'word')) \
        == ['j uː', 'n oʊ', 'dʒ ʌ s t']
    assert list(s.tokenize(t, 'phone')) \
        == ['j', 'uː', 'n', 'oʊ', 'dʒ', 'ʌ', 's', 't']

    s = Separator(phone='_', word=' ')
    t = 'j_uː_ n_oʊ_ dʒ_ʌ_s_t_ '
    assert list(s.tokenize(t, 'word')) \
        == ['j_uː', 'n_oʊ', 'dʒ_ʌ_s_t']
    assert list(s.tokenize(t, 'phone')) \
        == ['j', 'uː', 'n', 'oʊ', 'dʒ', 'ʌ', 's', 't']

    s = Separator(phone='_', syllable=';', word=' ')
    t = 'j_uː_ n_oʊ_ dʒ_ʌ_s_;t_ '
    assert list(s.tokenize(t, 'word')) \
        == ['j_uː', 'n_oʊ', 'dʒ_ʌ_s_;t']
    assert list(s.tokenize(t, 'syllable')) \
        == ['j_uː', 'n_oʊ', 'dʒ_ʌ_s', 't']
    assert list(s.tokenize(t, 'phone')) \
        == ['j', 'uː', 'n', 'oʊ', 'dʒ', 'ʌ', 's', 't']
