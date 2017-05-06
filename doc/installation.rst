Installation
============

.. note::

   Before going further, please clone the repository from
   github and go in it's root directory::

     git clone https://github.com/mmmaat/wordseg.git ./wordseg
     cd ./wordseg


The ``wordseg`` package is made of a collection of command-line
programs that can be installed using the above instructions. It is
implemented in Python3 and C++, so the minimal dependencies to install
``wordseg`` are *python3*, *make*, *gcc*. The C++ part also depends on
the *boost-program-options* library.



System-wide installation
------------------------

This is the recommended installation if you want to use ``wordseg`` on
your personal computer (and you do not want to modify/contribute to
the code). In all other cases, consider installing ``wordseg`` in a
virtual environment.

.. todo::

   Actually only documented for Debian/Ubuntu (``apt-get``)

::
   sudo apt-get install python3 python3-setuptools python3-pandas \
        build-essential libboost-program-options-dev

   python3 setup.py build

   sudo python3 setup.py install


.. note:: Uninstall

   With this system-wide installation there is no clear uninstalling
   process, you need to delete the files manually (see
   https://stackoverflow.com/questions/1550226 )::

     sudo python3 setup.py install --record files.txt
     cat files.txt | sudo xargs rm -rf
     sudo rm files.txt


Installation in a virtual environment
-------------------------------------

This is the recommended installation if you are not administrator of
your machine, if you are working in multiuser environment (e.g. a
computing cluster) or if you are developing with ``wordseg``.

This installation process is based on the conda_ python package
manager and can be performed on any Linux, Mac OS or Windows system
supported by conda.

* First install conda from `here <https://conda.io/miniconda.html>`_

* Create a new Python 3 virtual environment named *wordseg* and
  install the required dependencies (include pytest and sphinx for
  test and documentation, see above)::

    conda create --name wordseg python=3 \
          pytest pytest-runner sphinx sphinx_rtd_theme \
          boost joblib numpy pandas

* Activate your virtual environment::

    source activate wordseg

* Install the wordseg package (this will install the commandline tools
  in your $HOME and make them callable from the terminal). If you do
  not want to edit the code::

    python setup.py install

  Or if you want to edit the code::

    python setup.py develop

* To uninstall it, simply remove the ``wordseg`` directory in your
  conda folder (generally ``~/.conda/envs`` or ``~/.anaconda/envs``)


Running the tests
-----------------

* To run the test suite have a::

    python setup.py test

* The tests are located in ``./test`` and are executed by
  pytest_. ``python setup.py test`` is in fact an alias for ``pytest
  --verbose``.

* pytest supports a lot of options. For exemple to stop the execution
  at the first failure, use ``pytest -x``. To execute a single test
  case, use ``pytest ./test/test_separator.py::test_bad_separators``.


Build the documentation
-----------------------

To build the html documentation (the one you are currently reading),
have a::

  python setup.py build_sphinx

The main page is then ``./build/sphinx/html/index.html``.

.. _conda: https://conda.io/miniconda.html
.. _pytest: https://docs.pytest.org/en/latest/
