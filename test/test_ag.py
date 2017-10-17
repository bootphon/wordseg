"""Test of the wordseg.algos.ag module"""

import codecs
import os
import pytest

from wordseg.algos import ag
from . import prep


test_arguments = (
    '-E -P -R -1 -n 10 -a 0.0001 -b 10000.0 '
    '-e 1.0 -f 1.0 -g 100.0 -h 0.01 -x 2')

grammar_dir = os.path.dirname(ag.get_grammar_files()[0])
grammars = [('Coll3syllfnc_enFestival.lt', 'Word'),
            ('Colloc0_enFestival.lt', 'Colloc0')]


def test_grammar_files():
    assert len(ag.get_grammar_files()) != 0


def test_ag_single_run(prep):
    raw_parses = ag._run_ag_single(
        prep, os.path.join(grammar_dir, grammars[1][0]), args=test_arguments)
    trees = [tree for tree in ag._yield_trees(raw_parses)]

    assert len(trees) == 6
    for tree in trees:
        assert len(tree) == len(prep)


@pytest.mark.parametrize('grammar, level', grammars)
def test_grammars(prep, grammar, level):
    grammar = os.path.join(grammar_dir, grammar)
    segmented = ag.segment(prep, grammar, level, test_arguments, nruns=1)
    assert len(segmented) == len(prep)

    segmented = ''.join(utt.replace(' ', '').strip() for utt in segmented)
    prep = ''.join(utt.replace(' ', '').strip() for utt in prep)
    assert segmented == prep


def test_mark_jonhson(tmpdir):
    # this is a transcription of the original "toy run" delivered with
    # the original AG code (as a target in the Makefile)
    data_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), 'data'))
    assert os.path.isdir(data_dir)

    grammar_file = os.path.join(data_dir, 'ag_testengger.lt')
    text = codecs.open(os.path.join(data_dir, 'ag_testeng.yld'), 'r', encoding='utf8')
    arguments = (
        '-r 1234 -P -D -R -1 -d 100 -a 1e-2 -b 1 -e 1 -f 1 '
        '-g 1e2 -h 1e-2 -n 10 -C -E -A {prs} -N 10 '
        '-F {trace} -G {wlt} -X "cat > {X1}" -X "cat > {X2}" '
        '-u {testeng1} -U "cat > {prs1}" -v {testeng2} '
        '-V "cat > {prs2}"'.format(
            testeng1=os.path.join(data_dir, 'ag_testeng1.yld'),
            testeng2=os.path.join(data_dir, 'ag_testeng2.yld'),
            trace=tmpdir.join('trace'), wlt=tmpdir.join('wlt'),
            X1=tmpdir.join('X1'), X2=tmpdir.join('X2'), prs=tmpdir.join('prs'),
            prs1=tmpdir.join('prs1'), prs2=tmpdir.join('prs2')))
    ag._run_ag_single(text, grammar_file, args=arguments)
