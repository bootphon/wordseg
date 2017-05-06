#!/usr/bin/env python
#
# Copyright 2017 Alex Cristia, Elin Larsen, Mathieu Bernard
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

"""Prepare an input text for word segmentation

The input text must be in a phonologized from

"""

# TODO define clearly the format of the input (maybe as a formal grammar?)
# utterances -> utterance utterances
# utterance -> words
# words -> word words
# word -> syllables ;eword
# syllables -> syllable syllables
# syllable -> phones ;esyll
# phones -> phone ' ' phones
# phone -> 'p'

import string
import re

from wordseg import utils


punctuation_re = re.compile('[%s]' % re.escape(string.punctuation))
"""A regular expression matching all the punctuation characters"""


def check_utterance(utt, separator):
    """Raise ValueError if any error is detected on `utt`

    The following errors are checked:
      * `utt` is empty or is not a string
      * `utt` contains any punctuation character (once separators removed)
      * `utt` begins with a separator
      * `utt` does not end with a word separator

    Return True if no error detected

    """
    # utterance is empty or not a string
    if not utt or not isinstance(utt, str):
        raise ValueError('utterance is not a string')
    if not len(utt):
        raise ValueError('utterance is an empty string')

    # search any punctuation in utterance (take care to remove token
    # separators first)
    clean_utt = separator.remove(utt)
    if punctuation_re.sub('', clean_utt) != clean_utt:
        raise ValueError('punctuation found in utterance')

    # utterance begins with a separator
    for sep in separator.iterate():
        if sep and re.match('^{}'.format(re.escape(sep)), utt):
            raise ValueError(
                'utterance begins with the separator "{}"'.format(sep))

    # utterance ends with a word separator
    if not utt.endswith(separator.word):
        raise ValueError('utterance does not end with a word separator')

    return True


def prepare(text, separator, unit='phoneme'):
    """Return a text prepared for word segmentation from a tagged text

    Remove syllable and word separators from a sequence of tagged
    utterances. Marks boundaries at a unit level defined by `unit`.

    :param (sequence of str) text: is the input text to process, each
      string in the sequence is an utterance

    :param Separator separator: token separation in the `text`

    :param str unit: the unit representation level, must be 'syllable'
      or 'phoneme'. This put a space between two syllables or phonemes
      respectively.

    :return: the `text` with separators removed, prepared for
      segmentation at a syllable or phoneme representation level
      (separated by space).

    """
    # raise an error if unit is not valid
    if unit != 'phoneme' and unit != 'syllable':
        raise ValueError(
            "unit must be 'phoneme' or 'syllable', it is '{}'".format(unit))

    if unit == 'phoneme':
        def func(line):
            return line.replace(separator.syllable, '')\
                       .replace(separator.word, '')
    else:  # syllable
        def func(line):
            return line.replace(separator.word, '')\
                       .replace(' ', '')\
                       .replace(separator.syllable, ' ')

    return (utils.strip(func(line)) for line in text
            if check_utterance(line.strip(), separator))


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-prep' command"""
    # add a command-specific argument
    def add_arguments(parser):
        parser.add_argument(
            '-u', '--unit', type=str,
            choices=['phoneme', 'syllable'], default='phoneme', help='''
            output level representation, must be "phoneme" or "syllable"''')

    # command initialization
    streamin, streamout, separator, log, args = utils.prepare_main(
        name='wordseg-gold',
        description=__doc__,
        separator=utils.Separator(' ', ';esyll', ';eword'),
        add_arguments=add_arguments)

    # check all the utterances are correctly formatted. One the first
    # error detected, this raises a ValueError exception catched in
    # the CatchException class: display an error message and exit
    prep = prepare(streamin, separator, unit=args.unit)

    # write prepared text, one utterance a line, ending with a newline
    streamout.write('\n'.join(prep) + '\n')


if __name__ == '__main__':
    main()
