.. note::
   Version numbers follow the `semantic versioning
   <https://semver.org/>`_ principles.

not yet released
----------------

* New evaluation metric in **wordseg-eval**: adjusted rand index. This
  requires the prepared text to be computed (whereas the other metrics
  only rely on segmented and gold texts), so it is implemented as an
  option ``--rand-index <prep-file>`` in **wordseg-eval**.

  An easiest implementation would have been to change the
  specifications of **wordseg-eval** to take the prepared text instead
  of the gold one, but with prefeered the optional ``--rand-index``
  for backward compatibility.

* Bugfix in ``tools/wordseg-qsub.sh``: wordseg-dibs is correctly handled
  (was a problem with the train file)

wordseg-0.7
-----------

* Added ``tools/wordseg-qsub.sh``, a script to schedule a list of
  segmentation jobs to a cluster running Sun Grid Engine and the
  ``qsub`` scheduler.

* Added example phonological rules and updated contributong guide in
  documentation.

* In **wordseg-prep** ignore empty lines in both gold and segmented
  texts.

* In **wordseg-syll** the syllabification is improved: syllabification
  of words with no vowel, better error messages (see #35, #36).

* In **wordseg-tp** add of the mutual information dependancy
  measure. In the bash command, the argument ``--probability
  {forward,backward}`` is replaced by ``--dependency {ftp,btp,mi}``
  (maintained for backward compatibility). See #40.

* In **wordseg-ag**:

  * niteration is now 2000 by default (was 100),
  * improved log of iterations with ``-vv``,
  * refactored postprocessing code:

    * parallelized
    * constant memory usage (was linear wrt niterations*nutts)
    * tree to words conversion in C++ instead of Python
    * temporary parses file is now gziped (gains a factor of 20 in disk usage)
    * new --temdir option to specify another path for tempfile (default is /tmp)
    * detection of incomplete parses (if any issues a warning)
    * better comments in code, more unit tests


wordseg-0.6.2
-------------

* Improved documentation and algorithms description.

* Docker image now uses python-3.6 from anaconda,

* New tests to ensure replication of scores from `CDSWordSeg
  <https://github.com/alecristia/CDSwordSeg>`_ to wordseg for puddle,
  tp, dibs and dpseg.

* In **wordseg-ag** the ``<grammar>`` and ``<segment-category>``
  parameters are now optional. When omitted a default colloc0 grammar
  is generated from the input text.

* In **wordseg-dpseg**

  * fixed forwarding of some arguments from Python to C++,
  * implementation of dpseg bugfix when single char on first line of
    a fold,
  * use the original random number generator to replicate exactly
    CDSWordSeg.
  * fixed default ngram to bigram (was already bigram but documented
    as unigram).

* In **wordseg-dibs**

  * fixed bug when loading train text at syllable level (new
    *--unit** option)
  * safer use of train text (ensure there are word separators in
    it, ignore empty lines).

* In **wordseg-eval**

  * when called from bash, the scores are now displayed in a fixed
    order. New test to ensure bash and python calls to wordseg lead to
    identical results. See #31.
  * distinction between edge/no edge in boundary scoring. See #21.

* In **wordseg-stats** the scores are now displayed in a fixed order.

* In **wordseg-syll**

  * the ``--tolerant`` option allows to ignore utterances where the
    syllabification failed (the default is to exit the program on the
    first error). See #36.


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
    ``data/syllabification``.

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
