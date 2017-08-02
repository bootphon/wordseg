.. _algorithms:

Algorithms
==========

.. todo::

   Write here all you want about the different algorithms


wordseg-ag
----------

**notes by Mark Johnson**

* Pitman-Yor Context-Free Grammars

  Rules are of format::

    [w [a [b]]] X --> Y1 ... Yn

  where X is a nonterminal and Y1 ... Yn are either terminals or
  nonterminals,

  w is the Dirichlet hyper-parameter (i.e., pseudo-count) associated
  with this rule (a positive real)

  a is the PY "a" constant associated with X (a positive real less
  than 1)

  b is the PY "b" constant associated with X (a positive real)


* Brief recap of Pitman-Yor processes

  Suppose there are n samples occupying m tables.  Then the probability
  that the n+1 sample occupies table 1 <= k <= m is:

  .. math::

     P(x_{n+1} = k) = \frac{n_k - a}{n + b}

  and the probability that the n+1 sample occupies the new table m+1
  is:

  .. math::

     P(x_{n+1} = m+1) = \frac{m*a + b}{n + b}

  The probability of a configuration in which a restaurant contains n
  customers at m tables, with n_k customers at table k is:


  .. math::

     a^{-m} \frac{G(m+b/a)}{G(b/a)} \frac{G(b)}{G(n+b)} \prod_{k=1}^m \frac{G(n_k-a)}{G(1-a)}

  where G is the Gamma function.

* Improving running time

  Several people have been running this code on larger data sets, and
  long running times have become a problem.

  The "right thing" would be to rewrite the code to make it run
  efficiently, but until someone gets around to doing that, I've added
  very simple multi-threading support using OpenMP.

  To compile wordseg-ag with OpenMP support, use the `AG_PARALLEL`
  option for cmake::

    cmake -DAG_PARALLEL=ON ..

  On my 8 core desktop machine, the multi-threaded version runs about
  twice as fast as the single threaded version, albeit using on average
  about 6 cores (i.e., its parallel efficiency is about 33%).

* Quadruple precision

  On very long strings the probabilities estimated by the parser can
  sometimes underflow, especially during the first couple of
  iterations when the probability estimates are still very poor.

  The right way to fix this is to rewrite the program so it rescales
  all of its probabilities during the computation to avoid unflows,
  but until someone gets around to doing this, I've implemented a
  hack, which is just to compile the code using new new
  quadruple-precision floating point maths.

  To compile wordseg-ag on quadruple float precision, use the
  `AG_QUADRUPLE` option for cmake::

    cmake -DAG_QUADRUPLE=ON ..


wordseg-dibs
------------

wordseg-dpseg
-------------

On the original implementation, which comes from `here
<https://github.com/lawphill/phillips-pearl2014>`_, we applied the
following changes:

* conversion to C++11 standard (removed tr1/unsorted_map and mt19937,
  use stdlib implementation instead)
* code cleanup, removed useless functions and code
* complete rewrite of the build process (Makefile, link on boost)


wordseg-puddle
--------------

wordseg-tps
-----------


The case of iterative algorithms
--------------------------------

.. todo::

   explain folding here.
