"""Test we have the same results from bash and python

We had a bug using python 3.5 on either Linux or Mac, where the bash
version of algorithms gave a slitly different result than the one
expected from Python. Here we make sure python and bash calls to
wordseg lead to exactly the same result.

"""

import os
import pytest
import subprocess

params = [(a, u)
          for a in ['tp', 'dibs', 'puddle']
          for u in ['phone', 'syllable']]

@pytest.mark.parametrize('algo, unit', params)
def test_bash_python(algo, unit):
    # the bash script to execute is test_bash_python.sh
    test_script = os.path.abspath(__file__).replace('.py', '.sh')

    # run it as a subprocess, res is a list of token f-score obtained
    # from bash and python
    res = subprocess.run(
        [test_script, algo, unit],
        check=True,
        stdout=subprocess.PIPE,
        encoding='utf8').stdout.strip().split()

    # make sure all the scores are equal
    assert len(set(res)) == 1
