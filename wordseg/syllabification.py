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
import six

from wordseg import utils
from wordseg.separator import Separator


class Syllabifier(object):
    """Syllabify a text given in phonological or orthographic form

    Syllabification errors can occur when the onsets and/or vowels are
    not adapted to the input text (see the `tolerant` parameter).

    Parameters
    ----------
    onsets : list
        The list of valid onsets in the `text`
    vowels : list
        The list of vowels in the `text`
    separator : Separator, optional
        Token separation in the `text`
    silent : bool, optional
        When True, append a silent vowel to the end of words without
        vowel (the vowel is removed after processing so the text is
        unchanged). When False those words cannot be syllabified.
    log : logging.Logger, optional
        Where to send log messages

    Raises
    ------
    ValueError
        If `onsets` or `vowels` are empty or are not lists.

    """
    def __init__(self, onsets, vowels, separator=Separator(),
                 filling_vowel=False, log=utils.null_logger()):
        self.onsets = onsets
        self.vowels = vowels
        self.separator = separator
        self.log = log

        # ensure onsets and vowels are not empty
        if not isinstance(vowels, list) or not len(vowels):
            raise ValueError('unvalid or empty vowels list')
        if not isinstance(onsets, list) or not len(onsets):
            raise ValueError('unvalid or empty onsets list')

        # concatenation of all chars in onsets and vowels (usefull to
        # detect any char during syllabification)
        self.symbols = (
            set(''.join(v for v in vowels)).union(
                set(''.join(o for o in onsets))))

        # if defined, ensure the silent vowel is not already used
        if filling_vowel:
            # find a silent vowel (some char not already prensent in
            # the symbols)
            code = 1
            while six.unichr(code) in self.symbols:
                code += 1
            self.silent = six.unichr(code)
            self.symbols.add(self.silent)
            self.vowels.append(self.silent)
        else:
            self.silent = None

    def syllabify(self, text, strip=False, tolerant=False):
        """Returns the text with syllable boundaries added

        Parameters
        ----------
        text : sequence
            The input text to be syllabified. Each element of the sequence
            is assumed to be a single and complete utterance in valid
            phonological form.
        strip : bool, optional
            When True, removes the syllable boundary at the end of words.
        tolerant : bool, optional
            When False (the default), the function raise a ValueError on
            the first utterance that have not been correctly
            syllabified. When True, ignore the failed utterances in output
            but issue a log warning instead.

        Returns
        -------
        The text with estimated syllable boundaries added. If `tolerant`
        is True some utterances may be missing in the output.

        Raises
        ------
        ValueError
            If an utterance has not been correctly syllabified . If
            `separator.syllable` is found in the text, or if `onsets`
            or `vowels` are empty.

        """
        # we are syllabifying utterance per utterance
        syllabified_text = []
        nerrors = 0
        for n, utt in enumerate(text):
            utt = utt.strip()

            # first ensure the utterance is compatible with the given
            # syllable separator
            if self.separator.syllable in utt:
                raise ValueError(
                    'syllable separator "{}" found in text (line {}): {}'
                    .format(self.separator.syllable, n+1, utt))

            # if we have phone separators, removes them and store
            # their positions
            utt, index = self._remove_phone_separators(utt)

            # estimate the syllable boundaries on the utterance
            try:
                syllables = self._syllabify_utterance(utt, strip=strip)
            except RuntimeError as err:
                error = 'line {}, {}'.format(n+1, err)
                if tolerant:
                    # issue a warning and ignore that utterance
                    self.log.warning(error)
                    nerrors += 1
                    continue
                else:
                    # fail with error
                    raise ValueError(error)

            # restore the phones separators as they were before
            syllables = self._restore_phone_separators(syllables, index, strip)

            syllabified_text.append(syllables)

        if tolerant and nerrors > 0:
            self.log.error(
                'syllabification failed for {} utterances'.format(nerrors))

        return syllabified_text

    @staticmethod
    def open_datafile(data_file):
        """Read a vowel or onsets file as a list"""
        data = codecs.open(data_file, 'r', encoding='utf8').readlines()

        # ignore empty lines in data file
        return [line for line in (line.strip() for line in data) if line]

    def _syllabify_utterance(self, utterance, strip=False):
        """Syllabify a single utterance

        Auxiliary function to syllabify_text().

        Raises
        ------
        RuntimeError
            If the syllabification failed

        """
        # split the utterances into words
        words = self.separator.tokenize(
            utterance, level='word', keep_boundaries=False)

        # estimate syllables boundaries word per word, read them from
        # end to start
        output = ''
        for n, word in enumerate(words[::-1]):
            try:
                output_word = self._syllabify_word(word, strip)
            except RuntimeError as err:
                # forward the exception with word id added
                raise RuntimeError(
                    'word {}: {}'.format(len(words) - n, err))

            # concatenate the syllabified word to the output, do not
            # append a word separator at the end if stripped
            if strip and not self.separator.remove(output):
                output = output_word
            else:
                output = output_word + self.separator.word + output

        return output

    def _syllabify_word(self, word, strip):
        """Return a single word with syllable boundaries added

        Auxiliary function to syllabify_utterance().

        Raises
        ------
        RuntimeError
            If the word has no vowel, contains an unknown symbol (not
            present in vowels or onsets) or if the syllabification
            failed.

        """
        # ensure all the chars in word are defined in vowels or onsets
        unknown = self._unknown_char(word)
        if unknown:
            raise RuntimeError(
                'unknown symbol "{}" in word "{}"'.format(unknown, word))

        # ensure the word containe at least a vowel
        if not self._has_vowels(word):
            if not self.silent:
                raise RuntimeError(
                    'no vowel in word "{}"'.format(word))
            else:
                word += self.silent

        input_word = word
        output_word = ''
        syllable = ''

        # read characters of the current word from end to start
        while len(word) > 0:
            char, word = word[-1], word[:-1]

            # append current char to current syllable - that will be
            # necessary regardless of whether it's a vowel or a coda
            syllable = char + syllable

            if char in self.vowels:
                word, syllable = self._build_onset(word, syllable)

                # add the syllable to words entry
                if strip and not output_word:
                    output_word = syllable
                else:
                    output_word = (
                        syllable + self.separator.syllable + output_word)
                syllable = ''

        if input_word != self.separator.remove(output_word, 'syllable'):
            raise RuntimeError(
                'onset not found in "{}"'.format(
                    input_word,
                    self.separator.remove(output_word, 'syllable')))

        if self.silent:
            return re.sub(self.silent, '', output_word)
        else:
            return output_word

    def _build_onset(self, word, syllable):
        try:
            prevchar = word[-1]
            if prevchar not in self.vowels:
                # if this char is a vowel and the previous one is not,
                # then we need to make the onset, start with nothing as
                # the onset
                onset = ''

                # then we want to take one letter at a time and check
                # whether their concatenation makes a good onset
                while len(word) and word[-1] + onset in self.onsets:
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

    def _remove_phone_separators(self, utt):
        # special case when there is no phone separator in the utterance
        if not re.search(self.separator.phone, utt):
            return utt, []

        # the returned index is a list of lists (for each word, length
        # of each phone)
        index = []

        # split the utterance in words and index the phones length
        for word in self.separator.split(utt, 'word', keep_boundaries=True):
            phones = self.separator.split(word, 'phone', keep_boundaries=False)
            current_index = [len(p) for p in phones if len(p)]
            if current_index:
                index.append(current_index)

        return self.separator.remove(utt, level='phone'), index

    def _restore_phone_separators(self, utt, index, strip):
        # special case when there is no phone separator in the
        # utterance
        if index == []:
            return utt

        # restore the utterance word per word (index[i]) and within
        # words, phone per phone (index[i][j]).
        restored = ''
        for i, word in enumerate(
                self.separator.split(utt, 'word', keep_boundaries=True)):
            if len(word) == 0 and strip is False:
                # coherent behavior for non striped texts
                restored += self.separator.word
            else:
                j = 0  # iterate on syllables
                for syllable in self.separator.split(
                        word, 'syllable', keep_boundaries=True):
                    # for each phone in the syllable, append a phone
                    # separator
                    k = 0
                    while k < len(syllable):
                        restored += (
                            syllable[k:k+index[i][j]] + self.separator.phone)
                        k += index[i][j]
                        j += 1

                    # end of the syllable, append a separator
                    if strip:
                        restored = restored[:-len(self.separator.phone)]
                    restored += self.separator.syllable

                # end of the word, remove the last syllable boundary
                # append a word separator
                restored = (restored[:-len(self.separator.syllable)] +
                            self.separator.word)

        # remove the last word boundary of the utterance
        return restored[:-len(self.separator.word)]

    def _unknown_char(self, word):
        """Returns the unknown char if anyone if found, False otherwise"""
        for w in word:
            if w not in self.symbols:
                return w
        return False

    def _has_vowels(self, word):
        """True if the `word` contains any vowel, False otherwise"""
        for v in self.vowels:
            if v in word:
                return True
        return False


