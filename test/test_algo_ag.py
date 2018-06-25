"""Test of the wordseg.algos.ag module"""

import codecs
import os
import itertools
import joblib
import pytest

from wordseg.algos import ag
from wordseg.separator import Separator
from wordseg.prepare import gold, prepare
from wordseg.evaluate import evaluate
from wordseg import utils
from . import prep, datadir


test_arguments = (
    '-E -P -R -1 -n 10 -a 0.0001 -b 10000.0 '
    '-e 1.0 -f 1.0 -g 100.0 -h 0.01 -x 2')

grammar_dir = os.path.dirname(ag.get_grammar_files()[0])
grammars = [('Coll3syllfnc_enFestival.lt', 'Word'),
            ('Colloc0_enFestival.lt', 'Colloc0')]


def test_grammar_files():
    assert len(ag.get_grammar_files()) != 0


def test_check_grammar():
    g = [g for g in ag.get_grammar_files() if 'Colloc0_aesp' in g][0]
    assert ag.check_grammar(g, 'Colloc0')

    with pytest.raises(RuntimeError):
        ag.check_grammar(g, 'Phonemes')

    with pytest.raises(RuntimeError):
        ag.check_grammar(g + 'nonexistingfile', '')


def test_segment_single(prep):
    pc = ag.ParseCounter(len(prep))

    ag._segment_single(
        pc,
        prep,
        os.path.join(grammar_dir, grammars[1][0]),
        'Colloc0',
        0,
        args=test_arguments)

    assert len(prep) == pc.nutts == len(pc.counters)


def test_counter():
    c = ag.ParseCounter(5)

    # 5 utterances in 3 trees
    matrix = [
        [1, 1, 1, 1, 1],
        [1, 1, 2, 1, 1],
        [2, 2, 2, 2, 2]]
    expected = [1, 1, 2, 1, 1]

    for v in matrix:
        c.update(v)
    assert list(c.most_common()) == expected


def test_yield_parses():
    def aux(trees, n):
        return list(ag.yield_parses(trees, ignore_firsts=n))

    assert aux(['a', '', 'b', '', 'c'], 0) == [['a'], ['b'], ['c']]
    assert aux(['a', '', 'b', '', 'c'], 1) == [['b'], ['c']]
    assert aux(['a', '', 'b', '', 'c'], 2) == [['c']]
    assert aux(['a', '', 'b', '', 'c'], 3) == []
    assert aux([], 3) == []


def test_setup_seed():
    s = ag._setup_seed
    assert len(s('', 2)) == 2
    assert s('-b -r 1 -a 2', 1) == ['-b -r 1 -a 2']
    assert s('-b -r 1 -a 2', 1) == ['-b -r 1 -a 2']
    assert s('-b -r 5 -a 2', 2) == ['-b -r 5 -a 2', '-b -r 6 -a 2']


@pytest.mark.parametrize('ignore', [-10, -5, -1, 0, 5, 6, 10])
def test_ignore_first_parses(prep, ignore):
    # we use the default test value -n 10 -x 2 (10 iterations yields to 6
    # parses, initial one and 5 each 2 iterations)
    if ignore < 6:
        segmented = ag.segment(
            prep, args=test_arguments, nruns=1, ignore_first_parses=ignore)
        assert len(segmented) == len(prep)
    else:
        # ignoring more than the extracted parses raises an error
        with pytest.raises(RuntimeError):
            ag.segment(prep, args=test_arguments, nruns=1,
                       ignore_first_parses=ignore)


def _update(counter, parse):
    counter.update(parse)


@pytest.mark.parametrize('njobs', [1, 2, 4, 10])
def test_parse_counter(njobs):
    counter = ag.ParseCounter(2)
    parses = [['a', 'a']] * 50 + [['a', 'b']] * 10 + [['b', 'b']] * 45

    joblib.Parallel(n_jobs=njobs, backend='threading', verbose=0)(
        joblib.delayed(_update)(counter, parses[n])
        for n in range(len(parses)))

    assert counter.nparses == 105
    assert counter.most_common() == ['a', 'b']


