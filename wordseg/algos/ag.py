"""Learn parse trees from a grammar (Adaptor Grammar)

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

"""

# Commented out because those options are not exposed in this wrapper.
#
# The -u and -v flags specify test-sets which are parsed using the
# current PCFG approximation every eval-every iterations, but they are
# not trained on.  These parses are piped into the commands specified by
# the -U and -V parameters respectively.  Just as for the -X eval-cmd,
# these commands are only run _once_.

# Some notes on this AG implementation.
#
# the option --ag-median is not implemented here (running 5 times the
# ag pipeline and take the median result) because it relies on the
# gold file we do not want to expose here. An alternative (still to
# code) is to have something like "for i in 1..5 do wordseg-ag |
# wordseg-eval done > extract_median_result""


import codecs
import collections
import joblib
import logging
import os
import random
import re
import shlex
import shutil
import subprocess
import tempfile

from wordseg import utils


AG_ARGUMENTS = [
    utils.Argument(
        short_name='-d', name='--debug', type=int, default=0,
        help='debugging level'),

    utils.Argument(
        short_name='-x', name='--eval-every', type=int,
        help='estimates parse trees every <int> iterations'),

    utils.Argument(
        short_name='-u', name='--test-file', type='file', default=None,
        help=('test strings to be parsed (but not trained on) '
              'every eval-every iterations, default is to test on input')),

    utils.Argument(
        short_name='-n', name='--niterations', type=int,
        help='number of iterations'),

    utils.Argument(
        short_name='-A', name='--print-analyses-train', type='file',
        help='print analyses of training data at termination'),

    utils.Argument(
        short_name='-N', name='--print-analyses-last', type='file',
        help='print analyses during last nanal-its iterations'),

    utils.Argument(
        short_name='-C', name='--print-compact-trees', type=bool,
        help='print compact trees omitting uncached categories'),

    utils.Argument(
        short_name='-G', name='--print-grammar-file', type='file',
        help='print grammar at termination to a file'),

    utils.Argument(
        short_name='-D', name='--delay-init', type=bool,
        help='delay grammar initialization until all sentences are parsed'),

    utils.Argument(
        short_name='-E', name='--dirichlet-prior', type=bool,
        help='estimate rule prob (theta) using Dirichlet prior'),

    utils.Argument(
        short_name='-H', name='--skip-hastings', type=bool,
        help='skip Hastings correction of tree probabilities'),

    utils.Argument(
        short_name='-I', name='--ordered-parse', type=bool,
        help='parse sentences in order (default is random order)'),

    utils.Argument(
        short_name='-P', name='--predictive-parse-filter', type=bool,
        help='use a predictive Earley parse to filter useless categories'),

    utils.Argument(
        short_name='-R', name='--resample-pycache-niter', type=int,
        help=('resample PY cache strings during first n iterations '
              '(-1 = forever)')),

    utils.Argument(
        short_name='-r', name='--randseed', type=int,
        help=('seed for random number generation, '
              'default is based on current time')),

    utils.Argument(
        short_name='-a', name='--pya', type=float,
        help='default PY a parameter'),

    utils.Argument(
        short_name='-b', name='--pyb', type=float,
        help='default PY b parameter'),

    utils.Argument(
        short_name='-e', name='--pya-beta-a', type=float,
        help=('if positive, parameter of Beta prior on pya; '
              'if negative, number of iterations to anneal pya')),

    utils.Argument(
        short_name='-f', name='--pya-beta-b', type=float,
        help='if positive, parameter of Beta prior on pya'),

    utils.Argument(
        short_name='-g', name='--pyb-gamma-s', type=float,
        help='if non-zero, parameter of Gamma prior on pyb'),

    utils.Argument(
        short_name='-i', name='--pyb-gamma-c', type=float,
        help='parameter of Gamma prior on pyb'),

    utils.Argument(
        short_name='-w', name='--weight', type=float,
        help='default value of theta (or Dirichlet prior) in generator'),

    utils.Argument(
        short_name='-s', name='--train-frac', type=float,
        help=('train only on train-frac percentage of training '
              'sentences (ignore remainder)')),

    utils.Argument(
        short_name='-S', name='--random-training', type=bool,
        help=('randomise training fraction of sentences'
              '(default: training fraction is at front)')),

    utils.Argument(
        short_name='-T', name='--tstart', type=float,
        help='start annealing with this temperature'),

    utils.Argument(
        short_name='-t', name='--tstop', type=float,
        help='stop annealing with this temperature'),

    utils.Argument(
        short_name='-m', name='--anneal-iterations', type=int,
        help='anneal for this many iterations'),

    utils.Argument(
        short_name='-Z', name='--zstop', type=float,
        help='temperature used just before stopping'),

    utils.Argument(
        short_name='-z', name='--ziterations', type=int,
        help='perform zits iterations at temperature ztemp at end of run'),

    # We ignore the following options because they conflict with the
    # wordseg workflow (stdin > wordseg-cmd > stdout). In this AG
    # wrapper the test2 file is ignored and the test1 is the input
    # text sent to stdout.
    #
    # utils.Argument(
    #     short_name='-X', name='--eval-parses-cmd', type='file',
    #     help=('pipe each run\'s parses into this command '
    #           '(empty line separates runs)')),
    # utils.Argument(
    #     short_name='-Y', name='--eval-grammar-cmd', type='file',
    #     help=('pipe each run\'s grammar-rules into this command '
    #           '(empty line separates runs)')),
    # utils.Argument(
    #     short_name='-U', name='--test1-eval', type='file',
    #     help='parses of test1-file are piped into this command'),
    # utils.Argument(
    #     short_name='-v', name='--test2-file', type='file',
    #     help=('test strings to be parsed (but not trained on) '
    #           'every eval-every iterations')),
    # utils.Argument(
    #     short_name='-V', name='--test2-eval', type='file',
    #     help='parses of test2-file are piped into this command')
]


