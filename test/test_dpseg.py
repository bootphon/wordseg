# Copyright 2017 Mathieu Bernard, Elin Larsen
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

"""Test of the 'wordseg-dpseg' command"""

import os
import pytest

import wordseg
from wordseg import utils, Separator
from wordseg.algos.dpseg import segment
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
    confs = wordseg.algos.dpseg.get_dpseg_conf_files()
    assert len(confs) > 0
    for conf in confs:
        assert os.path.isfile(conf)
        assert conf[-5:] == '.conf'
        assert 'dpseg' in conf


@pytest.mark.parametrize('conf', wordseg.algos.dpseg.get_dpseg_conf_files())
def test_dpseg_from_config_file(prep, conf):
    segmented = segment(prep[:5], nfolds=1, args='--config-file {}'.format(conf))
    assert len(list(segmented)) == 5
