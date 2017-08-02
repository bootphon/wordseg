#!/usr/bin/env python
#
# Copyright 2015-2017 Mathieu Bernard
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

"""Bayesian word segmentation algorithm.

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
import pkg_resources
import re
import shlex
import subprocess
import tempfile
import threading

from wordseg import utils, folding


class UnicodeGenerator(object):
    """Iterate on unicode characters starting at code `start`

    Exclude the space characters. This class is a perl to python
    simplified transcription of the original script
    create-unicode-dict-flexible.pl

    """
    def __init__(self, start=3001):
        self.index = start

        # A little tweak for python 2/3 compatibility
        try:
            self._chr = unichr  # python2
        except NameError:
            self._chr = chr  # python 3

    def __call__(self):
        char = self._chr(self.index)
        while re.match('\s', char):
            self.index += 1
            char = self._chr(self.index)
        self.index += 1
        return char


def _dpseg(text, args, log_level=logging.ERROR):
    log = utils.get_logger(name='wordseg-dpseg', level=log_level)

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
            args='--ngram 1 --a1 0 --b1 1', log=utils.null_logger()):
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
    unicode_text = [''.join(unicode_mapping[unit] for unit in utt.split()) for utt in text]

    log.debug('building %s folds', nfolds)
    folded_texts, fold_index = folding.fold(unicode_text, nfolds)

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
    segmented_text = (''.join(unit_mapping[char] for char in utt) for utt in output_text)

    return (utt for utt in segmented_text if utt)


def add_arguments(parser):
    """Add algorithm specific options to the parser"""
    parser.add_argument(
        '-f', '--nfolds', type=int, metavar='<int>', default=5,
        help='number of folds to segment the text on, default is %(default)s')

    parser.add_argument(
        '-j', '--njobs', type=int, metavar='<int>', default=1,
        help='number of parallel jobs to use, default is %(default)s')

    # a list of dpseg options we don't want to expose in wordseg-dpseg
    excluded = ['--help', '--data-file', '--data-start-index',
                '--data-num-sents', '--eval-start-index',
                '--eval-num-sents', '--output-file', '--nsubjects']

    group = parser.add_argument_group('algorithm options')
    for arg in utils.yield_binary_arguments(utils.get_binary('dpseg'), excluded=excluded):
        if arg.name == '--config-file':
            arg.help += ', for example configuration files see {}'.format(
                os.path.dirname(utils.get_config_files('dpseg')[0]))
        arg.add(group)


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-dpseg' command"""
    streamin, streamout, _, log, args = utils.prepare_main(
        name='wordseg-dpseg',
        description=__doc__,
        add_arguments=add_arguments)

    assert args.nfolds > 0

    ignored_args = ['verbose', 'quiet', 'input', 'output', 'nfolds', 'njobs']
    _args = {k: v for k, v in vars(args).items() if k not in ignored_args and v}
    _args = ' '.join('--{} {}'.format(k, v) for k, v in _args.items()).replace('_', '-')

    segmented = segment(
        streamin, nfolds=args.nfolds, njobs=args.njobs, args=_args, log=log)
    streamout.write('\n'.join(segmented) + '\n')


if __name__ == '__main__':
    main()
