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

import pytest

from wordseg import utils, Separator
from wordseg.algos.dpseg import segment
from . import prep


args = [
    '',
    '--ngram 1 --a1 0 --b1 1',
    '--ngram 1 --a1 0.1 --b1 0.9 --do-mbdp 1',
    '--ngram 1 --a1 0 --b1 1 --forget-method P',
    '--ngram 1 --a1 0 --b1 1 --estimator V --mode batch',
    '--ngram 1 --a1 0 --b1 1 --estimator F',
    '--ngram 1 --a1 0 --b1 1 --estimator T',
    # '--ngram 1 --a1 0 --b1 1 --estimator D --mode online',
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
    text = prep[:5]

    segmented = segment(text, nfolds=1, args=args)
    assert len(list(segmented)) == 5
