"""Manage token separation at phone, syllable and word levels"""

import re


class Separator(object):
    """Token separation at phone, syllable and word levels

    A Separator is made of 3 entries *phone*, *syllable* and *word*
    defining the token separators for each of these levels within an
    utterance.

    A token separator can be a simple string, a regular expression or
    None. If not None, the entries 'phone', 'syllable' and 'word' must
    be all different.

    """
    def __init__(self, phone=' ', syllable=';esyll', word=';eword'):
        # check we have different separators, None excluded
        g1 = list(sep for sep in (phone, syllable, word) if sep)
        g2 = set(sep for sep in (phone, syllable, word) if sep)
        if len(g1) != len(g2):
            raise ValueError(
                'cannot init separator: phone, syllable and word must be '
                'different, they are: "%s", "%s" and "%s"'
                .format(phone, syllable, word))

        self.phone = str(phone) if phone else None
        self.syllable = str(syllable) if syllable else None
        self.word = str(word) if word else None

        # store the tokens as precompiled regular expressions for
        # faster lookup in strings
        self._regexp = {
            'phone': re.compile(self.phone) if phone else None,
            'syllable': re.compile(self.syllable) if syllable else None,
            'word': re.compile(self.word) if word else None}

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

    def split(self, utt, level, remove=True):
        """Split the string `utt` at a given token `level`

        Parameters
        ----------
        utt : str
            The string to split in tokens.
        level : str
            Token level to split the string with. Must be 'phone',
            'syllable' or 'word', raise ArgumentError otherwise.
        remove : bool
            If True, remove all the separators from the returned
            sub-utterances.

        Returns
        -------
        A sequence of substrings of `utt`.

        Raises
        ------
        ArgumentError
            If the `level` is not 'phone', 'syllable' or 'word'.

        """
        if level not in self._regexp.keys():
            raise ArgumentError(
                "level must be 'phone', 'syllable' or 'word', "
                "it is {}".format(level))

        sep = self._regexp[level]
        utts = (u for u in re.split(sep, utt)) if sep else [utt]
        return (self.remove(u) for u in utts) if remove else utts

    def remove(self, utt):
        """Returns the string `utt` with all separators removed

        Multiple spaces are removed.

        """
        if self.phone:
            utt = re.sub(self._regexp['phone'], '', utt)
        if self.syllable:
            utt = re.sub(self._regexp['syllable'], '', utt)
        if self.word:
            utt = re.sub(self._regexp['word'], '', utt)
        return re.sub(' +', ' ', utt)

    def iterate(self, type='value'):
        """Returns a generator on phone, syllable and word tokens, in that order

        Parameters
        ----------
        type : str, optional
            Type of separator representation to be yield, must be
            'value' or 'pair'.

        Yields
        ------
        token : str or tuple
            In the form *token_value* if `type` is 'value'. In the
            form (*token_name*, *token_value*) if `type` is 'pair'.

        Raises
        ------
        ArgumentError
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
            raise ArgumentError(
                'iteration type must be "value" or "pair", it is "{}"'.format(type))
