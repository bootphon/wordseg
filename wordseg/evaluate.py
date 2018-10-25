"""Word segmentation evaluation

Evaluates a segmented text against it's gold version: outputs the
precision, recall and f-score at type, token and boundary levels. We
distinguish whether utterance edges (begin and end of the utterance)
are counted towards the boundary performance or not. The evaluation
optionally computes the adjusted rank index (requires the prepared
text to be provided) and a summary of segmentation errors (requires an
output JSON file to be specified).

"""

import codecs
import collections
import json
import numpy as np
import warnings
from sklearn.metrics.cluster import adjusted_rand_score

from wordseg import utils
from wordseg.separator import Separator


class TokenEvaluation(object):
    """Evaluation of token f-score, precision and recall"""
    def __init__(self):
        self.test = 0
        self.gold = 0
        self.correct = 0
        self.n = 0
        self.n_exactmatch = 0

    def precision(self):
        return float(self.correct) / self.test if self.test != 0 else None

    def recall(self):
        return float(self.correct) / self.gold if self.gold != 0 else None

    def fscore(self):
        total = self.test + self.gold
        return float(2 * self.correct) / total if total != 0 else None

    def exact_match(self):
        return float(self.n_exactmatch) / self.n if self.n else None

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


class TypeEvaluation(TokenEvaluation):
    """Evaluation of type f-score, precision and recall"""
    @staticmethod
    def lexicon_check(textlex, goldlex):
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

    def update_lists(self, text, gold):
        lt, lg = self.lexicon_check(text, gold)
        super(TypeEvaluation, self).update_lists(lt, lg)


class BoundaryEvaluation(TokenEvaluation):
    """Evaluation of boundary f-score, precision and recall

    Includes first and last boundary of an utterance

    """
    @staticmethod
    def get_boundary_positions(stringpos):
        return [{idx for pair in line for idx in pair} for line in stringpos]

    def update_lists(self, text, gold):
        lt = self.get_boundary_positions(text)
        lg = self.get_boundary_positions(gold)
        super(BoundaryEvaluation, self).update_lists(lt, lg)


class BoundaryNoEdgeEvaluation(BoundaryEvaluation):
    """Evaluation of boundary f-score, precision and recall

    Excludes first and last boundary of an utterance

    """
    @staticmethod
    def get_boundary_positions(stringpos):
        return [{left for left, _ in line if left > 0} for line in stringpos]


class _StringPos(object):
    """Compute start and stop index of words in an utterance"""
    def __init__(self):
        self.idx = 0

    def __call__(self, n):
        """Return the position of the current word given its length `n`"""
        start = self.idx
        self.idx += n
        return start, self.idx


def read_data(text, separator=Separator(None, None, ' ')):
    """Load text data for evaluation

    Parameters
    ----------
    text : list of str
        The list of utterances to read for the evaluation.
    separator : Separator, optional
        Separators to tokenize the text with, default to space
        separated words.

    Returns
    -------
    (words, positions, lexicon) : three lists
        where `words` are the input utterances with word separators
        removed, `positions` stores the start/stop index of each word
        for each utterance, and `lexicon` is the list of words.

    """
    words = []
    positions = []
    lexicon = {}

    # ignore empty lines
    for utt in (utt for utt in text if utt.strip()):
        # the utterance with word separators removed
        words.append(separator.remove(utt, 'word'))

        utt = list(separator.tokenize(utt, 'word'))

        # loop over words in line and add to dictionary
        for word in utt:
            lexicon[word] = 1

        idx = _StringPos()
        positions.append({idx(len(word)) for word in utt})

    # return the words lexicon as a sorted list
    lexicon = sorted([k for k in lexicon.keys()])
    return words, positions, lexicon


