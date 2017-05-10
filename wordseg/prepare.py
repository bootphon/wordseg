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

The input text must be in a phonologized form (TODO define that). The
input text is checked for errors in formatting (presence of
punctuation, missing separators, etc...). The program fails on the
first encountered error, or ignore them if the "--tolerant" option is
used.

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

import six
import string
import re

from wordseg import utils, Separator


punctuation_re = re.compile('[%s]' % re.escape(string.punctuation))
"""A regular expression matching all the punctuation characters"""


def _pairwise(l):
    """l -> (l[0], l[1]), (l[1], l[2]), ..."""
    for a, b in zip(l[:-1], l[1:]):
        yield a, b


def check_utterance(utt, separator):
    """Raise ValueError if any error is detected on `utt`

    The following errors are checked:
      * `utt` is empty or is not a string
      * `utt` contains any punctuation character (once separators removed)
      * `utt` begins with a separator
      * `utt` does not end with a word separator
      * `utt` contains syllable tokens but a word does not end with a
        syllable separator

    Return True if no error detected

    """
    # utterance is empty or not a string (or unicode for python2)
    if not utt or not isinstance(utt, six.string_types):
        raise ValueError(
            'utterance is not a string ({}): {}'.format(type(utt), utt))

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
                'utterance begins with a separator: "{}"'.format(utt))

    # utterance ends with a word separator
    if not utt.endswith(separator.word):
        raise ValueError(
            'utterance does not end with a word separator: "{}"'.format(utt))

    # a words does not finish with a syllable separator
    if separator.syllable and separator.syllable in utt and not all(
            a == separator.syllable
            for a, b in _pairwise(utt.split(separator.phone))
            if b == separator.word):
        raise ValueError(
            'a word does not end with a syllable separator: "{}"'.format(utt))

    return True


def prepare(text, separator=Separator(), unit='phoneme', tolerant=False,
            log=utils.null_logger()):
    """Return a text prepared for word segmentation from a tagged text

    Remove syllable and word separators from a sequence of tagged
    utterances. Marks boundaries at a unit level defined by `unit`.

    :param (sequence of str) text: is the input text to process, each
      string in the sequence is an utterance

    :param Separator separator: token separation in the `text`

    :param str unit: the unit representation level, must be 'syllable'
      or 'phoneme'. This put a space between two syllables or phonemes
      respectively.

    :param bool tolerant: if False, raise ValueError on the first
      format error detected in the `text`. If True, the badly formated
      utterances are filtered out from the output.

    :param logger log: a logger instance where to send messages

    :raise: ValueError on the first format error in `text` (see the
       prepare.check_utterance function), only if `tolerant` is False.

    :return: a generator of utterances from the `text` with separators
      removed, prepared for segmentation at a syllable or phoneme
      representation level (separated by space).

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

    nremoved = 0
    for n, line in enumerate(text):
        line = line.strip()

        # ignore empty lines
        if line == '':
            log.debug('ignoring empty line %d', n)
            nremoved +=1
            continue

        try:
            check_utterance(line, separator)
            yield utils.strip(func(line))
        except ValueError as err:
            if tolerant:
                log.debug('removing line %d: "%s"', n, line)
                nremoved += 1
            else:
                raise err

    if nremoved:
        log.warning('removed %d badly formatted utterances', nremoved)


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-prep' command"""
    # add a command-specific argument
    def add_arguments(parser):
        parser.add_argument(
            '-u', '--unit', type=str,
            choices=['phoneme', 'syllable'], default='phoneme', help='''
            output level representation, must be "phoneme" or "syllable"''')

        parser.add_argument(
            '-t', '--tolerant', action='store_true',
            help='''tolerate the badly formated utterances in input, but ignore them in
            output (default is to exit on the first encountered
            error)''')

    # command initialization
    streamin, streamout, separator, log, args = utils.prepare_main(
        name='wordseg-prep',
        description=__doc__,
        separator=utils.Separator(' ', ';esyll', ';eword'),
        add_arguments=add_arguments)

    log.debug('separator is %s', separator)

    # check all the utterances are correctly formatted.
    prep = utils.CountingIterator(prepare(
        streamin, separator, unit=args.unit, tolerant=args.tolerant))

    # write prepared text, one utterance a line, ending with a newline
    streamout.write('\n'.join(prep) + '\n')
    log.debug('prepared %s utterances', prep.count)


if __name__ == '__main__':
    main()
