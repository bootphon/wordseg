"""Bayesian word segmentation algorithm

See Goldwater, Griffiths, Johnson (2010) and Phillips & Pearl (2014).

1. Uses a hierarchical Pitman-Yor process rather than a hierarchical
   Dirichlet process model. The HDP model can be recovered by setting
   the PY parameters appropriately (set --a1 and --a2 to 0, --b1 and
   --b2 then correspond to the HDP parameters).

2. Implements several different estimation procedures, including the
   original Gibbs sampler (*flip sampler*) as well as a sentence-based
   Gibbs sampler that uses dynamic programming (*tree sampler*) and a
   similar dynamic programming algorithm that chooses the best
   segmentation of each utterance rather than a sample. The latter
   two algorithms can be run either in batch mode or in online mode.
   If in online mode, they can also be set to "forget" parts of the
   previously analysis. This is described in more detail below.

3. Functionality for using separate training and testing files.  If
   you provide an evaluation file, the program will first run through
   its full training procedure (i.e., using whichever algorithm for
   however many iterations, kneeling, etc.). After that, it will
   freeze the lexicon in whatever state it is in and then make a
   single pass through the evaluation data, segmenting each sentence
   according to the probabilities computed from the frozen lexicon.
   No new words/counts will be added to the lexicon during evaluation.
   Evaluation can be set to either sample segmentations or choose the
   maximum probability segmentation for each utterance. Scores will
   be printed out at the end of the complete run based on either the
   evaluation data (if provided) or the training data (if not).

"""

import joblib
import logging
import os
import re
import shlex
import six
import subprocess
import tempfile
import threading

from wordseg import utils, folding


