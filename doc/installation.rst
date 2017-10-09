.. _installation:

Installation
============


The ``wordseg`` package is made of a collection of command-line
programs and a Python library that can be installed using the
instructions below.

.. note::

   Before going further, please clone the repository from
   github and go in it's root directory::

     git clone https://github.com/bootphon/wordseg.git ./wordseg
     cd ./wordseg


Dependencies
------------

* The package is implemented in python and C++. Minimal dependencies to
  install ``wordseg`` are:

  - cmake_ for building,
  - the boost_ C++ libraries,
  - python3 (python2 is supported but python3 preferred for it's native
    unicode support),
  - a C++ compiler supporting the ``-std=c++11`` option (should be the
    case on most modern compilers),

* Install the required dependencies:

  - on **Ubuntu/Debian**::

      sudo apt-get install python3 python3-pip cmake libboost-program-options-dev

  - on **Mac OSX**::

      brew install python3 boost cmake

  - optionally you can install ``cmake-gui``


System-wide installation
------------------------

This is the recommended installation if you want to use ``wordseg`` on
your personal computer (and you do not want to modify/contribute to
the code). In all other cases, consider installing ``wordseg`` in a
virtual environment.

* Create a directory where to store intermediate (built) files::

      mkdir -p build
      cd build

* Configure the installation with::

    cmake ..

  Or use ``cmake-gui ..`` for a graphical interface where you can
  easily tune installation directories and options.

* Finally compile and install ``wordseg``::

      make
      sudo make install

* If you planned to modify the wordseg's code, use ``make develop``
  instead of ``make install``


Installation in a virtual environment
-------------------------------------

This is the recommended installation if you are not administrator of
your machine, if you are working in multiuser environment (e.g. a
computing cluster) or if you are developing with ``wordseg``.

This installation process is based on the conda_ python package
manager and can be performed on any Linux, Mac OS or Windows system
supported by conda (but you can virtualenv_ as well).

* First install conda from `here <https://conda.io/miniconda.html>`_.

* Create a new Python 3 virtual environment named *wordseg* and
  install some required dependencies::

    conda create --name wordseg python=3 pytest joblib numpy pandas

* Activate your virtual environment::

    source activate wordseg

* Install the wordseg package by following the previous section
  *System-wide installation*.

* To uninstall it, simply remove the ``wordseg`` directory in your
  conda installation folder (once activated it is ``$CONDA_PREFIX``).

.. note::

   Do not forget to activate your virtual environment before using wordseg::

     source activate wordseg


Running the tests
-----------------

* From the `build` directory have a::

    make test

* The tests are located in ``../test`` and are executed by pytest_. In
  case of test failure, you may want to rerun the tests with the
  command `pytest -v ../test` to have a more detailed output.

* pytest supports a lot of options. For exemple to stop the execution
  at the first failure, use ``pytest -x``. To execute a single test
  case, use ``pytest ../test/test_separator.py::test_bad_separators``.


Build the documentation
-----------------------

To build the html documentation (the one you are currently reading),
first install some dependancies::

  sudo apt-get install texlive textlive-latex-extra dvipng
  pip install sphinx sphinx_rtd_theme numpydoc

Then just have a::

  make html

The main page is built as ``build/html/index.html``.


.. _boost: http://www.boost.org/
.. _cmake: https://cmake.org/
.. _conda: https://conda.io/miniconda.html
.. _pytest: https://docs.pytest.org/en/latest/
.. _virtualenv: https://virtualenv.pypa.io/en/stable/
