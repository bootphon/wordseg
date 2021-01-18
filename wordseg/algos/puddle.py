"""Puddle word segmentation algorithm

Implementation of the puddle philosophy developped by P. Monaghan.

See "Monaghan, P., & Christiansen, M. H. (2010). Words in
puddles of sound: modelling psycholinguistic effects in speech
segmentation. Journal of child language, 37(03), 545-564."

"""

import collections
import codecs
import logging
import os

import joblib

from wordseg import utils, folding


class _Puddle(object):
    def __init__(self, window=2, by_frequency=False, log=utils.null_logger()):
        self.log = log
        self.window = window
        self.by_frequency = by_frequency
        self.lexicon = collections.Counter()
        self.beginning = collections.Counter()
        self.ending = collections.Counter()

    def __eq__(self, other):
        return (
            self.window == other.window and
            self.by_frequency == other.by_frequency and
            self.lexicon == other.lexicon and
            self.beginning == other.beginning and
            self.ending == other.ending)

    def filter_by_frequency(self, phonemes, i, j):
        all_candidates = []
        for k in range(j, len(phonemes)):
            try:
                all_candidates.append(
                    (k, self.lexicon[''.join(phonemes[i:k+1])]))
            except KeyError:
                pass
        j, _ = sorted(all_candidates, key=lambda x: x[1])[-1]
        return j

    def filter_by_boundary_condition(self, phonemes, i, j, found):
        if found:
            previous_biphone = ''.join(phonemes[i - self.window:i])
            # previous must be word-end
            if i != 0 and previous_biphone not in self.ending:
                return False

            following_biphone = ''.join(phonemes[j + 1:j + 1 + self.window])
            if (
                    len(phonemes) != j - i
                    and following_biphone not in self.beginning
            ):
                return False

            return True
        return None

    def update_counters(self, segmented, phonemes, i, j):
        self.lexicon.update([''.join(phonemes[i:j+1])])
        segmented.append(''.join(phonemes[i:j+1]))

        if len(phonemes[i:j+1]) == len(phonemes):
            self.log.debug(
                'utterance %s added in lexicon', ''.join(phonemes[i:j+1]))
        else:
            self.log.debug(
                'match %s added in lexicon', ''.join(phonemes[i:j+1]))

        if len(phonemes[i:j+1]) >= 2:
            w = self.window
            self.beginning.update([''.join(phonemes[i:i+w])])
            self.ending.update([''.join(phonemes[j+1-w:j+1])])

            self.log.debug(
                'biphones %s added in beginning', ''.join(phonemes[i:i+w]))
            self.log.debug(
                'biphones %s added in ending', ''.join(phonemes[j+1-w:j+1]))

        return segmented

    def update_utterance(self, utterance, segmented=[]):
        """Recursive function implementing puddle

        Parameters
        ----------
        utterance : list
            A non-empty list of phonological symbols (phones or syllables)
            corresponding to an utterance.
        segmented : list
            Recursively build lexicon of pseudo words.

        Raises
        ------
        ValueError
            If `phonemes` is empty.

        """
        # check if the utterance is empty
        if not utterance:
            raise ValueError('The utterance is empty')

        found = False

        # index of start of word candidate
        i = 0
        while i < len(utterance):
            j = i
            while j < len(utterance):
                candidate_word = ''.join(utterance[i:j+1])
                self.log.debug('word candidate: %s', candidate_word)

                if candidate_word in self.lexicon:
                    found = True

                    if self.by_frequency:
                        # choose the best candidate by looking at the
                        # frequency of different candidates
                        j = self.filter_by_frequency(utterance, i, j)

                    # check if the boundary conditions are respected
                    found = self.filter_by_boundary_condition(
                        utterance, i, j, found)

                    if found:
                        self.log.info('match found : %s', candidate_word)
                        if i != 0:
                            # add the word preceding the word found in
                            # lexicon; update beginning and ending
                            # counters and segment
                            segmented = self.update_counters(
                                segmented, utterance, 0, i-1)

                        # update the lexicon, beginning and ending counters
                        segmented = self.update_counters(
                            segmented, utterance, i, j)

                        if j != len(utterance) - 1:
                            # recursion
                            return self.update_utterance(
                                utterance[j+1:], segmented=segmented)

                        # go to the next chunk and apply the same condition
                        self.log.info(
                            'go to next chunk : %s ', utterance[j+1:])
                        break

                    else:
                        j += 1
                else:
                    j += 1
            i += 1  # or go to the next phoneme

        if not found:
            self.update_counters(segmented, utterance, 0, len(utterance) - 1)

        return segmented

    def segment_counters(self, segmented, phonemes, i, j):
        self.lexicon.update([''.join(phonemes[i:j+1])])
        segmented.append(''.join(phonemes[i:j+1]))

        if len(phonemes[i:j+1]) == len(phonemes):
            self.log.debug(
                'utterance %s added in lexicon', ''.join(phonemes[i:j+1]))
        else:
            self.log.debug(
                'match %s added in lexicon', ''.join(phonemes[i:j+1]))

        if len(phonemes[i:j+1]) >= 2:
            w = self.window

            self.log.debug(
                'biphones %s added in beginning', ''.join(phonemes[i:i+w]))
            self.log.debug(
                'biphones %s added in ending', ''.join(phonemes[j+1-w:j+1]))

        return segmented

    def segment_utterance(self, utterance, segmented=[]):
        """Recursive function implementing puddle

        Parameters
        ----------
        utterance : list
            A non-empty list of phonological symbols (phones or syllables)
            corresponding to an utterance.
        segmented : list
            Recursively build lexicon of pseudo words.

        Raises
        ------
        ValueError
            If `phonemes` is empty.

        """
        # check if the utterance is empty
        if not utterance:
            raise ValueError('The utterance is empty')

        found = False

        # index of start of word candidate
        i = 0
        while i < len(utterance):
            j = i
            while j < len(utterance):
                candidate_word = ''.join(utterance[i:j+1])
                self.log.debug('word candidate: %s', candidate_word)

                if candidate_word in self.lexicon:
                    found = True

                    if self.by_frequency:
                        # choose the best candidate by looking at the
                        # frequency of different candidates
                        j = self.filter_by_frequency(utterance, i, j)

                    # check if the boundary conditions are respected
                    found = self.filter_by_boundary_condition(
                        utterance, i, j, found)

                    if found:
                        self.log.info('match found : %s', candidate_word)
                        if i != 0:
                           
                            # counters and segment
                            segmented = self.segment_counters(
                                segmented, utterance, 0, i-1)

                        # seg
                        segmented = self.segment_counters(
                            segmented, utterance, i, j)

                        if j != len(utterance) - 1:
                            # recursion
                            return self.segment_utterance(
                                utterance[j+1:], segmented=segmented)

                        # go to the next chunk and apply the same condition
                        self.log.info(
                         'go to next chunk : %s ', utterance[j+1:])
                        break

                    else:
                        j += 1
                else:
                    j += 1
            i += 1  # or go to the next phoneme

        if not found:
            self.segment_counters(segmented, utterance, 0, len(utterance) - 1)

        return segmented

