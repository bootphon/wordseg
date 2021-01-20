"""Test of the wordseg.algos.puddle module"""

import codecs
import copy
import os
import pytest

from wordseg.separator import Separator
from wordseg.prepare import gold, prepare
from wordseg.evaluate import evaluate
from wordseg.algos import puddle


@pytest.mark.parametrize(
    'window, nfolds, njobs, by_frequency',
    [(w, f, j, b)
     for w in (1, 3) for f in (1, 3) for j in (3, 5) for b in (True, False)])
def test_puddle(prep, window, nfolds, njobs, by_frequency):
    out = list(puddle.segment(
        prep, window=window, by_frequency=by_frequency,
        nfolds=nfolds, njobs=njobs))
    s = Separator().remove

    assert len(out) == len(prep)
    for n, (a, b) in enumerate(zip(out, prep)):
        assert s(a) == s(b), 'line {}: "{}" != "{}"'.format(n+1, s(a), s(b))


def test_empty_line(prep):
    with pytest.raises(ValueError) as err:
        puddle.segment(prep[:2] + [''] + prep[4:])
    assert 'utterance is empty' in str(err)


def test_replicate(datadir):
    sep = Separator()

    _tags = [utt for utt in codecs.open(
        os.path.join(datadir, 'tagged.txt'), 'r', encoding='utf8')
            if utt][:100]  # 100 first lines only
    _prepared = prepare(_tags, separator=sep)
    _gold = gold(_tags, separator=sep)

    segmented = puddle.segment(_prepared, nfolds=1)
    score = evaluate(segmented, _gold)

    # we obtained that score from the dibs version in CDSWordSeg
    # (using wordseg.prepare and wordseg.evaluate in both cases)
    expected = {
        'type_fscore': 0.06369,
        'type_precision': 0.1075,
        'type_recall': 0.04525,
        'token_fscore': 0.06295,
        'token_precision': 0.2056,
        'token_recall': 0.03716,
        'boundary_all_fscore': 0.4605,
        'boundary_all_precision': 1.0,
        'boundary_all_recall': 0.2991,
        'boundary_noedge_fscore': 0.02806,
        'boundary_noedge_precision': 1.0,
        'boundary_noedge_recall': 0.01423}

    assert score == pytest.approx(expected, rel=1e-3)


def test_train_text(prep):
    train_text = prep[:10]
    test_text = prep[10:]

    # offline learning on train_text
    segmented1 = list(puddle.segment(test_text, train_text=train_text))

    # online learning
    segmented2 = list(puddle.segment(test_text, nfolds=1))

    def join(s):
        return ''.join(s).replace(' ', '')

    assert len(test_text) == len(segmented1) == len(segmented2)
    assert join(test_text) == join(segmented1) == join(segmented2)


def test_segment_only(prep):
    train_text = prep[:10]
    test_text = prep[10:]

    # train a model on train_text
    model = puddle.Puddle()
    model.train(train_text)
    model_backup = copy.deepcopy(model)

    # ensure the model is not updated during segmentation
    segmented = list(model.segment(test_text, update_model=False))
    assert len(segmented) == len(test_text)
    assert model == model_backup

    # ensure the model is updated during segmentation
    segmented = list(model.segment(test_text, update_model=True))
    assert len(segmented) == len(test_text)
    assert model != model_backup
