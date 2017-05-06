#!/bin/bash
#
# This example script demonstrate the segmentation of... TODO link
# that with the tutorial

# the directory where we put the intermediate data and results
data=$(mktemp -d)
trap "rm -rf $data" EXIT

# the input text to segment
cat $(dirname ${BASH_SOURCE[0]})/segmentation/test/data/tags.txt | sort -R | head -30 > $data/tags.txt

# build the gold version
cat $data/tags.txt | wordseg-gold > $data/gold.txt

# segmentation at phoneme level
cat $data/tags.txt | wordseg-prep -u phoneme | wordseg-$algo $opts > $data/seg.$algo.txt

# evaluation
cat $data/seg.$algo.txt | wordseg-eval $data/gold.txt
