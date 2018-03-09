#!/usr/bin/env python

import json
import sys

from wordseg.evaluate import evaluate
from wordseg.prepare import prepare, gold
from wordseg.algos import tp, puddle, dpseg, baseline
from wordseg.statistics import CorpusStatistics
from wordseg.separator import Separator


# load the input text file
text = open(sys.argv[1], 'r').readlines()

# compute some statistics on the input text (text tokenized at phone
# and word levels)
separator = Separator(phone=' ', syllable=None, word=';eword')
stats = CorpusStatistics(text, separator).describe_all()

# display the computed statistics
sys.stdout.write(
    '* Statistics\n\n' +
    json.dumps(stats, indent=4) + '\n')


# prepare the input for segmentation
prepared = list(prepare(text))

# generate the gold text
gold = list(gold(text))

# segment the prepared text with different algorithms
segmented_baseline = baseline.segment(prepared, probability=0.2)
segmented_tp = tp.segment(prepared, threshold='relative')
segmented_puddle = puddle.segment(prepared, njobs=4, window=2)
segmented_dpseg = dpseg.segment(prepared, nfolds=1)

# evaluate them against the gold file
eval_baseline = evaluate(segmented_baseline, gold)
eval_tp = evaluate(segmented_tp, gold)
eval_puddle = evaluate(segmented_puddle, gold)
eval_dpseg = evaluate(segmented_dpseg, gold)


# a little function to display score with 4-digits precision
def display(score):
    if score is None:
        return 'None'
    else:
        return '%.4g' % score


# concatenate the evaluations in a table and display them
sys.stdout.write(
    '\n* Evaluation\n\n' +
    ' '.join(('score', 'baseline', 'tp', 'puddle', 'dpseg')) + '\n' +
    ' '.join(('-'*18, '-'*8, '-'*8, '-'*8, '-'*8)) + '\n')
for score in eval_tp.keys():
    line = '\t'.join((
        score,
        display(eval_baseline[score]),
        display(eval_tp[score]),
        display(eval_puddle[score]),
        display(eval_dpseg[score])))
    sys.stdout.write(line + '\n')
