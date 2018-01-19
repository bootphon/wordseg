.. appendix:

Additional information
===========================================

Information on Adaptor Grammar provided by their original contributors
-------------------------------

The grammar consists of a sequence of rules, one per line, in the
following format::

    [theta [a [b]]] Parent --> Child1 Child2 ...

where theta is the rule's probability (or, with the -E flag, the
Dirichlet prior parameter associated with this rule) in the generator,
and a, b (0<=a<=1, 0<b) are the parameters of the Pitman-Yor adaptor
process.

If a==1 then the Parent is not adapted. If a==0 then the Parent is
sampled with a Chinese Restaurant process (rather than the more
general Pitman-Yor process). If theta==0 then we use the default value
for the rule prior (given by the -w flag).

The start category for the grammar is the Parent category of the first
rule.

If you specify the -C flag, these trees are printed in compact format,
i.e., only cached categories are printed. If you don't specify the -C
flag, cached nodes are suffixed by a '#' followed by a number, which
is the number of customers at this table.

The -A parses-file causes it to print out analyses of the training
data for the last few iterations (the number of iterations is
specified by the -N flag).

The -X eval-cmd causes the program to run eval-cmd as a subprocess and
pipe the current sample trees into it (this is useful for monitoring
convergence).  Note that the eval-cmd is only run _once_; all the
sampled parses of all the training data are piped into it.  Trees
belonging to different iterations are separated by blank lines.

The program can now estimate the Pitman-Yor hyperparameters a and b
for each adapted nonterminal.  To specify a uniform Beta prior on the
a parameter, set "-e 1 -f 1" and to specify a vague Gamma prior on the
b parameter, set "-g 10 -h 0.1" or "-g 100 -h 0.01".

If you want to estimate the values for a and b hyperparameters, their
initial values must be greater than zero.  The -a flag may be useful
here. If a nonterminal has an a value of 1, this means that the
nonterminal is not adapted.

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

     P(x_{n+1} = k) = \\frac{n_k - a}{n + b}

  and the probability that the n+1 sample occupies the new table m+1
  is:

  .. math::

     P(x_{n+1} = m+1) = \\frac{m*a + b}{n + b}

  The probability of a configuration in which a restaurant contains n
  customers at m tables, with n_k customers at table k is:


  .. math::

     a^{-m} \\frac{G(m+b/a)}{G(b/a)} \\frac{G(b)}{G(n+b)} \\prod_{k=1}^m \\frac{G(n_k-a)}{G(1-a)}

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


Information on DIBS provided by original contributor
---------------------------------------------------

A DiBS model assigns, for each phrase-medial diphone, a value between
0 and 1 inclusive (representing the probability the model assigns that
there is a word-boundary there). In practice, these probabilities are
mapped to hard decisions, with the optimal threshold being 0.5.

Without any training at all, a phrasal-DiBS model can assign sensible
defaults (namely, 0, or the context-independent probability of a
medial word boundary, which will always be less than 0.5, and so
effectively equivalent to 0 in a hard-decisions context). The outcome
in this case would be total undersegmentation (for default 0; total
oversegmentation for default 1).

It takes relatively little training to get a DiBS model up to
near-ceiling (*i.e.* the model's intrinsic ceiling: "as good as
that model will get even if you train it forever", rather than
"perfect for that dataset"). Moreover, in principle you can have the
model do its segmentation for the nth sentence based on the stats it
has accumulated for every preceding sentence (and with a little
effort, even on the nth sentence as well). In practice, since I was
never testing on the training set for publication work, but I was
testing on *huge* test sets, I optimized the code for mixed
iterative/batch training, meaning it could read in a training set,
update parameteres, test, and then repeat ad infinitum.