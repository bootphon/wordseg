"""Word segmentation evaluation.

Evaluates a segmented text against it's gold version. Display the
precision, recall and f-score at type, token and boundary levels.

"""

import codecs

from wordseg import utils
from wordseg.separator import Separator


DEFAULT_SEPARATOR = Separator(None, None, ' ')
"""Separation for words only, separated by ' '"""


class Evaluation:
    def __init__(self):
        self.test = 0
        self.gold = 0
        self.correct = 0
        self.n = 0
        self.n_exactmatch = 0

    def precision(self):
        return self.correct / (self.test + 1e-100)

    def recall(self):
        return self.correct / (self.gold + 1e-100)

    def fscore(self):
        return 2 * self.correct / (self.test + self.gold + 1e-100)

    def exact_match(self):
        return self.n_exactmatch / (self.n + 1e-100)

    def update(self, test_set, gold_set):
        self.n += 1

        if test_set == gold_set:
            self.n_exactmatch += 1

        # omit empty items for type scoring (should not affect token
        # scoring). Type lists are prepared with '_' where there is no
        # match, to keep list lengths the same
        self.test += len([x for x in test_set if x != '_'])
        self.gold += len([x for x in gold_set if x != '_'])
        self.correct += len(test_set & gold_set)

    def update_lists(self, test_sets, gold_sets):
        if len(test_sets) != len(gold_sets):
            raise ValueError(
                '#words different in test and gold: {} != {}'
                .format(len(test_sets), len(gold_sets)))

        for t, g in zip(test_sets, gold_sets):
            self.update(t, g)


class StringPos(object):
    """Compute start and stop index of words in an utterance"""
    def __init__(self):
        self.idx = 0

    def __call__(self, n):
        """Return the position of the current word given its length `n`"""
        start = self.idx
        self.idx += n
        return start, self.idx


def read_data(text, separator=DEFAULT_SEPARATOR):
    """Load text data for evaluation

    Parameters
    ----------
    text : list of str
        The list of utterances to read for the evaluation
    separator : Separator
        Separators to tokenize the text with

    Returns
    -------
    (words, positions, lexicon) : three lists
        where `words` are the input utterences with word separators
        removed, `positions` stores the start/stop index of each word
        for each utterance, and `lexicon` is the list of words

    """
    words = []
    positions = []
    lexicon = {}

    for utt in text:
        # the utterance with word separators removed
        words.append(separator.remove(utt, 'word'))

        # utt = list(separator.split(utt.strip(), level='word'))
        utt = list(separator.tokenize(utt, 'word'))

        # loop over words in line and add to dictionary
        for word in utt:
            lexicon[word] = 1

        idx = StringPos()
        positions.append({idx(len(word)) for word in utt})

    # return the words lexicon as a sorted list
    lexicon = sorted([k for k in lexicon.keys()])
    return words, positions, lexicon


def _stringpos_boundarypos(stringpos):
    return [{left for left, _ in line if left > 0} for line in stringpos]


def _lexicon_check(textlex, goldlex):
    """Compare hypothesis and gold lexicons"""
    textlist = []
    goldlist = []
    for w in textlex:
        if w in goldlex:
            # set up matching lists for the true positives
            textlist.append(w)
            goldlist.append(w)
        else:
            # false positives
            textlist.append(w)
            # ensure matching null element in text list
            goldlist.append('_')

    for w in goldlex:
        if w not in goldlist:
            # now for the false negatives
            goldlist.append(w)
            # ensure matching null element in text list
            textlist.append('_')

    textset = [{w} for w in textlist]
    goldset = [{w} for w in goldlist]
    return textset, goldset


def evaluate(text, gold, separator=DEFAULT_SEPARATOR):
    text_words, text_stringpos, text_lex = read_data(text, separator)
    gold_words, gold_stringpos, gold_lex = read_data(gold, separator)

    if len(gold_words) != len(text_words):
        raise RuntimeError(
            'gold and train have different size: len(gold)={}, len(train)={}'
            .format(len(gold_words), len(text_words)))

    for i, (g, t) in enumerate(zip(gold_words, text_words)):
        if g != t:
            raise RuntimeError(
                'gold and train differ at line {}: gold="{}", train="{}"'
                .format(i+1, g, t))

    # get text and gold sets from lexicons
    type_eval = Evaluation()
    tl, gl = _lexicon_check(text_lex, gold_lex)
    type_eval.update_lists(tl, gl)

    token_eval = Evaluation()
    token_eval.update_lists(text_stringpos, gold_stringpos)

    boundary_eval = Evaluation()
    boundary_eval.update_lists(
        _stringpos_boundarypos(text_stringpos),
        _stringpos_boundarypos(gold_stringpos))

    return {
        'type_fscore': type_eval.fscore(),
        'type_precision': type_eval.precision(),
        'type_recall': type_eval.recall(),
        'token_fscore': token_eval.fscore(),
        'token_precision': token_eval.precision(),
        'token_recall': token_eval.recall(),
        'boundary_fscore': boundary_eval.fscore(),
        'boundary_precision': boundary_eval.precision(),
        'boundary_recall': boundary_eval.recall()}


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-eval' command"""
    streamin, streamout, separator, log, args = utils.prepare_main(
        name='wordseg-eval',
        description=__doc__,
        separator=DEFAULT_SEPARATOR,
        add_arguments=lambda parser: parser.add_argument(
            'gold', metavar='<gold-file>',
            help='gold file to evaluate the input data on'))

    # load the gold text as a list of utterances
    gold = [l.strip() for l in codecs.open(args.gold, 'r', encoding='utf8')]

    # load the text as a list of utterances
    text = [l.strip() for l in streamin]

    # evaluation returns a dict of 'score name' -> float
    results = evaluate(text, gold)

    streamout.write('\n'.join(
        # display scores with 4-digit float precision
        '{}\t{}'.format(k, '%.4g' % v) for k, v in results.items()) + '\n')


if __name__ == '__main__':
    main()
