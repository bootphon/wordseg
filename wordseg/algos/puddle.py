"""Puddle word segmentation algorithm

Implementation of the puddle philosophy developped by P. Monaghan.

See "Monaghan, P., & Christiansen, M. H. (2010). Words in puddles of sound:
modelling psycholinguistic effects in speech segmentation. Journal of child
language, 37(03), 545-564."

"""

import codecs
import collections
import os

import joblib

from wordseg import utils, folding


class Puddle:
    """Train and segmenttext with a PUDDLE modelling

    Implementation of a PUDDLE model with `train()` and `segment()` methods.

    Parameters
    ----------
    window : int, optional
        Number of phonemes to be taken into account for boundary constraint.
        Default to 2.
    by_frequency : bool, optional
        When True choose the word candidates by filterring them by frequency.
        Default to False.
    log : logging.Logger, optional
        The logger instance where to send messages.

    """
    def __init__(self, window=2, by_frequency=False, log=utils.null_logger()):
        self._log = log
        self.window = window
        self.by_frequency = by_frequency

        self._lexicon = collections.Counter()
        self._beginning = collections.Counter()
        self._ending = collections.Counter()

    def __eq__(self, other):
        return (
            self.window == other.window and
            self.by_frequency == other.by_frequency and
            self._lexicon == other._lexicon and
            self._beginning == other._beginning and
            self._ending == other._ending)

    def train(self, text):
        """Train a PUDDLE model from `text`

        `text` must be a sequence of strings, each one considered as an
        utterance.

        """
        for utterance in text:
            self._process_utterance(
                utterance.strip().split(),
                segmented=[],
                do_update=True)

    def segment(self, text, update_model=True):
        """Segments a `text` using the trained PUDDLE model

        `text` must be a sequence of strings, each one considered as an
        utterance.

        If `update_model` is True, the model is trained online during
        segmentation. Otherwise it stays constant.

        Yields the segmented utterances.

        """
        for utterance in text:
            yield ' '.join(
                self._process_utterance(
                    utterance.strip().split(),
                    segmented=[], do_update=update_model))

    def _filter_by_frequency(self, utterance, i, j):
        all_candidates = []
        for k in range(j, len(utterance)):
            try:
                all_candidates.append(
                    (k, self._lexicon[''.join(utterance[i:k+1])]))
            except KeyError:
                pass
        j, _ = sorted(all_candidates, key=lambda x: x[1])[-1]
        return j

    def _filter_by_boundary_condition(self, utterance, i, j):
        # previous must be word-end
        prev_biphone = ''.join(utterance[i - self.window:i])
        if i != 0 and prev_biphone not in self._ending:
            return False

        next_biphone = ''.join(utterance[j + 1:j + 1 + self.window])
        if len(utterance) != j - i and next_biphone not in self._beginning:
            return False

        return True

    def _update_candidate(self, segmented, utterance, i, j):
        self._lexicon.update([''.join(utterance[i:j+1])])
        segmented += [''.join(utterance[i:j+1])]

        if len(utterance[i:j+1]) == len(utterance):
            self._log.debug(
                'utterance %s added in lexicon', ''.join(utterance[i:j+1]))
        else:
            self._log.debug(
                'match %s added in lexicon', ''.join(utterance[i:j+1]))

        if len(utterance[i:j+1]) >= 2:
            self._beginning.update([''.join(utterance[i:i+self.window])])
            self._ending.update([''.join(utterance[j+1-self.window:j+1])])

            self._log.debug(
                'biphones %s added in beginning',
                ''.join(utterance[i:i+self.window]))
            self._log.debug(
                'biphones %s added in ending',
                ''.join(utterance[j+1-self.window:j+1]))

        return segmented

    @staticmethod
    def _segment_candidate(segmented, utterance, i, j):
        return segmented.append(''.join(utterance[i:j+1]))

    def _process_utterance(self, utterance, segmented, do_update):
        """Recursive function implementing puddle

        Parameters
        ----------
        utterance : list
            A non-empty list of phonological symbols (phones or syllables)
            corresponding to an utterance.
        segmented : list
            Recursively build lexicon of pseudo words.
        do_update : bool
            When True, update the model while segmenting

        Raises
        ------
        ValueError
            If `utterance` is empty.

        """
        if not utterance:
            raise ValueError('The utterance is empty')

        # select the right method for segmenting pseudo words with respect to
        # `do_update`
        process_candidate = (
            self._update_candidate if do_update else
            self._segment_candidate)

        found = False

        # index of start of word candidate
        i = 0
        while i < len(utterance):
            j = i
            while j < len(utterance):
                candidate_word = ''.join(utterance[i:j+1])
                # self._log.debug('word candidate: %s', candidate_word)

                if candidate_word in self._lexicon:
                    if self.by_frequency:
                        # choose the best candidate by looking at the
                        # frequency of different candidates
                        j = self._filter_by_frequency(utterance, i, j)

                    # check if the boundary conditions are respected
                    found = self._filter_by_boundary_condition(utterance, i, j)

                    if found:
                        self._log.info('match found : %s', candidate_word)
                        if i != 0:
                            # add the word preceding the word found in
                            # lexicon; update beginning and ending
                            # counters and segment
                            segmented = process_candidate(
                                segmented, utterance, 0, i-1)

                        # update the lexicon, beginning and ending counters
                        segmented = process_candidate(
                            segmented, utterance, i, j)

                        if j != len(utterance) - 1:
                            # recursion
                            return self._process_utterance(
                                utterance[j+1:],
                                segmented=segmented,
                                do_update=do_update)

                        # go to the next chunk and apply the same condition
                        self._log.info(
                            'go to next chunk : %s', utterance[j+1:])
                        break

                j += 1
            i += 1  # or go to the next phoneme

        if not found:
            process_candidate(segmented, utterance, 0, len(utterance) - 1)

        return segmented


