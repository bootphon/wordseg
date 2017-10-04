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

The -u and -v flags specify test-sets which are parsed using the
current PCFG approximation every eval-every iterations, but they are
not trained on.  These parses are piped into the commands specified by
the -U and -V parameters respectively.  Just as for the -X eval-cmd,
these commands are only run _once_.

The program can now estimate the Pitman-Yor hyperparameters a and b
for each adapted nonterminal.  To specify a uniform Beta prior on the
a parameter, set "-e 1 -f 1" and to specify a vague Gamma prior on the
b parameter, set "-g 10 -h 0.1" or "-g 100 -h 0.01".

If you want to estimate the values for a and b hyperparameters, their
initial values must be greater than zero.  The -a flag may be useful
here. If a nonterminal has an a value of 1, this means that the
nonterminal is not adapted.

"""

import collections
import joblib
import logging
import os
import pkg_resources
import shlex
import subprocess
import sys

from wordseg import utils


def get_grammar_files():
    """Returns a list of example grammar files bundled with wordseg

    Grammar files have the *.lt extension and are stored in the
    directory `wordseg/config/ag`.

    Returns
    -------
    list
        Examples of grammar files

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
    # generate the command to run as a subprocess
    command = '{binary} --grammar {grammar} {args}'.format(
        binary=utils.get_binary('ag'), grammar=grammar, args=args)

    log.debug('running "%s"', command)

    process = subprocess.Popen(
        shlex.split(command),
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=None)

    parses = process.communicate('\n'.join(text).encode('utf8'))
    if process.returncode:
        raise RuntimeError(
            'fails with error code {}'.format(process.returncode))

    print(parses)

    return parses.decode('utf8').split('\n')


def yield_parses(raw_parses, ignore_firsts=0):
    """Yield parses as outputed by the ag binary

    :param sequence raw_parses: a sequence of lines (can be an opened
        file or a list). Parses are separated by an empty line.

    :yield: the current parse

    """
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
    return collections.Counter(("\n".join(d) for d in data)).most_common(1)[0][0]


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

    return most_frequent_parse(
        (parses for text in segmented_texts
         for parses in yield_parses(text, ignore_first=ignore_first_parses)))


def add_arguments(parser):
    """Add algorithm specific options to the parser"""
    parser.add_argument(
        '-j', '--njobs', type=int, metavar='<int>', default=1,
        help='number of parallel jobs to use, default is %(default)s')

    parser.add_argument(
        '--ignore-first-parses', type=int, metavar='<int>', default=0,
        help='discard the n first parses of each segmentation job, '
        'default is %(default)s')

    # a list of pycfg options we don't want to expose in wordseg-ag
    excluded = ['--help']

    group = parser.add_argument_group('algorithm options')
    for arg in utils.yield_binary_arguments(utils.get_binary('ag'), excluded=excluded):
        if arg.name == '--grammar-file':
            arg.help += ', for example grammar files see {}/*.lt'.format(
                os.path.dirname(utils.get_config_files('ag', extension='.lt')[0]))
        arg.add(group)


@utils.CatchExceptions
def main():
    """Entry point of the 'wordseg-ag' command"""
    streamin, streamout, _, log, args = utils.prepare_main(
        name='wordseg-ag',
        description=__doc__,
        add_arguments=add_arguments)

    ignored_args = ['verbose', 'quiet', 'input', 'output', 'njobs']
    ag_args = {k: v for k, v in vars(args).items()
               if k not in ignored_args and v}
    ag_args = ' '.join('--{} {}'.format(k.replace('_', '-'), v)
                       for k, v in ag_args.items())

    log.debug('call segment on grammar %s', args.grammar_file)
    segmented = segment(
        streamin, args.grammar_file, njobs=args.njobs,
        ignore_first_parses=args.ignore_first_parses,
        args=ag_args, log=log)
    log.debug('segment done')

    streamout.write('\n'.join(segmented) + '\n')


if __name__ == '__main__':
    main()
