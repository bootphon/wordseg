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


We have now a file ``orthographic.txt`` with the text utterances::

  us to play the solos
  no i i don't wanna
  terrible
  you'd learn a heck a lot from that uh
  angry that people like that even get

And a file ``phonologic.txt`` with the utterances represented as a
list of phones with word boundaries tagged as *;eword*::

  ˌʌ s ;eword t ə ;eword p l eɪ ;eword ð ə ;eword s oʊ l oʊ z ;eword
  n oʊ ;eword aɪ  ;eword aɪ ;eword d oʊ n t ;eword w ɑː n ə ;eword
  t ɛ ɹ ə b əl ;eword
  j uː d ;eword l ɜː n ;eword ɐ ;eword h ɛ k ;eword ɐ ;eword l ɑː t ;eword f ɹ ʌ m ;eword ð æ t ;eword ʌ ;eword
  æ ŋ ɡ ɹ i ;eword ð æ t ;eword p iː p əl ;eword l aɪ k ;eword ð æ t ;eword iː v ə n ;eword ɡ ɛ t ;eword



Bash tutorial
-------------

The following script is located in ``../doc/tutorial.sh`` and takes an
input text file (in phonological form) as argument. From the tutorial
directory, we can execute the script and display the result in a table
with ``../doc/tutorial.sh phonologic.txt | column -t``:

.. literalinclude:: tutorial.sh
   :language: bash

This should output someting like that::

  score            tp       puddle   dibs
  ---------------  -------  -------  -------
  type_f-score     0.3261   0.319    0.03954
  type_precision   0.3027   0.5297   0.02597
  type_recall      0.3536   0.2282   0.08281
  token_f-score    0.3261   0.319    0.03954
  token_precision  0.3027   0.5297   0.02597
  token_recall     0.3536   0.2282   0.08281
  bound_f-score    0.5824   0.462    0.4375
  bound_precision  0.5343   0.9278   0.28
  bound_recall     0.6399   0.3075   1


Python tutorial
---------------

The following script is located in ``../doc/tutorial.py``. It
implements exactly the same process as the bash one. We can execute it
using ``../doc/tutorial.py phonologic.txt | column -t``:

.. literalinclude:: tutorial.py
   :language: python

As previously, this should output something like::

  score             tp       puddle   dibs
  ----------------  -------  -------  -------
  type_f-score      0.3261   0.3836   0.03954
  type_precision    0.3027   0.569    0.02597
  type_recall       0.3536   0.2893   0.08281
  token_f-score     0.3261   0.3836   0.03954
  token_precision   0.3027   0.569    0.02597
  token_recall      0.3536   0.2893   0.08281
  bound_f-score     0.5824   0.5519   0.4375
  bound_precision   0.5343   0.9289   0.28
  bound_recall      0.6399   0.3926   1
