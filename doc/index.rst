.. wordseg documentation master file, created by
   sphinx-quickstart on Thu Apr  6 20:18:12 2017.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to the wordseg's documentation page
===========================================

* Provides **a collection of tools for text based word segmentation**.

* Covers the whole segmentation pipeline: data preprocessing,
  algorithms, evaluation and descriptive statistics.

* Implements 6 segmentation algorithms and a baseline (see
  :ref:`algorithms`).

* From **bash**: provides a set of commands for text preparation,
  segmentation and evaluation to be used in conjunction to any other
  bash utilities (see :ref:`commands`),

* From **python**: provides the ``wordseg`` Python package (see :ref:`python`).

.. note::

   * The source code of **wordseg** is available on its `github page
     <https://github.com/bootphon/wordseg>`_,

   * The latest release is available for download `here
     <https://github.com/bootphon/wordseg/releases/latest>`_.

.. important::

   **Citation information**

    * In order to cite **wordseg** in your own work, please use the
      following paper:

      .. code-block:: bibtex

         @article{bernard_brm_2019,
           author = {Mathieu Bernard, Roland Thiolliere, Amanda
             Saksida, Georgia R. Loukatou, Elin Larsen, Mark Johnson,
             Laia Fibla Reixachs, Emmanuel Dupoux, Robert Daland, Xuan
             Nga Cao, Alejandrina Cristia},
           title = {WordSeg: Standardizing unsupervised word form
             segmentation from text},
           journal      = {Behavioral Research Methods},
           year         = {2019, in press}
         }

    * To specifically cite the software, the following DOI always
      resolve to the latest version:
      https://doi.org/10.5281/zenodo.1101048. From the right panel of
      that page, export the citation to the format of your choice. For
      instance, *wordseg-0.7.1* is exported to BibTex as:

      .. code-block:: bibtex

         @misc{mathieu_bernard_2018_1471532,
           author       = {Mathieu Bernard},
           title        = {bootphon/wordseg: wordseg-0.7.1},
           month        = oct,
           year         = 2018,
           doi          = {10.5281/zenodo.1471532},
           url          = {https://doi.org/10.5281/zenodo.1471532}
         }


.. toctree::
   :maxdepth: 2
   :caption: Contents

   installation
   overview
   algorithms
   tutorial
   bash_reference
   python_reference
   release_notes
   contributing
   copyright