DEFAULT_ARGS = ('-E -d 0 -a 0.0001 -b 10000 -e 1 -f 1 '
                '-g 100 -h 0.01 -R -1 -P -x 10')
"""Default Adaptor Grammar parameters"""


def get_grammar_files():
    """Returns a list of example grammar files bundled with wordseg

    Grammar files have the \*.lt extension and are stored in the
    directory `wordseg/data/ag`.

    Raises
    ------
    RuntimeError
        If the configuration directory is not found or if there is no
        grammar files in it.
    pkg_resources.DistributionNotFound
        If 'wordseg' is not correctly installed

    """
    return utils.get_config_files('ag', '.lt')


def build_colloc0_grammar(phones):
    """Builds a Colloc0 grammar from a list of phones

    Parameters
    ----------
    phones : list of str
        The list of existing phones in the grammar

    Returns
    -------
    grammar : str
        The generated grammar as a string, just have a open(file,
        'w').write(grammar) to save it to disk.

    """
    g = '\n'.join([
        "1 1 Sentence --> Colloc0s",
        "1 1 Colloc0s --> Colloc0",
        "1 1 Colloc0s --> Colloc0 Colloc0s",
        "Colloc0 --> Phonemes",
        "1 1 Phonemes --> Phoneme",
        "1 1 Phonemes --> Phoneme Phonemes"]) + '\n'

    for p in sorted(phones):
        g += '1 1 Phoneme -->' + p + '\n'

    return g


def is_parent_in_grammar(grammar_file, parent):
    """Returns True if the `parent` is in the grammar

    Parents are the first word of each line in the grammar file.

    """
    for line in codecs.open(grammar_file, 'r', encoding='utf8'):
        if line.split(' ')[0] == parent:
            return True
    return False


