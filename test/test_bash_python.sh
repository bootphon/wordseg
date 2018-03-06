#!/bin/bash

# Display the token f-score obtained on tp, dibs and puddle from the
# same input file, calling the algo with 3 variant calls and
# displaying the results summary. This uses the default arguments for
# each algo, and the default separator for input text.

set -e

algo=$1  # must be dibs, puddle or tp
unit=$2  # must be phone or syllable

scriptpath="$( cd "$(dirname "$0")" ; pwd -P )"
tags=$scriptpath/data/tagged.txt

# create a temp data dir deleted at exit
data=$(mktemp -d)
trap "rm -rf $data" EXIT

# generate preparaed and gold text
gold=$data/gold.txt
prep=$data/prep.txt
head -10 $tags | wordseg-prep -u $unit -g $gold -o $prep

if [ $algo == "dibs" ]  # dibs needs a train corpus argument
then
    # generate train file
    train=$data/train
    head -200 $tags > $train

    # version 1: python
    cat <<EOF > $data/segment.py
import codecs
import sys
from wordseg.algos.dibs import segment, CorpusSummary
text = [l.strip() for l in codecs.open(sys.argv[1], 'r', encoding='utf8')]
train = [l.strip() for l in codecs.open(sys.argv[2], 'r', encoding='utf8')]
segmented = segment(text, CorpusSummary(train))
print('\n'.join(s for s in segmented))
EOF

    python $data/segment.py $prep $train | wordseg-eval $gold > $data/v1

    # version 3: bash, read from python
    wordseg-$algo $prep $train | wordseg-eval $gold > $data/v2

    # version 2: bash, read from bash
    cat $prep | wordseg-$algo $train | wordseg-eval $gold > $data/v3
else
    # version 1: python
    cat <<EOF > $data/segment.py
import codecs
import sys
from wordseg.algos.$algo import segment
segmented = segment([l.strip() for l in codecs.open(sys.argv[1], 'r', encoding='utf8')])
print('\n'.join(s for s in segmented))
EOF

    python $data/segment.py $prep | wordseg-eval $gold > $data/v1

    # version 2: bash, read from python
    wordseg-$algo $prep | wordseg-eval $gold > $data/v2

    # version 3: bash, read from bash
    cat $prep | wordseg-$algo | wordseg-eval $gold > $data/v3
fi

# collapse results
paste $data/v* | tr -s "\t" " " | cut -d' ' -f1-2,4,6,8 | \
    grep token_fscore | sed "s/token_fscore//"
