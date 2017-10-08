"""Adaptor Grammar

The grammar consists of a sequence of rules, one per line, in the
following format:

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

"""

# Commented out because those options are ignored in this wrapper.
#
# The -u and -v flags specify test-sets which are parsed using the
# current PCFG approximation every eval-every iterations, but they are
# not trained on.  These parses are piped into the commands specified by
# the -U and -V parameters respectively.  Just as for the -X eval-cmd,
# these commands are only run _once_.


import collections
import joblib
import logging
import os
import pkg_resources
import shlex
import subprocess
import sys
import tempfile

from wordseg import utils



AG_ARGUMENTS = [
    utils.Argument(
        short_name='-d', name='--debug', type=int, default=0,
        help='debugging level'),

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
        short_name='-D', name='--delay-init', type=bool,
        help='delay grammar initialization until all sentences are parsed'),

    utils.Argument(
        short_name='-E', name='--dirichlet-prior', type=bool,
        help='estimate rule prob (theta) using Dirichlet prior'),

    utils.Argument(
        short_name='-G', name='--print-grammar-file', type='file',
        help='print grammar at termination to a file'),

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
        help='resample PY cache strings during first n iterations (-1 = forever)'),

    utils.Argument(
        short_name='-n', name='--niterations', type=int,
        help='number of iterations'),

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

    utils.Argument(
        short_name='-X', name='--eval-parses-cmd', type='file',
        help=('pipe each run\'s parses into this command '
              '(empty line separates runs)')),

    utils.Argument(
        short_name='-Y', name='--eval-grammar-cmd', type='file',
        help=('pipe each run\'s grammar-rules into this command '
              '(empty line separates runs)')),

    utils.Argument(
        short_name='-x', name='--eval-every', type=int,
        help='pipe trees into the eval-parses-cmd every eval-every iterations'),

    # We ignore the following options because they conflict with the
    # wordseg workflow (stdin > wordseg-cmd > stdout). In this AG
    # wrapper the test2 file is ignored and the test1 is the input
    # text sent to stdout.
    #
    # utils.Argument(
    #     short_name='-u', name='--test-file', type='file',
    #     help=('test strings to be parsed (but not trained on) '
    #           'every eval-every iterations')),
    #
    # utils.Argument(
    #     short_name='-U', name='--test1-eval', type='file',
    #     help='parses of test1-file are piped into this command'),
    #
    # utils.Argument(
    #     short_name='-v', name='--test2-file', type='file',
    #     help=('test strings to be parsed (but not trained on) '
    #           'every eval-every iterations')),
    #
    # utils.Argument(
    #     short_name='-V', name='--test2-eval', type='file',
    #     help='parses of test2-file are piped into this command')
]


def get_grammar_files():
    """Returns a list of example grammar files bundled with wordseg

    Grammar files have the *.lt extension and are stored in the
    directory `wordseg/config/ag`.

    Raises
    ------
    RuntimeError
        If the configuration directory is not found or if there is no
        grammar files in it.

    """
    pkg = pkg_resources.Requirement.parse('wordseg')

    # case of 'python setup.py install'
    grammar_dir = pkg_resources.resource_filename(pkg, 'config/ag')

    # case of 'python setup.py develop' or local install
    if not os.path.isdir(grammar_dir):
        grammar_dir = pkg_resources.resource_filename(pkg, 'ag/config')

    if not os.path.isdir(grammar_dir):
        raise RuntimeError(
            'grammar directory not found: {}'.format(grammar_dir))

    grammar_files = [f for f in os.listdir(grammar_dir) if f.endswith('lt')]

    if len(grammar_files) == 0:
        raise RuntimeError('no *.lt files in {}'.format(grammar_dir))

    return [os.path.join(grammar_dir, f) for f in grammar_files]


def _ag(text, grammar, args, log=utils.null_logger()):
    with tempfile.TemporaryDirectory() as temp_dir:
        # write the text as a temporary file. ylt extension is the one
        # used in the original implementation
        input_tmpfile = os.path.join(temp_dir, 'input.ylt')
        open(input_tmpfile, 'w', encoding='utf8').write(
            '\n'.join(utt.strip() for utt in text))

        # generate the command to run as a subprocess
        command = (
            '{binary} {grammar} {args} -u {input} -U cat'
            .format(binary=utils.get_binary('ag'),
                    grammar=grammar,
                    args=args,
                    input=input_tmpfile,
                    output=os.path.join(temp_dir, 'output.prs')))

        # run the command, write output_tmpfile
        log.debug('running "%s"', command)

        process = subprocess.Popen(
            shlex.split(command),
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=None)

        parses, _ = process.communicate(
            '\n'.join(utt.strip() for utt in text).encode('utf8'))
        if process.returncode:
            raise RuntimeError(
                'fails with error code {}'.format(process.returncode))

        return parses.decode('utf8').split('\n')


