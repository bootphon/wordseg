.. _algorithms:

More on the algorithms
=======================



Baseline
---------------

We drew from Lignos (2012) the excellent idea of drawing baselines by
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

For more inforamtion, see Lignos, C. (2012). Infant word segmentation: An incremental, integrated model. In Proceedings of the West Coast Conference on Formal Linguistics (Vol. 30, pp. 13-15).


Diphone Based Segmenter (DiBS)
------------------------------

A DiBS model is any model which assigns, for each phrase-medial diphone, a value between 0 and 1 inclusive, representing the probability the model assigns that there is a word-boundary between the two phones. In practice, these probabilities are mapped to hard decisions (break or no break). 

Making these decisions requires knowing the chunk-initial and chunk-final probability of each phone, as well as all diphone probabilities; and additionally the probability of a sentence-medial word boundary. In our package, these 4 sets of probabilities are estimated from a training corpus also provided by the user, where word boundaries are marked. Please note we say "chunk-initial and chunk-final" because the precise chunk depends on the type of DiBS used, as follows.

Three versions of DiBS are available. *DiBS-baseline* is supervised in that "chunks" are the gold/oracle words. It is thus supposed to represent the optimal performance possible. *DIBS-phrasal* uses phrases (sentences) as chunks. Finally, *DIBS-lexical* uses as chunks the components of a seed lexicon provided by the user (who may want to input e.g. high frequency words, or words often said in isolation, or words known to be known by young infants). The probability of word boundary right now is calculated in the same way for all three DiBS, and it's the actual gold probability (i.e., the number of words minus number of sentences, divided by the number of phones minus number of sentences). Users can also provide the algorithm with a probability of word boundary calculated in some other way they feel is more intuitive.

DiBS was initially designed with phones as basic units. However, for increased flexibility we have rendered it possible to use syllables as input. 

For more information, see Daland, R., & Pierrehumbert, J.B. (2011). Learning
diphone-based segmentation. Cognitive science, 35(1), 119–155. You can also view an annotated
example produced by Laia Fibla on https://docs.google.com/document/d/1tmw3S-bsecrMR6IokKEwR6rRReWknnbviQMW2o0D5rM/edit?usp=sharing. 

Transitional probabilities
---------------------------

In conceptual terms, we can say that such algorithms assume that infants are looking for words defined as internally cohesive phone/syllable sequences, i.e. where the transitional probabilities between the items in the sequence are relatively high. Our code was provided by Amanda Saksida, where transitional probabilities (TPs) can be calculated in three ways: 

    (1) *Forward TPs* for XY are defined as the probability of the sequence XY  divided by the probability of X; 

    (2) *Backward TPs* for XY are defined as the probability of the sequence XY divided by the probability of Y; 

    (3) *Mutual information (MI)* for XY are defined as the probability of the sequence XY divided by the product of the two probabilities (that of X and that of Y).   

As to what is meant by "relatively high", Saksida's code flexibly allows two definitions. In the first, a boundary is posted when  a *relative dip* in TP/MI is found. That is, given the syllable or phone sequence WXYZ, there will be a boundary posited between  X and Y if the TP or MI between XY is  lower than that between WX and YZ. The second version uses the average of the TP/MI over the whole corpus as the threshold. Notice that both of these are unsupervised: knowledge of word boundaries is not necessary to compute any of the parameters.

TPs was initially designed with  syllables as basic units. However, for increased flexibility we have rendered it possible to use phones as input. 

For more inforamtion, see Saksida, A., Langus, A., & Nespor, M. (2017). Co‐occurrence statistics as a language‐dependent cue for speech segmentation. Developmental science, 20(3).  You can also view an annotated example produced by Georgia Loukatou on 
https://docs.google.com/document/d/1RdMlU-5qM22Qnv7u_h5xA4ST5M00et0_HfSuF8bbLsI/edit?usp=sharing


Phonotactics from Utterances Determine Distributional Lexical Elements (PUDDLE)
-------------------------------------------------------------------------------

This algorithm was proposed by Monaghan and Christiansen (2010), who kindly shared their awk code with us. It was  reimplemented in python in this package, in which process one aspect of the procedure may have been changed, as noted below.

