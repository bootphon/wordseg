"""Prepare an input text for word segmentation

* The input text must be in a phonologized form (a suite of phones,
  syllables or words tokens as specified by the token separator).

* The input text is checked for errors in formatting (presence of
  punctuation, missing separators, etc...).

* The output text contains space separated phones (or syllables
  according to the *unit* option).

* The program fails on the first encountered error, or ignore them if
  the *tolerant* option is used.

"""

import six
import string
import re

from wordseg import utils
from wordseg.separator import Separator


punctuation_re = re.compile('[%s]' % re.escape(string.punctuation))
"""A regular expression matching all the punctuation characters"""


def _pairwise(l):
    """Yields paiwise elements of a sequence

    Examples
    --------
    >>> list(pairwise([1, 2, 3]))
    [(1, 2), (2, 3)]

    """
    for a, b in zip(l[:-1], l[1:]):
        yield a, b


def check_utterance(utterance, separator=Separator(), check_punctuation=True):
    """Ensures an utterance is in a valid phonological form

    Parameters
    ----------
    utterance : str
        The utterance to be checked
    separator : Separator, optional
        The token separators used in the `utterance`
    check_punctuation : bool, optional
        When True (default), forbid any punctuation character in the
        utterance and raise ValueError if any punctuation is
        found. When False, do not check punctiation.

    Returns
    -------
    bool
        True if no error detected, raises otherwise

    Raises
    ------
    ValueError
        If one of the following errors is detected:

        * `utterance` is empty or is not a string
        * `utterance` contains any punctuation character (once the
          separators are removed), only if `check_punctuation` is
          True
        * `utterance` begins with a separator
        * `utterance` does not end with a word separator
        * `utterance` contains syllable tokens but a word does not end
          with a syllable separator

    """
    # utterance is empty or not a string (or unicode for python2)
    if not utterance or not isinstance(utterance, six.string_types):
        raise ValueError(
            'utterance is not a string ({}): {}'.format(
                type(utterance), utterance))

    if not len(utterance):
        raise ValueError('utterance is an empty string')

    # search any punctuation in utterance (take care to remove token
    # separators first)
    if check_punctuation is True:
        cleaned_utterance = separator.remove(utterance)
        if punctuation_re.sub('', cleaned_utterance) != cleaned_utterance:
            raise ValueError('punctuation found in utterance')

    # utterance begins with a separator
    for sep in separator.iterate():
        if sep and re.match('^{}'.format(re.escape(sep)), utterance):
            raise ValueError(
                'utterance begins with a separator: "{}"'.format(utterance))

    # utterance ends with a word separator
    if not utterance.endswith(separator.word):
        raise ValueError(
            'utterance does not end with a word separator: "{}"'
            .format(utterance))

    # a words does not finish with a syllable separator
    if separator.syllable and separator.syllable in utterance and not all(
            a == separator.syllable
            for a, b in _pairwise(utterance.split(separator.phone))
            if b == separator.word):
        raise ValueError(
            'a word does not end with a syllable separator: "{}"'
            .format(utterance))

    return True