@pytest.mark.parametrize('grammar, level', grammars)
def test_grammars(prep, grammar, level):
    grammar = os.path.join(grammar_dir, grammar)
    segmented = ag.segment(prep, grammar, level, test_arguments, nruns=1)
    assert len(segmented) == len(prep)

    segmented = ''.join(utt.replace(' ', '').strip() for utt in segmented)
    prep = ''.join(utt.replace(' ', '').strip() for utt in prep)
    assert segmented == prep


def test_default_grammar(prep):
    segmented = ag.segment(prep, args=test_arguments, nruns=1)
    assert len(segmented) == len(prep)

    segmented = ''.join(utt.replace(' ', '').strip() for utt in segmented)
    prep = ''.join(utt.replace(' ', '').strip() for utt in prep)
    assert segmented == prep


def test_mark_jonhson(tmpdir, datadir):
    # this is a transcription of the original "toy run" delivered with
    # the original AG code (as a target in the Makefile)
    assert os.path.isdir(datadir)

    grammar_file = os.path.join(datadir, 'ag_testengger.lt')
    text = list(codecs.open(
        os.path.join(datadir, 'ag_testeng.yld'), 'r', encoding='utf8'))
    arguments = (
        '-r 1234 -P -D -R -1 -d 100 -a 1e-2 -b 1 -e 1 -f 1 '
        '-g 1e2 -h 1e-2 -n 10 -C -E -A {prs} -N 10 -F {trace} -G {wlt} '
        # -X "cat > {X1}" -X "cat > {X2}" '
        # -U "cat > {prs1}" -v {testeng2} -V "cat > {prs2}"'
        '-u {testeng1} '.format(
            testeng1=os.path.join(datadir, 'ag_testeng1.yld'),
            testeng2=os.path.join(datadir, 'ag_testeng2.yld'),
            trace=tmpdir.join('trace'), wlt=tmpdir.join('wlt'),
            prs=tmpdir.join('prs')
            # X1=tmpdir.join('X1'), X2=tmpdir.join('X2'),
            # prs1=tmpdir.join('prs1'), prs2=tmpdir.join('prs2')))
        ))
    pc = ag.ParseCounter(len(text))
    output = ag.segment(text, grammar_file, category='VP', args=arguments,
                        ignore_first_parses=0, nruns=1)
    assert len(text) == len(output)
    for i in range(len(text)):
        assert text[i].strip().replace(' ', '') == output[i].replace(' ', '')



# # this test is not stable enough, so it is commented out
# def test_replicate_cdswordseg(datadir, tmpdir):
#     sep = Separator()

#     _tags = [utt for utt in codecs.open(
#         os.path.join(datadir, 'tagged.txt'), 'r', encoding='utf8')
#             if utt][:200]
#     _prepared = list(prepare(_tags, separator=sep))
#     _gold = gold(_tags, separator=sep)

#     # build a colloc0 grammar from the list of phones
#     phones = {p for utt in _prepared for p in utt.split(' ')}
#     grammar = ag.build_colloc0_grammar(phones)
#     grammar_file = tmpdir.join('grammar.lt')
#     grammar_file.write_text(grammar, encoding='utf8')

#     segmented = ag.segment(_prepared, str(grammar_file), 'Colloc0',
#                            # log=utils.get_logger(level=logging.DEBUG),
#                            args=test_arguments.replace('-n 10', '-n 2'),
#                            nruns=1)
#     score = evaluate(segmented, _gold)

#     # we obtained those results from the AG version in CDSwordseg
#     # (preparing with wordseg-prep and evaluating with wordseg-eval)
#     expected = {
#         'type_fscore': 0.2385,
#         'type_precision': 0.2038,
#         'type_recall': 0.2875,
#         'token_fscore': 0.2642,
#         'token_precision': 0.238,
#         'token_recall': 0.2969,
#         'boundary_fscore': 0.5361,
#         'boundary_precision': 0.4754,
#         'boundary_recall': 0.6145}

#     # Because we use so few data and iterations,  the results are
#     # not stable, we limit the precision to 0.3
#     for k, v in score.items():
#         assert expected[k] == pytest.approx(v, abs=0.3), k
