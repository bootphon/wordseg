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

  * Evaluation NEEDS TO BE UPDATED

  score                 baseline  tp        puddle    dpseg
  --------------------  --------  --------  --------  --------
  token_precision       0.087     0.2974    0.4848    0.5743
  token_recall          0.06668   0.3571    0.1801    0.6842
  token_fscore          0.07549   0.3246    0.2626    0.6244
  type_precision        0.07154   0.1505    0.1245    0.4503
  type_recall           0.1688    0.2457    0.1135    0.3125
  type_fscore           0.1005    0.1866    0.1188    0.369
  boundary_precision    0.2858    0.5357    0.9446    0.7396
  boundary_recall       0.2076    0.6616    0.249     0.9054
  boundary_fscore       0.2405    0.592     0.3941    0.8141
