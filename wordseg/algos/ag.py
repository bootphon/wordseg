"""Learn parse trees from a grammar (Adaptor Grammar)

This algorithm adds word boundaries after adapting a grammar.


"""


# Some notes on this AG implementation (with respect to CDSWordSeg).
#
# the option --ag-median is not implemented here (running 5 times the
# ag pipeline and take the median result) because it relies on the
# gold file we do not want to expose here. An alternative (still to
# code) is to have something like "for i in 1..5 do wordseg-ag |
# wordseg-eval done > extract_median_result""
#


import codecs
import collections
import datetime
import gzip
import joblib
import logging
import os
import random
import re
import shlex
import shutil
import subprocess
import tempfile
import threading

from wordseg import utils


#-------------------------------------------------------------------------------
#  Adaptor Grammar arguments
#-------------------------------------------------------------------------------


AG_ARGUMENTS = [
    utils.Argument(
        short_name='-d', name='--debug', type=int, default=100,
        help='debugging level, regulates the amount of debug messages, '
        'to be used in conjuction with -vv'),

    utils.Argument(
        short_name='-x', name='--eval-every', type=int,
        help='estimates parse trees every <int> iterations'),

    utils.Argument(
        short_name='-u', name='--test-file', type='file', default=None,
        help=('test strings to be parsed (but not trained on) '
              'every eval-every iterations, default is to test on input')),

    utils.Argument(
        short_name='-n', name='--niterations', type=int,
        help='number of iterations (default to 2000)'),

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


def _setup_seed(args, nruns):
    """Setup a unique seed for each run in `args`

    Return a list of `nruns` copies of `args` in which only the '-r
    <seed>' option is modified. This ensure all the AG runs are done
    with a different random seed.

    """
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


#-------------------------------------------------------------------------------
#  Grammar files and format
#-------------------------------------------------------------------------------


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

    for p in sorted(set(phones)):
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


def check_grammar(grammar_file, category):
    """Return True if the grammar is valid for that `category`

    Raise a RuntimeError if the category is not a parent in the
    grammar, or if the grammar file is not found or not readable.

    """
    # make sure the grammar file exists
    if not os.path.isfile(grammar_file):
        raise RuntimeError('grammar file not found: {}'.format(grammar_file))

    # make sure the file is readable
    if not os.access(grammar_file, os.R_OK):
        raise RuntimeError('grammar file not readable: {}'.format(grammar_file))

    # make sure the segment category is valid
    if not is_parent_in_grammar(grammar_file, category):
        raise RuntimeError(
            'category "{}" not found in the grammar'
            .format(category))

    return True


#-------------------------------------------------------------------------------
#  Wrapper on AG C++ program
#-------------------------------------------------------------------------------


def _segment_single(parse_counter, train_text, grammar_file,
                    category, ignore_first_parses, args, test_text=None,
                    log_level=logging.ERROR, log_name='wordseg-ag'):
    """Executes a single run of the AG program and postprocessing

    The function returns nothing but updates the `parse_counter` with
    the parses built during the AG's iterations. It does the following
    steps:

    * create a logger to forward AG messages.

    * create a temporary directory, write train/test data files in it.

    * execute the AG program with the given grammar and arguments
      (in a subprocess, using a bash script as proxy).

    * postprocess the resulting parse trees to extract the segmented
      utterance from the raw PTB format.

    * update the `parse_counter` with the segmented utterances.


    Parameters
    ----------
    parse_counter : ParseCounter
        Count the segmented utterances obtained for each parses
    train_text : sequence
        The list of utterances to train the model on, and to segment
        if `test_text` is None.
    grammar_file : str
        The path to the grammar file to use for segmentation
    category : str
        The category to segment the text with, must be an existing
        parent in the grammar (i.e. the `segment_category` must be
        present in the left column of the grammar file), default to
        'Colloc0'.
    ignore_first_parses : int
        Ignore the first parses from the algorithm output
    args : str
        Command line options to run the AG program with, use
        'wordseg-ag --help' to have a complete list of available
        options
    test_text : sequence, optional
        If not None, the test text contains the list of utterances to
        segment on the model learned from `train_text`
    log_level : logging.Level, optional
        The level of the wrapping log (must be DEBUG to display
        messages from AG, default to ERROR).
    log_name: str, optional
        The name of the logger where to send log messages, default to
        'wordseg-ag'.

    Raises
    ------
    RuntimeError
        If the AG program fails and returns an error code

    """
    log = utils.get_logger(name=log_name, level=log_level)

    # we need to write some intermediate files, so we create a
    # tempdir. The directory and its content is automatically erased
    # when done
    temp_dir = tempfile.mkdtemp()
    log.debug('created tempdir: %s', temp_dir)

    try:
        # setup the train text as a temp file. ylt extension is the
        # one used in the original AG implementation. TODO actually we
        # are copying train and test files for each run, this is
        # useless (maybe expose train_file and test_file as arguments
        # instead of train_text / test_text?).
        train_text = '\n'.join(utt.strip() for utt in train_text) + '\n'
        train_file = os.path.join(temp_dir, 'train.ylt')
        codecs.open(train_file, 'w', encoding='utf8').write(train_text)

        # setup the test text as well
        if test_text is None:
            test_file = train_file
        else:
            test_text = '\n'.join(utt.strip() for utt in test_text) + '\n'
            test_file = os.path.join(temp_dir, 'test.ylt')
            codecs.open(test_file, 'w', encoding='utf8').write(test_text)

        # create a file to store output (compressed PTB-format parse trees)
        output_file = os.path.join(temp_dir, 'output.gz')

        # write the call to AG in a bash script
        script_file = os.path.join(temp_dir, 'script.sh')
        command = ('cat {train} | {bin} {grammar} {args} -u {test} '
                   '| gzip -c > {output}'.format(
                       train=train_file,
                       bin=utils.get_binary('ag'),
                       grammar=grammar_file,
                       args=args,
                       test=test_file,
                       output=output_file))
        codecs.open(script_file, 'w', encoding='utf8').write(command + '\n')

        log.info('running "%s"', command)

        t1 = datetime.datetime.now()

        # run the command as a subprocess
        process = subprocess.Popen(
            shlex.split('bash {}'.format(script_file)),
            stdin=None,
            stdout=None,
            stderr=subprocess.PIPE)

        # log.debug the AG messages during execution
        def stderr2log(line):
            try:
                line = line.decode('utf8')
            except AttributeError:
                line = str(line)
            line = re.sub('^# ', '', line.strip())
            if line:
                log.debug(line)

        # join the command output to log (from
        # https://stackoverflow.com/questions/35488927)
        def consume_lines(pipe, consume):
            with pipe:
                # NOTE: workaround read-ahead bug
                for line in iter(pipe.readline, b''):
                    consume(line)
                consume('\n')

        threading.Thread(
            target=consume_lines,
            args=[process.stderr, lambda line: stderr2log(line)]).start()

        process.wait()

        t2 = datetime.datetime.now()

        # fail if AG returns an error code
        if process.returncode:
            raise RuntimeError(
                'segmentation fails with error code {}'
                .format(process.returncode))

        log.info('segmentation done, took {}'.format(t2 - t1))

        postprocess(
            parse_counter, output_file, category, ignore_first_parses, log)

        t3 = datetime.datetime.now()
        log.info('postprocessing done, took %s', t3 - t2)

    finally:
        shutil.rmtree(temp_dir)


#-------------------------------------------------------------------------------
#  Postprocessing
#
# The parse trees outputed by AG need further postprocessing to
# extract the segmented words. The trees are in PTB-format (Penn
# Treebank format). The following functions extract words from trees
# and count the most frequent segmentation for each utterance.
#
#-------------------------------------------------------------------------------


class TreeTokenizer(object):
    """Tokenize an AG tree in a suite of words

    This class parses raw PTB tress as outputed by AG. It extracts
    word-segmented utterances. For example, at Colloc0 level, the
    following tree become "s kr ahmp shaxs":

    (Sentence (Colloc0s (Colloc0#1 (Phonemes (Phoneme s))) (Colloc0s
    (Colloc0#3 (Phonemes (Phoneme k) (Phonemes (Phoneme r))))
    (Colloc0s (Colloc0#3 (Phonemes (Phoneme ah) (Phonemes (Phoneme m)
    (Phonemes (Phoneme p))))) (Colloc0s (Colloc0#3 (Phonemes (Phoneme
    sh) (Phonemes (Phoneme ax) (Phonemes (Phoneme s))))))))))

    """
    # some constant regexp to parse AG trees
    _openpar_re = re.compile(r"\s*\(\s*([^ \t\n\r\f\v()]*)\s*")
    _closepar_re = re.compile(r"\s*\)\s*")
    _terminal_re = re.compile(r"\s*((?:[^ \\\t\n\r\f\v()]|\\.)+)\s*")

    def __init__(self, category_re, ignore_terminals_re=r'^[$]{3}$'):
        self.word_re = re.compile(category_re)
        self.ignore_terminals_re = re.compile(ignore_terminals_re)

    def tree2words(self, tree):
        """Tokenize the string `tree` into a suite of words"""
        trees = ['ROOT'] + self.tree2list(tree.strip())
        return self.list2words(trees)

    def tree2list(self, tree):
        """Returns nested lists of the `tree` in PTB-format

        Exemple
        -------

        >>> tree2list("(A (B 1) (C 2))")
        ['A', ['B', 1], ['C', 2]]

        """
        lists = []
        self._tree2list_aux(lists, tree)
        return lists

    def list2words(self, tree):
        """Extract the segmented utterance from a PTB tree (in nested format)"""
        def visit(node, words_sofar, segs_sofar):
            """Does a preorder visit of the nodes in the tree"""
            # TODO slow and critical part to optimize
            if self.is_terminal(node):
                if not self.ignore_terminals_re.match(node):
                    segs_sofar.append(self.simplify_terminal(node))
                return words_sofar, segs_sofar

            for child in self.tree_children(node):
                words_sofar, segs_sofar = visit(child, words_sofar, segs_sofar)

            if self.word_re.match(self.tree_label(node)) and segs_sofar != []:
                words_sofar.append(''.join(segs_sofar))
                segs_sofar = []

            return words_sofar, segs_sofar

        words_sofar, segs_sofar = visit(tree, [], [])
        if segs_sofar:  # append any unattached segments as a word
            words_sofar.append(''.join(segs_sofar))

        return ' '.join(words_sofar).strip()

    def _tree2list_aux(self, trees, s, pos=0):
        """Recursive auxiliary method for tree2list()"""
        # TODO slow and critical part to optimize
        while pos < len(s):
            closepar_mo = self._closepar_re.match(s, pos)
            if closepar_mo:
                return closepar_mo.end()
            openpar_mo = self._openpar_re.match(s, pos)
            if openpar_mo:
                tree = [openpar_mo.group(1)]
                trees.append(tree)
                pos = self._tree2list_aux(tree, s, openpar_mo.end())
            else:
                terminal_mo = self._terminal_re.match(s, pos)
                trees.append(terminal_mo.group(1))
                pos = terminal_mo.end()
        return pos

    @staticmethod
    def simplify_terminal(t):
        """Ignore first character in `t` if it is a backslash"""
        if len(t) > 0 and t[0] == '\\':
            return t[1:]
        else:
            return t

    @staticmethod
    def is_terminal(subtree):
        """True if this subtree consists of a single terminal node

        (i.e., a word or an empty node).

        """
        return not isinstance(subtree, list)

    @staticmethod
    def tree_children(tree):
        """Returns a list of the child subtrees of tree."""
        if isinstance(tree, list):
            return tree[1:]
        else:
            return []

    @staticmethod
    def tree_label(tree):
        """Returns the label on the root node of tree."""
        if isinstance(tree, list):
            return tree[0]
        else:
            return tree


class ParseCounter(object):
    """Count the most frequent utterances in a sequence of parses"""
    def __init__(self, nutts):
        self.nutts = nutts
        self.counters = [collections.Counter() for _ in range(nutts)]
        self.nparses = 0

    def update(self, parse):
        if not len(parse) == self.nutts:
            raise RuntimeError(
                'ParseCounter.update: len(parse) != nutts: {} != {}'
                .format(len(parse), self.nutts))

        self.nparses += 1
        for i, utt in enumerate(parse):
            self.counters[i][utt] += 1

    def most_common(self):
        if self.nparses == 0:
            raise RuntimeError('ParseCounter.most_common: no parse counted!')
        return [c.most_common(1)[0][0] for c in self.counters]


def yield_parses(lines, ignore_firsts=0):
    """Yields parse trees, ignoring the first ones

    In the raw output of AG the parse , this function yields the successive tress, ignoring the
    first ones.

    Parameters
    ----------
    lines: sequence
        The parse trees as outputed by the AG program, the trees are
        separated by an empty line.
    ignore_first: int, optional
        The first trees are computed during the first iterations of AG
        and are usually less accurate. They can be ignored with that
        argument (default to 0).

    Yields
    ------
    tree: list
        The list of lines composing a full parse tree of the input
        text. Each line is an utterance in the PTB-format

    """
    ntrees = 0
    tree = []

    # read input line per line, yield at each empty line
    for line in lines:
        line = line.strip()
        if line == '':
            if len(tree) > 0:
                ntrees += 1
                if ntrees > ignore_firsts:
                    yield tree
                tree = []
        else:
            tree.append(line)

    # yield the last tree
    if len(tree) > 0 and ntrees >= ignore_firsts:
        yield tree


def postprocess(parse_counter, output_file, category, ignore_first_parses, log):
    tokenizer = TreeTokenizer(category)
    lines = gzip.open(output_file, 'rt', encoding='utf8')
    nwarnings = 0

    for parse in yield_parses(lines, ignore_firsts=ignore_first_parses):
        if len(parse) != parse_counter.nutts:
            nwarnings += 1
            log.warning(
                'ignoring incomplete parse of %d lines (must be %d): %s',
                len(parse), parse_counter.nutts, parse)
        else:
            # convert the PTB parentized expressions as words
            parse = [tokenizer.tree2words(utt) for utt in parse]

            # count the occurences of each utt in the parse
            parse_counter.update(parse)

    if nwarnings != 0:
        log.warning(
            'ignored %d parses (%d accepted) during postprocessing',
            nwarnings, parse_counter.nparses)


#-------------------------------------------------------------------------------
#  Segment function
#-------------------------------------------------------------------------------


def segment(train_text, grammar_file=None, category='Colloc0',
            args=DEFAULT_ARGS, test_text=None, ignore_first_parses=0,
            nruns=8, njobs=1, log=utils.null_logger()):
    """Segment a text using the Adaptor Grammar algorithm

    The algorithm is ran 8 times in parallel and the results are
    collapsed. We ensure the random seed to be different for each run.

    Parameters
    ----------
    train_text : sequence
        The list of utterances to train the model on, and to segment
        if `test_text` is None.
    grammar_file : str, optional
        The path to the grammar file to use for segmentation. If not
        specified, a Colloc0 grammar is generated from the input text.
    category : str, optional
        The category to segment the text with, must be an existing
        parent in the grammar (i.e. the `segment_category` must be
        present in the left column of the grammar file), default to
        'Colloc0'.
    args : str, optional
        Command line options to run the AG program with, use
        'wordseg-ag --help' to have a complete list of available
        options
    test_text : sequence, optional
        If not None, the list of utterances to segment using the model
        learned from `text`
    ignore_first_parses : int, optional
        Ignore the first parses from the algorithm output. If
        negative, keep only the last ones (e.g. -1 keeps only the last
        one, -2 the last two).
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
    t1 = datetime.datetime.now()

    # force the train text from sequence to list
    if not isinstance(train_text, list):
        train_text = list(train_text)
    nutts =  len(train_text)
    log.info('train data: %s utterances loaded', nutts)

    # if any, force the test text from sequence to list
    if test_text is not None:
        if not isinstance(test_text, list):
            test_text = list(test_text)
            nutts = len(test_text)
        log.info('test data: %s utterances loaded', nutts)
    else:
        log.info('no test data provided, segmentation on train data')

    # display the AG algorithm parameters
    log.info('parameters are: "%s"', args)

    # setup ignore_first_parses and make sure ignore_first_parses <=
    # niterations
    if '-n' in args:
        nparses = int(re.sub(r'^.*\-n *([0-9]+).*$', '\g<1>', args))
        if '-x' in args:
            interval = int(re.sub(r'^.*\-x *([0-9]+).*$', '\g<1>', args))
            nparses = int(nparses / interval)
        nparses += 1  # include the initial one (independant of iterations)
    else:
        nparses = 2000 + 1  # the default value fixed in C++
    if ignore_first_parses < 0:
        ignore_first_parses = max(0, nparses + ignore_first_parses)
    if ignore_first_parses >= nparses:
        raise RuntimeError('cannot ignore {} parses (max is {})'.format(
            ignore_first_parses, nparses - 1))

    # ensure we have a different seed for all runs. If the seed is
    # specified in command line (-r SEED) then feed SEED+i for i the
    # ith run. Else put a random seed to each run.
    args = _setup_seed(args, nruns)
    log.info('random seeds are: %s', ', '.join(
        [arg.split('-r ')[1].split(' ')[0] for arg in args]))

    # we may use a temp file to write the grammar, it is automatically
    # erased when done
    with tempfile.NamedTemporaryFile() as grammar_temp:
        # if grammar is not specified, generate a Colloc0 one from the
        # set of phones in the input text and write it in the tempfile
        if grammar_file is None:
            grammar_file = grammar_temp.name
            log.info('generating Colloc0 grammar in %s ...', grammar_file)
            # extract all the phones in both train and test data
            phones = set(p for utt in train_text for p in utt.split() if p)
            if test_text is not None:
                phones.update(
                    set(p for utt in test_text for p in utt.split() if p))
            # build the grammar from the phoneset
            grammar = build_colloc0_grammar(phones)
            codecs.open(grammar_file, 'w', encoding='utf8').write(grammar)

        check_grammar(grammar_file, category)
        log.info('valid grammar for level %s: %s', category, grammar_file)

        # parallel runs of the AG algorithm
        log.info('running AG (%d times)...', nruns)
        parse_counter = ParseCounter(nutts)

        joblib.Parallel(
            n_jobs=njobs, backend="threading", verbose=0)(
                joblib.delayed(_segment_single)(
                    parse_counter,
                    train_text,
                    grammar_file,
                    category,
                    ignore_first_parses,
                    args[n],
                    test_text=test_text,
                    log_level=log.getEffectiveLevel(),
                    log_name='wordseg-ag - run {}'.format(n + 1))
                for n in range(nruns))

        t2 = datetime.datetime.now()
        log.info('total processing time: %s', t2 - t1)
        log.info('extracting most common utterances in %d parses',
                 parse_counter.nparses)

        return parse_counter.most_common()


#-------------------------------------------------------------------------------
#  Command line arguments
#-------------------------------------------------------------------------------


def _add_arguments(parser):
    """Add algorithm specific options to the parser"""
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
        help=('discard the n first parses of each run, default is '
              '%(default)s. If negative, keeps only the <int> last parses.'))

    group = parser.add_argument_group('grammar arguments')
    group.add_argument(
        '--grammar', metavar='<grammar-file>', default=None,
        help=('read grammar from this file, for exemple of grammars see {}. '
              'Default is to generate a Colloc0 grammar from the input.'
              .format(os.path.dirname(get_grammar_files()[0]))))

    group.add_argument(
        '--category', metavar='<segment-category>', default='Colloc0',
        help=('the grammar category to segment the text with, '
              'must be a valid parent in the grammar (ie defined in the '
              'left column of the grammar file). Default is %(default)s.'))

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
        args.category,
        args=cmd_args,
        test_text=test_text,
        ignore_first_parses=args.ignore_first_parses,
        nruns=args.nruns, njobs=args.njobs, log=log)

    # output the results
    streamout.write('\n'.join(segmented) + '\n')


if __name__ == '__main__':
    main()
