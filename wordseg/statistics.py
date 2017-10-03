"""Extract statistics about gold or segmented text"""

import collections
import pandas as pd

from wordseg import utils
from wordseg.separator import Separator


default_separator = Separator(phone=None, syllable=None, word=' ')
"""Default separator when processing gold text

Only word tokens, separated by spaces.

"""


def stat_corpus(text, separator=default_separator):
    """Return basis descriptive statistics of a text corpus

    :param sequence(str) text: the input sequence to process, each
      string in the sequence is an utterance

    :param Separator separator: token separation in the `text`

    :return: A pandas.DataFrame containing the number of tokens, types
      and utterances in the input `text`

    :todo: add average word length in number of syllables, average
       word length in number of phonemes

    """
    # force the text as a list if it is a generator
    text = list(text)

    list_of_words = []
    dict_of_types = collections.Counter()
    for line in text:
        for word in separator.split(line, 'word'):
            list_of_words.append(word)
            dict_of_types.update(word)

    df = pd.DataFrame(
        index=['stat'],
        columns=['number_tokens', 'number_types', 'number_utterances'])

    df.number_tokens = len(list_of_words)
    df.number_types = len(dict_of_types)
    df.number_utterance = len(text)

    return df


def top_frequency_tokens(text, n=1000, separator=default_separator, level='word'):
    """Return the `n` most common tokens in `text`

    :param list(str) text: the nput text split in utterances

    :param int n: the most common tokens to return

    :param Separator separator: tokens separation in `text`

    :param str level: the token level to consider, must be 'word',
        syllable' or 'phoneme'.

    :return: a list of `n` (token, count) pairs of most common tokens in `text`

    """
    tokens = (token for line in text for token
              in separator.split(line.strip(), level))
    return collections.Counter(tokens).most_common(n)


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-stats' command"""
    # command initialization
    streamin, streamout, separator, log, args = utils.prepare_main(
        name='wordseg-stats',
        description=__doc__,
        separator=default_separator)

    top = top_frequency_tokens(streamin, separator=separator)
    streamout.write(
        '{} top frequency tokens:\n'.format(len(top))
        + '\n'.join('{} {}'.format(t[0], t[1]) for t in top)
        + '\n')

    stat = stat_corpus(streamin, separator=separator)
    streamout.write(stat)


if __name__ == '__main__':
    main()