def compute_class_labels(words, units):
    """Compute class labels to be used for cluster similarity measures

    Each word is considered a class, and each unit is mapped to the
    word it belongs to.

    Parameters
    ----------
    words: list of str
        Utterances made of space separated words.
    units : list of str
        Utterances made of space separated atomic units (phonemes or
        syllables).

    Returns
    -------
    class_labels : numpy array of int
        Each unit mapped to the word it belongs to (with words coded
        as integers)

    Raises
    ------
    ValueError:
        If `words` and `units` do not match together

    Examples
    --------

    >>> from wordseg.evaluate import compute_class_labels
    >>> words = ['hello world', 'python']
    >>> units = ['h el lo wo r ld', 'py th on']
    >>> compute_class_labels(words, units)
    array([0, 0, 0, 1, 1, 1, 2, 2, 2])

    """
    # in case the inputs are generators, force them as lists
    words, units = list(words), list(units)

    # make sure we have the number of utterances in words and units
    if not len(words) == len(units):
        raise ValueError(
            'words and units do not have the same number of utterances: '
            'len(words)={}, len(units)={}'.format(len(words), len(units)))

    # for each utterance, checks words and units are the same
    nunits = 0
    for n, (words_utt, units_utt) in enumerate(zip(words, units)):
        words_utt = words_utt.replace(' ', '')
        nunits += len(units_utt.split())
        units_utt = units_utt.replace(' ', '')
        if not words_utt == units_utt:
            raise ValueError(
                'utterance {}: words/units do not match: '
                'words="{}" / units="{}"'.format(n, words_utt, units_utt))

    # collapse words and units from nested lists to numpy array
    words = np.asarray([word for utt in words for word in utt.split()])
    units = np.asarray([unit for utt in units for unit in utt.split()])

    # count the number of units in a word
    def _word_len(word, units):
        l, w = 0, ''
        while w != word:
            w += units[l]
            l += 1
        return l

    # build the class labels
    class_labels = np.zeros((nunits,), dtype=np.int)
    index = 0
    for class_id, word in enumerate(words):
        try:
            new_index = index + _word_len(word, units[index:])
        except IndexError:
            raise ValueError(
                'word "{}" not found in "{}"'.format(word, units[index:]))
        class_labels[index:new_index] = class_id
        index = new_index

    return class_labels


class SegmentationSummary(object):
    """Computes a summary of the segmentation errors

    The errors can be oversegmentations, undersegmentations or
    missegmentations. Correct segmentations are also reported.

    """
    def __init__(self):
        # token separation on words only
        self.separator = Separator(phone=None, syllable=None, word=' ')

        # count over/under/mis/good segmentation for each word type
        self.over_segmentation = collections.defaultdict(int)
        self.under_segmentation = collections.defaultdict(int)
        self.mis_segmentation = collections.defaultdict(int)
        self.correct_segmentation = collections.defaultdict(int)

    def to_dict(self):
        """Exports the summary as a dictionary

        Returns
        -------
        summary : dict
            A dictionary with the complete summary in the following
            entries: 'over', 'under', 'mis', 'correct'. In each entry,
            the words are sorted by decreasing frequency, and
            alphabetically (for equivalent frequency).

        """
        # collapse all the dicts in a single one
        summary = {
            'over': self.over_segmentation,
            'under': self.under_segmentation,
            'mis': self.mis_segmentation,
            'correct': self.correct_segmentation}

        # sort by most frequent word decreasing order (and then
        # alphabetically increasing order)
        summary = {k: {w[0]: w[1] for w in sorted(
            v.items(), key=lambda x: (-x[1], x[0]), reverse=False)}
                   for k, v in summary.items()}

        return summary

    def summarize(self, text, gold):
        """Computes segmentation errors on a whole text

        Call :meth:`summarize_utterance` on each utterance of gold
        and text.

        Parameters
        ----------
        text : list of str
            The list of utterances for the segmented text (to be
            evaluated)
        gold : list of str
            The list of utterances for the gold text

        Raises
        ------
        ValueError
            If `text` and `gold` do not have the same number of
            utterances. If :meth:`summarize_utterance` raise a
            ValueError.

        """
        if not len(gold) == len(text):
            raise ValueError(
                'text and gold do not have the same number of utterances')

        for t, g in zip(text, gold):
            self.summarize_utterance(t, g)

    def summarize_utterance(self, text, gold):
        """Computes segmentation errors on a single utterance

        This method returns no result but update the intern summary,
        accessible using :meth:`to_dict`.

        Parameters
        ----------
        text : str
            A segmented utterance
        gold : str
            A gold utterance

        Raises
        ------
        ValueError
            If `text` and `gold` are mismatched, i.e. they do not
            contain the same suite of letters (once all the spaces
            removed).

        """
        # check gold and text match (with all spaces removed)
        if self.separator.remove(gold) != self.separator.remove(text):
            raise ValueError(
                'mismatch in gold and text: {} != {}'.format(gold, text))

        # get text and gold as lists of words
        gold_words = self.separator.tokenize(gold, level='word')
        text_words = self.separator.tokenize(text, level='word')

        # silly case where gold and text are identical
        if gold_words == text_words:
            for word in gold_words:
                self.correct_segmentation[word] += 1
            return

        # divide gold and text in chunks, packing chunks where gold
        # and text share a common boundary.
        chunks = self._boundary_chunks(text_words, gold_words)

        # classify each chunk as under/over/mis/good segmentation
        for text_chunk, gold_chunk in chunks:
            category = self._classify_chunk(text_chunk, gold_chunk)

            if category == 'correct':
                d = self.correct_segmentation
            elif category == 'under':
                d = self.under_segmentation
            elif category == 'over':
                d = self.over_segmentation
            else:
                d = self.mis_segmentation

            # register the chunk's words into the summary for the
            # relevant category
            for word in gold_chunk:
                d[word] += 1

    @classmethod
    def _boundary_chunks(cls, text, gold):
        """Returns the list of chunks in a pair of text/gold utterance"""
        return cls._boundary_chunks_aux(text, gold, [])

    @classmethod
    def _boundary_chunks_aux(cls, text, gold, chunks):
        lg = len(gold)
        lt = len(text)

        # end of recursion
        if not lg and not lt:
            return chunks

        # impossible to have one empty but not the other. Should be
        # the case by construction, this assert is not required.
        assert lg and lt

        # compute the next chunk
        chunk = cls._compute_chunk(text, gold)

        # recursion
        return cls._boundary_chunks_aux(
            text[len(chunk[0]):],
            gold[len(chunk[1]):],
            chunks + [chunk])

    @staticmethod
    def _compute_chunk(text, gold):
        """Find the first chunk in a pair of text/gold utterances

        A chunk is a pair of lists of words sharing a common boundary
        (begin and end of a sequence of words).

        Example
        -------
        >>> gold = 'baby going home'.split()

        >>> text = 'ba by going home'.split()
        >> _compute_chunk(text, gold)
        (['ba', 'by'], ['baby'])

        >>> text = 'babygoinghome'.split()
        >> _compute_chunk(text, gold)
        (['babygoinghome'], ['baby', 'going', 'home'])

        """
        # non empty texts and same letters. This should be the case by
        # construction, those asserts are not required.
        assert len(gold) and len(text)
        assert ''.join(gold) == ''.join(text)

        # easy case, first word is the same
        if gold[0] == text[0]:
            return ([text[0]], [gold[0]])

        text_concat, text_index = text[0], 0
        gold_concat, gold_index = gold[0], 0
        while len(gold_concat) != len(text_concat):
            if len(gold_concat) < len(text_concat):
                gold_index += 1
                gold_concat = gold_concat + gold[gold_index]
            else:
                text_index += 1
                text_concat = text_concat + text[text_index]
        return (text[:text_index+1], gold[:gold_index+1])

    def _classify_chunk(self, text, gold):
        """A chunk is either over/under/mis/correct"""
        if len(gold) == len(text):
            if len(gold) == 1:
                return 'correct'
            return 'mis'
        elif len(gold) < len(text):
            if len(gold) == 1:
                return 'over'
            return 'mis'
        else:  # len(gold) > len(text)
            if len(text) == 1:
                return 'under'
            return 'mis'


