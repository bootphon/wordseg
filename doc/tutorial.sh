#!/bin/bash

# prepare the input for segmentation and generate the gold text
cat $1 | wordseg-prep -u phone --gold gold.txt > prepared.txt

# compute statistics on the tokenized input text
cat $1 | wordseg-stats --json > stats.json

# segment the prepared text with different algorithms (we show few
# options for them, use --help to list all of them)
cat prepared.txt | wordseg-baseline -P 0.5 > segmented.baseline.txt
cat prepared.txt | wordseg-tp -p forward -t relative > segmented.tp.txt
cat prepared.txt | wordseg-puddle -w 2 > segmented.puddle.txt
cat prepared.txt | wordseg-dpseg -f 1 -r 1 > segmented.dpseg.txt

# AG has a similar call but additional files can be provided
GRAMMAR=../data/ag/Colloc0_enFestival.lt
CATEGORY=Colloc0
cat prepared.txt | wordseg-ag $GRAMMAR $CATEGORY --njobs 4 > segmented.ag.txt

# dibs' call looks pretty different (we provide a training file)
wordseg-dibs -t baseline prepared.txt $1 > segmented.dibs.txt

# evaluate them against the gold file
for algo in baseline tp puddle dpseg dibs ag
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
(
    echo "score baseline tp puddle dpseg ag dibs"
    echo "------------------ ------- ------- ------- ------- -------"
    for i in $(seq 1 12)
    do
        awk -v i=$i 'NR==i {printf $0}; END {printf " "}' eval.baseline.txt
        awk -v i=$i 'NR==i {printf $2}; END {printf " "}' eval.tp.txt
        awk -v i=$i 'NR==i {printf $2}; END {printf " "}' eval.puddle.txt
        awk -v i=$i 'NR==i {printf $2}; END {printf " "}' eval.dpseg.txt
        awk -v i=$i 'NR==i {printf $2}; END {printf " "}' eval.ag.txt
        awk -v i=$i 'NR==i {print $2}' eval.dibs.txt
    done
) | column -t
