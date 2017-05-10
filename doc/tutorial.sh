#!/bin/bash

# Prepare the input for segmentation
cat $1 | wordseg-prep > prepared.txt

# Generate the gold text
cat $1 | wordseg-gold > gold.txt

# segment the prepared text with different algorithms (we show few
# options for them, use --help to list all of them)
cat prepared.txt | wordseg-tp -t relative > segmented.tp.txt
cat prepared.txt | wordseg-puddle -j 4 -w 2 > segmented.puddle.txt
cat prepared.txt | wordseg-dibs -p 0.1 > segmented.dibs.txt

# evaluate them against the gold file
for algo in tp puddle dibs
do
    cat segmented.$algo.txt | wordseg-eval gold.txt > eval.$algo.txt
done

# concatenate the evaluations in a table
echo "score tp puddle dibs"
echo "---------------- ------- ------- -------"
for i in $(seq 1 9)
do
    awk -v i=$i 'NR==i {printf $0}; END {printf " "}' eval.tp.txt
    awk -v i=$i 'NR==i {printf $2} END {printf " "}' eval.puddle.txt
    awk -v i=$i 'NR==i {print $2}' eval.dibs.txt
done
