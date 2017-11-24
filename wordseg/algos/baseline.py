"""Baseline algorithm for word segmentation

This algorithm randomly adds word boundaries after the input phones
with a given probability.

"""

import random
from wordseg import utils


def segment(text, probability=0.2):
    """Random word segmentation given a boundary probability

    Parameters
    ----------
    text : sequence
        The input sequence of utterances to segment
    probability: float, optional
        The probability to have a word boundary after an input token

    Yields
    ------
    The randomly segmented utterances

    Raises
    ------
    ValueError if the probability is not a float or is not in [0, 1].

    """
    # make sure the probability is valid
    if not isinstance(probability, float):
        raise ValueError('probability must be a float')
    if probability < 0 or probability > 1:
        raise ValueError(
            'probability must be in [0, 1], it is {}'.format(probability))

    for utt in text:
        yield ''.join(
            token + ' ' if random.random() < probability else token
            for token in utt.strip().split(' '))


def add_arguments(parser):
    """Add algorithm specific options to the parser"""
    parser.add_argument(
        '-p', '--probability', type=float, default=0.2, metavar='<float>',
        help='the probability to have a word boundary after a phone, '
        'default is %(default)s')

    parser.add_argument(
        '-s', '--seed', type=int, default=None, metavar='<int>',
        help='the seed for initializing the random number generator, '
        'default is based on system time')


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-baseline' command"""
    streamin, streamout, _, log, args = utils.prepare_main(
        name='wordseg-puddle',
        description=__doc__,
        add_arguments=add_arguments)

    # setup the seed for random number generation
    if args.seed:
        log.debug('setup random seed to %s', args.seed)
    random.seed(args.seed)

    segmented = segment(streamin, probability=args.probability)

    streamout.write('\n'.join(segmented) + '\n')


if __name__ == '__main__':
    main()
