"""Baseline algorithm for word segmentation

This algorithm randomly adds word boundaries after the input tokens
with a given probability.

"""

import random
from wordseg import utils


def segment(text, probability=0.2):
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

    for utt in text:
        yield ''.join(
            token + ' ' if random.random() < probability else token
            for token in utt.strip().split(' '))


def _add_arguments(parser):
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
        add_arguments=_add_arguments)

    # setup the seed for random number generation
    if args.seed:
        log.debug('setup random seed to %s', args.seed)
    random.seed(args.seed)

    segmented = segment(streamin, probability=args.probability)

    streamout.write('\n'.join(segmented) + '\n')


if __name__ == '__main__':
    main()
