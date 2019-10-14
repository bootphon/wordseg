"""Test of the wordseg_syll module"""

import os
import re

import pkg_resources
import pytest

from wordseg.separator import Separator
from wordseg.syllabification import Syllabifier


@pytest.fixture(scope='session')
def datadir():
    # we are using files from wordseg/data/syllabification
    return pkg_resources.resource_filename(
        pkg_resources.Requirement.parse('wordseg'),
        'data/syllabification')


@pytest.fixture(scope='session')
def onsets(datadir):
    return Syllabifier.open_datafile(
        os.path.join(datadir, 'cspanish_onsets.txt'))


@pytest.fixture(scope='session')
def vowels(datadir):
    return Syllabifier.open_datafile(
        os.path.join(datadir, 'cspanish_vowels.txt'))


def test_bad_input():
    separator = Separator(phone=';', syllable='_', word=' ')

    with pytest.raises(ValueError) as err:
        s = Syllabifier(['a', 'd'], ['b', 'dd'], separator=separator)
        s.syllabify(['ab c_ dd'])  # syllable separator in input
        assert 'line 1' in err

    with pytest.raises(ValueError) as err:
        Syllabifier(['a'], [])
        assert 'empty vowels' in err

    with pytest.raises(ValueError) as err:
        Syllabifier([], ['a'])
        assert 'empty onsets' in err


def test_remove_phones():
    separator = Separator(phone=' ', syllable=';esyll', word=';eword')
    s = Syllabifier(['foo'], ['bar'], separator=separator)
    text = 'a b ;ewordc ;eword'
    clean, index = s._remove_phone_separators(text)
    assert clean == 'ab;ewordc;eword'
    assert index == [[1, 1], [1]]

    separator = Separator(phone=';', syllable='_', word=' ')
    s = Syllabifier(['foo'], ['bar'], separator=separator)
    text = 'a;b; c;'
    clean, index = s._remove_phone_separators(text)
    assert index == [[1, 1], [1]]
    assert clean == 'ab c'

    separator = Separator(phone=';', syllable='_', word=' ')
    s = Syllabifier(['foo'], ['bar'], separator=separator)
    text = 'ab c'
    clean, index = s._remove_phone_separators(text)
    assert index == []
    assert clean == 'ab c'


@pytest.mark.parametrize(
    'text', ['', 'ab c', 'a;b;', 'a;b; ', 'a;b; c;', 'n;o; s;e; k;a;e; '])
def test_remove_restore_phones(text):
    separator = Separator(phone=';', syllable='_', word=' ')
    s = Syllabifier(['foo'], ['bar'], separator=separator)

    clean, index = s._remove_phone_separators(text)
    assert not re.search(separator.phone, clean)

    restored = s._restore_phone_separators(clean, index, strip=False)
    assert restored == text


@pytest.mark.parametrize('syllable', ['_', ';esyll'])
def test_cspanish_good(onsets, vowels, syllable):
    separator = Separator(phone=';', syllable=syllable, word=' ')

    text = [
        'no se kae',
        'si aj aj al aj',
        'esta aj la tata e9u',
        'mira esta xugan9o']

    expected = [
        'no_ se_ ka_e_ ',
        'si_ aj_ aj_ al_ aj_ ',
        'es_ta_ aj_ la_ ta_ta_ e_9u_ ',
        'mi_ra_ es_ta_ xu_gan_9o_ ']

    s = Syllabifier(onsets, vowels, separator=separator)
    sylls = s.syllabify(text)
    assert sylls == [e.replace('_', syllable) for e in expected]


def test_cspanish_bad(onsets, vowels):
    separator = Separator(phone=';', syllable='_', word=' ')

    text = [
        # here kk and hh are out of onsets
        'nkko sehh kae',
        'si aj aj al aj',
        'esta aj la tata e9u',
        'mira esta xugan9o']

    s = Syllabifier(onsets, vowels, separator=separator)
    with pytest.raises(ValueError):
        s.syllabify(text)


def test_cspanish_strip(onsets, vowels):
    separator = Separator(phone=';', syllable='_', word=' ')

    text = [
        'no se kae',
        'si aj aj al aj',
        'esta aj la tata e9u',
        'mira esta xugan9o']

    expected = [
        'no se ka_e',
        'si aj aj al aj',
        'es_ta aj la ta_ta e_9u',
        'mi_ra es_ta xu_gan_9o']

    s = Syllabifier(onsets, vowels, separator=separator)
    sylls = s.syllabify(text, strip=True, tolerant=True)
    assert sylls == expected


@pytest.mark.parametrize('strip', [True, False])
def test_cspanish_phones(onsets, vowels, strip):
    separator = Separator(phone=';', syllable='_', word=' ')

    text = [
        'n;o; s;e; k;a;e; ',
        's;i; a;j; a;j; a;l; a;j; ',
        'es;t;a; a;j; l;a; t;a;t;a; e;9u; ',
        'm;i;r;a; es;t;a; x;u;g;a;n;9o; '
    ]

    if strip:
        expected = [
            'n;o s;e k;a_e',
            's;i a;j a;j a;l a;j',
            'es_t;a a;j l;a t;a_t;a e_9u',
            'm;i_r;a es_t;a x;u_g;a;n_9o'
        ]
    else:
        expected = [
            'n;o;_ s;e;_ k;a;_e;_ ',
            's;i;_ a;j;_ a;j;_ a;l;_ a;j;_ ',
            'es;_t;a;_ a;j;_ l;a;_ t;a;_t;a;_ e;_9u;_ ',
            'm;i;_r;a;_ es;_t;a;_ x;u;_g;a;n;_9o;_ ']

    s = Syllabifier(onsets, vowels, separator=separator)
    sylls = s.syllabify(text, strip=strip)
    assert sylls == expected


@pytest.mark.parametrize('strip', [True, False])
def test_cspanish_default_separator(onsets, vowels, strip):
    text = ['m i r a ;eword']
    expected = (
        ['m i ;esyllr a ;esyll;eword'] if not strip else ['m i;esyllr a'])

    s = Syllabifier(onsets, vowels, separator=Separator())
    sylls = s.syllabify(text, strip=strip)
    assert sylls == expected


@pytest.mark.parametrize('text, error', [
    ('k;a; brla;', 'onset not found in "brla"'),
    ('s;i; a;j; l;j; a;l; a;j; ', 'no vowel in word "lj"'),
    ('es;t;?; ', 'unknown symbol "?" in word "est?"')])
def test_errors(onsets, vowels, text, error):
    s = Syllabifier(onsets, vowels, separator=Separator(';', '_', ' '))
    with pytest.raises(ValueError) as err:
        s.syllabify([text])
    assert error in str(err.value)


def test_no_vowel(onsets, vowels):
    text = 's;i; a;j; l;j; a;l; a;j; '

    s = Syllabifier(onsets, vowels, separator=Separator(';', '_', ' '))
    with pytest.raises(ValueError) as err:
        s.syllabify([text])
    assert 'no vowel in word' in str(err.value)

    s = Syllabifier(onsets, vowels, separator=Separator(';', '_', ' '))
    assert [] == s.syllabify([text], tolerant=True)

    s = Syllabifier(
        onsets, vowels, separator=Separator(';', '_', ' '), filling_vowel=True)
    assert ['s;i;_ a;j;_ l;j;_ a;l;_ a;j;_ '] == s.syllabify([text])
