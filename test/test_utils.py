"""Test of the wordseg.utils module"""

import pytest
import wordseg.utils


# CountingIterator

@pytest.mark.parametrize('sequence', [
    range(1, 11),
    [1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
    (1, 2, 3, 4, 5, 6, 7, 8, 9, 10),
    (c for c in range(1, 11)),
    {c: str(c) for c in range(1, 11)}])
def test_counting_iterator_int(sequence):
    counter = wordseg.utils.CountingIterator(sequence)
    assert sum(counter) == 55
    assert counter.count == 10


def test_counting_iterator_str():
    counter = wordseg.utils.CountingIterator('sequence')
    assert counter.count == 0
    for c in counter:
        pass
    assert counter.count == 8


@pytest.mark.parametrize('not_sequence', [1, None])
def test_counting_iterator_bad(not_sequence):
    with pytest.raises(TypeError):
        wordseg.utils.CountingIterator(not_sequence)


# get_binary

@pytest.mark.parametrize('binary', ['ag', 'dpseg'])
def test_get_binary_good(binary):
    # raise RuntimeError on error
    wordseg.utils.get_binary(binary)


@pytest.mark.parametrize('binary', ['AG', 'Dpseg', 'puddle'])
def test_get_binary_bad(binary):
    with pytest.raises(RuntimeError):
        wordseg.utils.get_binary(binary)


# get_config_files

@pytest.mark.parametrize('binary', ['ag', 'dpseg'])
def test_get_config_file_good(binary):
    # raise RuntimeError on error
    wordseg.utils.get_config_files(binary)


@pytest.mark.parametrize('binary', ['puddle', 'tp', 'dibs'])
def test_get_config_file_bad(binary):
    with pytest.raises(RuntimeError):
        wordseg.utils.get_config_files(binary)


# strip

@pytest.mark.parametrize('string', [' ab  c ', 'ab\nc', ' \t\tab\n c '])
def test_strip(string):
    assert wordseg.utils.strip(string) == 'ab c'