def _do_puddle(text, window, by_frequency, log_level, log_name):
    """Auxiliary function to segment"""
    model = Puddle(
        window=window,
        by_frequency=by_frequency,
        log=utils.get_logger(name=log_name, level=log_level))

    return list(model.segment(text, update_model=True))


def segment(text, train_text=None, window=2, by_frequency=False, nfolds=5,
            njobs=1, log=utils.null_logger()):
    """Returns a word segmented version of `text` using the puddle algorithm

    Parameters
    ----------
    text : sequence of str
        A sequence of lines with syllable (or phoneme) boundaries
        marked by spaces and no word boundaries. Each line in the
        sequence corresponds to a single and complete utterance.
    train_text : sequence of str
        The list of utterances to train the model on. If None (default) the
        model is trained online during segmentation. When `train_text` is
        specified, the options `nfolds` and `njobs` are ignored.
    window : int, optional
        Number of phonemes to be taken into account for boundary constraint.
        Default to 2.
    by_frequency : bool, optional
        When True choose the word candidates by filterring them by frequency.
        Default to False.
    nfolds : int, optional
        The number of folds to segment the `text` on. This option is ignored if
        a `train_text` is provided.
    njobs : int, optional
        The number of subprocesses to run in parallel. The folds are
        independant of each others and can be computed in parallel. Requesting
        a number of jobs greater then `nfolds` have no effect. This option is
        ignored if a `train_text` is provided.
    log : logging.Logger, optional
        The logger instance where to send messages.

    Returns
    -------
    generator
        The utterances from `text` with estimated words boundaries.

    See also
    --------
    wordseg.folding.fold

    """
    # force the text to be a list of utterances
    text = list(text)

    if not train_text:
        log.info('not train data provided, will train model on test data')

        log.debug('building %s folds', nfolds)
        folded_texts, fold_index = folding.fold(text, nfolds)

        # segment the folds in parallel
        segmented_texts = joblib.Parallel(n_jobs=njobs, verbose=0)(
            joblib.delayed(_do_puddle)(
                fold, window, by_frequency,
                log.getEffectiveLevel(),
                f'wordseg-puddle - fold {n+1}')
            for n, fold in enumerate(folded_texts))

        log.debug('unfolding the %s folds', nfolds)
        output_text = folding.unfold(segmented_texts, fold_index)

        return (utt for utt in output_text if utt)

    # force the train text from sequence to list
    train_text = list(train_text)
    log.info('train data: %s utterances loaded', len(train_text))

    # init a puddle model and train it
    model = Puddle(window=window, by_frequency=by_frequency, log=log)
    model.train(train_text)

    # segmentation of the test text, keeping the model constant
    return (utt for utt in model.segment(text, update_model=False) if utt)


def _add_arguments(parser):
    """Add algorithm specific options to the parser"""
    parser.add_argument(
        '-f', '--nfolds', type=int, metavar='<int>', default=5,
        help='number of folds to segment the text on, default is %(default)s, '
        'ignored if <training-file> specified.')

    parser.add_argument(
        '-j', '--njobs', type=int, metavar='<int>', default=1,
        help='number of parallel jobs to use, default is %(default)s, '
        'ignored if <training-file> specified.')

    parser.add_argument(
        '-w', '--window', type=int, default=2, metavar='<int>', help='''
        Number of phonemes to be taken into account for boundary constraint,
        default is %(default)s.''')

    parser.add_argument(
        '-F', '--by-frequency', action='store_true',
        help='choose word candidates based on frequency '
        '(deactivated by default)')

    # parser.add_argument(
    #     '-d', '--decay', action='store_true',
    #     help='Decrease the size of lexicon, modelize memory of lexicon.')


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-puddle' command"""
    streamin, streamout, _, log, args = utils.prepare_main(
        name='wordseg-puddle',
        description=__doc__,
        add_arguments=_add_arguments,
        train_file=True)

    # load the train text if any
    train_text = None
    if args.train_file:
        if not os.path.isfile(args.train_file):
            raise RuntimeError(
                f'test file not found: {args.train_file}')
        train_text = codecs.open(args.train_file, 'r', encoding='utf8')

    # load train and test texts, ignore empty lines
    test_text = (line for line in streamin if line)
    if train_text:
        train_text = (line for line in train_text if line)

    segmented = segment(
        test_text, train_text=train_text,
        window=args.window, by_frequency=args.by_frequency,
        nfolds=args.nfolds, njobs=args.njobs, log=log)
    streamout.write('\n'.join(segmented) + '\n')


if __name__ == '__main__':
    main()
