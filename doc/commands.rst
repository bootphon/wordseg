.. _commands:

Commands reference
==================

List of wordseg commands
------------------------

Once **wordseg** is installed on your plaform, the commands are
available from the terminal as any other command line tool. The
commands composing the **wordseg** package are:

* ``wordseg-prep`` prepares a text in a phonological-like form for
  segmentation, basically removing word separators and checking format.

* ``wordseg-gold`` generates a gold text from a phonological one,
  replacing word separators by spaces.

* ``wordseg-<algorithm>`` computes word segmentation on a prepared
  text, outputing the text with word boundaries added. The algorithms
  available are Adaptor Grammar, dibs, dpseg, puddle and Transition
  Probabilities. See more on the :ref:`algorithms` page.

* ``wordseg-eval`` evaluates a segmented text on a gold version,
  computing the precision, recall and f-score at type, token and
  boundary levels. See more on the :ref:`evaluation` page.

* ``wordseg-stats`` computes basic statistics on a segmdented or gold
  text.

.. note::

   To get all the details of a command arguments, have a::

     wordseg-<command> --help


Hello world example
-------------------

This is a minimalistic "hello world" example of word segmentation with
wordseg. We are simply segmenting a two-words utterance with the TP
algorithm, showing the input, output and gold texts.

.. literalinclude:: hello_world.sh
   :language: bash

This should output::

  hh ax l ow w er l d
  hhaxl owwerld
  hhaxlow werld


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
