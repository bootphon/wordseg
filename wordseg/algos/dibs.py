"""Diphone based segmentation algorithm

    A DiBS model assigns, for each phrase-medial diphone, a value between
    0 and 1 inclusive (representing the probability the model assigns that
    there is a word-boundary there).

    The particularity of DiBS, with repect to the other segmentation algorithms
    in wordseg, is that it requires a little training set with word boundaries
    (ie. in phonologized form, not in prepared form). User has two choices:

    - Train and segment on the same text:
      Specify <input-text> only, <input-text> must be in phonologized form. The
      algorithm will the provided word boundaries to train the model and will
      remove them to generate the text to segment.

    - Train and segment on different texts:
      Specify <input-text> AND --train-file <training-file>. Here <input-text>
      must be in prepared form (without word boundaries) whereas
      <training-file> must contain word boundaries.

    For details, see Daland, R., Pierrehumbert, J.B., "Learning
    diphone-based segmentation". Cognitive science 35(1), 119-155 (2011).

"""

# this DiBS was written by Robert Daland <r.daland@gmail.com>

import abc
import codecs
import operator
import os
import six

from wordseg.separator import Separator
from wordseg import utils
from wordseg.prepare import prepare


class Counter(dict):
    """A Counter is a (key -> count) dictionnary for counting elements

    Update the counter with the `increment(key, count)` method. If an
    element is absent from the dictionary, its count defaults to 0.

    Examples
    --------
    >>> c = Counter()
    >>> c['a']
    0
    >>> c['a'] = 10
    >>> c['a']
    10
    >>> c.increment('a')
    >>> c['a']
    11
    >>> c.increment('a', 9)
    >>> c['a']
    20

    """
    def increment(self, key, value=1):
        self[key] = self.get(key, 0) + value

    def __getitem__(self, key):
        return self.get(key, 0)


class CorpusSummary(object):
    """Compute statistics on a phonemized corpus

    This is the "training" step of DiBS. It computes some statistics
    on phones (and diphones) on a tokenized training text.

    Parameters
    ----------
    text : sequence of str
        The input text must be tokenized at phone and word levels
        (syllables boundaries are ignored if any)
    separator : Separator, optional
        Token separation in the input text
    level : 'phone' or 'syllable', optional
        The token level to train the model on. Default to 'phone'.
    log : logging.Logger, optional
        Where to send log messages

    Attributes
    ----------
    summary : Counter
        Basic stats on the entire text: 'nlines', 'nwords' and 'nphones'
    lexicon : Counter
        Word count on the entire text
    phrase_initial : Counter
        The phones at first position in an utterance
    phrase_final : Counter
        The phones at last position in an utterance
    internal_diphones : Counter
        The count of within word diphones
    spanning_diphones : Counter
        The count of across words diphones
    diphones : Counter
        The count of all diphones, sum of internal and spanning diphones.

    Raises
    ------
    ValueError if a line in the `text` does not contain a word separator.

    """
    def __init__(self, text, separator=Separator(), level='phone',
                 log=utils.null_logger()):
        if level not in ('phone', 'syllable'):
            raise ValueError(
                'Unknown level {}, must be hone or syllable'.format(level))
        log.info('reading data at %s level', level)

        self.separator = separator
        self.summary = Counter()
        self.lexicon = Counter()
        self.phrase_initial = Counter()
        self.phrase_final = Counter()
        self.internal_diphones = Counter()
        self.spanning_diphones = Counter()

        nremoved = 0
        for index, utt in enumerate(text):
            # ignore empty lines (as in wordseg-prep, to have a
            # consistant behavior between the tools) and let the user
            # know how many lines we ignored
            if utt.strip() == '':
                log.debug('ignoring empty line %d', index+1)
                nremoved += 1
            else:
                if separator.word not in utt:
                    raise ValueError(
                        'word separator "{}" not found in train text: line {}'
                        .format(separator.word, index + 1))

                self._read_utterance(utt, level)

        self.diphones = Counter(self.internal_diphones)
        for k, v in self.spanning_diphones.items():
            self.diphones.increment(k, v)

        if nremoved > 0:
            log.info('ignored %d empty lines in train text', nremoved)

        log.info('train data summary: %s', self.summary)

    def _read_utterance(self, utterance, level):
        # no stats on empty text
        if not utterance:
            return

        # list of words in the utterance
        words = self.separator.tokenize(utterance, 'word')

        # nested list of phones or syllables (per word)
        phones = [self.separator.tokenize(word, level) for word in words]

        self.summary.increment('nlines')
        self.summary.increment('nwords', len(words))
        self.summary.increment('nphones', sum(len(p) for p in phones))

        # first and last phones of the utterance
        self.phrase_initial.increment((phones[0][0],))
        self.phrase_final.increment((phones[-1][-1],))

        for i, word in enumerate(words):
            self.lexicon.increment(word)

            # intra words diphones
            for j in range(len(phones[i]) - 1):
                self.internal_diphones.increment(
                    (phones[i][j], phones[i][j+1]))

            # inter words diphones
            if i < len(words) - 1:
                self.spanning_diphones.increment(
                    (phones[i][-1], phones[i+1][0]))


