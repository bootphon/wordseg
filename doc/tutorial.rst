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

You can find a sample text in test/data.
The file ``orthographic.txt`` contains the text utterances:

   | you could eat it with a spoon
   | you have to cut that corn too
   | and banana
   | good cheese

And the file ``tagged.txt`` with the utterances represented as a
list of phones with word boundaries tagged as *;eword*:

   | y uw ;esyll ;eword k uh d ;esyll ;eword iy t ;esyll ;eword ih t ;esyll ;eword w ih dh ;esyll ;eword ax ;esyll ;eword s p uw n ;esyll ;eword
   | y uw ;esyll ;eword hh ae v ;esyll ;eword t ax ;esyll ;eword k ah t ;esyll ;eword dh ae t ;esyll ;eword k ao r n ;esyll ;eword t uw ;esyll ;eword
   | ae n d ;esyll ;eword b ax ;esyll n ae ;esyll n ax ;esyll ;eword
   | g uh d ;esyll ;eword ch iy z ;esyll ;eword


Bash tutorial
-------------

The following script is located in ``../doc/tutorial.sh`` and takes an
input text file (in phonological form) as argument:

.. literalinclude:: tutorial.sh
   :language: bash

From the tutorial directory, we can execute the script and display the
result in a table with ``../doc/tutorial.sh ../test/data/tagged.txt | column -t``.


Python tutorial
---------------

The following script is located in ``../doc/tutorial.py``. It
implements exactly the same process as the bash one:

.. literalinclude:: tutorial.py
   :language: python

We can execute it using ``../doc/tutorial.py ../test/data/tagged.txt | column -t``.

Expected output
---------------

The bash and python give the same result, it should be something like::

  * Statistics

  {
  "phones":             {
    "tokens":             6199,
    "hapaxes":            0,
    "types":              39
    },
  "corpus":             {
    "nutts_single_word":  35,
    "nutts":              301,
    "entropy":            0.014991768574252533,
    "mattr":              0.9218384697130766
  },
  "syllables":          {
    "tokens":             2451,
    "hapaxes":            264,
    "types":              607
  },
  "words":              {
    "tokens":             1892,
    "hapaxes":            276,
    "types":              548
  }
  }

  * Evaluation

  score                      baseline  tp        puddle    dpseg     ag        dibs
  -------------------------- --------- --------- --------- --------- --------- ---------
  token_precision            0.06654   0.3325    0.2617    0.3312    0.4851    0.7084
  token_recall               0.1147    0.4059    0.05338   0.4408    0.6195    0.6226
  token_fscore               0.08422   0.3655    0.08867   0.3782    0.5441    0.6627
  type_precision             0.1097    0.2344    0.1058    0.4081    0.4087    0.4916
  type_recall                0.2172    0.3631    0.05657   0.3485    0.4288    0.6387
  type_fscore                0.1457    0.2849    0.07372   0.376     0.4185    0.5556
  boundary_all_precision     0.4043    0.6542    0.9884    0.6662    0.7376    0.9353
  boundary_all_recall        0.6566    0.7788    0.3096    0.8564    0.9138    0.8377
  boundary_all_fscore        0.5004    0.7111    0.4715    0.7494    0.8163    0.8838
  boundary_noedge_precision  0.2831    0.5505    0.9059    0.5756    0.6629    0.9068
  boundary_noedge_recall     0.5267    0.6952    0.0484    0.802     0.8812    0.7762
  boundary_noedge_fscore     0.3683    0.6144    0.09189   0.6702    0.7566    0.8364
