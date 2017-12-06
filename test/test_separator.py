# coding: utf-8

"""Test of the Separator class"""

import pytest
import re

from wordseg.separator import Separator


def test_levels():
    assert Separator(phone='a', syllable='b', word='c').levels() \
        == ['phone', 'syllable', 'word']
    assert Separator(phone='a', syllable='b', word=None).levels() \
        == ['phone', 'syllable']
    assert Separator(phone='a', syllable=None, word=None).levels() \
        == ['phone']


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


@pytest.mark.parametrize('text, expected, keep_boundaries', [
    ('..p.s.p.', ['..p.s.p.'], True),
    ('..p.s.p.', ['.....'], False),
    ('..p.s.p.w', ['..p.s.p.', ''], True),
    ('..p.s.p.w', ['.....', ''], False),
    ('..p.s.p.w..p.s.w', ['.....', '....', ''], False),
    ('..p.s.p.w..p.s.', ['.....', '....'], False),
    ('..p.s.p.w..p.s.', ['..p.s.p.', '..p.s.'], True),
])
def test_split_vs_tokenize(text, expected, keep_boundaries):
    s = Separator(phone='p', syllable='s', word='w')

    assert list(s.split(text, 'word', keep_boundaries=keep_boundaries)) \
        == expected

    assert list(s.tokenize(text, 'word', keep_boundaries=keep_boundaries)) \
        == [e for e in expected if len(e)]


def test_tokenize_2():
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

    s = Separator(phone=' ', syllable=None, word='_')
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


def test_tokenize_noboundaries():
    s = Separator(phone=None, syllable=' ', word=';eword')
    t = 'j uː ;eword n oʊ ;eword dʒ ʌ s t ;eword'
    assert list(s.tokenize(t, 'word', keep_boundaries=False)) \
        == ['juː', 'noʊ', 'dʒʌst']
    assert list(s.tokenize(t, 'syllable', keep_boundaries=False)) \
        == ['j', 'uː', 'n', 'oʊ', 'dʒ', 'ʌ', 's', 't']

    s = Separator(phone=' ', word='_')
    t = 'j uː _ n oʊ _ dʒ ʌ s t _'
    assert list(s.tokenize(t, 'word', keep_boundaries=False)) \
        == ['juː', 'noʊ', 'dʒʌst']
    assert list(s.tokenize(t, 'phone', keep_boundaries=False)) \
        == ['j', 'uː', 'n', 'oʊ', 'dʒ', 'ʌ', 's', 't']

    s = Separator(phone='_', word=' ')
    t = 'j_uː_ n_oʊ_ dʒ_ʌ_s_t_ '
    assert list(s.tokenize(t, 'word', keep_boundaries=False)) \
        == ['juː', 'noʊ', 'dʒʌst']
    assert list(s.tokenize(t, 'phone', keep_boundaries=False)) \
        == ['j', 'uː', 'n', 'oʊ', 'dʒ', 'ʌ', 's', 't']

    s = Separator(phone='_', syllable=';', word=' ')
    t = 'j_uː_ n_oʊ_ dʒ_ʌ_s_;t_ '
    assert list(s.tokenize(t, 'word', keep_boundaries=False)) \
        == ['juː', 'noʊ', 'dʒʌst']
    assert list(s.tokenize(t, 'syllable', keep_boundaries=False)) \
        == ['juː', 'noʊ', 'dʒʌs', 't']
    assert list(s.tokenize(t, 'phone', keep_boundaries=False)) \
        == ['j', 'uː', 'n', 'oʊ', 'dʒ', 'ʌ', 's', 't']


def test_tokenize_full_nosyll():
    t = 'j_uː_ n_oʊ_ dʒ_ʌ_s_t_ '

    s = Separator(phone='_', syllable=None, word=' ')
    assert list(s.tokenize(t)) \
        == [['j', 'uː'], ['n', 'oʊ'], ['dʒ', 'ʌ', 's', 't']]

    s = Separator(phone='_', syllable=';', word=' ')
    assert list(s.tokenize(t)) \
        == [[['j', 'uː']], [['n', 'oʊ']], [['dʒ', 'ʌ', 's', 't']]]

    # tokenize phones only
    t = t.replace(' ', '')
    s = Separator(phone='_', syllable=None, word=None)
    assert list(s.tokenize(t)) == \
        ['j', 'uː', 'n', 'oʊ', 'dʒ', 'ʌ', 's', 't']


def test_tokenize_full_syll():
    t = 'j_uː_ n_oʊ_ dʒ_ʌ_s_;t_ '

    s = Separator(phone='_', syllable=None, word=' ')
    assert list(s.tokenize(t)) \
        == [['j', 'uː'], ['n', 'oʊ'], ['dʒ', 'ʌ', 's', ';t']]

    s = Separator(phone='_', syllable=';', word=' ')
    assert list(s.tokenize(t)) \
        == [[['j', 'uː']], [['n', 'oʊ']], [['dʒ', 'ʌ', 's'], ['t']]]


def test_upper_levels():
    s = Separator(phone='p', syllable='s', word='w')
    assert s.upper_levels('phone') == ['syllable', 'word']
    assert s.upper_levels('syllable') == ['word']
    assert s.upper_levels('word') == []

    s = Separator(phone='p', syllable=None, word='w')
    assert s.upper_levels('phone') == ['word']
    assert s.upper_levels('word') == []

    with pytest.raises(ValueError):
        s.upper_levels('unexisting_level')
