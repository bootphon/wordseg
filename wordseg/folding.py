"""Folding and unfolding texts for use in iterative word segmenters

Iterative algorithms pass through the input text only once, the model
is learned online. Thus only the end of the text is relevent for
the algorithm evaluation.

To use the whole input for evaluation, the folding module create
"folded" versions of a text to be used in iterative text based
algorithms.

Let "A B C" be a text made of three blocks A, B and C having roughly
the same number of lines. Folding that text in 3 generates a list of 3
versions ["A1 B1 C1", "C2 A2 B2", and "B3 C3 A3"]. The algorithm is
ran over the 3 versions and their outputs are then unfolded to
retrieve the original text "A3 B2 C1".

"""

import itertools
import numpy


def permute(l):
    """Pops the last element of a list an push it at beginning"""
    return [l[-1]] + l[0:-1]


def flatten(l):
    """Flattens a list of lists in a single list"""
    return list(itertools.chain(*l))


def fold_boundaries(text, nfolds):
    """Returns `nfolds` boundaries as a list of line indices in `text`

    Parameters
    ----------
    text : list
        The input text as a list of utterances.
    nfolds : int
        The number of fold boundaries to compute.

    Returns
    -------
    list
        The list of indices in `text` corresponding to the computed
        fold boundaries.

    Raises
    ------
    ValueError
        If the `text` has not enought lines to build the requested
        `nfolds`, or if `nfolds` is not strictly positive.

    """
    if nfolds < 1:
        raise ValueError(
            'nfolds must be a stricly positive interger, it is {}'
            .format(nfolds))

    if len(text) < nfolds:
        raise ValueError(
            'not enought lines in text to make {} folds'
            .format(nfolds))

    return [i * int(len(text)/nfolds) for i in range(nfolds)]


def fold(text, nfolds):
    """Create `nfolds` versions of an input `text`.

    This function reorders the blocks given from `boundaries`
    folds. In order to serve the unfold operation, this functions also
    build the index of the beginning of the last block in each fold.


    :param list text: The input text as a list of utterances

    :param list boundaries: as returned by the fold_boundaries function

    :return: a tuple (folds, index) , where index is a list of int
      and folds a list of equal length lists

    """
    if nfolds == 1:
        return [text], [0]

    # create data blocks from boundaries
    b = fold_boundaries(text, nfolds)
    blocks = [text[b[i]:b[i+1]] for i in range(len(b)-1)]
    blocks.append(text[b[-1]:])

    # build the folds from the blocks and store index of the last
    # block in each fold
    nfolds = len(b)
    folds = []
    index = []
    idx = list(range(nfolds))
    for _ in range(nfolds):
        folds += [flatten([blocks[idx[j]] for j in range(nfolds)])]
        index += [numpy.cumsum(
            [len(blocks[idx[j]]) for j in range(nfolds)])[-2]]
        idx = permute(idx)

    assert all([len(f) == len(text) for f in folds])
    return folds, index


def unfold(folds, index):
    """Concatenate the last block of each fold to form the unfolded text"""
    last_blocks = [f[index[i]:] for i, f in enumerate(folds)]
    return flatten(last_blocks[::-1])
