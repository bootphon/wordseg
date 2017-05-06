#!/usr/bin/env python
#
# Copyright 2015 - 2017 Alex Cristia, Mathieu Bernard
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

"""Build a gold text from a phonologized one.

Remove syllable and word separators from a sequence of tagged
utterances. The returned text is the gold version, against which the
algorithms are evaluated.

"""

import re

from wordseg import utils, Separator


def gold(text, separator=Separator()):
    """Return a gold text from a phonologized one

    Remove syllable and word separators from a sequence of tagged
    utterances. The returned text is the gold version, against which
    the algorithms are evaluated.

    :param sequence(str) text: the input sequence to process, each
      string in the sequence is an utterance

    :param Separator separator: token separation in the `text`

    :return sequence(str): text with separators removed, with word
      separated by spaces

    """
    # delete syllable and word separators
    gold = (line.replace(separator.syllable, '')
            .replace(separator.phone or '', '')
            .replace(separator.word, ' ') for line in text)

    # delete any duplicate, begin or end spaces
    return (utils.strip(line) for line in gold)


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-gold' command"""
    # command initialization
    streamin, streamout, separator, log, args = utils.prepare_main(
        name='wordseg-gold',
        description=__doc__,
        separator=utils.Separator(' ', ';esyll', ';eword'))

    gold_text = gold(streamin, separator=separator)

    # write gold, one utterance per line, add a newline at the end
    streamout.write('\n'.join(gold_text) + '\n')


if __name__ == '__main__':
    main()
