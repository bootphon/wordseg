# the utterance is a list of phones with word boundaries
HELLO="hh ax l ow _ w er l d _"

# display the text prepared for segmentation
# (echo -n is to not append \n to the line)
echo -n $HELLO | wordseg-prep -w "_" --gold ./gold.txt

# display the segmented output
echo -n $HELLO | wordseg-prep -w "_" | wordseg-tp

# display the gold segmentation
cat ./gold.txt
