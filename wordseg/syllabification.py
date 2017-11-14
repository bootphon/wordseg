"""Estimates syllable boundaries on a phonologized text

Uses the maximum onset principle to fully syllabify a corpus from a
list of onsets and vowels.

"""

# Created by Lawrence Phillips & Lisa Pearl (2013), adapted by Alex
# Cristia (2015), converted from perl to python and integration in
# wordseg by Mathieu Bernard (2017). Credit is owed mainly to the
# original authors.


import codecs
import os

from wordseg import utils
from wordseg.separator import Separator


def syllabify(text, onsets, vowels, separator=Separator(),
              log=utils.null_logger()):
    """Syllabify a text given in phonological form

    Parameters
    ----------
    text : sequence
        The input text to be prepared for segmentation. Each element
        of the sequence is assumed to be a single and complete
        utterance in valid phonological form.
    onsets : list
        The list of valid onsets in the `text`
    vowels : list
        The list of vowels in the `text`
    separator : Separator, optional
        Token separation in the `text`

    """
    return text


def _add_arguments(parser):
    """Add command line arguments for wordseg-syll"""
    parser.add_argument(
        'onsets_file', type=str, metavar='<onsets-file>',
        help=('a file containing valid onsets for the '
              'input text, one onset per line'))

    parser.add_argument(
        'vowels_file', type=str, metavar='<vowels-file>',
        help=('a file containing the list of vowels for '
              'the input text, one vowel per line'))


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-syll' command"""
    streamin, streamout, separator, log, args = utils.prepare_main(
        name='wordseg-syll',
        description=__doc__,
        separator=utils.Separator(' ', ';esyll', ';eword'),
        add_arguments=_add_arguments)

    # loads the onsets
    if not os.path.isfile(args.onsets_file):
        raise RuntimeError('unknown onsets file "{}"'.format(args.onsets_file))
    onsets = [o.strip() for o in
              codecs.open(args.onsets_file, 'r', encoding='utf8').readlines()]

    # loads the vowels
    if not os.path.isfile(args.vowels_file):
        raise RuntimeError('unknown vowels file "{}"'.format(args.vowels_file))
    vowels = [v.strip() for v in
              codecs.open(args.vowels_file, 'r', encoding='utf8').readlines()]

    log.info('loaded %s onsets', len(onsets))
    log.debug('onsets are %s', onsets)
    log.info('loaded %s vowels', len(vowels))
    log.debug('vowels are %s', vowels)
    log.debug('separator is %s', separator)

    # syllabify the input text
    sylls = utils.CountingIterator(syllabify(
        streamin, onsets, vowels, separator=separator, log=log))

    # display the output
    streamout.write('\n'.join(sylls) + '\n')
    log.debug('syllabified %s utterances', sylls.count)


if __name__ == '__main__':
    main()