Unlike local-based algorithms such as DiBS and TPs, PUDDLE takes larger chunks -- utterances -- and breaks them into candidate words.  The system has three long-term storage buffers: 1. a "lexicon", 2. a set of onset bigrams, and 3. a set offset bigrams. At the beginning, all three are empty. The "lexicon" will be fed as a function of input utterances, and the bigrams will be fed by extracting onset and offset bigrams from the "lexicon".  The algorithm is incremental, as follows.
 
The model scans each utterance, one at a time and in the order of presentation, looking for a match between every possible sequence of units in the utterance and items in the "lexicon".
 We can view this step as a  search made by the learner as she tries to retrieve words from memory a word to match them against the input. _The one change we fear our re-implementation may have caused is that the original PUDDLE engaged in a serial search in a list sorted by inverse frequency (starting from the most frequent items), whereas our search is not serial but uses hash tables._ In any case, if, for a considered sequence of phones, a match is found, then the model checks whether the two basic units (e.g. phones) preceding and following the candidate match belong to the list of beginning and ending bigrams. If they do, then the frequency of the matched item and the beginning and ending bigrams are all increased by one, and the remainders of the sentence (with the matched item removed) are added to the "lexicon". If a substring match is not found, then the utterance is stored in the long-term "lexicon" as it is, and its onset and offset bigrams will be added to the relevant buffers. 

In our implementation of PUDDLE, we have rendered it more flexible by assuming that users may want to use syllables, rather than phones, as basic units. Additionally, users may want to set  the length of the onset and offset n-grams. Some may prefer to use trigrams rather than biphones; conversely, when syllables are the basic unit, it may be more sensible to use unigrams for permissible onsets and offsets. Our implementation is, however, less flexible than the original PUDDLE in that it also included a parameter for memory decay.


For more information, see  Monaghan, P., & Christiansen, M. H. (2010). Words in puddles of sound: modelling psycholinguistic effects in speech segmentation. Journal of child language, 37(03), 545-564. You can also view an annotated example produced by Alejandrina Cristia on https://docs.google.com/document/d/1OQg1Vg1dvbfz8hX_GlWVqqfQxLya6CStJorVQG19IpE/edit?usp=sharing


Adaptor grammar
---------------

In the adaptor grammar framework, parsing a corpus involves infering the probabilities with which a set of rewrite rules (a "grammar") may have been used in the generation of that corpus.  The WordSeg suite natively contains two grammars that differ somewhat in complexity, but users can also create their own (see https://github.com/alecristia/CDSwordSeg/tree/master/algoComp/algos/AG/grammars for more examples). In the simplest grammar included with the WordSeg suite,  the learner is assumed to have an `innate' architecture captured by two types of re-write rules, one stating that ``words are sequences of sounds/syllables'' and the other type specifying all of the possible terminals (sounds/syllables). The fact that these are rewrite rules is not crucial, and it has in fact been demonstrated that such unigram adaptor grammars are formally equivalent to another class of algorithms that attempts to find the optimal description of a text on the basis of the smallest set of words possible. 


For more information, see Johnson, M., Griffiths, T. L., & Goldwater, S. (2007). Adaptor grammars: A framework for specifying compositional nonparametric Bayesian models. In Advances in neural information processing systems (pp. 641-648). You can also view an annotated example produced by Elin Larsen on https://docs.google.com/document/d/1NZq-8vOroO7INZolrQ5OsKTo0WMB7HUatNtDGbd24Bo/edit?usp=sharing

Bayesian Segmentor aka DPSEG aka DMCMC
--------------------------------------

This algorithm uses a different adaptor grammar implementation, with several crucial differences: 

    - the corpus is processed incrementally, rather than in a batch
    - For a given sentence, a parse is selected depending on the probability of that parse, estimated using the Forward Algorithm (rather than choosing the most likely parse)

The code for this algorithm was pulled from Lawrence Phillips' github repo (https://github.com/lawphill/phillips-pearl2014). 

For more information, see Phillips, L. (2015). The role of empirical evidence in modeling speech segmentation. University of California, Irvine. and Pearl, L., Goldwater, S. and  Steyvers, M. (2011). Online learning mechanisms for Bayesian models of word segmentation. Research on Language and Computation, 8(2):107–132.



