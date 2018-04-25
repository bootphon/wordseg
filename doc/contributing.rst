.. _contributing:

Contributing
============

The **wordseg** package is free software made for open science, we
welcome all the ideas and contributions.

Reporting bugs, issues or ideas
-------------------------------

  Please open an issue on the wordseg's `github page
  <https://github.com/bootphon/wordseg/issues>`_.

  .. note::

     When reporting installation issues, please include the full
     output of the `cmake`, `make` and `make install` commands in your
     message.

Contributing to the code
------------------------
* We strongly welcome direct contributions. This will require some
  familiarity with the GitHub system. For readers who do not have 
  previous experience with git, we recommend the excellent introduction 
  to `git offered by Software Carpentry <https://swcarpentry.github.io/git-novice/>`,
  followed by GitHub's tutorials for `forking <https://help.github.com/articles/fork-a-repo/>`
  and creating `pull requests <https://help.github.com/articles/creating-a-pull-request-from-a-fork/`.

* To contribute directly to the package, please create a `fork
  <https://github.com/bootphon/wordseg/fork>`_ of wordseg in your
  github account.

* Make sure to install wordseg with the ``make develop`` command.

* Submit your work by sending a pull request to the `wordseg master
  branch <https://github.com/bootphon/wordseg/pulls>`_. See `here
  <https://help.github.com/articles/about-pull-requests/>`_ for
  github's help on pull requests.

* Please make sure all the tests are passing before any pull
  request. When possible add new tests for your code as well.

* The package has the following conventions:

  * C++ code follows the `google style guide`_
  * Python code follows the pep8_ standard.
  * Python documentation follows the numpydoc_ standard.
  * HTML documentation (located in the ``./doc`` directory) are
    written in *reStructuredText*, a format similar to markdown (a
    `brief guide here <http://www.sphinx-doc.org/en/stable/rest.html>`_).


.. _Sphinx: http://www.sphinx-doc.org
.. _pep8: http://www.python.org/dev/peps/pep-0008/
.. _numpydoc: https://github.com/numpy/numpy/blob/master/doc/HOWTO_DOCUMENT.rst.txt
.. _google style guide: https://google.github.io/styleguide/cppguide.html
