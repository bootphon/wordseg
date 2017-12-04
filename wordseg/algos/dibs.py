# coding: utf-8

"""Diphone based segmentation algorithm

A DiBS model assigns, for each phrase-medial diphone, a value between
0 and 1 inclusive (representing the probability the model assigns that
there is a word-boundary there). In practice, these probabilities are
mapped to hard decisions, with the optimal threshold being 0.5.

Without any training at all, a phrasal-DiBS model can assign sensible
defaults (namely, 0, or the context-independent probability of a
medial word boundary, which will always be less than 0.5, and so
effectively equivalent to 0 in a hard-decisions context). The outcome
in this case would be total undersegmentation (for default 0; total
oversegmentation for default 1).

It takes relatively little training to get a DiBS model up to
near-ceiling (*i.e.* the model's intrinsic ceiling: "as good as
that model will get even if you train it forever", rather than
"perfect for that dataset"). Moreover, in principle you can have the
model do its segmentation for the nth sentence based on the stats it
has accumulated for every preceding sentence (and with a little
effort, even on the nth sentence as well). In practice, since I was
never testing on the training set for publication work, but I was
testing on *huge* test sets, I optimized the code for mixed
iterative/batch training, meaning it could read in a training set,
update parameteres, test, and then repeat ad infinitum.

For details, see Daland, R., Pierrehumbert, J.B., "Learning
diphone-based segmentation". Cognitive science 35(1), 119–155 (2011).

"""

# this DiBS was written by Robert Daland <r.daland@gmail.com>

import codecs
import operator
import os

from wordseg.separator import Separator
from wordseg import utils


class Counter(dict):
    def increment(self, key, value=1):
        self[key] = self.get(key, 0) + value

    def __getitem__(self, key):
        return self.get(key, 0)


class Summary(object):
    def __init__(self, text, separator=Separator(), log=utils.null_logger()):
        self.separator = separator
        self.summary = Counter()
        self.lexicon = Counter()
        self.phrase_initial = Counter()
        self.phrase_final = Counter()
        self.internal_diphones = Counter()
        self.spanning_diphones = Counter()
        self._diphones = None

        for utt in text:
            self._read_utterance(utt)

        log.info('train data summary: %s', self.summary)

    def _read_utterance(self, utterance):
        wordseq = [
            tuple(word.split())
            for word in utterance.split(self.separator.word)
            if word.split()]

        if not wordseq:
            return

        self.summary.increment('nlines')
        self.summary.increment('ntokens', len(wordseq))
        self.summary.increment('nphones', sum([len(w) for w in wordseq]))

        self.phrase_initial.increment((wordseq[0][0],))
        self.phrase_final.increment((wordseq[-1][-1],))

        for i, word in enumerate(wordseq):
            self.lexicon.increment(word)

            for j in range(len(word)-1):
                self.internal_diphones.increment(word[j:j+2])

            if i < len(wordseq) - 1:
                self.spanning_diphones.increment(
                    tuple([word[-1], wordseq[i+1][0]]))

    def diphones(self):
        if not self._diphones:
            # compute the final list of all the diphones
            self._diphones = Counter(self.internal_diphones)
            for d in self.spanning_diphones:
                self._diphones.increment(d, self.spanning_diphones[d])
        return self._diphones


class _AbstractSegmenter(Counter):
    def __init__(self, model, pwb=None, threshold=0.5,
                 log=utils.null_logger()):
        self.model = model
        self.wordsep = model.separator.word
        self.log = log

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

        self._init_diphones()

    def _init_diphones(self):
        raise NotImplementedError

    @staticmethod
    def _norm2pdf(d):
        s = sum(d.values())
        return Counter([(item[0], float(item[1])/s) for item in d.items()])

    def _pwb(self):
        _s = self.model.summary
        return float(
            _s['ntokens'] - _s['nlines']) / (_s['nphones'] - _s['nlines'])

    def segment(self, utterance):
        phoneseq = tuple(utterance.replace(self.wordsep, ' ').split())

        out = [phoneseq[0]]
        for iPos in range(len(phoneseq) - 1):
            if self.get(phoneseq[iPos:iPos+2], 1.0) > self.thresh:
                out.append(self.wordsep)
            out.append(phoneseq[iPos+1])

        return ' '.join(out).replace(' ', '').replace(self.wordsep, ' ')


class BaselineSegmenter(_AbstractSegmenter):
    def _init_diphones(self):
        self.log.warning(
            'pwb specified at %s but unused in baseline segmenter', self.pwb)
        within = self.model.internal_diphones
        across = self.model.spanning_diphones
        for d in self.model.diphones():
            self[d] = float(across[d]) / (within[d] + across[d])


