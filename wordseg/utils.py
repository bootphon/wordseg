# Copyright 2017 Mathieu Bernard
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

"""Provide utility functions to the wordseg package"""

import argparse
import codecs
import logging
import os
import pkg_resources
import re
import subprocess
import sys

from wordseg import Separator


class CountingIterator(object):
    """A class for counting elements in a generator

    Usefull because this avoid to convert a generator to list
    (preserving time and memory) to count it's elements. Use it as
    follows:

    >>> counter = CountingIterator(range(10))
    >>> for c in counter: pass
    >>> counter.count
    10

    """
    def __init__(self, generator):
        self.generator = generator
        self.count = 0

    def __iter__(self):
        return self

    def next(self):
        nxt = next(self.generator)
        self.count += 1
        return nxt

    __next__ = next


class CatchExceptions(object):
    """Decorator wrapping a function in a try/except block

    When an exception occurs, log a critical message before
    exiting with error code 1.

    """
    def __init__(self, function):
        self.function = function

    def __call__(self):
        try:
            self.function()

        except (ValueError, OSError, RuntimeError, AssertionError) as err:
            self.exit('fatal error: {}'.format(err))

        except pkg_resources.DistributionNotFound:
            self.exit(
                'fatal error: wordseg package not found\n'
                'please install wordseg on your system')

        except KeyboardInterrupt:
            self.exit('keyboard interruption, exiting')

    @staticmethod
    def exit(msg):
        """Write `msg` on stderr and exit with error code 1"""
        sys.stderr.write(msg.strip() + '\n')
        sys.exit(1)


def get_binary(name):
    """Return the path to the program `name`

    :param str name: name of the binary file to be searched in the
      wordseg installation directory.

    :return: path to the binary (which has been compiled during
      the wordseg installation).

    :raise: AssertionError if the binary is not found

    """
    pkg = pkg_resources.Requirement.parse('wordseg')

    # case of 'python setup.py install'
    binary = pkg_resources.resource_filename(pkg, 'bin/{}'.format(name))

    # case of 'python setup.py develop' or 'make'
    if not os.path.isfile(binary):
        binary = pkg_resources.resource_filename(pkg, 'build/{name}/{name}'.format(name=name))

    assert os.path.isfile(binary), 'binary "{}" not found: {}'.format(name, binary)
    return binary


def get_config_files(name, extension=None):
    """Return a list of example configuration files bundled with wordseg

    :param str name: the name of the algorithm (only ag and dpseg have
       config files)

    :param str extension: If specified, return only the files mathcing
       this extension

    :return: a list of abspath to configuration files

    :raise: AssertionError if no configuration files found

    """
    pkg = pkg_resources.Requirement.parse('wordseg')

    # case of 'python setup.py install'
    config_dir = pkg_resources.resource_filename(pkg, 'config/{}'.format(name))

    # case of 'python setup.py develop' or local install
    if not os.path.isdir(config_dir):
        config_dir = os.path.abspath(
            os.path.join(os.path.dirname(__file__), '..', 'config', name))

    assert os.path.isdir(config_dir), 'directory not found: {}'.format(config_dir)

    config_files = [f for f in os.listdir(config_dir)]
    if extension:
        config_files = [f for f in config_files if f.endswith(extension)]

    assert len(config_files) > 0, 'no {}files found in {}'.format(
        '*{} '.format(extension) if extension else '', config_dir)

    return [os.path.join(config_dir, f) for f in config_files]


class Argument(object):
    """Commandline argument adpater class

    Read argument from a C++ binary (argument parsing must be handled
    by boost::program_options) and convert it to a python argparse
    argument.

    """
    def __init__(self, name=None, default=None,  help='', excluded=[]):
        self.name = name
        self.default = default
        self.help = help
        self.excluded = excluded

    def is_valid(self):
        if not self.name:
            return False
        if self.name in self.excluded:
            return False
        return True

    def send(self):
        # adapting help message for some options
        if self.name == '--debug-level':
            self.help = (
                'increase the amount of debug messages, use together with -vv '
                '(a reasonable value is 1000)')

        if self.name == '--config-file':
            self.help += ' (in case of duplicates, precedence goes to the command line option)'

        # adding the default argument value in help
        if self.default:
            self.help += ', default is "%(default)s"'
        return self

    def add(self, parser):
        parser.add_argument(self.name, default=self.default,
                            help=self.help, metavar='<arg>')


def yield_binary_arguments(binary, excluded=[]):
    """Yields commandline arguments parsed from "`binary` --help" message"""
    # get the help message of the binary
    help_msg= subprocess.Popen(
        [binary, '--help'], stderr=subprocess.PIPE).communicate()[1].decode()

    # parse the message line by line to build arguments for argparse
    short_opts = '\s+-\w \[ (--[\w_\-]+) \]'
    long_opts = '\s+(--[\w_\-]+)'
    argument_re = ('({}|{})\s*'
                   '(arg)?\s*(\(\=([a-zA-Z0-9\._\-\s]+)\))?(.*)'
                   .format(short_opts, long_opts))

    argument = Argument(excluded=excluded)
    for line in help_msg.split('\n'):
        m = re.match(argument_re, line)
        if m:  # continuation of the previous help message
            # yield the previous argument
            if argument.is_valid():
                yield argument.send()
            argument = Argument(excluded=excluded)

            argument.name = m.group(2) if m.group(2) else m.group(3)
            argument.default = (m.group(6).replace('(=', '').replace(')', '')
                                if m.group(6) else None)
            argument.help = m.group(7).strip()
        else:  # the regext is not matched: this is the continuation
               # of a help message
            assert '--' not in line
            argument.help += ' ' + line.strip()

    if argument.is_valid():
        argument.help += ', default is %(default)s'
        yield argument.send()