# # a list of dpseg options we don't want to expose in wordseg-dpseg
# ['--help', '--data-file', '--data-start-index', '--data-num-sents',
#  '--eval-start-index', '--eval-num-sents', '--output-file']
DPSEG_ARGUMENTS = [
    utils.Argument(
        short_name='-d', name='--debug-level', type=int, default=0,
        help='debugging level'),

    utils.Argument(
        short_name='-e', name='--eval-file', type='file',
        help=('testing data file. If no eval-file is listed, evaluation will '
              'be on the training file. Note that listing the same file for '
              'both training and testing has different functionality than '
              'not listing a test file, due to the way that the test file '
              'is segmented.')),

    utils.Argument(
        name='--eval-maximize', type=bool,
        help=('choose max probability segmentation of test sentences, '
              'default is sample')),

    utils.Argument(
        name='--eval-interval', type=int,
        help=('how many iterations are run before the test set is evaluated, '
              '0 (default) means to only evaluate the test set after all'
              'iterations are complete')),

    utils.Argument(
        short_name='-E', name='--estimator',
        type=['viterbi', 'flip', 'tree', 'decayed-flip'],
        help=('Viterbi does dynamic programming maximization, '
              'Tree does dynamic programming sampling, '
              'Flip does original Gibbs sampler (default)')),

    utils.Argument(
        short_name='-D', name='--decay-rate', type=float,
        help='decay rate for decayed-flip estimator, default = 1.0'),

    utils.Argument(
        short_name='-S', name='--samples-per-utt', type=int,
        help='samples per utterance for decayed-flip estimator, '
        'default is 1000'),

    utils.Argument(
        short_name='-m', name='--mode', type=['batch', 'online'],
        help='default is batch'),

    utils.Argument(
        short_name='-n', name='--ngram', type=['unigram', 'bigram'],
        help='default is bigram'),

    utils.Argument(
        name='--do-mbdp', type=bool,
        help='maximize using Brent ngram instead of DP'),

    utils.Argument(
        name='--a1', type=float,
        help='unigram Pitman-Yor a parameter, default = 0.0'),

    utils.Argument(
        name='--b1', type=float,
        help='unigram Pitman-Yor b parameter, default = 1.0'),

    utils.Argument(
        name='--a2', type=float,
        help='bigram Pitman-Yor a parameter, default = 0.0'),

    utils.Argument(
        name='--b2', type=float,
        help='bigram Pitman-Yor a parameter, default = 1.0'),

    utils.Argument(
        short_name='-p', name='--Pstop', type=float,
        help='monkey model stop probability, default = 0.5'),

    utils.Argument(
        short_name='-H', name='--hypersamp-ratio', type=float,
        help='standard deviation for new hyperparmeters proposals, '
        'default = 0.1'),

    utils.Argument(
        name='--nchartypes', type=int,
        help=('number of characters assumed in P_0, '
              '0 will compute from input, default = 0')),

    utils.Argument(
        name='--aeos', type=float,
        help='beta prior on end of sentence prob, default is 2'),

    utils.Argument(
        short_name='-b', name='--init-pboundary', type=float,
        help='initial segmentation boundary probability (-1 = gold, '
        'default = 0)'),

    utils.Argument(
        name='--pya-beta-a', type=float,
        help='if non-zero, a parameter of Beta prior on pya, default = 1.0'),

    utils.Argument(
        name='--pya-beta-b', type=float, default=1.0,
        help='if non-zero, b parameter of Beta prior on pya, default = 1.0'),

    utils.Argument(
        name='--pya-gamma-s', type=float, default=10.0,
        help='if non-zero, parameter of Gamma prior on pyb, default = 10.0'),

    utils.Argument(
        name='--pya-gamma-c', type=float, default=0.1,
        help='if non-zero, parameter of Gamma prior on pyb, default = 0.1'),

    utils.Argument(
        name='--trace-every', type=int, default=0,
        help=('epochs between printing out trace information '
              '(0 = don\'t trace), default = 0')),

    utils.Argument(
        short_name='-s', name='--nsubjects', type=int,
        help='number of subjects to simulate, default = 1'),

    utils.Argument(
        short_name='-F', name='--forget-rate', type=int,
        help='number of utterances whose words can be remembered, '
        'default = 1'),

    utils.Argument(
        short_name='-i', name='--burnin-iterations', type=int,
        help=('number of burn-in epochs. This is actually the total '
              'number of iterations through the training data, '
              'default = 2000')),

    utils.Argument(
        name='--anneal-iterations', type=int,
        help=('number of epochs to anneal for. So e.g. burn-in = 100 '
              'and anneal = 90 would leave 10 iters at the end at '
              'the final annealing temp, default = 0')),

    utils.Argument(
        name='--anneal-start-temperature', type=float,
        help='start annealing at this temperature, default = 1'),

    utils.Argument(
        name='--anneal-stop-temperature', type=float,
        help='stop annealing at this temperature, default = 1'),

    utils.Argument(
        name='--anneal-a', type=float,
        help=('parameter in annealing temperature sigmoid function, '
              '(0 = use ACL06 schedule, deafult = 0')),

    utils.Argument(
        name='--anneal-b', type=float,
        help='parameter in annealing temperature sigmoid function, '
        'default = 0.2'),

    utils.Argument(
        name='--result-field-separator', type=str,
        help='Field separator used to print results, default is "\t"'),

    utils.Argument(
        name='--forget-method', type=['uniformly', 'proportional'],
        help='method for deleting lexical items, default is uniformly'),

    utils.Argument(
        short_name='-N', name='--token-memory', type=int,
        help='number of tokens that can be remembered, default = 0'),

    utils.Argument(
        short_name='-L', name='--type-memory', type=int,
        help='number of types that can be remembered, default = 0')
]


class UnicodeGenerator(object):
    """Iterates on unicode characters

    Excludes the space characters. Used to build a (unit -> char)
    mapping. The actual dpseg implementation requires that all units
    (phones or syllables) are encoded as a unicode char.

    Parameters
    ----------
    start : int
        The first unicode character to be generated

    Notes
    -----
    This class is a perl to python simplified transcription of the
    original script create-unicode-dict-flexible.pl

    Examples
    --------
    This shows a basic usage mapping a list of strings to unicode.

    >>> units = ['unit1', 'unit2', 'unit3']
    >>> unicode_gen = UnicodeGenerator()
    >>> unicode_mapping = {unit: unicode_gen() for unit in units}

    """
    def __init__(self, start=3001):
        self.index = start

    def __call__(self):
        """Returns the next unicode character"""
        char = six.unichr(self.index)
        while re.match('\s', char):
            self.index += 1
            char = six.unichr(self.index)
        self.index += 1
        return char


