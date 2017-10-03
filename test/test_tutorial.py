"""Test of the doc/tutorial.{py, sh} scripts"""

import os
import pytest
import subprocess
import tempfile

from . import tags


@pytest.mark.parametrize('ext', ['py', 'sh'])
def test_tutorial(tags, tmpdir, ext):
    tutorial_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'doc'))
    assert os.path.isdir(tutorial_dir)

    script = os.path.join(tutorial_dir, 'tutorial.' + ext)
    assert os.path.isfile(script)

    p = tmpdir.join('input.txt')
    p.write('\n'.join(tags) + '\n')
    subprocess.run([script, str(p)], check=True)
