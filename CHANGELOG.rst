on master branch (not yet released)
-----------------------------------

* Features

    * New methods for basic statistics and normalized segmentation
      entropy in **wordseg-stats**

    * New forward/backward option in **wordseg-tp**.

    * New command **wordseg-baseline** that produces a random
      segmentation given the probability of a word boundary.

    * New command **wordseg-syll** estimates syllable boundaries on a
      text using the maximal onset principle. Exemples of onsets and
      vowels files for syllabifications are given in the directory
      `data/syllabification`.

    * For citation purposes a DOI is now automatically attached to
      each wordseg release.

* Bugfixes

    * The following characters are now forbidden in separators, they
      interfer with regular expression matching::

        !#$%&'*+-.^`|~:\\\"

    * Type scoring is now correctly implemented in **wordseg-eval**
      (#10, #14).


wordseg-0.5.1
-------------

* Support for punctuation in input of **wordseg-prep** with the
  ``--punctuation`` option (#10).


wordseg-0.5
-----------

* Implementation of Adaptor Grammar as ``wordseg-ag``,
* Installation now relies on cmake (was python setuptools),
* Improvements in tests and documentation,
* Various bugfixes.


wordseg-0.4.1
-------------

* First public release, adaptation from Alex Cristia's
  `CDSWordSeg <https://github.com/alecristia/CDSwordSeg>`_.
* Four algorithms (tp, puddle, dpseg, dibs).
* Segmentation prepocessing and evaluation.
* Unit tests and documentation.
* On the `original implementation
  <https://github.com/lawphill/phillips-pearl2014>`_, we applied the
  following changes:

  * conversion to C++11 standard,
  * replaced ``tr1/unsorted_map`` and ``mt19937`` by the standard library,
  * code cleanup, removed useless functions and code,
  * complete rewrite of the build process (Makefile, link on boost).
