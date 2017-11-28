.. _tutorial:

Tutorial
========

This tutorial demonstrates the use of the **wordseg** tools on a
concrete example: comparing 3 algorithms on a 1000 utterances
English corpus.

* As **wordseg** can be used either in bash or python, this tutorial
  shows the two, choose your own!

* We will use the TP, dibs and puddle algorithms, which are the 3
  fastest ones.

* For the tutorial, work in a new directory you can delete afterward::

    mkdir -p ./wordseg_tutorial
    cd ./wordseg_tutorial


Input text
----------

In this section, we download some English text from the BUCKEYE
corpus, pick up 1000 random utterances in it, and convert them from an
orthographic form to a phonological one.

* Download about 50k english utterances (from the BUCKEYE corpus)::

    wget https://raw.githubusercontent.com/alecristia/CDSwordSeg/master/recipes/buckeye/buckeye-ortholines.txt

* Pick random 1000 utterances as segmentation input::

    cat buckeye-ortholines.txt | sort -R | head -1000 > orthographic.txt

* Convert the English text into a phonological from. We are using the
  `phonemizer <https://github.com/bootphon/phonemizer>`_, a
  third-party software we also authored. Please install it before
  going forward::

    cat orthographic.txt | phonemize --lang en-us -p " " -w ";eword " > phonologic.txt


We have now a file ``orthographic.txt`` with the text utterances:

   | us to play the solos
   | no i i don't wanna
   | terrible
   | you'd learn a heck a lot
   | people like that even get

And a file ``phonologic.txt`` with the utterances represented as a
list of phones with word boundaries tagged as *;eword*:

   | ˌʌ s ;eword t ə ;eword p l eɪ ;eword ð ə ;eword s oʊ l oʊ z ;eword
   | n oʊ ;eword aɪ  ;eword aɪ ;eword d oʊ n t ;eword w ɑː n ə ;eword
   | t ɛ ɹ ə b əl ;eword
   | j uː d ;eword l ɜː n ;eword ɐ ;eword h ɛ k ;eword ɐ ;eword l ɑː t ;eword
   | p iː p əl ;eword l aɪ k ;eword ð æ t ;eword iː v ə n ;eword ɡ ɛ t ;eword


Bash tutorial
-------------

The following script is located in ``../doc/tutorial.sh`` and takes an
input text file (in phonological form) as argument:

.. literalinclude:: tutorial.sh
   :language: bash

From the tutorial directory, we can execute the script and display the
result in a table with ``../doc/tutorial.sh phonologic.txt``.


Python tutorial
---------------

The following script is located in ``../doc/tutorial.py``. It
implements exactly the same process as the bash one:

.. literalinclude:: tutorial.py
   :language: python

We can execute it using ``../doc/tutorial.py phonologic.txt``.

Expected output
---------------

The bash and python give the same result, it should be something like::

  * Statistics

  {
  "corpus": {
          "nutts": 1000,
          "nutts_single_word": 22,
          "nword_tokens": 3916,
          "nword_types": 765,
          "nword_hapax": 457,
          "mattr": 0.7125704045058399,
          "entropy": 0.061493838936541596
      },
  "words": {
          "tokens": 3916,
          "tokens/utt": 3.916,
          "types": 765,
          "token/types": 5.118954248366013,
          "uniques": 457
      },
  "phones": {
          "tokens": 11010,
          "tokens/utt": 11.01,
          "tokens/word": 2.8115423901940755,
          "types": 58,
          "token/types": 189.82758620689654,
          "uniques": 2
      }
  }

  * Evaluation

  score              baseline tp       puddle   dibs
  ------------------ -------- -------- -------- --------
  type_fscore	        0.1194	0.2094	 0.03662  0.01944
  type_precision	0.09207	0.1692	 0.05497  0.1379
  type_recall	        0.1699	0.2745	 0.02745  0.01046
  token_fscore	        0.1152	0.2123	 0.02763  0.02921
  token_precision	0.1325	0.201	 0.06401  0.0198
  token_recall	        0.1019	0.225	 0.01762  0.05567
  boundary_fscore	0.2517	0.4584	 0.05077  0.4512
  boundary_precision	0.3083	0.4267	 0.9744	  0.2913
  boundary_recall	0.2126	0.4952	 0.02606  1
