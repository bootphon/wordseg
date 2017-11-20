"""Basic test of the 'wordseg-* --help' commands"""

import pytest
import subprocess


@pytest.mark.parametrize(
    'cmd', ['prep', 'eval', 'stats', 'syll',
            'ag', 'dibs', 'dpseg', 'tp', 'puddle'])
def test_command_help(cmd):
    command = 'wordseg-' + cmd
    subprocess.check_call([command, '--help'])
