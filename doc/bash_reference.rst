.. _commands:

Bash commands reference
=======================

List of wordseg commands
------------------------

Once wordseg is installed on your plaform, the commands are
available from the terminal as any other command line tool. The
commands for which we provide documentation are:

* **wordseg-prep** takes as input a text in phonological-like form with tags, and preps 
  it for segmentation by checking format, removing all tags but word boundaries to generate 
  a gold version, and all tags but the minimal unit boundaries to generate what we will call
  prepared.txt, which is the input for segmentation.

* **wordseg-<algorithm>** always takes as input a prepared.txt 
  file, outputing the same text with word boundaries added. 
  Please note that some algorithms require more input than just that.
  For details, see the :ref:*overview* page. The calls for the algorithms
  are:

  - **wordseg-baseline** for random baseline,
  - **wordseg-dibs** for the diphone based segmentation,
  - **wordseg-tp** for the transitional probabilities,
  - **wordseg-dpseg** for the DPSeg or DMCMC,
  - **wordseg-puddle** for PUDDLE,
  - **wordseg-ag** for the adaptor grammar.

* **wordseg-eval** takes as input a segmented text and a gold version,
  to compute the precision, recall and f-score at type, token and
  boundary levels. See more on the :ref:*overview* page.

* **wordseg-stats** takes as input a segmented or gold text and
  computes basic statistics.

.. note::

   * To get all the details of a command arguments, have a ``wordseg-<command> --help``,
   * More information on the algorithms and functions are in :ref:`overview`.


Hello world example
-------------------

This is a minimalistic "hello world" example of word segmentation with
wordseg. We are simply segmenting a two-words utterance with the TP
algorithm, showing the input, output and gold texts.

.. literalinclude:: hello_world.sh
   :language: bash

This should output:

  | hh ax l ow w er l d
  | hhaxl owwerld
  | hhaxlow werld


Input text format
-----------------

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


Commands input/output
---------------------

All the commands **read from standard input** and **write to standard
output** by default (this allows easy communication with other
tools). But you can specify input and output files as arguments if you
want to. For example the command::

  cat my_input.txt | wordseg-prep > my_output.txt

is equivalent to::

  wordseg-prep my_input.txt -o my_output.txt


Logging messages
----------------

The commands write log messages to standard error. The messages are
either an **error**, a **warning**, an **info** or a **debug**
message. You can choose the level of logging you want to display with
the following arguments (available for all commands):

* ``-v | --verbose`` displays errors, warnings and infos,
* ``-vv | --verbose --verbose`` displays all the messages,
* ``-q | --quiet`` does not display any message.

When running several commands in scripts or in parallel, the standard
error can become a mess. It is possible to redirect stderr to a file using::

   wordseg-<command> 2> ./my_log.txt
