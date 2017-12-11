"""Baseline algorithm for word segmentation

This algorithm randomly adds word boundaries after the input tokens
with a given probability. This probability can be specified by the
user or estimated from an oracle text.

"""

import codecs
import os
import random

from wordseg import utils
from wordseg.separator import Separator


def segment(text, probability=0.5, log=utils.null_logger()):
    """Random word segmentation given a boundary probability

    Given a probability :math:`p`, the probability :math:`P(t_i)` to
    add a word boundary after each token :math:`t_i` is:

    .. math::

        P(t_i) = P(X < p), X \sim \mathcal{U}(0, 1).

    Parameters
    ----------
    text : sequence
        The input utterances to segment, tokens are
        assumed to be space separated.
    probability: float, optional
        The probability to append a word boundary after each token.
    log : logging.Logger
        Where to send log messages

    Yields
    ------
    segmented_text : generator
        The randomly segmented utterances.

    Raises
    ------
    ValueError
        if the probability is not a float in [0, 1].

    """
    # make sure the probability is valid
    if not isinstance(probability, float):
        raise ValueError('probability must be a float')
    if probability < 0 or probability > 1:
        raise ValueError(
            'probability must be in [0, 1], it is {}'.format(probability))

    log.info('P(word boundary) = %s', probability)
    for utt in text:
        yield ''.join(
            token + ' ' if random.random() < probability else token
            for token in utt.strip().split(' '))


def segment_oracle(text, oracle_text,
                   oracle_separator=Separator(),
                   oracle_level='phone',
                   log=utils.null_logger()):
    """Random oracle word segmentation

    The probability of word boundary :math:`p` is estimated from an
    `oracle` text as the ration ``nwords / (nphones or nsyllables)``,
    according to ``oracle_level``. The segmentation is then delegated
    to the segment(text, :math:`p`) method is called.

    Parameters
    ----------
    text : sequence of str
        The input utterances to segment, tokens are
        assumed to be space separated.
    oracle_text : sequence of str
        The text on which to estimate the probaility of word
        boundary. Must be tokenized at word and at least phone or
        syllable levels (according to ``oracle_level``).
    oracle_separator : Separator, optional
        Token separation in the oracle text.
    oracle_level : str, optional
        The level to consider when estimating :math:`p`, must be
        'phone' or 'syllable', default to 'phone'.
    log : logging.Logger
        Where to send log messages

    Yields
    ------
    segmented_text : generator
        The randomly segmented utterances.

    """
    # estimate the word probability boundary in the text
    nphones = sum(
        len(list(oracle_separator.tokenize(utt, level=oracle_level)))
        for utt in oracle_text)
    nwords = sum(
        len(list(oracle_separator.tokenize(utt, level='word')))
        for utt in oracle_text)

    log.info('nwords = %s, n%ss = %s', nwords, oracle_level, nphones)
    if nwords == nphones:
        log.warning(
            'nwords==nphones. Is the oracle\'s token separation correct?')

    probability = float(nwords) / float(nphones)
    return segment(text, probability, log=log)


def _add_arguments(parser):
    """Add algorithm specific options to the parser"""

    parser.add_argument(
        '-r', '--random', type=int, default=None, metavar='<int>',
        help='the seed for initializing the random number generator, '
        'default is based on system time')

    group = parser.add_argument_group('probability of word boundary')
    group = group.add_mutually_exclusive_group()
    group.add_argument(
        '-P', '--probability', type=float, default=0.5, metavar='<float>',
        help='the probability to have a word boundary after a phone, '
        'default is %(default)s')

    group.add_argument(
        '-O', '--oracle', type=str, metavar='<file>',
        help='the word boundary probability is estimated on this oracle text. '
        'Must be tokenized at word and at least phone or syllable levels')

    group = parser.add_argument_group(
        'oracle tokens separation', 'to be used with the --oracle option '
        'to estimate word boundary probability\nas the ratio '
        'nwords / (nphones or nsyllables).')
    separator = Separator()

    group.add_argument(
        '-l', '--level', choices=['phone', 'syllable'], default='phone',
        help='level to consider when computing pwb, default is %(default)s')

    group.add_argument(
        '-p', '--phone-separator', metavar='<str>',
        default=separator.phone,
        help='phone separator in oracle, default is "%(default)s"')

    group.add_argument(
        '-s', '--syllable-separator', metavar='<str>',
        default=separator.syllable,
        help='syllable separator in oracle, default is "%(default)s"')

    group.add_argument(
        '-w', '--word-separator', metavar='<str>',
        default=separator.word,
        help='word separator in oracle, default is "%(default)s"')


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-baseline' command"""
    streamin, streamout, _, log, args = utils.prepare_main(
        name='wordseg-baseline',
        description=__doc__,
        add_arguments=_add_arguments)

    # setup the seed for random number generation
    if args.random:
        log.info('setup random seed to %s', args.random)
    random.seed(args.random)

    if args.oracle:
        # load the oracle text
        if not os.path.isfile(args.oracle):
            raise ValueError('oracle file not found: {}'.format(args.oracle))
        oracle_text = [utt for utt in codecs.open(args.oracle, 'r')]
        log.info('loaded %s utterances from oracle text', len(oracle_text))

        # init the oracle tokens separator
        oracle_separator = Separator(
            phone=args.phone_separator,
            syllable=args.syllable_separator,
            word=args.word_separator)

        segmented = segment_oracle(
            streamin, oracle_text, oracle_separator, args.level, log=log)
    else:
        segmented = segment(
            streamin, probability=args.probability, log=log)

    streamout.write('\n'.join(segmented) + '\n')


if __name__ == '__main__':
    main()
