.. _overview:

Overview
========

This section provides an overview of the whole process as we have
conceived it.  Individual uses may actually only need a subset of the
steps, or they may need to write additional scripts to replace some
other steps.

The following are the key steps in any wordseg analysis.

1. Input corpus selection and cleaning
--------------------------------------

Most users will start with an annotated, orthographic text which
contains some sequences that were not spoken. For instance, we often
use CHILDES transcripts, which contain:

- a header with background information,

- a code for the speaker followed by a tab, and what the speaker said

- and even inside this transcription, there are codes to mark
  onomatopeia, etc.

The first step will thus be to make decisions as to what will be
analyzed.  Scripts for this "selection and cleaning" are provided
which work for .cha files. Users working with other transcription
conventions can get inspired from these, or they may need to change
ours to accommodate some annotation not present in the corpora we
worked with.


2. Phonologization
------------------

The next step involves converting orthographic to phonological
annotation. There are 2 choices currently implemented:

- a text-to-speech system (either FESTIVAL or eSpeak) is available for
  English and a few other major languages,

- a sample set of scripts and input files for doing grapheme to
  phoneme conversion in a minor language

Users who start with a transcript in phonological format will thus
skip steps 1 and 2.


3. Final preparation
--------------------

This package assumes a certain input: TODO

Every user should run this step to make sure their input text is
formatted correctly.


4. Segmentation
---------------

The next step involves modeling the segmentation process with some
segmentation algorithm. Six families of algorithms are provided, with
many parameters each, such that numerous combinations can be
achieved. In all cases, the system will "erase" word boundaries and
segment the text.

Individual users may need additional algorithms.


5. Evaluation
-------------

Finally, the segmented output is compared agains the gold input to
check the algorithms' performance.


Descriptive tools
-----------------

The WordSeg package also includes some commonly used descriptive
statistics, which can be applied to the gold input corpus, or to the
output of segmentation. This will give them an idea of basic
statistics (size, lexical diversity, etc.) of their corpus or the
segmented output.
