#!/usr/bin/env python

import sys

from wordseg.evaluate import evaluate
from wordseg.prepare import prepare, gold
from wordseg.algos import tp, puddle, dibs, baseline


# load the input text file
text = open(sys.argv[1], 'r').readlines()

# Prepare the input for segmentation
prepared = list(prepare(text))

# Generate the gold text
gold = list(gold(text))

# segment the prepared text with different algorithms
segmented_baseline = baseline.segment(prepared, probability=0.2)
segmented_tp = tp.segment(prepared, threshold='relative')
segmented_puddle = puddle.segment(prepared, njobs=4, window=2)
segmented_dibs = dibs.segment(prepared, prob_word_boundary=0.1)

# evaluate them against the gold file
#eval_baseline = evaluate(segmented_baseline, gold)
eval_tp = evaluate(segmented_tp, gold)
#eval_puddle = evaluate(segmented_puddle, gold)
eval_dibs = evaluate(segmented_dibs, gold)

# # concatenate the evaluations in a table
# sys.stdout.write(
#     ' '.join(('score', 'baseline', 'tp', 'puddle', 'dibs')) + '\n' +
#     ' '.join(('-'*18, '-'*8, '-'*8, '-'*8, '-'*8)) + '\n')
# for score in eval_tp.keys():
#     line = ' '.join((
#         score,
#         # '%.4g' % eval_baseline[score],
#         '%.4g' % eval_tp[score],
#         '%.4g' % eval_puddle[score],
#         '%.4g' % eval_dibs[score]))
#     sys.stdout.write(line + '\n')
