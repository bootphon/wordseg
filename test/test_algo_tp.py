"""Test of the wordseg.algos.tp module"""

import codecs
import os
import pytest

from wordseg.separator import Separator
from wordseg.prepare import gold, prepare
from wordseg.evaluate import evaluate
from wordseg.algos import tp


@pytest.mark.parametrize(
    'threshold, dependency',
    [(t, p) for t in ('relative', 'absolute')
     for p in ('ftp', 'btp', 'mi')])
def test_tp(prep, threshold, dependency):
    """Check input and output are the same, once the separators are removed"""
    out = list(tp.segment(prep, threshold=threshold, dependency=dependency))
    s = Separator().remove

    assert len(out) == len(prep)
    for n, (a, b) in enumerate(zip(out, prep)):
        assert s(a) == s(b), 'line {}: "{}" != "{}"'.format(n+1, s(a), s(b))

#hello world test
def test_hello_world():
    test_text = ['hh ax l ow w er l d']

    assert list(tp.segment(
        test_text, threshold='absolute', dependency='ftp')) \
        == ['hhaxl owwerl d']

    assert list(tp.segment(
        test_text, threshold='relative', dependency='ftp')) \
        == ['hhaxl owwerld']

    assert list(tp.segment(
        test_text, threshold='absolute', dependency='btp')) \
        == ['hhax lowwer ld']

    assert list(
        tp.segment(test_text, threshold='relative', dependency='btp')) \
        == ['hhax lowwer ld']

# was a bug on tp relative when the last utterance of the text
# contains a single phone

def test_last_utt_relative():
    test_text = ['waed yuw waant naw', 'buwp']
    expected = ['waedyuwwaantnaw', 'buwp']
    assert expected == list(tp.segment(test_text, threshold='relative'))

def test_replicate(datadir):
    sep = Separator()

    _tags = [utt for utt in codecs.open(
        os.path.join(datadir, 'tagged.txt'), 'r', encoding='utf8')
            if utt][:100]  # 100 first lines only
    _prepared = prepare(_tags, separator=sep)
    _gold = gold(_tags, separator=sep)

    segmented = tp.segment(_prepared)#add the train_text
    score = evaluate(segmented, _gold)

    # we obtained that score from the dibs version in CDSWordSeg
    # (using wordseg.prepare and wordseg.evaluate in both cases)
    expected = {
        'type_fscore': 0.304,
        'type_precision': 0.2554,
        'type_recall': 0.3756,
        'token_fscore': 0.3994,
        'token_precision': 0.3674,
        'token_recall': 0.4375,
        'boundary_all_fscore': 0.7174,
        'boundary_all_precision': 0.6671,
        'boundary_all_recall': 0.776,
        'boundary_noedge_fscore': 0.6144,
        'boundary_noedge_precision': 0.557,
        'boundary_noedge_recall': 0.685}

    assert score == pytest.approx(expected, rel=1e-3)



#Test with train_text is None
def test_train_text_None():
    train_text = None
    test_text = ['hh ax l ow w er l d']

    assert list(tp.segment(
        test_text,train_text=None, threshold='absolute', dependency='ftp')) \
        == ['hhaxl owwerl d']

    assert list(tp.segment(
        test_text,train_text=None, threshold='relative', dependency='ftp')) \
        == ['hhaxl owwerld']

    assert list(tp.segment(
        test_text,train_text=None, threshold='absolute', dependency='btp')) \
        == ['hhax lowwer ld']

    assert list(
        tp.segment(test_text, train_text=None,threshold='relative', dependency='btp')) \
        == ['hhax lowwer ld']

'''
#Test with train_text=test_text
def test_traintext_equal_testtext():
    train_text = ['hh ax l ow w er l d']
    test_text = ['hh ax l ow w er l d']

    assert list(tp.segment(
        test_text,train_text, threshold='absolute', dependency='ftp')) \
        == ['hh ax l ow w er l d']

    assert list(tp.segment(
        test_text,train_text, threshold='relative', dependency='ftp')) \
        == ['hhaxl owwerld']

    assert list(tp.segment(
        test_text,train_text, threshold='absolute', dependency='btp')) \
        == ['hhax lowwer ld']

    assert list(
        tp.segment(test_text,train_text, threshold='relative', dependency='btp')) \
        == ['hhax lowwer ld']

#partiel test
def test_traintext_notequal_testtext():
    train_text = ['hh ax l ow w er l d']#good morning
    test_text = ['g uh d ;eword m ao r n ax ng ;eword']#hellow world

    #it crash
    assert list(tp.segment(
        test_text,train_text, threshold='absolute', dependency='ftp')) \
        == ['g uh d ;eword m ao r n ax ng ;eword']

    assert list(tp.segment(
        test_text,train_text, threshold='relative', dependency='ftp')) \
        == ['hhaxl owwerld']

    assert list(tp.segment(
        test_text,train_text, threshold='absolute', dependency='btp')) \
        == ['hhax lowwer ld']

    assert list(
        tp.segment(test_text,train_text, threshold='relative', dependency='btp')) \
        == ['hhax lowwer ld']

#Test with train_text!=test_text
def test_partial():
    train_text = ['hh ax l ow hh ax l ow ']#hellow world
    test_text = ['hh ax l ow hh ax l ow']#hellow hellow


    assert list(tp.segment(
        test_text,train_text, threshold='absolute', dependency='ftp')) \
        == ['hh ax l ow hh ax l ow']

    assert list(tp.segment(
        test_text,train_text, threshold='relative', dependency='ftp')) \
        == ['hhaxl owhhaxlow']

    assert list(tp.segment(
        test_text,train_text, threshold='absolute', dependency='btp')) \
        == ['hhax lowhhaxl ow']

    assert list(
        tp.segment(test_text,train_text, threshold='relative', dependency='btp')) \
        == ['hhax lowhhaxl ow']
'''