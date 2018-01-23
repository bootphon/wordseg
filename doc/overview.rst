.. _overview:

============
Overview
============

This section provides an overview of the whole process as we have
conceived it.  Individual users may actually only need a subset of the
steps, or they may need to write additional scripts to replace some
other steps.

The following are the key steps in any wordseg analysis.

--------------------------------------------------------
1. Input corpus selection, cleaning, and phonologization
--------------------------------------------------------

** This step is not covered by the WordSeg package. **

Most users will start with an annotated, orthographic text which
contains some sequences that were not spoken, or sequences spoken
by people who are not interesting (e.g., children or adults).
Cleaning will require removing of these offending sequences or lines.
Users will then need to phonologize the text by using a
dictionary look-up or other system.


--------------------
2. Input preparation
--------------------

This package assumes that word boundaries and basic units are coded in the input text.
Text can have one or both of the following basic units: phones, syllables.
By default, the word boundary coding is ";eword". If this is not the
case, the user can signal this by indicating the code for word boundaries
using the parameter -w at all processing stages.
The same can be said for phones (default is space, parameter is -p);
and syllables (default is ";esyll", parameter is -s). So imagine the phrase
"hello world" in FESTIVAL phonological format would look like::

hh ax ;esyll l ow ;esyll ;eword w er l d ;esyll ;eword

To feed the input to subsequent analyses, the user must generate a prepared text,
and a gold text. In the prepared text, the only boundaries correspond to tokenized
basic units. For instance, if one wants the basic unit to be syllables, then one
will tokenize by syllables, such that the prepared text looks like this::

hhax low werld

The same input tokenized into phones looks like this::

hh ax l ow w er l d

In the gold text, the only boundaries correspond to words. For instance::

hhaxlow werld

.. note::

Every user should run the prepare step to make sure their input text is
formatted correctly.

So to sum up:

* For all the commands, the input must be a multi-line text, one
  utterance per line, with **no punctuation** (excepted for token
  separators, see below).

* Each utterance is made of a sequence of phonological units separated
  by token boundaries (at word, phone or syllable levels).

* The **phonological units** can be any unicode characters, or even
  strings. In the example above (``""hh ax l ow _ w er l d _""``) the
  phonetic units for the first word are ``"hh"``, ``"ax"``, ``"l"``
  and ``"ow"``.

* Phonological units are separated by **phone, syllable and word
  boundaries**. In the hello world example, phones are separated by
  ``" "`` and words by ``"_"``.

* Syllable boundaries are optional. When provided, you can tell
  **wordseg-prep** to prepare your input at the syllable level with
  the option ``wordseg-prep --unit syllable``.

.. note::

   The default separators used in wordseg are:

   * ``" "`` as **phone boundary**
   * ``";esyll"`` as **syllable boundary**
   * ``";eword"`` as **word boundary**

   You can specify other separators using the ``-p``, ``-s`` and
   ``-w`` options of the related wordseg commands.




---------------
3. Segmentation
---------------

The next step involves modeling the segmentation process with some
segmentation algorithm. Six families of algorithms are provided, with
many parameters each, such that numerous combinations can be
achieved. For more information on the algorithms, see :ref:`algorithms`:. 
For examples of use, see :ref:`tutorial`:.


Individual users may need additional algorithms. We strongly encourage
users to develop algorithms that can be reincorporated into this package!


---------------
4. Evaluation
---------------

Finally, the segmented output is compared agains the gold input to
check the algorithms' performance.


-----------------
Descriptive tools
-----------------

The WordSeg package also includes some commonly used descriptive
statistics, which can be applied to the gold version of the input corpus, or to the
output of segmentation. This will give users an idea of basic
statistics (size, lexical diversity, etc.) of their corpus or the
segmented output.
