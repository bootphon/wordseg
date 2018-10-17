"""Test of the doc/tutorial.{py, sh} scripts"""

import codecs
import os
import subprocess

import pytest


@pytest.mark.parametrize('ext', ['py', 'sh'])
def test_tutorial(tags, tmpdir, ext):
    tutorial_dir = os.path.abspath(
        os.path.join(os.path.dirname(__file__), '..', 'doc'))
    assert os.path.isdir(tutorial_dir)

    script = os.path.join(tutorial_dir, 'tutorial.' + ext)
    assert os.path.isfile(script)

    p = os.path.join(str(tmpdir), 'input.txt')
    codecs.open(p, 'w', encoding='utf8').write('\n'.join(tags) + '\n')

    subprocess.check_call([script, p], cwd=str(tmpdir))
