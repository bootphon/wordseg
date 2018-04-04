"""Test of the 'wordseg-dpseg' command"""

import codecs
import os
import pytest

import wordseg
from wordseg import utils
from wordseg.separator import Separator
from wordseg.prepare import gold, prepare
from wordseg.evaluate import evaluate
from wordseg.algos.dpseg import segment, _dpseg_bugfix
from . import prep, datadir


args = [
    '',
    '--ngram 1 --a1 0 --b1 1  --a2 0 -- b2 1',
    '--ngram 1 --a1 0.1 --b1 0.9 --do-mbdp 1',
    '--ngram 1 --a1 0 --b1 1 --forget-method P',
    '--ngram 1 --a1 0 --b1 1 --estimator V --mode batch',
    '--ngram 1 --a1 0 --b1 1 --estimator F',
    '--ngram 1 --a1 0 --b1 1 --estimator T',
    utils.strip('''
    --ngram 1 --a1 0 --b1 1 --estimator D --mode online --eval-maximize 1
    --eval-interval 50 --decay-rate 1.5 --samples-per-utt 20
    --hypersamp-ratio 0 --anneal-a 10 --burnin-iterations 1
    --anneal-iterations 0'''.replace('\n', ' ')),
]


@pytest.mark.parametrize('nfolds, njobs', [(1, 1), (1, 4), (2, 2), (10, 1)])
def test_dpseg_parallel_folds(prep, nfolds, njobs):
    text = prep[:5]
    if nfolds > 5:
        with pytest.raises(ValueError):
            segment(text, nfolds=nfolds, njobs=njobs)
    else:
        assert len(list(segment(text, nfolds=nfolds, njobs=njobs))) == 5


@pytest.mark.parametrize('args', args)
def test_dpseg_args(prep, args):
    segmented = segment(prep[:5], nfolds=1, args=args)
    assert len(list(segmented)) == 5


def test_config_files_are_here():
    confs = wordseg.utils.get_config_files('dpseg')
    assert len(confs) > 0
    for conf in confs:
        assert os.path.isfile(conf)
        assert conf[-5:] == '.conf'
        assert 'dpseg' in conf


# skip the bi_ideal config because it is time consuming
@pytest.mark.parametrize(
    'conf', (f for f in wordseg.utils.get_config_files('dpseg')
             if 'bi_ideal' not in f))
def test_dpseg_from_config_file(prep, conf):
    segmented = segment(
        prep[:5], nfolds=1, args='--config-file {}'.format(conf))
    assert len(list(segmented)) == 5


def test_dpseg_bugfix(prep):
    with pytest.raises(ValueError):
        _dpseg_bugfix(['.', '.', '.', '.'], [0, 2])

    with pytest.raises(ValueError):
        _dpseg_bugfix(['..', '.', '.', '.'], [0, 2])

    assert _dpseg_bugfix(['..', '.', '..', '.'], [0, 2]) == [0, 2]
    assert _dpseg_bugfix(['..', '.', '.', '..'], [0, 2]) == [0, 3]

    with pytest.raises(ValueError):
        _dpseg_bugfix(['..', '.', '.', '..'], [0, 1, 2])

    assert _dpseg_bugfix(['..', '..', '.', '..'], [0, 1, 2]) == [0, 1, 3]


def test_replicate_cds_wordseg(datadir):
    sep = Separator()

    # only the last 10 lines, for a fast test. We cannot take the 10
    # first lines because they cause the dpseg_bugfix to correct a
    # fold (the implementation of that fix differs in CDS and wordseg,
    # so the results are not replicated exactly)
    _tags = [utt for utt in codecs.open(
        os.path.join(datadir, 'tagged.txt'), 'r', encoding='utf8')
            if utt][-10:]

    _prepared = prepare(_tags, separator=sep, unit='syllable')
    _gold = gold(_tags, separator=sep)

    uni_dmcmc_conf = [c for c in wordseg.utils.get_config_files('dpseg')
                      if 'uni_dmcmc' in c][0]
    args = '--ngram 1 --a1 0 --b1 1 -C {}'.format(uni_dmcmc_conf)
    segmented = segment(_prepared, nfolds=5, njobs=4, args=args)
    score = evaluate(segmented, _gold)

    # we obtained that scores from the dpseg version in CDSWordSeg
    expected = {
        'type_fscore': 0.3768,
        'type_precision': 0.3939,
        'type_recall': 0.3611,
        'token_fscore': 0.3836,
        'token_precision': 0.4118,
        'token_recall': 0.359,
        'boundary_all_fscore': 0.7957,
        'boundary_all_precision': 0.8409,
        'boundary_all_recall': 0.7551,
        'boundary_noedge_fscore': 0.6415,
        'boundary_noedge_precision': 0.7083,
        'boundary_noedge_recall': 0.5862}

    assert score == pytest.approx(expected, rel=1e-3)