def _run_ag_single(text, grammar_file, args, test_text=None,
                   log_level=logging.ERROR, log_name='wordseg-ag'):
    """Runs the AG program a single time and returns the computed parse trees

    Parameters
    ----------
    text : sequence
        The list of utterances to train the model on, and to segment
        if `test_text` is None.
    grammar_file : str
        The path to the grammar file to use for segmentation
    args : str
        Command line options to run the AG program with, use
        'wordseg-ag --help' to have a complete list of available
        options
    test_text : sequence, optional
        If not None, the list of utterances to segment on the model
        learned from `text`
    log : logging.Logger, optional
        A logger where to send log messages

    Returns
    -------
    A list of parse trees, one tree per input utterance in `text`

    Raises
    ------
    RuntimeError
        If the AG program fails and returns an error code

    """
    log = utils.get_logger(name=log_name, level=log_level)

    # setup the train text as a multiline string
    train_text = '\n'.join(utt.strip() for utt in text) + '\n'

    # setup the test text as well
    test_text = train_text if test_text is None else (
        '\n'.join(utt.strip() for utt in test_text) + '\n')

    # we need to write the test text as a tempfile, so we create a
    # tempdir. The directory and its content is automatically erased
    # when done
    temp_dir = tempfile.mkdtemp()
    try:
        # write the test text as a temporary file. ylt extension is
        # the one used in the original AG implementation
        test_tmpfile = os.path.join(temp_dir, 'test.ylt')
        codecs.open(test_tmpfile, 'w', encoding='utf8').write(test_text)

        # generate the command to run as a subprocess
        command = ('{binary} {grammar} {args} -u {test} '.format(
            binary=utils.get_binary('ag'),
            grammar=grammar_file,
            args=args,
            test=test_tmpfile))

        log.info('running %s', command)

        # run the command as a subprocess
        process = subprocess.Popen(
            shlex.split(command),
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        parses, messages = process.communicate(train_text.encode('utf8'))

        # log.debug the AG messages AFTER EXECUTION
        messages = messages.decode('utf8').split('\n')
        for msg in messages:
            log.debug(re.sub('^# ', '', msg).strip())

        # fail if AG returns an error code
        if process.returncode:
            raise RuntimeError(
                'fails with error code {}'.format(process.returncode))

        return parses.decode('utf8').strip().split('\n')
    finally:
        shutil.rmtree(temp_dir)


def _yield_trees(trees, ignore_firsts=0):
    """Yields parse trees, ignoring the first ones"""
    ntrees = 0
    tree = []

    # read input line per line, yield at each empty line
    for line in trees:
        line = line.strip()
        if len(line) == 0 and len(tree) > 0:
            ntrees += 1
            if ntrees > ignore_firsts:
                yield tree
            tree = []
        else:
            tree.append(line)

    # yield the last tree
    if len(tree) > 0 and ntrees >= ignore_firsts:
        yield tree


def _tree_string(tree, ignore_terminal_re, word_re):
    def simplify_terminal(t):
        if len(t) > 0 and t[0] == '\\':
            return t[1:]
        else:
            return t

    def is_terminal(subtree):
        """True if this subtree consists of a single terminal node

        (i.e., a word or an empty node).

        """
        return not isinstance(subtree, list)

    def tree_children(tree):
        """Returns a list of the child subtrees of tree."""
        if isinstance(tree, list):
            return tree[1:]
        else:
            return []

    def tree_label(tree):
        """Returns the label on the root node of tree."""
        if isinstance(tree, list):
            return tree[0]
        else:
            return tree

    def visit(node, words_sofar, segs_sofar):
        """Does a preorder visit of the nodes in the tree"""
        if is_terminal(node):
            if not ignore_terminal_re.match(node):
                segs_sofar.append(simplify_terminal(node))
            return words_sofar, segs_sofar

        for child in tree_children(node):
            words_sofar, segs_sofar = visit(child, words_sofar, segs_sofar)

        if word_re.match(tree_label(node)) and segs_sofar != []:
            words_sofar.append(''.join(segs_sofar))
            segs_sofar = []

        return words_sofar, segs_sofar

    words_sofar, segs_sofar = visit(tree, [], [])
    if segs_sofar:  # append any unattached segments as a word
        words_sofar.append(''.join(segs_sofar))

    return ' '.join(words_sofar)


def _tree2words(tree, nepochs=0, skip=0, rate=1,
                score_category_re=r'Word\b',
                ignore_terminals_re=r'^[$]{3}$'):
    """Extracts segmented words from raw parse trees

    Parameters
    ----------
    tree : sequence
        The parse tree on which to extract the words
    nepochs : int, optional
        Total number of epochs
    skip : int, optional
        Initial fraction of epochs to skip
    rate : int, optional
        Input provides samples every rate epochs
    score_category_re : str, optional
        Score categories in tree input that match this regex
    ignore_terminals_re : str, optional
        Ignore terminals that match this regular expression

    Returns
    -------
    words : list
        The words extracted from the parse tree

    """
    _openpar_re = re.compile(r"\s*\(\s*([^ \t\n\r\f\v()]*)\s*")
    _closepar_re = re.compile(r"\s*\)\s*")
    _terminal_re = re.compile(r"\s*((?:[^ \\\t\n\r\f\v()]|\\.)+)\s*")

    def string_trees(s):
        """Returns a list of the trees in PTB-format string s"""
        trees = []
        _string_trees(trees, s)
        return trees

    def _string_trees(trees, s, pos=0):
        """Reads a sequence of trees in string s[pos:]

        Appends the trees to the argument trees. Returns the ending
        position of those trees in s.

        """
        while pos < len(s):
            closepar_mo = _closepar_re.match(s, pos)
            if closepar_mo:
                return closepar_mo.end()
            openpar_mo = _openpar_re.match(s, pos)
            if openpar_mo:
                tree = [openpar_mo.group(1)]
                trees.append(tree)
                pos = _string_trees(tree, s, openpar_mo.end())
            else:
                terminal_mo = _terminal_re.match(s, pos)
                trees.append(terminal_mo.group(1))
                pos = terminal_mo.end()
        return pos

    word_re = re.compile(score_category_re)
    ignore_terminals_re = re.compile(ignore_terminals_re)

    nskip = int(skip * nepochs / rate)

    words = []
    for line in tree:
        line = line.strip()
        if len(line) > 0 and nskip <= 0:
            trees = string_trees(line)
            trees.insert(0, 'ROOT')
            words.append(
                _tree_string(trees, ignore_terminals_re, word_re).strip())
        else:
            if nskip <= 0:
                words.append('\n')
            nskip -= 1
        trees = string_trees(line)
        trees.insert(0, 'ROOT')

    return words


def _most_common_parses(trees):
    """For each utterance in the text find its more common parse in trees"""
    nutts = len(trees[0])
    for utt_idx in range(nutts):
        utterances = (tree[utt_idx] for tree in trees)
        yield collections.Counter(utterances).most_common(1)[0][0]


def _setup_seed(args, nruns):
    """Setup a unique seed for each run in `args`"""
    new = [args] * nruns
    for n in range(nruns):
        if '-r' in args:
            # extract the seed from the arguments string
            seed = int(re.sub(r'^.*\-r *([0-9]+).*$', '\g<1>', args))

            # setup new seed for each run
            new[n] = re.sub('\-r *([0-9]+)', '-r {}'.format(seed + n), args)
        else:
            new[n] = args + ' -r {}'.format(random.randint(0, 2**16))
    return new


def segment(text, grammar_file, segment_category,
            args=DEFAULT_ARGS, test_text=None, ignore_first_parses=0,
            nruns=8, njobs=1, log=utils.null_logger()):
    """Segment a text using the Adaptor Grammar algorithm

    The algorithm is ran 8 times in parallel and the results are
    collapsed. We ensure the random seed to be different for each run.

    Parameters
    ----------
    text : sequence
        The list of utterances to train the model on, and to segment
        if `test_text` is None.
    grammar_file : str
        The path to the grammar file to use for segmentation
    segment_category : str
        The category to segment the text with, must be an existing
        parent in the grammar (i.e. the `segment_category` must be
        present in the left column of the grammar file)
    args : str, optional
        Command line options to run the AG program with, use
        'wordseg-ag --help' to have a complete list of available
        options
    test_text : sequence, optional
        If not None, the list of utterances to segment using the model
        learned from `text`
    ignore_first_parses : int, optional
        Ignore the first parses from the algorithm output
    nruns : int, optional
        number of runs to execute and output parses to collapse. This
        number 8 comes from the original recipe provided by M Jonhson.
    njobs : int, optional
        The number of parallel subprocesses to run
    log : logging.Logger, optional
        A logger where to send log messages

    Returns
    -------
    segmented : list
        The test utterances with estimated word boundaries

    Raises
    ------
    RuntimeError
        If one of the AG subprocesses fails or returns an error code.
        If the `score_category` is not found in the grammar.

    """
    # make sure the grammar file exists
    if not os.path.isfile(grammar_file):
        raise RuntimeError('grammar file not found: {}'.format(grammar_file))

    # make sure the segment category is valid
    if not is_parent_in_grammar(grammar_file, segment_category):
        raise RuntimeError(
            'category "{}" not found in the grammar'
            .format(segment_category))

    # force the train text from sequence to list
    text = list(text)
    log.info('train data: %s utterances loaded', len(text))

    # if any, force the test text from sequence to list
    if test_text is not None:
        test_text = list(test_text)
        log.info('segmentation data: %s utterances loaded', len(test_text))
    else:
        log.info('no test text provided, segmentation on train data')
    log.info('parameters are: "%s"', args)

    # ensure we have a different seed for all runs. If the seed is
    # specified in command line (-r SEED) then feed SEED+i for i the
    # ith run. Else put a random seed to each run.
    args = _setup_seed(args, nruns)
    log.debug('random seeds are: %s', ', '.join(
        [arg.split('-r ')[1].split(' ')[0] for arg in args]))

    # parallel runs of the AG algorithm, raw_parses is a list of
    # `nruns_inner` raw parse trees we need to postprocess to obtain
    # words.
    log.info('running (%d times)...', nruns)

    raw_trees = joblib.Parallel(
        n_jobs=njobs, backend="threading", verbose=0)(
            joblib.delayed(_run_ag_single)(
                text, grammar_file, args[n], test_text=test_text,
                log_level=log.getEffectiveLevel(),
                log_name='wordseg-ag - run {}'.format(n + 1))
            for n in range(nruns))

    log.info(
        'collapsing the parse trees%s', '' if ignore_first_parses == 0 else
        ', ignore the {} first parses of each run'.format(ignore_first_parses))

    trees = [
        tree for trees in raw_trees for tree
        in _yield_trees(trees, ignore_firsts=ignore_first_parses)]

    log.info(
        'collapsed %s trees, extracting boundaries for category "%s"',
        len(trees), segment_category)

    segmented_trees = [
        _tree2words(tree, score_category_re=segment_category)
        for tree in trees]

    # for each utterance, get the most frequent parse found in the
    # sequence of parses
    log.info('extracting most common parse of each utterance')
    segmented = _most_common_parses(segmented_trees)
    return list(segmented)


def _add_arguments(parser):
    """Add algorithm specific options to the parser"""
    parser.add_argument(
        'grammar', metavar='<grammar-file>',
        help=('read the grammar from this file, for exemple of grammars see {}'
              .format(os.path.dirname(get_grammar_files()[0]))))

    parser.add_argument(
        'segment_category', metavar='<segment-category>',
        help=('the grammar category to segment the text with, '
              'must be a valid parent in the grammar (ie defined in the '
              'left column of the grammar file)'))

    parser.add_argument(
        '-j', '--njobs', type=int, metavar='<int>', default=1,
        help='number of parallel jobs to use, default is %(default)s')

    # TODO not yet implemented
    # parser.add_argument(
    #     '-c', '--config-file', metavar='<file>',
    #     help=('configuration file to read the algorithm parameters from, '
    #           'for example configuration files see {}'.format(
    #               os.path.dirname(utils.get_config_files('ag')[0]))))

    parser.add_argument(
        '--nruns', type=int, default=8, metavar='<int>',
        help=('number of runs to execute and output parses to collapse. '
              '8 (default) comes from the original recipe '
              'provided by M Jonhson.'))

    parser.add_argument(
        '--ignore-first-parses', type=int, metavar='<int>', default=0,
        help='discard the n first parses of each run, '
        'default is %(default)s')

    group = parser.add_argument_group('algorithm options')
    for arg in AG_ARGUMENTS:
        arg.add_to_parser(group)


def _command_line_arguments(args):
    """Returns a string of command line options for the AG binary

    Builds the command line options of the AG program from Python
    options. Options are in the form '-{short_name} {value}' (the AG
    program takes only short option names)

    Parameters
    ----------
    args : Namespace
        Argument objects as outputed by the parse_args() method

    Returns
    -------
    str
        Options to feed the AG program with

    """
    # options short name
    short_names = {arg.parsed_name(): arg.short_name for arg in AG_ARGUMENTS}

    # arguments for use in Python we don't forward to the AG program
    excluded_args = ['verbose', 'quiet', 'input',
                     'output', 'njobs', 'test_file']

    ag_args = {}
    for k, v in vars(args).items():
        # ignored arguments
        if k in excluded_args or v in (None, False):
            continue

        # convert the options to their shortname
        try:
            k = short_names[k]

            # special case: -i -> -h conversion because -h is reserved
            # for --help in Python
            if k == '-i':
                k = '-h'

            ag_args[k] = v
        except KeyError:
            pass

    # flag options in the form in the form '-{short_name}'
    novalue_options = [short_names[arg.parsed_name()]
                       for arg in AG_ARGUMENTS if arg.type == bool]

    # return the options formatted with short names
    return ' '.join(
        '{}{}'.format(k, ' ' + str(v) if k not in novalue_options else '')
        for k, v in ag_args.items())


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-ag' command"""
    # initializing standard i/o and arguments
    streamin, streamout, _, log, args = utils.prepare_main(
        name='wordseg-ag',
        description="""Adaptor Grammar word segmentation algorithm""",
        add_arguments=_add_arguments)

    # build the AG command line (C++ side) from the parsed arguments
    # (Python side)
    cmd_args = _command_line_arguments(args)

    # load the test text if any
    test_text = None
    if args.test_file is not None:
        if not os.path.isfile(args.test_file):
            raise RuntimeError(
                'test file not found: {}'.format(args.test_file))
        test_text = codecs.open(args.test_file, 'r', encoding='utf8')

    # call the AG algorithm
    segmented = segment(
        streamin,
        args.grammar,
        args.segment_category,
        args=cmd_args,
        test_text=test_text,
        ignore_first_parses=args.ignore_first_parses,
        nruns=args.nruns, njobs=args.njobs, log=log)

    # output the results
    streamout.write('\n'.join(segmented) + '\n')


if __name__ == '__main__':
    main()