class PhrasalSegmenter(_AbstractSegmenter):
    def _init_diphones(self):
        px2_ = self._norm2pdf(self.model.phrase_final)
        p_2y = self._norm2pdf(self.model.phrase_initial)
        pxy = self._norm2pdf(self.model.diphones())
        pwb = self.pwb or self._pwb()
        self.log.info('phrasal pwb = ' + str(pwb))

        for d in self.model.diphones():
            x = (d[0],)
            y = (d[1],)
            num = px2_[x] * pwb * p_2y[y]
            denom = pxy[d]
            if num >= denom:
                self[d] = 1
            else:
                self[d] = num / denom


class LexicalSegmenter(_AbstractSegmenter):
    def _init_diphones(self):
        word_initial = Counter()
        word_final = Counter()
        for word in self.model.lexicon:
            word_initial.increment((word[0],))
            word_final.increment((word[-1],))

        px2_ = self._norm2pdf(word_final)
        p_2y = self._norm2pdf(word_initial)
        pxy = self._norm2pdf(self.model.diphones())

        pwb = self.pwb or self._pwb()
        self.log.info('lexical pwb = ' + str(pwb))

        for d in self.model.diphones():
            x = (d[0],)
            y = (d[1],)
            num = px2_[x] * pwb * p_2y[y]
            denom = pxy[d]
            if denom == 0 or num > denom:
                self[d] = 1
            else:
                self[d] = num / denom


def segment(text, model, type='phrasal', threshold=0.5, pwb=None,
            log=utils.null_logger()):
    # retrieve the segmenter from the 'type' argument
    try:
        Segmenter = {
            'phrasal': PhrasalSegmenter,
            'lexical': LexicalSegmenter,
            'baseline': BaselineSegmenter}[type]
    except KeyError:
        raise ValueError(
            'unknown segmenter {}, must be phrasal, lexical or baseline'
            .format(type))

    # init the segmenter with the trained model
    segmenter = Segmenter(model, pwb=pwb, threshold=threshold, log=log)
    for utt in text:
        yield segmenter.segment(utt)


def _add_arguments(parser):
    """Add Dibs command specific options to the `parser`"""
    parser.add_argument(
        '-d', '--diphones', metavar='<output-file>',
        help='''optional file to write diphones,
        ignore diphones output if not specified''')

    group = parser.add_argument_group('training parameters')
    separator = Separator()

    group.add_argument(
        'train_file', metavar='<train-file>', type=str,
        help='Dibs requires a little train corpus to compute some statistics')

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

    group = parser.add_argument_group('testing parameters')
    group.add_argument(
        '-t', '--type',  default='phrasal',
        choices=['baseline', 'phrasal', 'lexical'],
        help='type of DiBS segmenter')

    group.add_argument(
        '-T', '--threshold', type=float, default=0.5, metavar='<float>',
        help='threshold on word boudary probabilities, must be in [0, 1]')

    group.add_argument(
        '-b', '--pboundary', type=float, metavar='<float>',
        help='''probability of word boundary, if not specified it is
        estimated from the train text as (nwords - nlines)/(nphones - nlines).
        This option is not used in 'baseline' segmentation type,
        must be in [0, 1]''')


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-dibs' command"""
    streamin, streamout, _, log, args = utils.prepare_main(
        name='wordseg-dibs',
        description=__doc__,
        add_arguments=_add_arguments)

    # setup the separator from parsed arguments TODO for now only
    # word, implement custom phone sep as well
    separator = Separator(
        phone=args.phone_separator,
        syllable=args.syllable_separator,
        word=args.word_separator)

    # ensure the train file exists
    if not os.path.isfile(args.train_file):
        raise ValueError(
                'train file does not exist: {}'.format(args.train_file))

    # load train and test texts, ignore empty lines
    train_text = codecs.open(args.train_file, 'r', encoding='utf8')
    train_text = (line for line in train_text if line)
    test_text = (line for line in streamin if line)

    # train the model (learn diphone statistics)
    dibs_model = Summary(train_text, separator=separator, log=log)

    # segment the test text on the trained model
    output = segment(
        test_text,
        dibs_model,
        type=args.type,
        threshold=args.threshold,
        pwb=args.pboundary,
        log=log)

    streamout.write('\n'.join(output) + '\n')

    if args.diphones:
        log.info(
            'saving %s diphones to %s',
            len(dibs_model.diphones()), args.diphones)

        output = ('{} {} {}'.format(v, k[0], k[1]) for k, v in sorted(
            dibs_model.diphones().items(),
            key=operator.itemgetter(1), reverse=True))

        codecs.open(args.diphones, 'w', encoding='utf8').write(
            '\n'.join(output) + '\n')


if __name__ == '__main__':
    main()
