# coding: utf-8

"""Diphone based segmentation algorithm

This algorithm uses diphone probabilities to decide whether a specific
sequence is likely to span a word boundary (typically because the
diphone is rare) or not.

For details, see Daland, R., Pierrehumbert, J.B., "Learning
diphone-based segmentation". Cognitive science 35(1), 119–155 (2011).

"""

import codecs
import os

from wordseg import utils


class _Counter(dict):
    def increment(self, key, value=1):
        self[key] = self.get(key, 0) + value

    def __getitem__(self, key):
        return self.get(key, 0)


class _Summary(object):
    def __init__(self, multigraphemic=False, wordsep='##'):
        self.wordsep = wordsep
        self.multigraphemic = multigraphemic
        self.summary = _Counter()

        self.phraseinitial = _Counter()
        self.phrasefinal = _Counter()
        self.lexicon = _Counter()

        self.internaldiphones = _Counter()
        self.spanningdiphones = _Counter()

    def readstream(self, instream):
        for line in instream:
            if self.multigraphemic:
                wordseq = [
                    tuple(word.split()) for word in line.split(self.wordsep)
                    if word.split()]
            else:
                wordseq = line.split()

            if not wordseq:
                continue

            self.summary.increment('nLines')
            self.summary.increment('nTokens', len(wordseq))
            self.summary.increment(
                'nPhones', sum([len(word) for word in wordseq]))

            if self.multigraphemic:
                self.phraseinitial.increment((wordseq[0][0],))
                self.phrasefinal.increment((wordseq[-1][-1],))
            else:
                self.phraseinitial.increment(wordseq[0][0])
                self.phrasefinal.increment(wordseq[-1][-1])

            for i_word in range(len(wordseq)):
                word = wordseq[i_word]
                self.lexicon.increment(word)

                for i_pos in range(len(word)-1):
                    self.internaldiphones.increment(word[i_pos:i_pos+2])

                if i_word < len(wordseq)-1:
                    if self.multigraphemic:
                        self.spanningdiphones.increment(
                            tuple([word[-1], wordseq[i_word+1][0]]))
                    else:
                        self.spanningdiphones.increment(
                            word[-1]+wordseq[i_word+1][0])

    def diphones(self):
        alldiphones = _Counter(self.internaldiphones)
        for diphone in self.spanningdiphones:
            alldiphones.increment(diphone, self.spanningdiphones[diphone])
        return(alldiphones)

    def save(self, outstream):
        if self.multigraphemic:
            def outdic(d):
                lambda d: '\t'.join(
                    ['-'.join(item[0])+' '+str(item[1]) for item in d.items()])
        else:
            def outtdic(d):
                '\t'.join(
                    [str(item[0])+' '+str(item[1]) for item in d.items()])

        outstream.write(
            'multigraphemic\t' + str(self.multigraphemic) +
            '\twordsep\t' + self.wordsep)

        for data in ['summary', 'phraseinitial', 'phrasefinal',
                     'internaldiphones', 'spanningdiphones', 'lexicon']:
            outstream.write(data + '\t' + outdic(self.__dict__[data]))


class _Dibs(_Counter):
    def __init__(self, multigraphemic=False, threshold=0.5, wordsep=' '):
        super(_Dibs, self).__init__()
        self.multigraphemic = multigraphemic
        self.threshold = threshold
        self.wordsep = wordsep

    def test(self, text):
        wordsep = (self.wordsep * self.multigraphemic
                   + ' ' * (not self.multigraphemic))

        for line in text:
            if self.multigraphemic:
                phoneseq = tuple(line.replace(self.wordsep, ' ').split())
            else:
                phoneseq = ''.join(line.split())

            if not phoneseq:
                continue

            out = [phoneseq[0]]
            for i_pos in range(1, len(phoneseq)):
                if self.get(phoneseq[i_pos-1:i_pos+1], 1.0) > self.threshold:
                    out.append(wordsep)
                out.append(phoneseq[i_pos])

            yield utils.strip((' ' * self.multigraphemic).join(out))

    def save(self, outstream):
        if self.multigraphemic:
            rows = sorted(dict([((key[0],), 1) for key in self]).keys())
            cols = sorted(dict([((key[1],), 1) for key in self]).keys())
        else:
            rows = '#$123456789@DEHIJNPQRSTUVZ_bdfghijklmnprstuvwxz{~'
            cols = '#$123456789@DEHIJNPQRSTUVZ_bdfghijklmnprstuvwxz{~'

        outstream.write('\t'+'\t'.join([str(y) for y in cols]))

        for x in rows:
            try:
                outstream.write(
                    str(x) + '\t' + '\t'.join([str(self[x+y]) for y in cols]))
            except KeyError:
                outstream.write(
                    str(x) + '\t' + '\t'.join(
                        [str(self.get(x + y, None)) for y in cols]))


def _norm2pdf(fdf):
    return _Counter([(item[0], float(item[1]) / sum(fdf.values()))
                    for item in fdf.items()])