def _add_arguments(parser):
    """Add command line arguments for wordseg-syll"""
    parser.add_argument(
        'onsets_file', type=str, metavar='<onsets-file>',
        help=('a file containing the list of valid onsets for the '
              'input text, one onset per line'))

    parser.add_argument(
        'vowels_file', type=str, metavar='<vowels-file>',
        help=('a file containing the list of vowels for '
              'the input text, one vowel per line'))

    parser.add_argument(
        '-S', '--strip', action='store_true',
        help='removes the end separators in syllabified output')

    parser.add_argument(
        '-t', '--tolerant', action='store_true',
        help='tolerate syllabification failures and report them as warnings, '
        'default is to fail at the first error')

    parser.add_argument(
        '-f', '--filling-vowel', action='store_true',
        help='add a silent filling vowel to groups of consonants '
        'with no vowel, by default words with no vowel cannot be syllabified')


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
    onsets = Syllabifier.open_datafile(args.onsets_file)

    # loads the vowels
    if not os.path.isfile(args.vowels_file):
        raise RuntimeError(
            'unknown vowels file "{}"'.format(args.vowels_file))
    vowels = Syllabifier.open_datafile(args.vowels_file)

    log.info('loaded %s onsets', len(onsets))
    log.debug('onsets are "%s"', ', '.join(onsets))
    log.info('loaded %s vowels', len(vowels))
    log.debug('vowels are "%s"', ', '.join(vowels))
    log.debug('separator is %s', separator)

    syllabifier = Syllabifier(
        onsets, vowels, separator=separator,
        filling_vowel=args.filling_vowel, log=log)

    # syllabify the input text
    sylls = utils.CountingIterator(syllabifier.syllabify(
        streamin, strip=args.strip, tolerant=args.tolerant))

    # display the output
    log.info('syllabified %s utterances', sylls.count)
    streamout.write('\n'.join(sylls) + '\n')


if __name__ == '__main__':
    main()
