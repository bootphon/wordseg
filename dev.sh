#!/bin/bash

set -e

ag_old=~/dev/CDSwordSeg/algoComp/algos/AG/py-cfg-new/py-cfg
ag_new=~/dev/wordseg/build/wordseg/algos/ag/ag
grammar=~/dev/wordseg/config/ag/Colloc0_enFestival.lt
input=~/dev/wordseg/wordseg_tutorial/prepared.txt

# # AG in CDSWordSeg
# resfolder=./test_ag/old
# mkdir -p $resfolder
# rm -f $resfolder/*
# $ag_old -d 101 -n 10 -E -r $RANDOM -a 0.0001 -b 10000 \
#         -e 1 -f 1 -g 100 -h 0.01 -R -1 -P -U cat \
#         > $resfolder/output.prs $grammar < $input || exit 1

# AG in wordseg
resfolder=./test_ag/new
mkdir -p $resfolder
rm -f $resfolder/*
$ag_new -g $grammar -d debug -n 10 -E -r $RANDOM -a 0.0001 -b 10000 \
        -e 1 -f 1 -s 100 -i 0.01 -R -1 -P -U cat \
        > $resfolder/output.prs < $input || exit 1

exit 0