@six.add_metaclass(abc.ABCMeta)
class AbstractSegmenter(object):
    """An interface for DiBS segmentation

    Subclasses must implement the ``_init_diphones()`` method.

    Parameters
    ----------
    summary : CorpusSummary
        Some diphones stats computed on a train text
    pwb : float, optional
        Probability of word boundary, if not specified it is estimated
        from the train text as (nwords - nlines)/(nphones - nlines).
        When defined must in [0, 1].
    threshold : float, optional
        Threshold on word boundary probabilities. If a diphone has a
        word boundray probability greater than this threshold, a word
        boudary is added. Must be in [0, 1]. The optimal threshold is
        0.5 (default).
    log : logging.Logger, optional
        The log instance where to send messages.

    Raises
    ------
    ValueError:
        If `threshold` and `pwb` are not floats in [0, 1].


    """
    def __init__(self, summary, pwb=None, threshold=0.5,
                 log=utils.null_logger()):
        self.summary = summary
        self.wordsep = summary.separator.word
        self.log = log
        self.diphones = Counter()

        self.pwb = pwb
        if self.pwb and (self.pwb < 0 or self.pwb > 1):
            raise ValueError(
                'pwb must be a float in [0, 1], it is: {}'
                .format(str(self.pwb)))

        self.thresh = threshold
        if self.thresh < 0 or self.thresh > 1:
            raise ValueError(
                'threshold must be a float in [0, 1], it is: {}'
                .format(self.thresh))

        self.init_diphones()

    @abc.abstractmethod
    def init_diphones(self):
        """Initializes diphone probabilities from the ``summary``"""
        raise NotImplementedError

    @staticmethod
    def _norm2pdf(d):
        s = sum(d.values())
        return Counter([(item[0], float(item[1])/s) for item in d.items()])

    def _pwb(self):
        _s = self.summary.summary
        return float(
            _s['nwords'] - _s['nlines']) / (_s['nphones'] - _s['nlines'])

    def segment(self, utterance):
        """Estimates word boundaries based on diphone probabilities

        Parameters
        ----------
        utterance : str
            The utterance to segment must be a suite of phones
            or syllables separated by spaces.

        Returns
        -------
        The segmented utterance, with phone separation removed and
        spaces at estimated word boundaries.

        """
        phoneseq = tuple(utterance.replace(self.wordsep, ' ').split())

        out = [phoneseq[0]]
        for iPos in range(len(phoneseq) - 1):
            if self.diphones.get(phoneseq[iPos:iPos+2], 1.0) > self.thresh:
                out.append(self.wordsep)
            out.append(phoneseq[iPos+1])

        return ' '.join(out).replace(' ', '').replace(self.wordsep, ' ')


