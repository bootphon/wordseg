"""Test of the wordseg_syll module"""

import os
import pkg_resources
import pytest
import re

from wordseg.separator import Separator
from wordseg.syllabification import (
    syllabify,
    open_datafile,
    _remove_phone_separators,
    _restore_phone_separators)


@pytest.fixture(scope='session')
def datadir():
    # we are using files from wordseg/data/syllabification
    return pkg_resources.resource_filename(
        pkg_resources.Requirement.parse('wordseg'),
        'data/syllabification')


def test_bad_input():
    separator = Separator(phone=';', syllable='_', word=' ')

    with pytest.raises(ValueError) as err:
        syllabify(['ab c_ dd'], ['a', 'd'], ['b', 'dd'], separator=separator)
        assert 'line 1' in err

    with pytest.raises(ValueError) as err:
        syllabify([], ['a'], [])
        assert 'empty vowels' in err

    with pytest.raises(ValueError) as err:
        syllabify([], [], ['a'])
        assert 'empty onsets' in err


def test_remove_phones():
    separator = Separator(phone=' ', syllable=';esyll', word=';eword')
    text = 'a b ;ewordc ;eword'
    clean, index = _remove_phone_separators(text, separator)
    assert clean == 'ab;ewordc;eword'
    assert index == [[1, 1], [1]]

    separator = Separator(phone=';', syllable='_', word=' ')
    text = 'a;b; c;'
    clean, index = _remove_phone_separators(text, separator)
    assert clean == 'ab c'
    assert index == [[1, 1], [1]]

    separator = Separator(phone=';', syllable='_', word=' ')
    text = 'ab c'
    clean, index = _remove_phone_separators(text, separator)
    assert clean == 'ab c'
    assert index == []


@pytest.mark.parametrize(
    'text', ['', 'ab c', 'a;b;', 'a;b; ', 'a;b; c;', 'n;o; s;e; k;a;e; '])
def test_remove_restore_phones(text):
    separator = Separator(phone=';', syllable='_', word=' ')

    clean, index = _remove_phone_separators(text, separator)
    assert not re.search(separator.phone, clean)

    restored = _restore_phone_separators(clean, index, separator)
    assert restored == text


def test_cspanish(datadir):
    vowels = open_datafile(os.path.join(datadir, 'cspanish_vowels.txt'))
    onsets = open_datafile(os.path.join(datadir, 'cspanish_onsets.txt'))
    separator = Separator(phone=';', syllable='_', word=' ')

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

    sylls = syllabify(text, onsets, vowels, separator=separator, strip=False)
    assert sylls == expected


def test_cspanish_bad(datadir):
    vowels = open_datafile(os.path.join(datadir, 'cspanish_vowels.txt'))
    onsets = open_datafile(os.path.join(datadir, 'cspanish_onsets.txt'))
    separator = Separator(phone=';', syllable='_', word=' ')

    text = [
        # here kk and hh are out of onsets
        'nkko sehh kae',
        'si aj aj al aj',
        'esta aj la tata e9u',
        'mira esta xugan9o']

    expected = [
        'no_ se_ ka_e_ ',
        'si_ aj_ aj_ al_ aj_ ',
        'es_ta_ aj_ la_ ta_ta_ e_9u_ ',
        'mi_ra_ es_ta_ xu_gan_9o_ ']

    with pytest.raises(ValueError):
        sylls = syllabify(text, onsets, vowels, separator=separator, strip=False)


def test_cspanish_strip(datadir):
    vowels = open_datafile(os.path.join(datadir, 'cspanish_vowels.txt'))
    onsets = open_datafile(os.path.join(datadir, 'cspanish_onsets.txt'))
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

    sylls = syllabify(text, onsets, vowels, separator=separator, strip=True)
    assert sylls == expected


def test_cspanish_phones(datadir):
    vowels = open_datafile(os.path.join(datadir, 'cspanish_vowels.txt'))
    onsets = open_datafile(os.path.join(datadir, 'cspanish_onsets.txt'))
    separator = Separator(phone=';', syllable='_', word=' ')

    text = [
        'n;o; s;e; k;a;e; ',
        's;i; a;j; a;j; a;l; a;j; ',
        'es;t;a; a;j; l;a; t;a;t;a; e;9u; ',
        'm;i;r;a; es;t;a; x;u;g;a;n;9o; '
    ]

    expected = [
        'n;o;_ s;e;_ k;a;_e;_ ',
        's;i;_ a;j;_ a;j;_ a;l;_ a;j;_ ',
        'es;_t;a;_ a;j;_ l;a;_ t;a;_t;a;_ e;_9u;_ ',
        'm;i;_r;a;_ es;_t;a;_ x;u;_g;a;n;_9o;_ '
    ]

    sylls = syllabify(text, onsets, vowels, separator=separator, strip=False)
    assert sylls == expected


def test_cspanish_phones_strip(datadir):
    vowels = open_datafile(os.path.join(datadir, 'cspanish_vowels.txt'))
    onsets = open_datafile(os.path.join(datadir, 'cspanish_onsets.txt'))
    separator = Separator(phone=';', syllable='_', word=' ')

    text = [
        'n;o s;e k;a;e',
        's;i a;j a;j a;l a;j',
        'es;t;a a;j l;a t;a;t;a e;9u',
        'm;i;r;a es;t;a x;u;g;a;n;9o'
    ]

    expected = [
        'n;o s;e k;a_e',
        's;i a;j a;j a;l a;j',
        'es_t;a a;j l;a t;a_t;a e_9u',
        'm;i_r;a es_t;a x;u_g;a;n_9o'
    ]

    sylls = syllabify(text, onsets, vowels, separator=separator, strip=True)
    assert sylls == expected
