.. _installation:

============
Installation
============

The ``wordseg`` package is made of a collection of command-line programs and a
Python library that can be installed using the instructions below.

* **On Linux:** native support, tested on `continuous integration
  <https://travis-ci.com/bootphon/wordseg>`_.

* **On MacOS:** two algorithms are actually unsupported on MacOS, ``wordseg-ag``
  and ``wordseg-dpseg``. You can still use them with docker (see
  :ref:`use_docker`).

* **On Windows:** use docker or the `Linux subsytem for Windows
  <https://msdn.microsoft.com/en-us/commandline/wsl/about>`_, see `issue #6
  <https://github.com/bootphon/wordseg/issues/6>`_ for a discussion.

.. note::

   Before going further, please clone the repository from github and go in its
   root directory::

     git clone https://github.com/bootphon/wordseg.git ./wordseg
     cd ./wordseg


Dependencies
============

The package is implemented in Python and C++ and requires extra softwares to
work:

- **Python3** (Python2 is no more supported),

- a **C++ compiler** supporting the C++11 standard,

- **cmake** for configuration and build (see `here <https://cmake.org/>`_),

- the **boost program options** C++ library for option parsing (see `here
  <http://www.boost.org/doc/libs/1_65_1/doc/html/program_options.html>`_),

- the **joblib** Python package for parallel processing (see `here
  <https://joblib.readthedocs.io>`_).

- the **scikit-learn** Python package for statistical analysis (see `here
  <http://scikit-learn.org>`_).


Installation of the wordseg package
===================================

There are three options:

  - :ref:`system_wide`: This is the recommended installation if you want to use
    ``wordseg`` on your personal computer (and you do not want to
    modify/contribute to the code).

  - :ref:`virtual_env`: This is the recommended installation if you are not
    administrator of your machine, if you are working in a multi-user
    environment (e.g. a computing cluster) or if you are developing with
    ``wordseg``.

  - :ref:`use_docker`: This is the recommended installation if you are working
    on Windows or Mac, or in a cloud infrastructure, or if you want a
    reproducible environment.


.. _system_wide:

System-wide installation
------------------------

* Install the required dependencies:

  - on **Ubuntu/Debian**::

      sudo apt-get install python3 python3-pip cmake libboost-program-options-dev

  - on **Mac OSX**::

      brew install python3 boost cmake


* Finally compile and install ``wordseg``::

    [sudo] make install

.. note::

   If you planned to modify the wordseg's code, use ``make develop`` instead of
   ``make install``.


.. _virtual_env:

Installation in a virtual environment
-------------------------------------

.. note::

   If you have already followed the instructions under :ref:`system_wide` skip
   this section to go directly to :ref:`run_tests`.


This installation process is based on the conda_ python package
manager and can be performed on any Linux, Mac OS or Windows system
supported by conda (but you can use virtualenv_ as well).

* First install conda from `here <https://conda.io/miniconda.html>`_.

* Create a new Python 3 virtual environment named ``wordseg`` and
  install the required dependencies::

    conda env create -f environment.yml

* Activate your virtual environment::

    conda activate wordseg

* Install the wordseg package::

    make install


.. note::

   Do not forget to activate your virtual environment before using wordseg::

     conda activate wordseg


.. _use_docker:

Installation in docker
----------------------

We provide a `Dockerfile` to build a docker image of wordseg that can
be run on Linux, Mac and Windows.

* First install docker for you OS:

  - `docker for Mac <https://docs.docker.com/docker-for-mac/install/>`_
  - `docker for Windows <https://docs.docker.com/docker-for-windows/install/>`_,
  - `docker for Linux <https://docs.docker.com/install/linux/docker-ce/ubuntu/>`_.

* Build the `wordseg` image::

    [sudo] docker build -t wordseg .

* Now you can run `wordseg` from within a docker container.

  For exemple run an interactive bash session in docker, mapping a
  data directory on your local host to `/data` in docker::

    [sudo] docker run -v $PWD/test/data/:/data -it wordseg /bin/bash
    # you are now in the docker machine, run wordseg as usual
    root@1d32398b8c8e:/wordseg# head -5 /data/tagged.txt | wordseg-prep | wordseg-dpseg --nfolds 1
    yuw kuhdiytihtwihdhaxspuwn
    yuw hhaev t axkaht dhaet kaorn tuw
    aen d baxnaenax
    guhdchiyz
    ehmehm teystiy kaorn

.. note::

   On Mac use **wordseg-ag** and **wordseg-dpseg** within docker. For
   exemple, if you already have a wordseg installation on your
   computer, you can use it for all but ag an dpseg algorithms, and
   use those two from docker. Here we use the local `wordseg-prep`
   along with the docker `wordseg-dpseg`::

     user@host:~/dev/wordseg$ head -5 $PWD/test/data/tagged.txt | wordseg-prep | docker run -i wordseg wordseg-dpseg --nfolds 1
     yuw kuhdiytihtwihdhaxspuwn
     yuw hhaev t axkaht dhaet kaorn tuw
     aen d baxnaenax
     guhdchiyz
     ehmehm teystiy kaorn


.. _run_tests:

Optional: Run tests to check your installation
==============================================

We recommend you always run this tests suite, because that will allow you to
make sure that all dependencies and all subparts of the package have been
appropriately installed. Simply have a::

    make test


.. note::

   If all your tests passed, then you can skip this section. You have
   successfully installed wordseg. If some of the tests failed, then the
   package's capabilities may be reduced.


* The tests are located in ``./test`` and are executed by pytest_. In
  case of test failure, you may want to rerun the tests with the
  command ``pytest -v ./test`` to have a more detailed output.

* pytest supports a lot of options. For exemple to stop the execution
  at the first failure, use ``pytest -x``. To execute a single test
  case, use ``pytest ./test/test_separator.py::test_bad_separators``.


.. _conda: https://conda.io/miniconda.html
.. _pytest: https://docs.pytest.org/en/latest/
.. _virtualenv: https://virtualenv.pypa.io/en/stable/


Optional: Build the documentation
=================================

To build the html documentation (the one you are currently reading),
first install some dependencies. On **Ubuntu/Debian**::

    sudo apt-get install texlive textlive-latex-extra dvipng
    [sudo] pip install sphinx sphinx_rtd_theme numpydoc

Then have a::

     make html

The documentation is built, it's homepage being ``build/doc/html/index.html``.
