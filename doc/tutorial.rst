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
result in a table with ``../doc/tutorial.sh phonologic.txt | column -t``.


Python tutorial
---------------

The following script is located in ``../doc/tutorial.py``. It
implements exactly the same process as the bash one:

.. literalinclude:: tutorial.py
   :language: python

We can execute it using ``../doc/tutorial.py phonologic.txt | column -t``.

Expected output
---------------

The bash and python give the same result, it should be something like::

  * Statistics

  {
  "corpus":             {
    "nutts":              1000,
    "nutts_single_word":  118,
    "nword_tokens":       6824,
    "nword_types":        1392,
    "nword_hapax":        857,
    "mattr":              0.9354710889345312,
    "entropy":            0.015144673474006031
  },
  "words":              {
    "tokens":             6824,
    "tokens/utt":         6.824,
    "types":              1392,
    "token/types":        4.902298850574713,
    "uniques":            857
  },
  "phones":             {
    "tokens":             22157,
    "tokens/utt":         22.157,
    "tokens/word":        3.2469226260257913,
    "types":              60,
    "token/types":        369.28333333333336,
    "uniques":            0
  }
  }

  * Evaluation

  score                 baseline  tp        puddle    dpseg
  ------------------ -------- -------- -------- --------
  type_fscore           0.1005    0.1866    0.1188    0.369
  type_precision        0.07154   0.1505    0.1245    0.4503
  type_recall           0.1688    0.2457    0.1135    0.3125
  token_fscore          0.07549   0.3246    0.2626    0.6244
  token_precision       0.087     0.2974    0.4848    0.5743
  token_recall          0.06668   0.3571    0.1801    0.6842
  boundary_fscore       0.2405    0.592     0.3941    0.8141
  boundary_precision    0.2858    0.5357    0.9446    0.7396
  boundary_recall       0.2076    0.6616    0.249     0.9054
