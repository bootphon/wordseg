"""Transitional Probabilities word segmentation"""

import collections
import re

from wordseg import utils


def _threshold_relative(syls, tps):
    """Relative threshold segmentation method"""
    prelast = syls[0]
    last = syls[1]
    syl = syls[2]

    cword = [prelast, last]
    cwords = [cword]  # initialisation
    for _next in syls[3:]:
        # relative threshold condition
        cond = (tps[prelast, last] > tps[last, syl]
                and tps[last, syl] < tps[syl, _next])

        if cond or last == "UB" or syl == "UB":
            cword = []
            cwords.append(cword)

        cword.append(syl)
        prelast = last
        last = syl
        syl = _next

    cwords[-1].append(syl)
    return cwords


def _absolute_threshold(syls, tps):
    """Absolute threshold segmentation method"""
    last = syls[0]
    last_word = [last]

    tp_mean = sum(tps.values()) / len(tps) if len(tps) != 0 else 0

    cwords = [last_word]
    for syl in syls[1:]:
        if tps[last, syl] <= tp_mean or last == "UB" or syl == "UB":
            last_word = []
            cwords.append(last_word)

        last_word.append(syl)
        last = syl

    return cwords


def segment(text, threshold='relative', probability='forward',
            log=utils.null_logger()):
    """Returns a word segmented version of `text` using the TP algorithm

    Parameters
    ----------
    text : sequence
        A sequence of lines with syllable (or phoneme) boundaries
        marked by spaces and no word boundaries. Each line in the
        sequence corresponds to a single and complete utterance.
    threshold : str, optional
        Type of threshold to use, must be 'relative' or 'absolute'.
    probability : str, optional
        Type of transition probabilities to compute, must be 'forward'
        or 'backward'.
    log : logging.Logger, optional
        The logging instance where to send messages.

    Returns
    -------
    list
        The utterances from `text` with estimated words boundaries.

    Raises
    ------
    ValueError
        If `threshold` is not 'relative' or 'absolute'. If
        `probability` is not 'forward' or 'backward'.

    """
    # raise on invalid threshold type
    if threshold != 'relative' and threshold != 'absolute':
        raise ValueError(
            "invalid threshold, must be 'relative' or 'absolute', it is '{}'"
            .format(threshold))

    # raise on invalid probability type
    if probability not in ('forward', 'backward'):
        raise ValueError(
            "invalid probability type, must be 'forward' or 'backward',"
            "it is {}".format(probability))

    log.info('running TP with %s threshold and %s probabilities',
             threshold, probability)

    # join all the utterances together, seperated by ' UB '
    syls = [syl for syl in ' UB '.join(line.strip() for line in text).split()]

    # dictionary of bigram and its forward or backward transition
    # probability
    tps = dict(
        (bigram, float(freq) / collections.Counter(syls)[
            bigram[0 if probability == 'forward' else 1]])
        for bigram, freq in collections.Counter(
                zip(syls[0:-1], syls[1:])).items())

    # segment the input given the transition probalities
    cwords = (_threshold_relative(syls, tps) if threshold == 'relative'
              else _absolute_threshold(syls, tps))

    # format the segment text for output (' UB ' -> '\n', remove
    # multiple spaces)
    segtext = ' '.join(''.join(c) for c in cwords)
    return re.sub(' +', ' ', segtext).split(' UB ')


def add_arguments(parser):
    """Add algorithm specific options to the parser"""
    parser.add_argument(
        '-t', '--threshold', type=str,
        choices=['relative', 'absolute'], default='relative',
        help='''Use a relative or absolute threshold for boundary decisions on
        transition probabilities. When absolute, the threshold is set
        to the mean transition probability over the entire text.''')

    parser.add_argument(
        '-P', '--probability', type=str,
        choices=['forward', 'backward'], default='forward',
        help='''Compute forward or backward transition probabilities''')


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-tp' command"""
    # command initialization
    streamin, streamout, separator, log, args = utils.prepare_main(
        name='wordseg-tp',
        description=__doc__,
        add_arguments=add_arguments)

    # segment the input text
    text = segment(
        streamin,
        threshold=args.threshold,
        probability=args.probability,
        log=log)

    # output the result
    streamout.write('\n'.join(text) + '\n')


if __name__ == '__main__':
    main()