def summary(text, gold):
    """Computes the summary of segmentation errors

    This function is a simple wrapper on :class:`SegmentationSummary`

    Parameters
    ----------
    text : list of str
        The list of utterances for the segmented text (to be
        evaluated)
    gold : list of str
        The list of utterances for the gold text

    Returns
    -------
    summary : dict
        A dictionary with the complete summary in the following
        entries: 'over', 'under', 'mis', 'correct'.

    Raises
    ------
    ValueError
        If `text` and `gold` do not match, or something went wrong
        during the summary computation.

    """
    s = SegmentationSummary()
    s.summarize(text, gold)
    return s.to_dict()


def evaluate(text, gold, units=None):
    """Scores a segmented text against its gold version

    Parameters
    ----------
    text : sequence of str
        A suite of utterances made of space separated words.
    gold : sequence of str
        A suite of utterances made of space separated words.
    units : sequence of str, optional
        A suite of utterances made of space separated atomic units
        (phonemes or syllables). When specified, the function also
        computes the adjusted rand index.

    Returns
    -------
    scores : ordered dict
        A dictionary with the following entries in that fixed order:

        * 'type_fscore'
        * 'type_precision'
        * 'type_recall'
        * 'token_fscore'
        * 'token_precision'
        * 'token_recall'
        * 'boundary_all_fscore'
        * 'boundary_all_precision'
        * 'boundary_all_recall'
        * 'boundary_noedge_fscore'
        * 'boundary_noedge_precision'
        * 'boundary_noedge_recall'

        If `units` is specified in arguments, this additional entry is
        added:

        * 'adjusted_rand_index'

    Raises
    ------
    ValueError
        If `gold` and `text` have different size or differ in tokens

    """
    # force text and gold to be lists
    text, gold = list(text), list(gold)

    word_separator = Separator(None, None, ' ')
    text_words, text_stringpos, text_lex = read_data(text, word_separator)
    gold_words, gold_stringpos, gold_lex = read_data(gold, word_separator)

    if len(gold_words) != len(text_words):
        raise ValueError(
            'gold and train have different size: len(gold)={}, len(train)={}'
            .format(len(gold_words), len(text_words)))

    for i, (g, t) in enumerate(zip(gold_words, text_words)):
        if g != t:
            raise ValueError(
                'gold and train differ at line {}: gold="{}", train="{}"'
                .format(i+1, g, t))

    # token evaluation
    token_eval = TokenEvaluation()
    token_eval.update_lists(text_stringpos, gold_stringpos)

    # type evaluation
    type_eval = TypeEvaluation()
    type_eval.update_lists(text_lex, gold_lex)

    # boundary evaluation (with edges)
    boundary_eval = BoundaryEvaluation()
    boundary_eval.update_lists(text_stringpos, gold_stringpos)

    # boundary evaluation (no edges)
    boundary_noedge_eval = BoundaryNoEdgeEvaluation()
    boundary_noedge_eval.update_lists(text_stringpos, gold_stringpos)

    # return the scores in a fixed order (the default dict does not
    # repect insertion order). This is needed for python<3.6, see
    # https://docs.python.org/3.6/whatsnew/3.6.html#new-dict-implementation
    results = collections.OrderedDict((k, v) for k, v in (
        ('token_precision', token_eval.precision()),
        ('token_recall', token_eval.recall()),
        ('token_fscore', token_eval.fscore()),
        ('type_precision', type_eval.precision()),
        ('type_recall', type_eval.recall()),
        ('type_fscore', type_eval.fscore()),
        ('boundary_all_precision', boundary_eval.precision()),
        ('boundary_all_recall', boundary_eval.recall()),
        ('boundary_all_fscore', boundary_eval.fscore()),
        ('boundary_noedge_precision', boundary_noedge_eval.precision()),
        ('boundary_noedge_recall', boundary_noedge_eval.recall()),
        ('boundary_noedge_fscore', boundary_noedge_eval.fscore())))

    if units:
        labels_text = compute_class_labels(text, units)
        labels_gold = compute_class_labels(gold, units)
        with warnings.catch_warnings():
            # ignore a warning issued by sklearn
            warnings.simplefilter('ignore', category=PendingDeprecationWarning)
            results['adjusted_rand_index'] = adjusted_rand_score(
                labels_gold, labels_text)

    return results


