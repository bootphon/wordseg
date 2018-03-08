"""Test of the 'wordseg-dpseg' command"""

import os
import pytest

import wordseg
from wordseg import utils
from wordseg.separator import Separator
from wordseg.algos.dpseg import segment, _dpseg_bugfix
from . import prep


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
