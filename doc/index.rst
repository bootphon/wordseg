.. wordseg documentation master file, created by
   sphinx-quickstart on Thu Apr  6 20:18:12 2017.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to the wordseg's documentation page
===========================================

The wordseg package provides **a collection of tools for text based
word segmentation** covering both algorithms, evaluation and
statistics relevent for word segmentation studies.

The segmentation pipeline provided by wordseg is **command line
oriented**. We provide a set of usefull commands for text preparation,
segmentation and evaluation than be used in conjunction to any other
bash utilities.

The wordseg commands are:

* **wordseg-prep** prepares a text in a phonological-like form for
  segmentation, basically removing word separators and checking format.

* **wordseg-gold** generates a gold text from a phonological one,
  replacing word separators by spaces.

* **wordseg-<algorithm>** computes word segmentation on a prepared
  text, outputing the text with word boundaries added. The algorithms
  available are ag, dibs, dpseg, puddle and tp. See more on the
  :ref:`algorithms` page.

* **wordseg-eval** evaluates a segmented text on a gold version,
  computing the precision, recall and f-score at type, token and
  boundary levels.

* **wordseg-stats** computes basic statistics on a segmdented or gold
  text.

Here is a minimalist "hello world" exemple. We are segmenteting a
two-words utterance with the tp algorithm::

  $ # uterance as a list of phones with word boundaries
  $ HELLO="hh ax l ow ;eword w er l d ;eword"

  $ # make the words separated by spaces
  $ echo $HELLO | wordseg-gold
  hhaxlow werld

  $ # make the phones separated by spaces
  $ echo $HELLO | wordseg-prep
  hh ax l ow w er l d

  $ # estimates word boundaries by spaces
  $ echo $HELLO | wordseg-prep | wordseg-tp
  hhaxl owwerld


.. toctree::
   :maxdepth: 2
   :caption: Contents

   installation
   algorithms
   tutorial
   modules
   contributing
   copyright