class GoldSegmenter(AbstractSegmenter):
    def init_diphones(self):
        if self.pwb:
            self.log.warning(
                'pwb specified at %s but unused in gold segmenter',
                self.pwb)

        within = self.summary.internal_diphones
        across = self.summary.spanning_diphones
        for d in self.summary.diphones:
            self.diphones[d] = float(across[d]) / (within[d] + across[d])


class PhrasalSegmenter(AbstractSegmenter):
    def init_diphones(self):
        px2_ = self._norm2pdf(self.summary.phrase_final)
        p_2y = self._norm2pdf(self.summary.phrase_initial)
        pxy = self._norm2pdf(self.summary.diphones)

        pwb = self.pwb or self._pwb()
        self.log.info('phrasal pwb = %s', pwb)

        for d in self.summary.diphones:
            x = (d[0],)
            y = (d[1],)
            num = px2_[x] * pwb * p_2y[y]
            denom = pxy[d]
            if num >= denom:
                self.diphones[d] = 1
            else:
                self.diphones[d] = num / denom


class LexicalSegmenter(AbstractSegmenter):
    def init_diphones(self):
        word_initial = Counter()
        word_final = Counter()
        for word in self.summary.lexicon:
            word_initial.increment((word[0],))
            word_final.increment((word[-1],))

        px2_ = self._norm2pdf(word_final)
        p_2y = self._norm2pdf(word_initial)
        pxy = self._norm2pdf(self.summary.diphones)

        pwb = self.pwb or self._pwb()
        self.log.info('lexical pwb = %s', pwb)

        for d in self.summary.diphones:
            x = (d[0],)
            y = (d[1],)
            num = px2_[x] * pwb * p_2y[y]
            denom = pxy[d]
            if denom == 0 or num > denom:
                self.diphones[d] = 1
            else:
                self.diphones[d] = num / denom


# -----------------------------------------------------------------------------
#  Segment function
# -----------------------------------------------------------------------------

def segment(test_text, trained_model, type='phrasal', threshold=0.5, pwb=None,
            log=utils.null_logger()):
    """Segment a corpus from a trained DiBS model

    This method is a simple wrapper on the Segmenter classes, namely
    GoldSegmenter, PhrasalSegmenter and LexicalSegmenter.

    Parameters
    ----------
    test_text : sequence of str
        The input text to segment is a sequence (list or generator) of
        utterances. Each utterance is composed of space seprated
        tokens (can be phones or syllables).
    trained_model : CorpusSummary
        The trained DiBS model used for segmentation of `test_text`.
    type : str, optional
        The type of DiBS segmenter to use, must be 'gold',
        'phrasal' or 'lexical'. Default is 'phrasal'.
    threshold: float, optional
        Threshold on word boundary probabilities. If a diphone has a
        word boundray probability greater than this threshold, a word
        boudary is added. Must be in [0, 1]. The optimal threshold is
        0.5 (default).
    pwb : float, optional
        Probability of word boundary, if not specified it is estimated
        from the train text as (nwords - nlines)/(nphones - nlines).
        This option is not used in 'gold' segmentation type. When
        defined must in [0, 1].
    log : logging.Logger, optional
        The log instance where to send messages.

    Yields
    ------
    utterance : str
        The current utterance segmented (with estimated word boundaries)

    Raises
    ------
    ValueError:
        If `type` is not 'gold', 'phrasal' or 'lexical'. If
        `threshold` and `pwb` are not floats in [0, 1].

    """
    # retrieve the segmenter from the 'type' argument
    try:
        segmenter = {
            'phrasal': PhrasalSegmenter,
            'lexical': LexicalSegmenter,
            'gold': GoldSegmenter}[type]
    except KeyError:
        raise ValueError(
            'unknown segmenter {}, must be phrasal, lexical or gold'
            .format(type))

    # init the segmenter with the trained model
    segmenter = segmenter(trained_model, pwb=pwb, threshold=threshold, log=log)
    for utt in test_text:
        yield segmenter.segment(utt)


