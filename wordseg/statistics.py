# coding: utf-8

"""Extract statistics relevant for word segmenation corpora"""

import collections
import json
from math import log

from wordseg import utils
from wordseg.separator import Separator


class CorpusStatistics(object):
    """Estimates descriptive statistics from a text corpus

    Parameters
    ----------
    corpus : sequence of str
        The text to describe is a suite of tokenized utterances.
    separator : Separator
        The token separators used in the `text`.
    log : logging.Logger
        Where to send log messages, disabled by default.

    Attributes
    ----------
    tokens : dict
        For all levels defined in `separator`, tokens[level] is the
        `corpus` utterances tokenized at that level. Each utterance is
        a list of tokens without any separator.
    unigram : dict
        For all levels defined in `separator`, unigram[level] is the
        tokens frequency as a dict (token: frequency).

    """
    def __init__(self, corpus, separator, log=utils.null_logger()):
        self.log = log

        # check the separator have words and possibly phones
        self.separator = separator
        if not self.separator.word:
            raise ValueError('word separator not defined')
        if not self.separator.phone:
            log.warning('phone separator not defined, some stats ignored')
        self.log.info('token separator is %s', self.separator)

        # force to list and ignore empty lines
        self.corpus = [
            utt for utt in (utt.strip() for utt in corpus) if len(utt)]
        self.log.info('loaded %s utterances', len(self.corpus))
        if len(self.corpus) == 0:
            raise ValueError('no text to load')

        # tokenize the entire text at each defined level ('word',
        # 'syllable' and/or 'phone') TODO can be optimized we are
        # tokenizing the entire text up to 3 times (implement nested
        # tokenization).
        self.tokens = {}
        for level in self.separator.levels()[::-1]:
            self.log.debug('tokenizing %s', level)
            self.tokens[level] = [
                self.separator.tokenize(utt, level, keep_boundaries=False)
                for utt in self.corpus]

            ntokens = sum(len(t) for t in self.tokens[level])
            self.log.info('parsed %s %ss', ntokens, level)
            if ntokens == 0:
                raise ValueError('{}s expected but 0 parsed'.format(level))

        # estimates token frequencies
        self.unigram = {}
        for level in self.separator.levels()[::-1]:
            self.unigram[level] = self._unigram(level)

    def _mattr(self, level, size=10):
        """Return the mean ratio of unique tokens per chunk of `size`"""
        # the list of all the tokens
        tokens = [w for u in self.tokens[level] for w in u]

        # ratio of uniques words per chunk of `size` words
        nuniques = [float(len(set(tokens[x:x + size]))) / size
                    for x in range(len(tokens) - size)]

        return float(sum(nuniques)) / len(nuniques)

    def _unigram(self, level):
        """Return dictionary of (token: frequency) items"""
        count = self.most_common_tokens(level)

        self.log.info('5 most common {}s: {}'.format(
            level, [t for t, c in count[:5]]))

        total_count = float(sum(c[1] for c in count))
        return {c[0]: c[1] / total_count for c in count}

    def describe_all(self):
        """Full description of the corpus at utterance and token levels

        This method is a simple wrapper on the other statistical
        methods. It call all the methods available for the defined
        separator (some of them requires 'phone' tokens) and wraps the
        results in a dictionary.

        """
        # store the output statistics in a dictionary
        results = {}

        # corpus description at utterance level
        results['corpus'] = self.describe_corpus()

        # if phone are defined, compute the corpus entropy
        if self.separator.phone:
            results['corpus']['entropy'] \
                = self.normalized_segmentation_entropy()

        # for each defined token level (from word to phone),
        # describe the corpus at that level
        for level in self.separator.levels()[::-1]:
            results[level + 's'] = self.describe_tokens(level)

        return results

    def describe_corpus(self):
        """Basic description of the corpus at word level

        Returns
        -------
        stats : dict
            A dictionnary made of the following entries (all counts
            being on the entire corpus):

            - 'nutts': number of utterances
            - 'nutts_single_word': number of utterances made of a single world
            - 'nword_tokens': number of word tokens
            - 'nword_types': number of types
            - 'nword_hapax': number of word types with a frequency of 1 (hapax)
            - 'mattr': mean ratio of unique words per chunk of 10 words

        Notes
        -----
        This method is a Python implementation of `this script`_ from
        CDSWordSeg.

        .. _this script: https://github.com/alecristia/CDSwordSeg/blob/master/
           recipes/CatalanSpanish/_describe_gold.sh

        """
        # length of utterances in number of words
        wlen = [len(utt) for utt in self.tokens['word']]

        stats = {
            # number of utterances
            'nutts': len(self.corpus),
            # number of single word utterances
            'nutts_single_word': wlen.count(1),
            # number of word tokens
            'nword_tokens': sum(wlen),
            # number of word types
            'nword_types': len(self.unigram['word']),
            # number of word types with a frequency of 1 (hapax)
            'nword_hapax': list(
                self.unigram['word'].values()).count(1 / float(sum(wlen))),
            # mean ratio of unique words per chunk of 10 words
            'mattr': self._mattr('word', size=10)
        }

        return stats

    def describe_tokens(self, level):
        """Basic description of the corpus at tokens level

        Parameters
        ----------
        level : str
            The tokens level to describe. Must be 'phone', 'syllable'
            or 'word'.

        Returns
        -------
        stats : dict
            A dictionnary made of the following entries (all counts
            being on the entire corpus):

            - 'tokens': number of tokens
            - 'tokens/utt': mean number of tokens per utterance
            - 'tokens/LEVEL': mean number of tokens per upper level token
            - 'types': number of types
            - 'tokens/type': mean number of token per types
            - 'uniques': number of types occuring only once in the corpus

        """
        stats = {}

        # length of utterances in number of words
        tokens_len = [len(utt) for utt in self.tokens[level]]

        # number of tokens
        stats['tokens'] = sum(tokens_len)

        # mean number of tokens per utterance, word and syllable
        stats['tokens/utt'] = stats['tokens'] / float(len(self.corpus))

        for l in self.separator.upper_levels(level):
            nupper = float(sum(len(utt) for utt in self.tokens[l]))
            stats['tokens/{}'.format(l)] = stats['tokens'] / nupper

        # types
        types_count = self.most_common_tokens(level)

        # number of types
        stats['types'] = len(types_count)

        # mean number of token per types
        stats['token/types'] = float(stats['tokens']) / stats['types']

        # number of types occuring only once in the corpus
        stats['uniques'] = len([k for k, v in types_count if v == 1])

        return stats

    def most_common_tokens(self, level, n=None):
        """Return the most common tokens and their count

        Parameters
        ----------
        level : str
            Must be 'phone', 'syllable' or word'.
        n : int, optional
            When specified returns only the `n` most commons tokens,
            when omitted or None returns all the tokens.

        Returns
        -------
        counts : list
           The list of (token, count) values sorted in decreasing
           count order.

        """
        return collections.Counter(
            (t for utt in self.tokens[level] for t in utt)).most_common(n)

    def normalized_segmentation_entropy(self):
        """Return the Normalized Segmentation Entropy computed on `text`

        Token separators must be defined for phones and words.

        Returns
        -------
        entropy : float
            The estimated NSE in bits.

        Raises
        ------
        KeyError if the corpus is not tokenized at 'phone' and 'word' levels.

        Notes
        -----
        As explained in [1]_ we are interested in the ambiguity generated
        by the different possible parses that result from a
        segmentation. In order to quantify this idea in general, we define
        a Normalized Segmentation Entropy. To do this, we need to assign a
        probability to every possible segmentation. To this end, we use a
        unigram model where the probability of a lexical item is its
        normalized frequency in the corpus and the probability of a parse
        is the product of the probabilities of its terms. In order to
        obtain a measure that does not depend on the utterance length, we
        normalize by the number of possible boundaries in the
        utterance. So for an utterance of length N, the Normalized
        Segmentation Entropy (NSE) is computed using Shannon formula
        (Shannon, 1948) as follows:

        .. math::

            NSE = -\sum_i P_ilog_2(P_i) / (N-1),

        where :math:`P_i` is the probability of the word :math:`i` and
        :math:`N` the number of phonemes in the text.

        .. [1] A. Fourtassi, B. Börschinger, M. Johnson and E. Dupoux,
           "Whyisenglishsoeasytosegment". In Proceedings of the Fourth Annual
           Workshop on Cognitive Modeling and Computational Linguistics
           (pp. 1-10), 2013.

        """
        # count the number of phones in the text
        N = sum(len(utt) for utt in self.tokens['phone'])

        # word lexicon with probabilities
        P = self.unigram['word']

        # the probability of each word in the text
        probs = (P[word] for utt in self.tokens['word'] for word in utt)

        # compute the entropy
        return -1 * sum(p * log(p, 2) / float(N - 1) for p in probs)


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-stats' command"""
    # options description
    def add_arguments(parser):
        parser.add_argument(
            '--json', action='store_true',
            help='print the results in JSON format, else print in raw text')

    # command initialization
    streamin, streamout, separator, log, args = utils.prepare_main(
        name='wordseg-stats',
        description=__doc__,
        add_arguments=add_arguments,
        separator=Separator())

    # compute the statistics
    stats = CorpusStatistics(streamin, separator, log=log)
    results = stats.describe_all()

    # display the results either as a JSON string or in raw text
    if args.json:
        streamout.write((json.dumps(results, indent=4)) + '\n')
    else:
        out = (' '.join((name, k, str(v)))
               for name, stats in results.items()
               for k, v in stats.items())
        streamout.write('\n'.join(out) + '\n')


if __name__ == '__main__':
    main()
