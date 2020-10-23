# coding: utf-8

"""Test of the segmentation pipeline from raw text to eval"""

import codecs
import os
import pytest

import wordseg.evaluate
import wordseg.prepare
import wordseg.algos.ag
import wordseg.algos.baseline
import wordseg.algos.dibs
import wordseg.algos.dpseg
import wordseg.algos.puddle
import wordseg.algos.tp

from wordseg.separator import Separator


ALGOS = {
    'baseline': wordseg.algos.baseline,
    'ag': wordseg.algos.ag,
    'dibs': wordseg.algos.dibs,
    'dpseg': wordseg.algos.dpseg,
    'puddle': wordseg.algos.puddle,
    'tp': wordseg.algos.tp}

PARAMS = [(a, e) for a in ALGOS.keys() for e in ('ascii', 'unicode')]


def add_unicode(lines):
    return [
        line.replace('ih', u'ଖ').replace('eh', u'ࠇ').replace('hh', u'ლ')
        for line in lines]


@pytest.mark.parametrize('algo, encoding', PARAMS)
def test_pipeline(algo, encoding, tags, tmpdir):
    # the token separator we use in the whole pipeline
    separator = Separator(phone=' ', syllable=';esyll', word=';eword')

    # add some unicode chars in the input text
    if encoding == 'unicode':
        tags = add_unicode(tags)

    # build the gold version from the tags
    gold = list(wordseg.prepare.gold(tags, separator=separator))
    assert len(gold) == len(tags)
    for a, b in zip(gold, tags):
        assert separator.remove(a) == separator.remove(b)

    # prepare the text for segmentation
    prepared_text = list(wordseg.prepare.prepare(tags, separator=separator))
    assert len(prepared_text) == len(tags)
    for a, b in zip(prepared_text, tags):
        assert separator.remove(a) == separator.remove(b)

    # segment it with the given algo (use default options)
    if algo in ('dpseg', 'puddle'):
        # only 1 fold for iterative algos: faster
        segmented = list(ALGOS[algo].segment(prepared_text, nfolds=1))
    elif algo == 'ag':
        # add grammar related arguments, if in unicode test adapt the
        # grammar too
        grammar_file = os.path.join(
            os.path.dirname(wordseg.algos.ag.get_grammar_files()[0]),
            'Colloc0_enFestival.lt')
        if encoding == 'unicode':
            grammar_unicode = add_unicode(
                codecs.open(grammar_file, 'r', encoding='utf8'))
            grammar_file = os.path.join(str(tmpdir), 'grammar.lt')
            codecs.open(grammar_file, 'w', encoding='utf8').write(
                '\n'.join(grammar_unicode))
        segmented = list(ALGOS[algo].segment(
            # we just use 10 iterations here to be fast
            prepared_text, grammar_file=grammar_file, category='Colloc0',
            nruns=1, args='-n 10'))
    elif algo == 'dibs':
        # dibs need training, train of test set for the test
        dibs_model = wordseg.algos.dibs.CorpusSummary(tags, separator=separator)
        segmented = list(ALGOS[algo].segment(prepared_text, dibs_model))
    else:
        segmented = list(ALGOS[algo].segment(prepared_text))

    s = separator.remove
    assert len(segmented) == len(tags)
    for n, (a, b) in enumerate(zip(segmented, tags)):
        assert s(a) == s(b), 'line {}: "{}" != "{}"'.format(n+1, s(a), s(b))

    results = wordseg.evaluate.evaluate(segmented, gold)
    assert len(results.keys()) % 3 == 0
    for v in results.values():
        if v is not None:
            assert v >= 0
            assert v <= 1
