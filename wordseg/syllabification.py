"""Estimates syllable boundaries on a text using the maximal onset principle.

This algorithm fully syllabifies a text from a list of onsets and
vowels. Input text must be in orthographic form (with word separators
only) or in phonemized form (with both word and phone
separators). Output text has syllable separators added at estimated
syllable boundaries. For exemples of vowels and onsets files, see the
directory `wordseg/data/syllabification`.

"""

# Created by Lawrence Phillips & Lisa Pearl (2013), adapted by Alex
# Cristia (2015), converted from perl to python and integration in
# wordseg by Mathieu Bernard (2017). Credit is owed mainly to the
# original authors.


import codecs
import os
import re

from wordseg import utils
from wordseg.separator import Separator


def _remove_phone_separators(utt, separator):
    # special case when there is no phone separator in the utterance
    if not re.search(separator.phone, utt):
        return utt, []

    # the returned index is a list of lists (for each word, length of
    # each phone)
    index = []

    # split the utterance in words and index the phones length
    for word in separator.split(utt, 'word', keep_boundaries=True):
        phones = separator.split(word, 'phone', keep_boundaries=False)
        current_index = [len(p) for p in phones if len(p)]
        if current_index:
            index.append(current_index)

    return separator.remove(utt, level='phone'), index


def _restore_phone_separators(utt, index, separator, strip=False):
    # special case when there is no phone separator in the utterance
    if index == []:
        return utt

    # restore the utterance word per word (index[i]) and within words,
    # phone per phone (index[i][j]).
    restored = ''
    for i, word in enumerate(separator.split(utt, 'word', keep_boundaries=True)):
        if len(word) == 0:
            # coherent behavior for non striped texts
            restored += separator.word
        else:
            j = 0  # iterate on syllables
            for syllable in separator.split(word, 'syllable', keep_boundaries=True):
                # for each phone in the syllable, append a phone
                # separator
                k = 0
                while k < len(syllable):
                    restored += syllable[k:k+index[i][j]] + separator.phone
                    k += index[i][j]
                    j += 1

                # end of the syllable, append a separator
                if strip:
                    restored = restored[:-len(separator.phone)]
                restored += separator.syllable

            # end of the word, remove the last syllable boundary
            # append a word separator
            restored = restored[:-len(separator.syllable)] + separator.word

    # remove the last word boundary of the utterance
    return restored[:-len(separator.word)]


def _build_onset(word, syllable, onsets, vowels):
    try:
        prevchar = word[-1]
        if prevchar not in vowels:
            # if this char is a vowel and the previous one is not,
            # then we need to make the onset, start with nothing as
            # the onset
            onset = ''

            # then we want to take one letter at a time and check
            # whether their concatenation makes a good onset
            while len(word) and word[-1] + onset in onsets:
                onset = word[-1] + onset
                word = word[:-1]

            # we get here either because we've concatenated the
            # onset+rest or because there was no onset and the
            # preceding element is a vowel, so this is the end of the
            # syllable
            syllable = onset + syllable
    except IndexError:  # there is no previous char
        pass

    return word, syllable


def _syllabify_utterance(utt, onsets, vowels, separator, strip, log):
    # split the utterances into words, read them from end to start
    words = list(separator.split(utt.strip(), 'word', keep_boundaries=True))[::-1]

    # estimate syllables boudaries word per word
    output = ''
    for word in words:
        output_word = ''
        syllable = ''

        # read characters of the current word from end to start
        while len(word) > 0:
            char, word = word[-1], word[:-1]

            # append current char to current syllable - that will be
            # necessary regardless of whether it's a vowel or a coda
            syllable = char + syllable

            if char in vowels:
                word, syllable = _build_onset(word, syllable, onsets, vowels)

                # add the syllable to words entry
                if strip and not output_word:
                    output_word = syllable
                else:
                    output_word = syllable + separator.syllable + output_word
                syllable = ''

        # concatenate the syllabified word to the output, do not
        # append a word separator at the end if stripped
        if strip and not output:
            output = output_word
        else:
            output = output_word + separator.word + output

    return output


def syllabify(text, onsets, vowels, separator=Separator(),
              strip=False, log=utils.null_logger()):
    """Syllabify a text given in phonological or orthographic form

    Parameters
    ----------
    text : sequence
        The input text to be syllabified. Each element of the sequence
        is assumed to be a single and complete utterance in valid
        phonological form.
    onsets : list
        The list of valid onsets in the `text`
    vowels : list
        The list of vowels in the `text`
    separator : Separator, optional
        Token separation in the `text`
    strip : bool, optional
        When True, removes the syllable boundary at the end of words.
    log : logging.Logger, optional
        Where to send log messages

    Returns
    -------
    The text with estimated syllable boundaries added

    Raises
    ------
    ValueError
        if `separator.syllable` is found in the text, or if `onsets`
        or `vowels` are empty.

    """
    # ensure onsets and vowels are not empty
    if not isinstance(vowels, list) or not len(vowels):
        raise ValueError('unvalid or empty vowels list')
    if not isinstance(onsets, list) or not len(onsets):
        raise ValueError('unvalid or empty onsets list')

    # we are syllabifying utterance per utterance
    syllabified_text = []
    for n, utt in enumerate(text):
        # first ensure the utterance is compatible with the given
        # syllable separator
        if separator.syllable in utt:
            raise ValueError(
                'syllable separator "{}" found in text (line {}): {}'
                .format(separator.syllable, n+1, utt))

        # if we have phone separators, removes them and store their positions
        utt, index = _remove_phone_separators(utt, separator)

        # estimate the syllable boundaries on the utterance
        syllables = _syllabify_utterance(
            utt, onsets, vowels, separator, strip, log)

        # restore the phones separators as they were before
        syllables = _restore_phone_separators(syllables, index, separator, strip)

        syllabified_text.append(syllables)

    return syllabified_text


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

    parser.add_argument(
        '--strip', action='store_true',
        help='removes the end separators in syllabified output')


def open_datafile(data_file):
    """Read a vowel or onsets file as a list"""
    return [o.strip() for o in
            codecs.open(data_file, 'r', encoding='utf8').readlines()
            if o.strip()]  # ignore empty lines


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
        raise RuntimeError(
            'unknown onsets file "{}"'.format(args.onsets_file))
    onsets = open_datafile(args.onsets_file)

    # loads the vowels
    if not os.path.isfile(args.vowels_file):
        raise RuntimeError(
            'unknown vowels file "{}"'.format(args.vowels_file))
    vowels = open_datafile(args.vowels_file)

    log.info('loaded %s onsets', len(onsets))
    log.debug('onsets are "%s"', ', '.join(onsets))
    log.info('loaded %s vowels', len(vowels))
    log.debug('vowels are "%s"', ', '.join(vowels))
    log.debug('separator is %s', separator)

    # syllabify the input text
    sylls = utils.CountingIterator(syllabify(
        streamin, onsets, vowels,
        separator=separator, strip=args.strip, log=log))

    # display the output
    log.info('syllabified %s utterances', sylls.count)
    streamout.write('\n'.join(sylls) + '\n')


if __name__ == '__main__':
    main()
