.. _release_notes:

Release notes
=============

This section details the changes over *wordseg* releases. This project
started as a complete rewrite of the word segmentation pipeline in the
`CDSwordSeg <https://github.com/alecristia/CDSwordSeg>`_ project.


wordseg-0.5.1
-------------

* Support for punctuation in input of ``wordseg-prep`` with the
  ``--punctuation`` option (disabled by default).


wordseg-0.5
-----------

* Implementation of Adaptor Grammar as ``wordseg-ag``,
* Installation now relies on cmake (was python setuptools),
* Improvements in tests and documentation,
* Various bugfixes.


wordseg-0.4.1
-------------

* First public release,
* Four algorithms (tp, puddle, dpseg, dibs),
* Segmentation prepocessing and evaluation,
* Unit tests and documentation,
* On the `original implementation
  <https://github.com/lawphill/phillips-pearl2014>`_, we applied the
  following changes:

  * conversion to C++11 standard,
  * replaced ``tr1/unsorted_map`` and ``mt19937`` by the standard library,
  * code cleanup, removed useless functions and code,
  * complete rewrite of the build process (Makefile, link on boost).