def _dpseg(text, args, log_level=logging.ERROR, log_name='wordseg-dpseg'):
    log = utils.get_logger(name=log_name, level=log_level)

    with tempfile.NamedTemporaryFile() as tmp_output:
        command = '{binary} --output-file {output} {args}'.format(
            binary=utils.get_binary('dpseg'),
            output=tmp_output.name,
            args=args)

        log.debug('running "%s"', command)

        process = subprocess.Popen(
            shlex.split(command), stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        def writer():
            for utt in text:
                process.stdin.write((utt.strip() + '\n').encode('utf8'))
            process.stdin.close()

        thread = threading.Thread(target=writer)
        thread.start()

        # Send stdout and stderr to logger, break if EOF reached
        while True:
            line_out = process.stdout.readline().decode('utf8')
            line_err = process.stderr.readline().decode('utf8')

            if line_out == "" and line_err == "":
                break

            if line_out != "":
                log.debug(line_out.strip())

            if line_err != "":
                log.error(line_err.strip())

        thread.join()
        process.wait()
        if process.returncode:
            raise RuntimeError(
                'failed with error code {}'.format(process.returncode))

        tmp_output.seek(0)
        return tmp_output.read().decode('utf8').split('\n')


def segment(text, nfolds=5, njobs=1,
            args='--ngram 1 --a1 0 --b1 1',
            log=utils.null_logger()):
    """Run the 'dpseg' binary on `nfolds` folds"""
    # force the text to be a list of utterances
    text = list(text)

    # set of unique units (syllables or phones) present in the text
    units = set(unit for utt in text for unit in utt.split())
    log.info('%s units found in %s utterances', len(units), len(text))

    # create a unicode equivalent for each unit and convert the text
    # to that unicode version
    log.debug('converting input to unicode')
    unicode_gen = UnicodeGenerator()
    unicode_mapping = {unit: unicode_gen() for unit in units}
    unicode_text = [''.join(unicode_mapping[unit] for unit in utt.split())
                    for utt in text]

    log.debug('building %s folds', nfolds)
    fold_boundaries = _dpseg_bugfix(
        unicode_text, folding.boundaries(unicode_text, nfolds), log)

    folded_texts, fold_index = folding.fold(
        unicode_text, nfolds, fold_boundaries=fold_boundaries)

    segmented_texts = joblib.Parallel(n_jobs=njobs, verbose=0)(
        joblib.delayed(_dpseg)(
            fold, args, log_level=log.getEffectiveLevel(),
            log_name='wordseg-dpseg - fold {}'.format(n+1))
        for n, fold in enumerate(folded_texts))

    log.debug('unfolding the %s folds', nfolds)
    output_text = folding.unfold(segmented_texts, fold_index)

    # convert the text back to unit level (from unicode level)
    log.debug('converting output back from unicode')
    unit_mapping = {v: k for k, v in unicode_mapping.items()}
    unit_mapping[' '] = ' '
    segmented_text = (
        ''.join(unit_mapping[char] for char in utt) for utt in output_text)

    return (utt for utt in segmented_text if utt)


def _find_first_line_with_min_len(lines, min_len=2):
    """Return the index of first line in `lines` with len >= `min_len`

    Or return None when no line found.

    """
    for i, l in enumerate(lines):
        if len(l) >= min_len:
            return i
    return None


def _dpseg_bugfix(text, boundaries, log=utils.null_logger()):
    """Ensure all folds have their first line with more than one symbol

    There is a bug in the C++ implementation of dpseg: when the first
    input line is composed of a single character (i.e. a unicode
    symbol), the program fails. To avoid this, this method ensures all
    the folds begin with at least two symbols. If this is not the
    case, the boundary position is moved to the next line containing
    at least two symbols.

    Raises
    ------
    ValueError if the bugfix is needed and cannot be applied

    Notes
    -----
    This implementation differs with the one in CDSWordSeg. In wordseg
    we modify the fold index and thus the fold size, whereas in
    CDSWordSeg we permute lines in the text (and thus to the gold
    file). In wordseg we don't want to expose the gold file at this
    level.

    """
    # we have something to fix if one of those lengths is 1
    first_len = [len(text[i]) for i in boundaries]
    if 1 not in first_len:
        log.debug('folds boundaries are OK for dpseg')
        return boundaries

    if first_len[0] == 1:
        raise ValueError(
            'The input text\'s first line has a single symbol, '
            'this will cause wordseg-dpseg to bug. '
            'Please re-arrange your text manually and try again.')

    need_to_fix = [i for i, length in enumerate(first_len) if length == 1]
    log.debug(
        'dpseg bugfix: need to fix folds {}'
        .format([i+1 for i in need_to_fix]))

    for i in need_to_fix:
        # find the first line of the fold with len >= 2
        index = _find_first_line_with_min_len(
            text[boundaries[i]:], min_len=2)

        if index is None:
            raise ValueError(
                'dpseg bugfix failed: all lines in the fold {} have len == 1'
                .format(i+1))

        log.debug(
            'dpseg bugfix: fixing fold {} index from {} to {}'
            .format(i+1, boundaries[i], boundaries[i] + index))
        boundaries[i] += index

    if not boundaries == sorted(set(boundaries)):
        raise ValueError(
            'dpseg bugfix failed: broke the folds order. '
            'Please re-arrange (shuffle) your text manually and try again.')

    return boundaries


def _add_arguments(parser):
    """Add algorithm specific options to the parser"""
    parser.add_argument(
        '-f', '--nfolds', type=int, metavar='<int>', default=5,
        help='number of folds to segment the text on, default is %(default)s')

    parser.add_argument(
        '-j', '--njobs', type=int, metavar='<int>', default=1,
        help='number of parallel jobs to use, default is %(default)s')

    parser.add_argument(
        '-r', '--randseed', type=int, metavar='<int>',
        help=('seed for random numbers generation, '
              'default is based on current time')),

    parser.add_argument(
        '-c', '--config-file', metavar='<file>',
        help=('configuration file to read the algorithm parameters from, '
              'for example configuration files see {}'.format(
                  os.path.dirname(utils.get_config_files('dpseg')[0]))))

    group = parser.add_argument_group('algorithm options')
    for arg in DPSEG_ARGUMENTS:
        arg.add_to_parser(group)


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-dpseg' command"""
    streamin, streamout, _, log, args = utils.prepare_main(
        name='wordseg-dpseg',
        description=__doc__,
        add_arguments=_add_arguments)

    assert args.nfolds > 0

    # build the dpseg command arguments
    dpseg_args = {}
    excluded_args = ['verbose', 'quiet', 'input', 'output', 'nfolds', 'njobs']
    for k, v in vars(args).items():
        # ignored arguments
        if k in excluded_args or (v in (None, False) and v != 0):
            continue

        if k == 'estimator':
            v = {'viterbi': 'V', 'flip': 'F',
                 'decayed-flip': 'D', 'tree': 'T'}[v]

        if k == 'ngram':
            v = {'unigram': 1, 'bigram': 2}[v]

        if k == 'forget_method':
            v = {'uniformly': 'U', 'proportional': 'P'}[v]

        dpseg_args[k.replace('_', '-')] = v

    dpseg_args = ' '.join('--{} {}'.format(k, v)
                          for k, v in dpseg_args.items())

    # adapt boolean values
    for orig, new in (
            ('--eval-maximize True', '--eval-maximize 1'),
            ('--eval-maximize False', ''),
            ('--do-mbdp True', '--do-mbdp 1'),
            ('--do-mbdp False', '')):
        dpseg_args = dpseg_args.replace(orig, new)

    segmented = segment(
        streamin, nfolds=args.nfolds, njobs=args.njobs,
        args=dpseg_args, log=log)
    streamout.write('\n'.join(segmented) + '\n')


if __name__ == '__main__':
    main()