def _yield_parses(raw_parses, ignore_firsts=0):
    """Yield parses as outputed by the ag binary, ignoring the first ones"""
    nparses = 0
    parse = []

    # read input line per line, yield at each empty line
    for line in raw_parses:
        line = line.strip()
        if len(line) == 0 and len(parse) > 0:
            nparses += 1
            if ignore_firsts > 0 and nparses > ignore_firsts:
                yield parse
                parse = []
        else:
            parse.append(line)

    # yield the last parse
    if len(parse) > 0:
        if ignore_firsts > 0 and nparses > ignore_firsts:
            yield parse


def most_frequent_parse(data):
    """Counts the number of times each parse appears, and returns the
    one that appears most frequently"""
    counter = collections.Counter(("\n".join(d) for d in data))
    return counter.most_common(1)[0][0]


def segment(text, grammar_file, njobs=1, ignore_first_parses=0, args='',
            log=utils.null_logger()):
    """Run the ag binary"""
    text = list(text)
    log.info('%s utterances loaded for segmentation', len(text))

    try:
        segmented_texts = joblib.Parallel(n_jobs=njobs, verbose=100)(
            joblib.delayed(_ag)(text, grammar_file, args, log=log)
            for _ in range(njobs))
    except RuntimeError as err:
        log.fatal('%s, exiting', err)
        sys.exit(1)

    log.warning(type(segmented_texts))
    log.warning(len(segmented_texts))
    # log.warning(segmented_texts[0])
    return

    return most_frequent_parse(
        (parses for text in segmented_texts
         for parses in _yield_parses(text, ignore_firsts=ignore_first_parses)))


def add_arguments(parser):
    """Add algorithm specific options to the parser"""
    parser.add_argument(
        '-j', '--njobs', type=int, metavar='<int>', default=1,
        help='number of parallel jobs to use, default is %(default)s')

    parser.add_argument(
        'grammar', metavar='<grammar-file>',
        help=('read the grammar from this file, for exemple of grammars see {}'
              .format(os.path.dirname(get_grammar_files()[0])))),

    parser.add_argument(
        '-c', '--config-file', metavar='<file>',
        help=('configuration file to read the algorithm parameters from, '
              'for example configuration files see {}'.format(
                  os.path.dirname(utils.get_config_files('ag')[0]))))

    parser.add_argument(
        '--ignore-first-parses', type=int, metavar='<int>', default=0,
        help='discard the n first parses of each segmentation job, '
        'default is %(default)s')

    group = parser.add_argument_group('algorithm options')
    for arg in AG_ARGUMENTS:
        arg.add_to_parser(group)


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-ag' command"""
    streamin, streamout, _, log, args = utils.prepare_main(
        name='wordseg-ag',
        description=__doc__,
        add_arguments=add_arguments)

    # build the ag command arguments
    ag_args = {}
    short_names = {arg.parsed_name(): arg.short_name for arg in AG_ARGUMENTS}
    flags = [short_names[arg.parsed_name()]
             for arg in AG_ARGUMENTS if arg.type == bool]
    excluded_args = ['verbose', 'quiet', 'input', 'output', 'njobs']
    for k, v in vars(args).items():
        # ignored arguments
        if k in excluded_args or v in (None, False):
            continue

        # convert some option shortnames from wordseg-ag to AG binary,
        # raises KetError if the short name is not defined for that
        # option
        try:
            k = short_names[k]
            if k == '-i':
                k = '-h'
            ag_args[k] = v
        except KeyError:
            pass

    ag_args = ' '.join('{} {}'.format(k, v if k not in flags else '')
                       for k, v in ag_args.items())

    log.debug('using the grammar file %s', args.grammar)
    segmented = segment(
        streamin, args.grammar, njobs=args.njobs,
        ignore_first_parses=args.ignore_first_parses,
        args=ag_args, log=log)
    log.debug('segment done')

    if segmented:
        streamout.write('\n'.join(segmented) + '\n')


if __name__ == '__main__':
    main()
