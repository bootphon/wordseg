.. _algorithms:

More on the algorithms
=======================



Baseline
---------------

We drew from Lignos 2012 the excellent idea of drawing baselines by
cutting with a given probability. Using this insight, you can draw
four baselines:

- the **random** baseline randomly labels each syllable boundary as a
  word boundary with a probability P = 0.5. If no P is specified by the 
  user, this is the one that is ran by default.

- the **utterance** baseline treats every utterance as a single word, 
  so P = 0.

- the **basic unit** baseline labels every phone (or syllable) boundary as a
  word boundary, so P = 1,

- Finally, the user can also get **oracle** baseline with a little bit more 
  effort. You can run the stats function to get the number of words and the
  number of phones and syllables. Then divide number of phones by number of
  words to get the probability of a boundary in this data set when encoded 
  in terms phones, let us call it PP. You then set the parameter P = PP in the
  input prepared in terms of phones. Or, if you are preparing your corpus
  tokenized by syllables, use PS= number of syllables / number of words.


Transitional probabilities
---------------------------

To be continued...


Diphone Based Segmenter (DiBS)
------------------------------
A DiBS model assigns, for each phrase-medial diphone, a value between
0 and 1 inclusive (representing the probability the model assigns that
there is a word-boundary there).

For details, see Daland, R., Pierrehumbert, J.B., "Learning
diphone-based segmentation". Cognitive science 35(1), 119–155 (2011).

To be continued...

Phonotactics from Utterances Determine Distributional Lexical Elements (PUDDLE)
-------------------------------------------------------------------------------

Implementation of the PUDDLE philosophy developped by P. Monaghan.

See “Monaghan, P., & Christiansen, M. H. (2010). Words in puddles of sound: modelling psycholinguistic effects in speech segmentation. Journal of child language, 37(03), 545-564.”

To be continued...


Adaptor grammar
---------------

To be continued...


Bayesian Segmentor aka DPSEG aka DMCMC
--------------------------------------

To be continued...