def _baseline(speech, pwb=None):
    dib = _Dibs(multigraphemic=speech.multigraphemic, wordsep=speech.wordsep)
    within, across = speech.internaldiphones, speech.spanningdiphones

    for diphone in speech.diphones():
        dib[diphone] = float(across[diphone]) / (
            within[diphone] + across[diphone])

    return dib


def _phrasal(speech, pwb=None, log=utils.null_logger()):
    px2_ = _norm2pdf(speech.phrasefinal)
    p_2y = _norm2pdf(speech.phraseinitial)
    pxy = _norm2pdf(speech.diphones())
    pwb = pwb or (
        float(speech.summary['nTokens'] - speech.summary['nLines']) /
        (speech.summary['nPhones'] - speech.summary['nLines']))

    log.info('phrasal: pwb = %s', pwb)

    dib = _Dibs(multigraphemic=speech.multigraphemic, wordsep=speech.wordsep)
    for diphone in speech.diphones():
        x = (diphone[0],) if speech.multigraphemic else diphone[0]
        y = (diphone[1],) if speech.multigraphemic else diphone[1]

        num = px2_[x] * pwb * p_2y[y]
        denom = pxy[diphone]
        dib[diphone] = max(1.0, num / denom)

    return dib


def _lexical(speech, lexicon=None, pwb=None, log=utils.null_logger()):
    wordinitial = _Counter()
    wordfinal = _Counter()
    lexicon = lexicon or speech.lexicon

    for word in lexicon:
        if speech.multigraphemic:
            wordinitial.increment((word[0],))
            wordfinal.increment((word[-1],))
        else:
            wordinitial.increment(word[0])
            wordfinal.increment(word[-1])

    px2_ = _norm2pdf(wordfinal)
    p_2y = _norm2pdf(wordinitial)
    pxy = _norm2pdf(speech.diphones())
    pwb = pwb or (
        float(speech.summary['nTokens'] - speech.summary['nLines']) /
        (speech.summary['nPhones'] - speech.summary['nLines']))

    log.info('lexical: pwb = %s', pwb)

    dib = _Dibs(multigraphemic=speech.multigraphemic, wordsep=speech.wordsep)
    for diphone in speech.diphones():
        x = (diphone[0],) if speech.multigraphemic else diphone[0]
        y = (diphone[1],) if speech.multigraphemic else diphone[1]

        num = px2_[x] * pwb * p_2y[y]
        denom = pxy[diphone]
        dib[diphone] = max(1.0, num / denom)

    return dib


# TODO fix this confusion between test/train texts, see if can
# completly avoid train text... For now the train is used to
# initialize diphones
def segment(text, prob_word_boundary=None, train_text=None,
            diphones=None, log=utils.null_logger()):
    """Word segmentation using the dibs algorithm"""

    text = list(text)

    if not train_text:
        log.warning('using the input text for training')
        train_text = text

    training = _Summary(multigraphemic=True, wordsep=' ')
    training.readstream(train_text)

    phrasal_dibs = _phrasal(training, pwb=prob_word_boundary, log=log)
    segmented = phrasal_dibs.test(text)

    if diphones:
        phrasal_dibs.save(codecs.open(diphones, 'w', encoding='utf8'))

    return segmented


def _add_arguments(parser):
    """Add Dibs command specific options to the `parser`"""
    group = parser.add_argument_group('algorithm parameters')
    group1 = group.add_mutually_exclusive_group()
    group1.add_argument(
        '-p', '--prob-word-boundary', metavar='<float>', type=float,
        help='''Word boundary probability, must be in [0, 1]''')

    group1.add_argument(
        '-t', '--train', metavar='<int or file>', type=str, default='200',
        help='''Dibs requires a little train corpus to compute some statistics.
        If the argument is a file, read this file as a train corpus. If
        the argument is a positive integer N, take the N first lines of the
        <input-file> (train) file for testing, default is %(default)s''')

    group.add_argument(
        '-d', '--diphones', metavar='<output-file>',
        help='''optional filename to write diphones,
        ignore diphones output if this argument is not specified''')


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-dibs' command"""
    streamin, streamout, _, log, args = utils.prepare_main(
        name='wordseg-dibs',
        description=__doc__,
        add_arguments=_add_arguments)

    # load the test input as a list
    test_text = [line for line in streamin]

    # prepare the train input according to --train: try to open the
    # file first, if file not found cast to int
    if os.path.isfile(args.train):
        train_text = codecs.open(
            args.train, 'r', encoding='utf8').readlines()
    else:
        try:
            ntrain = int(args.train)
        except ValueError:
            raise ValueError(
                '--train option must be an int or an existing file, '
                'it is: {}'.format(args.train))

        if ntrain <= 0:
            raise ValueError(
                '--train option must be positive, it is: {}'.format(ntrain))

        train_text = test_text[:ntrain]

    segmented = segment(
        test_text,
        prob_word_boundary=args.prob_word_boundary,
        train_text=train_text,
        diphones=args.diphones,
        log=log)

    streamout.write('\n'.join(segmented) + '\n')


if __name__ == '__main__':
    main()
