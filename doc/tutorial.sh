#!/bin/bash

# prepare the input for segmentation and generate the gold text
cat $1 | wordseg-prep --gold gold.txt > prepared.txt

# compute statistics on the tokenized input text
cat $1 | wordseg-stats --json > stats.json

# segment the prepared text with different algorithms (we show few
# options for them, use --help to list all of them)
cat prepared.txt | wordseg-baseline -p 0.2 > segmented.baseline.txt
cat prepared.txt | wordseg-tp -t relative > segmented.tp.txt
cat prepared.txt | wordseg-puddle -j 4 -w 2 > segmented.puddle.txt
cat prepared.txt | wordseg-dibs -p 0.1 > segmented.dibs.txt

# evaluate them against the gold file
for algo in baseline tp puddle dibs
do
    cat segmented.$algo.txt | wordseg-eval gold.txt > eval.$algo.txt
done

# display the statistics computed on the input text
echo "* Statistics"
echo
cat stats.json

# concatenate the evaluations in a table
echo
echo "* Evaluation"
echo
echo "score baseline tp puddle dibs"
echo "------------------ -------- -------- -------- --------"
for i in $(seq 1 9)
do
    awk -v i=$i 'NR==i {printf $0}; END {printf " "}' eval.baseline.txt
    awk -v i=$i 'NR==i {printf $2}; END {printf " "}' eval.tp.txt
    awk -v i=$i 'NR==i {printf $2} END {printf " "}' eval.puddle.txt
    awk -v i=$i 'NR==i {print $2}' eval.dibs.txt
done