def prepare(text, separator=Separator(), unit='phone',
            check_punctuation=True, tolerant=False,
            log=utils.null_logger()):
    """Prepares a text in phonological form for word segmentation

    The returned text is ready to be segmented. It consists in a suite
    of phonological symbols (can be phones or syllable depending on
    `unit`) separated by spaces.

    The function removes the word separators from all the lines in
    `text` and replaces boundaries at the unit level defined by `unit`
    by a space. If `unit` is 'phone' the syllable separators are
    removed, and vice-versa if `unit` is 'syllable' the phone
    separators are dicarded.

    Parameters
    ----------
    text : sequence
        The input text to be prepared for segmentation. Each element
        of the sequence is assumed to be a single and complete
        utterance in valid phonological form.
    separator : Separator, optional
        Token separation in the `text`
    unit : str, optional
        The unit representation level to prepare the `text` at, must
        be 'syllable' or 'phone'.
    check_punctuation : bool, optional
        When True (default), forbid any punctuation character in the
        utterance and raise ValueError if any punctuation is
        found. When False, do not check punctiation.
    tolerant : bool, optional
        If False, raise ValueError on the first format error detected
        in the `text`. If True, the badly formated utterances are
        filtered out from the output and a warning is issued.
    log : logging.Logger, optional
        The logger instance where to send messages.

    Returns
    -------
    prepared_text : generator
        Utterances from the `text` with separators removed, prepared
        for segmentation at a syllable or phoneme representation level
        (separated by space).

    Raises
    ------
    ValueError
        On the first format error encountered in `text` (see the
        prepare.check_utterance function), only if `tolerant` is
        False.

    """
    # raise an error if unit is not valid
    if unit != 'phone' and unit != 'syllable':
        raise ValueError(
            "unit must be 'phone' or 'syllable', it is '{}'".format(unit))

    if unit == 'phone':
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
            nremoved += 1
            continue

        try:
            check_utterance(
                line, separator, check_punctuation=check_punctuation)
            yield utils.strip(func(line))
        except ValueError as err:
            if tolerant is True:
                log.info('removing line %d: "%s"', n + 1, line)
                nremoved += 1
            else:
                raise ValueError('line {}: {}'.format(n + 1, err))

    if nremoved > 0:
        log.warning('removed %d badly formatted utterances', nremoved)


def gold(text, separator=Separator()):
    """Returns a gold text from a phonologized one

    The returned gold text is the ground-truth segmentation. It has
    phone and syllable separators removed and word separators replaced
    by a single space ' '. It is used to evaluate the output of
    segmentation algorithms.

    Parameters
    ----------
    text : sequence
        The input text to be prepared for segmentation. Each element
        of the sequence is assumed to be a single and complete
        utterance in valid phonological form.
    separator : Separator, optional
        Token separation in the `text`

    Returns
    -------
    gold_text : generator
        Gold utterances with separators removed and words separated by
        spaces. The returned text is the gold version, against which
        the algorithms are evaluated.

    """
    # delete phone and syllable separators. Replace word boundaries by
    # a single space.
    gold = (line.replace(separator.syllable, '')
            .replace(separator.phone or '', '')
            .replace(separator.word, ' ') for line in text)

    # delete any duplicate, begin or end spaces.
    return (utils.strip(line) for line in gold)


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-prep' command"""
    # add a command-specific argument
    def add_arguments(parser):
        parser.add_argument(
            '-u', '--unit', type=str,
            choices=['phone', 'syllable'], default='phone', help='''
            output level representation, must be "phone" or "syllable"''')

        parser.add_argument(
            '-t', '--tolerant', action='store_true',
            help='''tolerate the badly formated utterances in input,
            but ignore them in output (default is to exit on the first
            encountered error)''')

        parser.add_argument(
            '-P', '--punctuation', action='store_true',
            help='punctuation characters are not considered illegal')

        group = [g for g in parser._action_groups
                 if g.title == 'input/output arguments'][0]
        group.add_argument(
            '-g', '--gold', type=str, metavar='<gold-file>',
            help='''generates the gold text to the specified file,
            do not generate gold if no file specified''')

    # command initialization
    streamin, streamout, separator, log, args = utils.prepare_main(
        name='wordseg-prep',
        description=__doc__,
        separator=utils.Separator(' ', ';esyll', ';eword'),
        add_arguments=add_arguments)

    streamin = list(streamin)

    log.debug('separator is %s', separator)

    # check all the utterances are correctly formatted.
    prep = utils.CountingIterator(prepare(
        streamin, separator, unit=args.unit, log=log,
        check_punctuation=not args.punctuation, tolerant=args.tolerant))

    # write prepared text, one utterance a line, ending with a newline
    streamout.write('\n'.join(prep) + '\n')
    log.debug('prepared %s utterances', prep.count)

    if args.gold:
        log.info('generating gold text to %s', args.gold)
        gold_text = gold(streamin, separator=separator)
        open(args.gold, 'w').write('\n'.join(gold_text) + '\n')


if __name__ == '__main__':
    main()
