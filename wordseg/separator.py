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

"""Manage token separation at phone, syllable and word levels"""

import re


class Separator(object):
    """Token separation at phone, syllable and word levels

    A Separator is made of 3 entries 'phone', 'syllable' and 'word'
    defining the token separators for each of these levels.

    A token separator can be a simple string, a regular expression or
    None. If not None, the entries 'phone', 'syllable' and 'word' must
    be all different.

    """
    def __init__(self, phone=' ', syllable=';esyll', word=';eword'):
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

        self._regexp = {
            'phone': re.compile(self.phone) if phone else None,
            'syllable': re.compile(self.syllable) if syllable else None,
            'word': re.compile(self.word) if word else None}

    def __str__(self):
        return '({})'.format(
            ', '.join('{}: "{}"'.format(k, v) for k, v
                      in self.iterate(type='pair') if v))

    def split(self, utt, level, remove=True):
        """Split the string `utt` at a given token `level`

        :param str utt: the string to split

        :param str level: must be 'phone', 'syllable' or 'word', raise
          ValueError otherwise.

        :param bool remove: If True, remove all the separators from
          the returned sub-utterances.

        :return: A sequence of substrings of `utt`.

        """
        if level not in self._regexp.keys():
            raise ValueError(
                "level must be 'phone', 'syllable' or 'word', "
                "it is {}".format(level))

        sep = self._regexp[level]
        utts = (u for u in re.split(sep, utt)) if sep else [utt]
        return (self.remove(u) for u in utts) if remove else utts

    def remove(self, utt):
        """Return the string `utt` with all separators removed"""
        if self.phone:
            utt = re.sub(self._regexp['phone'], '', utt)
        if self.syllable:
            utt = re.sub(self._regexp['syllable'], '', utt)
        if self.word:
            utt = re.sub(self._regexp['word'], '', utt)
        return re.sub(' +', ' ', utt)

    def iterate(self, type='value'):
        """Return a generator on phone, syllable, word tokens (in that order)

        :param str type: must be 'value' or 'pair'

        :return:

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
