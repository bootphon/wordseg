# coding: utf-8

"""Manage token separation at phone, syllable and word levels"""

import itertools
import re


class Separator(object):
    """Token separation at phone, syllable and word levels

    A Separator is made of 3 entries *phone*, *syllable* and *word*
    defining the token separators for each of these levels within an
    utterance. A token separator can be a string or None. If not None,
    the entries 'phone', 'syllable' and 'word' must be all different.

    The following characters are forbidden in separators: !#$%&'*+-.^`|~:\\\"

    """
    def __init__(self, phone=' ', syllable=';esyll', word=';eword'):
        # check we have different separators, None excluded
        g1 = list(sep for sep in (phone, syllable, word) if sep)
        g2 = set(sep for sep in (phone, syllable, word) if sep)
        if len(g1) != len(g2):
            raise ValueError(
                'cannot init separator: phone, syllable and word must be '
                'different, they are: "{}", "{}" and "{}"'
                .format(phone, syllable, word))

        self.phone = str(phone) if phone else None
        self.syllable = str(syllable) if syllable else None
        self.word = str(word) if word else None

        # ensure the separators are valid
        for sep in (self.phone, self.syllable, self.word):
            self.check_separator(sep)

        # store the tokens as precompiled regular expressions for
        # faster lookup in strings
        self._regexp = {
            'phone': re.compile(self.phone) if phone else None,
            'syllable': re.compile(self.syllable) if syllable else None,
            'word': re.compile(self.word) if word else None}

    forbidden_chars = "!#$%&'*+-.^`|~:\\\""
    """Characters forbidden in separators

    They interfer with regular expression processing

    """

    def check_separator(self, sep):
        """Raise a ValueError if the `sep` contains a forbidden character"""
        if sep is None:
            return

        for c in self.forbidden_chars:
            if c in sep:
                raise ValueError(
                    'the separator "{}" contains the illegal character "{}", '
                    'the following characters are illegal: {}'.format(
                        sep, c, self.forbidden_chars))

    def __str__(self):
        """Returns a string representation of a separator

        Examples
        --------
        >>> sep = Separator(phone='_', syllable=None, word=' ')
        >>> print(sep)
        (phone: "_", word: " ")

        """
        return '({})'.format(
            ', '.join('{}: "{}"'.format(k, v) for k, v
                      in self.iterate(type='pair') if v))

    def check_level(self, level):
        """Raises ValueError if `level` is not defined in the separator"""
        if level not in self.levels():
            raise ValueError(
                'level "{}" undefined, choose in: {}'.format(
                    level, ', '.join(self.levels())))

    def strip(self, utterance, level=None):
        """Removes leading and ending separators of an `utterance`

        Parameters
        ----------
        utterance : str
            The utterance to be striped.
        level : str, optional
            Specify the level boundaries to strip. If not specified
            remove all the boundaries. If specified, must be 'phone',
            'syllable' or 'word'.

        Returns
        -------
        The striped `utterance`

        """
        # order matters: remove word separators, then syllables and
        # finally phones
        to_remove = ['word', 'syllable', 'phone']
        if level:
            self.check_level(level)
            to_remove = [level]

        # build a regular expression for separator suppression,
        # considers also spaces within contiguous separators
        pattern = '((' + '|'.join('({})'.format(
            self.__dict__[l]) for l in to_remove) + ')+\s*)+'

        # remove leading separators
        utterance = re.sub('^' + pattern, '', utterance)
        # remove ending ones
        utterance = re.sub(pattern + '$', '', utterance)

        return utterance.strip()

    def tokenize(self, utterance, level=None, keep_boundaries=True):
        """Return the tokens in `utterance` at the given `level`

        Iterates on phones, syllable or words within a given
        utterance, other levels being ignored.

        Parameters
        ----------
        utterance : str
            The utterance to be tokenized.
        level : str, optional
            The level to tokenize the utterance at, must be 'phone',
            'syllable' or 'word'. If not specified, tokenize at all
            the defined levels and return a nested list.
        keep_boundaries : bool, optional
            When True (default) preserve the sublevel token boundaries
            in the output. When False all token boundaries are
            removed.

        Returns
        -------
        token : list of (list of (list of)) str
            The successive phones, syllables or words tokenized from
            the utterance. From outer to inner levels in the returned
            nested list are words, syllables and phones. Empty tokens
            are ignored, tokens are striped.

        Raises
        ------
        ValueError
            If the `level` is not 'phone', 'syllable' or 'word'.

        Examples
        --------
        >>> from wordseg.separator import Separator
        >>> s = Separator(phone=' ', syllable=None, word=';eword')
        >>> t = 'j uː ;eword n oʊ ;eword dʒ ʌ s t ;eword'
        >>> list(s.tokenize(t, level='word'))
        ['j uː', 'n oʊ', 'dʒ ʌ s t']
        >>> list(s.tokenize(t, level='word', keep_boundaries=False))
        ['juː', 'noʊ', 'dʒʌst']
        >>> list(s.tokenize(t, level='phone'))
        ['j', 'uː', 'n', 'oʊ', 'dʒ', 'ʌ', 's', 't']
        >>> list(s.tokenize(t))
        [['j', 'uː'], ['n', 'oʊ'], ['dʒ', 'ʌ', 's', 't']]

        """
        if level:
            self.check_level(level)

        # auxiliary function tokenizing at a given level
        def _tokenize(utterance, level):
            if not self._regexp[level]:
                return [utterance]

            return [self.strip(token) for token in re.split(
                self._regexp[level], utterance) if token]

        if level is None:
            # fully tokenize the utterance as a nested list. Whatever the
            # separator we have here a 3-levels list
            tokens = [[_tokenize(s, 'phone')
                       for s in _tokenize(w, 'syllable')]
                      for w in _tokenize(utterance, 'word')]

            # remove the undefined levels from the list
            if not self.phone:
                tokens = [[tt[0] for tt in t] for t in tokens]
            if not self.syllable:
                tokens = [t[0] for t in tokens]
            if not self.word:
                tokens = tokens[0]

            return tokens

        # word tokens
        if self.word:
            tokens = _tokenize(utterance, 'word')
        else:
            tokens = [utterance]

        # syllable tokens
        if level in ('phone', 'syllable') and self.syllable:
            tokens = itertools.chain(
                syll for word in tokens
                for syll in _tokenize(word, 'syllable'))

        # phone tokens
        if level == 'phone' and self.phone:
            tokens = itertools.chain(
                phn for syll in tokens
                for phn in _tokenize(syll, 'phone'))

        # strip the tokens
        tokens = (self.strip(t) for t in tokens)

        # delete intermediate token boundaries when asked
        if not keep_boundaries:
            tokens = (self.remove(t) for t in tokens)

        return [t for t in tokens if t]

    def split(self, utterance, level, keep_boundaries=False):
        """Split the `utterance` at a given token `level`

        This method is sensitive to either the `utterance` is striped
        or not. It may output empty tokens.

        Parameters
        ----------
        utterance : str
            The string to split in tokens.
        level : str
            Token level to split the string with. Must be 'phone',
            'syllable' or 'word'.
        keep_boundaries : bool, optional
            If False (default), remove all the separators for all
            levels from the returned sub-utterances.

        Returns
        -------
        tokens : generator
             The tokens extracted from `utt`, may include empty tokens.

        Raises
        ------
        ValueError
            If the `level` is not 'phone', 'syllable' or 'word'.

        See Also
        --------
        tokenize : an higher-level method to split an utterance

        """
        self.check_level(level)

        sep = self._regexp[level]
        tokens = re.split(sep, utterance)

        if keep_boundaries:
            tokens = (re.sub(' +', ' ', u) for u in tokens)
        else:
            tokens = (self.remove(u) for u in tokens)

        # remove any leading ' '
        tokens = (t.lstrip(' ') for t in tokens)

        return tokens

    def remove(self, utterance, level=None):
        """Returns the `utterance` with separators removed

        Parameters
        ----------
        utterance : str
           The string to remove the separators from
        level : str, optional
           If specified (must be 'phone', 'syllable' or 'word'),
           remove only the separators of the given `level`. Else
           remove all the separators.

        Returns
        -------
        The utterance with specified separators removed. Multiple
        spaces are removed as well.

        Raises
        ------
        ValueError
            If the `level` is specified and is not 'phone', 'syllable'
            or 'word'.

        """
        if level:
            self.check_level(level)

        to_remove = {'phone', 'syllable', 'word'}
        if level:
            to_remove = {level}

        if self.word and 'word' in to_remove:
            utterance = re.sub(self._regexp['word'], '', utterance)

        if self.syllable and 'syllable' in to_remove:
            utterance = re.sub(self._regexp['syllable'], '', utterance)

        if self.phone and 'phone' in to_remove:
            utterance = re.sub(self._regexp['phone'], '', utterance)

        return re.sub(' +', ' ', utterance)

    def iterate(self, type='value'):
        """Yields on phone, syllable and word tokens, in that order

        Parameters
        ----------
        type : str, optional
            Type of separator representation to return, must be
            'value' or 'pair'.

        Yields
        ------
        token : str or tuple
            In the form **token_value** if `type` is 'value'. In the
            form **(token_name, token_value)** if `type` is 'pair'.

        Raises
        ------
        ValueError
            If the `type` is not 'value' or 'pair'.

        """
        if type == 'value':
            yield self.phone
            yield self.syllable
            yield self.word
        elif type == 'pair':
            yield ('phone', self.phone)
            yield ('syllable', self.syllable)
            yield ('word', self.word)
        else:
            raise ValueError(
                'iteration type must be "value" or "pair", it is "{}"'
                .format(type))

    def levels(self):
        """The list of defined token levels from inner to outer"""
        # curiously levels order and alphabetical order are the same
        # (phone < syllable < word)
        return sorted([k for k, v in self.iterate(type='pair') if v])

    def upper_levels(self, level):
        """Lists the defined levels upper than the given one

        Parameters
        ----------
        level : str
            Must be 'phone', 'syllable' or 'word'.

        Raises
        ------
        ValuError
            when `level` is not defined in the separator.

        Examples
        --------
        >>> from wordseg.separator import Separator
        >>> s = Separator(phone='p', syllable='s', word='w')
        >>> s.upper_levels('phone')
        ['syllable', 'word']
        >>> s.upper_levels('word')
        []
        >>> s = Separator(phone='p', syllable=None, word='w')
        >>> s.upper_levels('phone')
        ['word']

        """
        # ensure the required level exists
        self.check_level(level)

        # extract tue upper levels
        index = self.levels().index(level)
        return self.levels()[index+1:]