def _load_text(text):
    """Returns a list of non-empty striped lines from `text`"""
    return [l for l in (l.strip() for l in text) if l]


def _add_arguments(parser):
    """Defines custom command-line arguments for wordseg-eval"""
    parser.add_argument(
        'gold', metavar='<gold-file>',
        help='gold file to evaluate the input data on')
    parser.add_argument(
        '-r', '--rand-index', metavar='<prep-file>', default=None,
        help='computes the adjusted rand index, requires the prepared '
        'file as outputed by wordseg-prep (i.e. the atomic units, phonemes '
        'or syllables, separated by spaces)')
    parser.add_argument(
        '-s', '--summary', metavar='<summary-file>', default=None,
        help='computes a summary of the segmentation errors as '
        'over/under/mis segmentation of word types. Write the result in '
        '<summary-file> in a JSON format. <summary-file> should have the '
        '.json extension but this is not required')


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-eval' command"""
    streamin, streamout, _, log, args = utils.prepare_main(
        name='wordseg-eval',
        description=__doc__,
        add_arguments=_add_arguments)

    log.info('loads input and gold texts')

    # load the gold text as a list of utterances, remove empty lines
    gold = _load_text(codecs.open(args.gold, 'r', encoding='utf8'))

    # load the text as a list of utterances, remove empty lines
    text = _load_text(streamin)

    # load the prepared (unsegmented) text as a list of utterances,
    # remove empty lines
    if args.rand_index:
        units = _load_text(codecs.open(args.rand_index, 'r', encoding='utf8'))
    else:
        units = None

    # evaluation returns a dict of 'score name' -> float
    log.info('evaluates the segmentation')
    results = evaluate(text, gold, units=units)

    streamout.write('\n'.join(
        # display scores with 4-digit float precision
        '{}\t{}'.format(k, '%.4g' % v if v is not None else 'None')
        for k, v in results.items()) + '\n')

    if args.summary:
        log.info('computes errors summary, writes to %s', args.summary)
        with codecs.open(args.summary, 'w', encoding='utf8') as fsummary:
            fsummary.write(json.dumps(summary(text, gold), indent=4))


if __name__ == '__main__':
    main()
