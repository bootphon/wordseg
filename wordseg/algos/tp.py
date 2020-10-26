"""Transitional Probabilities word segmentation"""

# Author: Amanda Saksida, Mathieu Bernard, Manel Khentout

import collections
import math
import re
import os
import codecs
from wordseg import utils

#test
from collections import defaultdict


def _threshold_relative(units, tps):
    """Relative threshold segmentation method"""
    prelast = units[0]
    last = units[1]
    unit = units[2]

    cword = [prelast, last]
    cwords = [cword]  # initialisation
    for _next in units[3:]:
        # relative threshold condition
        cond = (tps[prelast, last] > tps[last, unit]
                and tps[last, unit] < tps[unit, _next])

        if cond or last == 'UB' or unit == 'UB':
            cword = []
            cwords.append(cword)

        cword.append(unit)
        prelast = last
        last = unit
        unit = _next

    cwords[-1].append(unit)
    return cwords


def _threshold_absolute(units, tps):
    """Absolute threshold segmentation method"""
    last = units[0]
    last_word = [last]

    tp_mean = sum(tps.values()) / len(tps) if len(tps) != 0 else 0

    cwords = [last_word]
    for unit in units[1:]:
        if tps[last, unit] <= tp_mean or last == 'UB' or unit == 'UB':
            last_word = []
            cwords.append(last_word)

        last_word.append(unit)
        last = unit

    return cwords


# -----------------------------------------------------------------------------
#  Training
# -----------------------------------------------------------------------------

def _train(train_units, dependency):
    # compute and count all the unigrams and bigrams (two successive units)
    unigrams = collections.Counter(train_units)
    bigrams = collections.Counter(zip(train_units[0:-1], train_units[1:]))
    #test
    tps = collections.defaultdict(lambda : 0)
    # compute the transitional probabilities accordoing to the given
    # dependency measure
    
    if dependency == 'ftp':
        tps = {bigram: float(freq) / unigrams[bigram[0]]
           for bigram, freq in bigrams.items()}
    elif dependency == 'btp':
        tps = {bigram: float(freq) / unigrams[bigram[1]]
           for bigram, freq in bigrams.items()}
    else:  # dependency == 'mi'
        tps = {bigram: math.log(float(freq) / (
            unigrams[bigram[0]] * unigrams[bigram[1]]), 2)
               for bigram, freq in bigrams.items()}
    return tps
    
    
    


# -----------------------------------------------------------------------------
#  Segmentation
# -----------------------------------------------------------------------------

def _segment(units, tps, threshold):
    # segment the input given the transition probalities
    cwords = (_threshold_relative(units, tps) if threshold == 'relative'
              else _threshold_absolute(units, tps))
    segtext = ' '.join(''.join(c) for c in cwords)
    return [utt.strip() for utt in re.sub(' +', ' ', segtext).split('UB')]


# -----------------------------------------------------------------------------
#  Segment function
# -----------------------------------------------------------------------------

def segment(text, train_text=None, threshold='relative', dependency='ftp',
            log=utils.null_logger()):
    """Returns a word segmented version of `text` using the TP algorithm

    The parameters `text` and `train_text` must be formatted as follows: A
        sequence of lines with syllable (or phoneme) boundaries marked by
        spaces and no word boundaries. Each line in the sequence corresponds to
        a single and complete utterance

    Parameters
    ----------
    text : sequence
        The text to segment into words
    train_text : sequence, optional
        The text used to train model on (estimation of transition
        probabilities). If not specified use the `text`.
    threshold : str, optional
        Type of threshold to use, must be 'relative' or 'absolute'.
    dependency : str, optional
        Type of dependency measure to compute, must be 'ftp' for
        forward transitional probability, 'btp' for backward
        transitional probability or 'mi' for mutual information.
    log : logging.Logger, optional
        The logging instance where to send messages.

    Returns
    -------
    list
        The utterances from `text` with estimated words boundaries.

    Raises
    ------
    ValueError
        If `threshold` is not 'relative' or 'absolute'.
        If `dependency` is not 'ftp', 'btp' or 'mi'.

    """
    # raise on invalid threshold type
    if threshold not in ('relative', 'absolute'):
        raise ValueError(
            "invalid threshold, must be 'relative' or 'absolute', it is '{}'"
            .format(threshold))

    # raise on invalid probability type
    if dependency not in ('ftp', 'btp', 'mi'):
        raise ValueError(
            "invalid dependency measure, must be 'ftp', 'btp' "
            "or 'mi', it is {}".format(dependency))

    log.info('running TP with %s threshold and %s dependency measure',
             threshold, dependency)

    # calculate test_unit and train_unit
    test_units = ' UB '.join(line.strip() for line in text).split()

    if train_text is None:
        train_units = test_units
    else:
        train_units = ' UB '.join(line.strip() for line in train_text).split()

    # estimate the transition probabilities
    tps = _train(train_units, dependency)
    # segment the text using those TPs
    return _segment(test_units, tps, threshold)


# -----------------------------------------------------------------------------
#  Command line arguments
# -----------------------------------------------------------------------------

def _add_arguments(parser):
    """Add algorithm specific options to the parser"""
    group = parser.add_argument_group('algorithm parameters')
    group.add_argument(
        '-t', '--threshold', type=str,
        choices=['relative', 'absolute'], default='relative',
        help='''Use a relative or absolute threshold for boundary decisions on
        transition probabilities. When absolute, the threshold is set
        to the mean transition probability over the entire text.
        Default is relative.''')

    group1 = group.add_mutually_exclusive_group()
    group1.add_argument(
        '-d', '--dependency', type=str,
        choices=['ftp', 'btp', 'mi'], default='ftp',
        help='''Dependency measure to use. ftp is forward transitional probability:
        ftp(XY) = freq(XY) / freq(X), btp is backward transitional
        probability: ftp(XY) = freq(XY) / freq(Y), mi is mutual
        information: mi(XY) = log2( freq(XY) / (freq(X) * freq(Y))).
        ''')

    group1.add_argument(
        '-p', '--probability', type=str, choices=['forward', 'backward'],
        help='''DEPRECATED, USE -d/--dependency INSTEAD. Compute forward or
        backward transitional probabilities. Equivalent to -d ftp / -d
        btp respectively.''')


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-tp' command"""
    # command initialization

    streamin, streamout, _, log, args = utils.prepare_main(
        name='wordseg-tp',
        description=__doc__,
        add_arguments=_add_arguments,
        train_file=True)

    # if the deprecated --probability option is used, raise a warning
    # and convert it to the new --dependency option.
    if args.probability is not None:
        log.warning(
            '''-p/--probability option is deprecated (maintained for
            backward compatibility), please use -d/--dependency instead.''')

        if args.probability == 'forward':
            args.dependency = 'ftp'
        else:  # 'backward'
            args.dependency = 'btp'

    # load the train text if any
    train_text = None
    if args.train_file:
        if not os.path.isfile(args.train_file):
            raise RuntimeError(
                'test file not found: {}'.format(args.train_file))
        train_text = codecs.open(args.train_file, 'r', encoding='utf8')

    # load train and test texts, ignore empty lines
    test_text = (line for line in streamin if line)
    if train_text:
        train_text = (line for line in train_text if line)

    # segment the input text with the train text
    text = segment(
        test_text,
        train_text = train_text,
        threshold = args.threshold,
        dependency = args.dependency,
        log  =log)

    # output the result
    streamout.write('\n'.join(text) + '\n')


if __name__ == '__main__':
    main()