def strip(utt):
    """Return the string `utt` with undesirable spaces removed

    This function is an extension of string.strip(), by so removing
    begining and ending spaces, that also subsitutes multiple spaces
    by a single one inside the string

    >>> strip(" a   b\\n")
    a b

    >>> strip("ab   c ")
    ab c

    >>> strip("ab\\n c ")
    ab c

    """
    return re.sub(r'\s+', ' ', utt.strip())


def null_logger():
    """Return a logger sending log messages to nowhere

    This is used as default logger for some functions.

    """
    log = logging.getLogger()
    log.addHandler(logging.NullHandler())
    return log


def get_logger(name=None, level=logging.WARNING):
    """Return a logger sending messages to stderr

    :param str name: The name of the logger, to be displayed in log
      messages

    :param logging.level level: The minimum log level handled by the
      logger (any message above this level will be ignored)

    """
    log = logging.getLogger(name)
    log.setLevel(level)

    formatter = logging.Formatter(
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    handler = logging.StreamHandler(sys.stderr)
    handler.setFormatter(formatter)

    log.addHandler(handler)
    return log


def get_parser(description=None, separator=Separator()):
    """Return an argument parser initiliazed with common options

    Provide --verbose/--quiet options regulating the logger,
    --phone/--word/--syllable setting the separator, and define
    input/output arguments for opening files/streams

    """
    parser = argparse.ArgumentParser(
        description=description,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    # add verbose/quiet options to control log level
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        '-v', '--verbose', action='count', default=0, help='''
        increase the amount of logging on stderr (by default only
        warnings and errors are displayed, a single '-v' adds info
        messages and '-vv' adds debug messages, use '--quiet' to
        disable logging)''')

    group.add_argument(
        '-q', '--quiet', action='store_true',
        help='do not output anything on stderr')

    # add token separation arguments
    if separator.phone or separator.syllable or separator.word:
        if separator.phone:
            parser.add_argument(
                '-p', '--phone-separator', metavar='<str>',
                default=separator.phone,
                help='phone separator, default is "%(default)s"')

        if separator.syllable:
            parser.add_argument(
                '-s', '--syllable-separator', metavar='<str>',
                default=separator.syllable,
                help='syllable separator, default is "%(default)s"')

        if separator.word:
            parser.add_argument(
                '-w', '--word-separator', metavar='<str>',
                default=separator.word,
                help='word separator, default is "%(default)s"')

    # add input and output arguments to the parser
    parser.add_argument(
        'input', default=sys.stdin, nargs='?', metavar='<input-file>',
        help='input text file to read, if not specified read from stdin')

    parser.add_argument(
        '-o', '--output', default=sys.stdout, metavar='<output-file>',
        help='output text file to write, if not specified write to stdout')

    return parser


def prepare_main(name=None,
                 description=None,
                 separator=Separator(phone=None, syllable=None, word=None),
                 add_arguments=None):
    """Initialize a binary program from the wordseg package

    This method provides an easy way to setup a command for the
    wordseg package. It defines an argument parser, parse the
    arguments and initialize a logger, a token separator and
    input/output streams to be used by the command itself.

    :param str name: the name of the command (shown on log messages)

    :param str description: the description of the command (shown with
      command --help option)

    :param Separator separator: Default value of the parsed separators
      at phone, syllable and word levels. Set any level to None to
      disable this token for the command.

    :param function(argparse.ArgumentParser) -> None add_argument: A
      function adding optional arguments to the parser.

    :return: A tuple that makes wordseg commands easy and homogeneous.
      The returned values are initialized from the command line
      arguments. They are: opened input and output streams, token
      separator, logger and extra arguments parsed from command line.

    """
    # define a basic command line parser
    parser = get_parser(description=description, separator=separator)

    # add any arguments to it
    if add_arguments:
        add_arguments(parser)

    # parse input arguments
    args = parser.parse_args()

    # setup the logger (level given by -q/-v arguments)
    if args.quiet:
        log = null_logger()
    else:
        if args.verbose == 0:
            level = logging.WARNING
        elif args.verbose == 1:
            level = logging.INFO
        else:  # verbose >= 2
            level = logging.DEBUG
        log = get_logger(name=name, level=level)

    # setup the separator from parsed arguments
    separator = Separator(
        phone=args.phone_separator if separator.phone else None,
        syllable=args.syllable_separator if separator.syllable else None,
        word=args.word_separator if separator.word else None)

    # open the input stream from parsed arguments
    streamin = args.input
    if isinstance(streamin, str):
        streamin = codecs.open(streamin, 'r', encoding='utf8')

    # open the output stream from parsed arguments
    streamout = args.output
    if isinstance(streamout, str):
        streamout = codecs.open(streamout, 'w', encoding='utf8')

    return streamin, streamout, separator, log, args