# -----------------------------------------------------------------------------
#  Command line arguments
# -----------------------------------------------------------------------------

def _add_arguments(parser):
    """Add Dibs command specific options to the `parser`"""
    parser.add_argument(
        '-d', '--diphones', metavar='<output-file>',
        help='''optional file to write diphones,
        ignore diphones output if not specified''')

    group = parser.add_argument_group('training parameters')
    separator = Separator()

    group.add_argument(
        '-p', '--phone-separator', metavar='<str>',
        default=separator.phone,
        help='phone separator in training, default is "%(default)s"')

    group.add_argument(
        '-s', '--syllable-separator', metavar='<str>',
        default=separator.syllable,
        help='syllable separator in training, default is "%(default)s"')

    group.add_argument(
        '-w', '--word-separator', metavar='<str>',
        default=separator.word,
        help='word separator in training, default is "%(default)s"')

    group.add_argument(
        '-u', '--unit', choices=['phone', 'syllable'], default='phone', help='''
        token level to train on, must be "phone" or "syllable",
        default is %(default)s''')

    group = parser.add_argument_group('testing parameters')
    group.add_argument(
        '-t', '--type',  default='phrasal',
        choices=['gold', 'phrasal', 'lexical'],
        help='type of DiBS segmenter')

    group.add_argument(
        '-U', '--threshold', type=float, default=0.5, metavar='<float>',
        help='threshold on word boudary probabilities, must be in [0, 1]')

    group.add_argument(
        '-b', '--pboundary', type=float, metavar='<float>',
        help='''probability of word boundary, if not specified it is
        estimated from the train text as (nwords - nlines)/(nphones - nlines).
        This option is not used in 'gold' segmentation type,
        must be in [0, 1]''')

# dibs_summary is train_text
@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-dibs' command"""
    streamin, streamout, _, log, args = utils.prepare_main(
        name='wordseg-dibs',
        description=__doc__,
        add_arguments=_add_arguments,
        train_file=True)

    # setup the separator from parsed arguments
    separator = Separator(
        phone=args.phone_separator,
        syllable=args.syllable_separator,
        word=args.word_separator)

    # load test text as a list of utterances, ignore empty lines
    test_text = [line for line in streamin if line]
    log.info('loaded %s utterances as test data', len(test_text))

    # user provided a train text, ensure it is valid and that test_text does
    # not include word separators
    if args.train_file:
        if not os.path.isfile(args.train_file):
            raise ValueError(
                'train file specified but does not exist: {}'.format(
                    args.train_file))

        # make sure test_text is in prepared form
        for n, line in enumerate(test_text):
            if separator.word in line:
                raise ValueError(
                    f'word separator found in test text (line {n+1})')

        # load train and test texts, ignore empty lines
        train_text = codecs.open(args.train_file, 'r', encoding='utf8')
        train_text = [line for line in train_text if line]
        log.info('loaded %s utterances as train data', len(train_text))
    else:
        log.info('using test data for training')
        # the presence of word separator in train utterance will be checked
        # during training
        train_text = test_text

        # remove the word separators for testing
        test_text = prepare(test_text)

    # train the model (learn diphone statistics)
    trained_model = CorpusSummary(
        train_text, separator=separator, level=args.unit, log=log)

    # segment the test text on the trained model
    segmented = segment(
        test_text,
        trained_model,
        type=args.type,
        threshold=args.threshold,
        pwb=args.pboundary,
        log=log)

    # output the segmented text
    streamout.write('\n'.join(segmented) + '\n')

    # save the computed diphones if required
    if args.diphones:
        log.info(
            'saving %s diphones to %s',
            len(trained_model.diphones), args.diphones)

        output = ('{} {} {}'.format(v, k[0], k[1]) for k, v in sorted(
            trained_model.diphones.items(),
            key=operator.itemgetter(1), reverse=True))

        codecs.open(args.diphones, 'w', encoding='utf8').write(
            '\n'.join(output) + '\n')


if __name__ == '__main__':
    main()
