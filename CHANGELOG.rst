.. note::

   Version numbers follow the `semantic versioning
   <https://semver.org/>`_ principles.


wordseg-0.6.2 (not yet released)
--------------------------------

* Docker image now uses python-3.6 from anaconda,

* New tests to ensure replication of scores from CDSWordSeg to
  wordseg for puddle, tp, dibs and dpseg.

* In **wordseg-dpseg**:

  * fixed forwarding of some arguments from Python to C++,
  * implementation of dpseg bugfix when single char on first line of
    a fold,
  * use the original random number generator to replicate exactly
    CDSWprdSeg.

* In **wordseg-dibs**

  * fixed bug when loading train text at syllable level (new
    *--unit** option)
  * safer use of train text (ensure there are word separators in
    it).

* In **wordseg-eval**, when called from bash, the scores are now
  displayed in a fixed order. New test to ensure bash and python
  calls to wordseg lead to identical results. See #31.

* In **wordseg-stats** the scores are now displayed in a fixed order.


wordseg-0.6.1
-------------

* Documentation improved, installation guide for working with docker.

* Removed dependancies to *numpy* and *pandas*.

* Tests are now done on a subpart of the CHILDES corpus (was Buckeye,
  under restrictive licence).

* Simplified output in **wordseg-stats**, removed redundancy, renamed
  'uniques' to 'hapaxes'. See #18.

* Bugfix in **wordseg-tp -t relative** when the last utterance of a
  text is made of a single phone. See #25.

* Bugfix in **wordseg-dpseg** when loading parameters from a configuration file

* In **wordseg-ag**:

  * Bugfix when compiling adaptor grammar on MacOS (removed pstream.h
    from AG). See #15.

  * Replaced std::tr1::unordered_{map,set} by std::unordered_{map,set},
    removed useless code (custom allocator).


wordseg-0.6
-----------

* Features

  * New methods for basic statistics and normalized segmentation
    entropy in **wordseg-stats**

  * New forward/backward option in **wordseg-tp**.

  * New command **wordseg-baseline** that produces a random
    segmentation given the probability of a word boundary. If an
    oracle text is provided, the probability of word boundary is
    estimated from that text.

  * New command **wordseg-syll** estimates syllable boundaries on a
    text using the maximal onset principle. Exemples of onsets and
    vowels files for syllabifications are given in the directory
    `data/syllabification`.

  * Support for punctuation in input of **wordseg-prep** with the
    ``--punctuation`` option (#10).

  * For citation purposes a DOI is now automatically attached to
    each wordseg release.

  * Improved documentation.

* Bugfixes

  * **wordseg-dibs** has been debugged (#16).

  * **wordseg-ag** has been debugged.

  * The following characters are now forbidden in separators, they
    interfer with regular expression matching::

      !#$%&'*+-.^`|~:\\\"

  * Type scoring is now correctly implemented in **wordseg-eval**
    (#10, #14).


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
