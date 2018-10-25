Contributing
============

The **wordseg** package is free software made for open science, we
welcome all the ideas and contributions.

.. note::

   We strongly welcome direct contributions. This will require some
   familiarity with the GitHub system. For readers who do not have
   previous experience with git, we recommend the excellent
   introduction to `git offered by Software Carpentry
   <https://swcarpentry.github.io/git-novice/>`_, followed by GitHub's
   tutorials for `forking
   <https://help.github.com/articles/fork-a-repo/>`_ and creating
   `pull requests
   <https://help.github.com/articles/creating-a-pull-request-from-a-fork/>`_.


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

* To contribute directly to the package, please create a `fork
  <https://github.com/bootphon/wordseg/fork>`_ of wordseg in your
  github account.

* Create a dedicated branch on your fork.

* Make sure to install wordseg with the ``make develop`` command.

* Submit your work by sending a pull request to the `wordseg master
  branch <https://github.com/bootphon/wordseg/pulls>`_. See `here
  <https://help.github.com/articles/about-pull-requests/>`_ for
  github's help on pull requests.

* Please make sure all the tests are passing before any pull
  request. When possible add new tests for your code as well.


Coding style
------------

In order to have a consistent style across all the code base, the
package follows the following conventions:

* C++ code follows the `google style guide`_,

* Python code follows the pep8_ standard,

* Python documentation follows the numpydoc_ standard, this allows
  automatic conversion of Python docstrings into a HTML based
  documentation of the code.

* HTML documentation (located in the ``./doc`` directory) are written
  in *reStructuredText*, a format similar to markdown (a `brief guide
  here <http://www.sphinx-doc.org/en/stable/rest.html>`_).


Add your own segmentation algorithm
-----------------------------------

* Once you forked the project as detailed above, you are ready to
  integrate your algorithm to the existing package. To get
  inspiration, you can have a look to the `wordseg-baseline
  <https://github.com/bootphon/wordseg/blob/master/wordseg/algos/baseline.py>`_
  implementation.

* Contact us by creating an issue on github if you need help.

* Once you are ready, you can contribute your algorithm to the project
  by sending a pull request on github.

.. _Sphinx: http://www.sphinx-doc.org
.. _pep8: http://www.python.org/dev/peps/pep-0008/
.. _numpydoc: https://github.com/numpy/numpy/blob/master/doc/HOWTO_DOCUMENT.rst.txt
.. _google style guide: https://google.github.io/styleguide/cppguide.html
