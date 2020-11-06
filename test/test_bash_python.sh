#!/bin/bash

# Display the token f-score obtained on tp, dibs or puddle from the
# wordseg/test/data/tagged.txt file, calling the algo with 3 variant
# calls (python and bash in 2 flavors) and displaying the results
# summary. This uses the default arguments for each algo, and the
# default separator for input text.

set -e

algo=$1  # must be dibs, puddle or tp
unit=$2  # must be phone or syllable

scriptpath="$( cd "$(dirname "$0")" ; pwd -P )"
tags=$scriptpath/data/tagged.txt

# create a temp data dir deleted at exit
data=$(mktemp -d)
trap "rm -rf $data" EXIT

# generate prepared and gold texts, take 10 first lines only
gold=$data/gold.txt
prep=$data/prep.txt
head -10 $tags | wordseg-prep -u $unit -g $gold -o $prep

if [ $algo == "dibs" ]  # dibs needs a train corpus argument
then
    # generate train file
    train=$data/train
    head -200 $tags > $train

    # dibs python script
    cat <<EOF > $data/segment.py
import codecs
import sys
from wordseg.algos.dibs import segment, CorpusSummary
text = [l.strip() for l in codecs.open(sys.argv[1], 'r', encoding='utf8')]
train = [l.strip() for l in codecs.open(sys.argv[2], 'r', encoding='utf8')]
segmented = segment(text, CorpusSummary(train))
print('\n'.join(s for s in segmented))
EOF

else  # $algo is tp or puddle
    # no train file
    train=

    # default python script
    cat <<EOF > $data/segment.py
import codecs
import sys
from wordseg.algos.$algo import segment
segmented = segment([l.strip() for l in codecs.open(sys.argv[1], 'r', encoding='utf8')])
print('\n'.join(s for s in segmented))
EOF
fi

# version 1: pure python
python $data/segment.py $prep $train | wordseg-eval $gold > $data/v1

if [ $algo == "dibs" ]
then
    train="--train-file $train"
fi

# version 2: bash, read from python
wordseg-$algo $prep $train | wordseg-eval $gold > $data/v2

# version 3: bash, read from bash
cat $prep | wordseg-$algo $train | wordseg-eval $gold > $data/v3

# collapse results
paste $data/v* | tr -s "\t" " " | cut -d' ' -f1-2,4,6,8 | \
    grep token_fscore | sed "s/token_fscore//"