def _puddle_train(model, text):
    return [' '.join(model.update_utterance(
        line.strip().split(), segmented=[])) for line in text]


def _puddle_test(model, text):
    return [' '.join(model.segment_utterance(
        line.strip().split(), segmented=[])) for line in text]


def _puddle(text, window, by_frequency=False,
            log_level=logging.ERROR, log_name='wordseg-puddle'):
    """Runs the puddle algorithm on the `text`"""
    # create a new puddle segmenter (with an empty lexicon)
    model = _Puddle(
        window=window,
        by_frequency=by_frequency,
        log=utils.get_logger(name=log_name, level=log_level))

    return _puddle_train(model, text)


def segment(text, train_text=None, window=2, by_frequency=False, nfolds=5,
            njobs=1, log=utils.null_logger()):
    """Returns a word segmented version of `text` using the puddle algorithm
    Parameters
    ----------
    text : sequence
        A sequence of lines with syllable (or phoneme) boundaries
        marked by spaces and no word boundaries. Each line in the
        sequence corresponds to a single and comlete utterance.

    train_text: #TBD
        The list of utterances to train the model on. If None train the model
        directly on `text`.
        No need to enter nfolds and njobs when we have a train_text.

    window : int, optional
        Number of phonemes to be taken into account for boundary constraint.
    by_frequency : bool, optional
        When True choose the word candidates by filterring them by frequency
    nfolds : int, optional
        The number of folds to segment the `text` on.
    njobs : int, optional
        The number of subprocesses to run in parallel. The folds are
        independant of each others and can be computed in
        parallel. Requesting a number of jobs greater then `nfolds`
        have no effect.
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

    if train_text is None:
        log.info('not train data provided, will train model on test data')

        log.debug('building %s folds', nfolds)
        folded_texts, fold_index = folding.fold(text, nfolds)

        segmented_texts = joblib.Parallel(n_jobs=njobs, verbose=0)(
            joblib.delayed(_puddle)(
                fold, window, by_frequency=by_frequency,
                log_level=log.getEffectiveLevel(),
                log_name='wordseg-puddle - fold {}'.format(n+1))
            for n, fold in enumerate(folded_texts))

        log.debug('unfolding the %s folds', nfolds)
        output_text = folding.unfold(segmented_texts, fold_index)

        return (utt for utt in output_text if utt)

    else:
        # force the train text from sequence to list
        if not isinstance(train_text, list):
            train_text = list(train_text)
        nutts = len(train_text)
        log.info('train data: %s utterances loaded', nutts)

        # init a puddle model
        model = _Puddle(
            window=window,
            by_frequency=by_frequency,
            log=log)

        # train it from train_text
        _puddle_train(model, train_text)

        # segment test_text
        return (utt for utt in _puddle_test(model, text) if utt)


def _add_arguments(parser):
    """Add algorithm specific options to the parser"""
    parser.add_argument(
        '-f', '--nfolds', type=int, metavar='<int>', default=5,
        help='number of folds to segment the text on, default is %(default)s')

    parser.add_argument(
        '-j', '--njobs', type=int, metavar='<int>', default=1,
        help='number of parallel jobs to use, default is %(default)s')

    parser.add_argument(
        '-w', '--window', type=int, default=2, metavar='<int>', help='''
        Number of phonemes to be taken into account for boundary constraint,
        default is %(default)s.''')

    parser.add_argument(
        '-F', '--by-frequency', action='store_true',
        help='choose word candidates based on frequency '
        '(deactivated by default)')

    #TBD: add when train no folds and no njobs
    '''
    parser.add_argument(
        '-t', '--train-text', type= ,action='store_true',
        help='train_text')
    '''

    # parser.add_argument(
    #     '-d', '--decay', action='store_true',
    #     help='Decrease the size of lexicon, modelize memory of lexicon.')


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-puddle' command"""
    streamin, streamout, _, log, args = utils.prepare_main(
        name='wordseg-puddle',
        description=__doc__,
        add_arguments=_add_arguments)

    # load the train text if any
    train_text=None
    if args.train_file:
        if not os.path.isfile(args.train_file):
            raise RuntimeError(
                'test file not found: {}'.format(args.train_file))
        train_text = codecs.open(args.train_file, 'r', encoding='utf8')

    # load train and test texts, ignore empty lines
    test_text = (line for line in streamin if line)
    if train_text:
        train_text = (line for line in train_text if line)

    segmented = segment(
        test_text,train_text=train_text,
        window=args.window, by_frequency=args.by_frequency,
        nfolds=args.nfolds, njobs=args.njobs, log=log)
    streamout.write('\n'.join(segmented) + '\n')


if __name__ == '__main__':
    main()
