"""Provide utility functions to the wordseg package"""

import argparse
import codecs
import logging
import os
import pkg_resources
import re
import subprocess
import sys

from wordseg.separator import Separator


def strip(string):
    """Strips the `string` from undesirable spaces

    Removes begining and ending spaces and subsitutes multiple spaces
    by a single one. Spaces characters, including newlines (*\\n*) and
    tabulations (*\\t*), are all substituted by ' '.

    Parameters
    ----------
    string: str
        The string on which to eliminate multiple spaces

    Returns
    -------
    str
        The input `string` with multiple spaces removed.

    Examples
    --------
    >>> strip(" ab\\t  b\\n")
    ab b
    >>> strip(" \\n ab   c ")
    ab c
    >>> strip("ab\\n c ")
    ab c

    """
    return re.sub(r'\s+', ' ', string.strip())


def null_logger():
    """Configures and returns a logger sending messages to nowhere

    This is used as default logger for some functions.

    Returns
    -------
    logging.Logger
        Logging instance ignoring all the messages.

    """
    log = logging.getLogger()
    log.addHandler(logging.NullHandler())
    return log


def get_logger(name=None, level=logging.WARNING):
    """Configures and returns a logger sending messages to standard error

    Parameters
    ----------
    name : str
        Name of the created logger, to be displayed in the header of
        log messages.
    level : logging.level
        The minimum log level handled by the logger (any message above
        this level will be ignored).

    Returns
    -------
    logging.Logger
        Logging instance displaying messages to the standard error
        stream.

    """
    log = logging.getLogger(name)
    log.setLevel(level)

    formatter = logging.Formatter(
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    handler = logging.StreamHandler(sys.stderr)
    handler.setFormatter(formatter)

    log.addHandler(handler)
    return log


class CountingIterator(object):
    """A class for counting elements in a generator

    Usefull because this avoid to convert a generator to list
    (preserving time and memory) to count it's elements.

    Parameters
    ----------
    elements: sequence
        The sequence on which we are counting elements while
        iterating on it. Can be a list or a generator.

    Raises
    ------
    TypeError
        If `elements` is not a sequence.

    Examples
    --------
    >>> counter = CountingIterator(range(1, 11))
    >>> sum(counter)
    55
    >>> counter.count
    10

    """
    def __init__(self, elements):
        self.elements = iter(elements)
        self.count = 0

    def __iter__(self):
        return self

    def __next__(self):
        nxt = next(self.elements)
        self.count += 1
        return nxt

    next = __next__


class CatchExceptions(object):
    """Decorator wrapping a function in a try/except block

    When an exception occurs, display a user friendly message on
    standard output before exiting with error code 1.

    The detected exceptions are ValueError, OSError, RuntimeError,
    AssertionError, KeyboardInterrupt and
    pkg_resources.DistributionNotFound.

    Parameters
    ----------
    function :
        The function to wrap in a try/except block

    """
    def __init__(self, function):
        self.function = function

    def __call__(self):
        """Executes the wrapped function and catch common exceptions"""
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


def get_binary(binary):
    """Returns the path to the program `binary`

    This function searchs for the `binary` program in the installation
    path of the wordseg package. This concerns only the C++ programs
    bundled with wordseg (namely ag and dpseg) which have been
    compiled during the wordseg installation.

    Parameters
    ----------
    binary : str
        Name of the binary file to be searched in the wordseg
        installation directory.

    Returns
    -------
    str
        The absolute path to the `binary`

    Raises
    ------
    RuntimeError
        If the `binary` is not found in the wordseg installation path
        or if it's not an executable file.

    """
    pkg = pkg_resources.Requirement.parse('wordseg')

    binary_path = ''
    try:
        # case of 'python setup.py install'
        binary_path = pkg_resources.resource_filename(
            pkg, 'bin/{}'.format(binary))
    except KeyError:
        pass

    try:
        # case of 'python setup.py develop' or 'make'
        if not os.path.isfile(binary_path):
            binary_path = pkg_resources.resource_filename(
                pkg, 'build/wordseg/algos/{binary}/{binary}'
                .format(binary=binary))
    except KeyError:
        pass

    if not os.path.isfile(binary_path):
        raise RuntimeError(
            'binary "{}" not found: {}'.format(binary, binary_path))

    if not os.access(binary_path, os.X_OK):
        raise RuntimeError(
            'binary "{}" not executable: {}'.format(binary_path))

    return binary_path


def get_config_files(algorithm, extension=None):
    """Returns the example configuration files bundled with algorithms

    Only *ag* and *dpseg* have configuration files.

    Parameters
    ----------
    algorithm : str
        Name of the algorithm to look for config files.
    extension : str, optional
        If specified, return only the files mathcing this
        extension. Otherwise return all the configuration files.

    Returns
    -------
    list
        The absolute paths to the requested configuration files.

    Raises
    ------
    RuntimeError
        If no configuration files found for the requested `algorithm`.

    """
    pkg = pkg_resources.Requirement.parse('wordseg')

    config_dir = ''
    try:
        # case of 'python setup.py install'
        config_dir = pkg_resources.resource_filename(
            pkg, 'config/{}'.format(algorithm))
    except KeyError:
        pass

    try:
        # case of 'python setup.py develop' or local install
        if not os.path.isdir(config_dir):
            config_dir = os.path.abspath(os.path.join(
                os.path.dirname(__file__), '..', 'config', algorithm))
    except KeyError:
        pass

    if not os.path.isdir(config_dir):
        raise RuntimeError('directory not found: {}'.format(config_dir))

    config_files = [f for f in os.listdir(config_dir)]
    if extension:
        config_files = [f for f in config_files if f.endswith(extension)]

    if len(config_files) == 0:
        raise RuntimeError('no {}files found in {}'.format(
        '*{} '.format(extension) if extension else '', config_dir))

    return [os.path.join(config_dir, f) for f in config_files]


class Argument(object):
    """Command line argument adapter class

    Read a command line argument from a C++ binary (argument parsing
    must be handled by boost::program_options) and convert it to a
    python argparse argument.

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
        parser.add_argument(
            self.name, default=self.default, help=self.help, metavar='<arg>')


def yield_binary_arguments(binary, excluded=[]):
    """Yields arguments as parsed from the "`binary` --help" message

    The `binary` file must be a C++ program with command line options
    implemented with the boost::program_options library.

    This function is used in the *ag* and *dpseg* wrappers.

    Parameters
    ----------
    binary : str
        Executable to execute with the option "--help".
    excluded : list
        List of excluded options from the ones parsed from `binary`.

    Yields
    ------


    Raises
    ------
    subprocess.SubprocessError
        When exectution of the command "`binary` --help" fails.

    """
    # get the help message of the binary (raise a SubprocessError on
    # failure)
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


def get_parser(description=None, separator=Separator()):
    """Returns an argument parser initiliazed with wordseg's common options

    Provides --verbose / --quiet options regulating the logger,
    --phone / --word / --syllable setting the separator, and define
    input/output arguments for opening files/streams.

    Parameters
    ----------
    description : str
        Description string displayed on the parser's help message.
    separator : wordseg.separator.Separator
        The default token separation as displayed in the parser's help
        message.

    Returns
    -------
    argparse.ArgumentParser
        Argument parser initialized with common options for wordseg
        executables.

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

    Provides an easy way to setup a command for the wordseg
    package. It defines an argument parser, parse the arguments and
    initialize a logger, a token separator and input/output streams to
    be used by the command itself.

    Parameters
    ----------
    name : str
        Name of the command displayed on log messages
    description : str
        Description string displayed on the command help message.
    separator : wordseg.separator.Separator
        The default token separation as displayed in the command help
        message at phone, syllable and word levels. Set any level to None to
      disable this token for the command.
    add_arguments : function
        function of prototype (argparse.ArgumentParser: None) adding
        optional arguments to the parser.

    Returns
    -------
    streamin : stream
        Opened input stream encoded in utf8.
    streamout : stream
        Opened output stream encoded in utf8.
    separator : wordseg.separator.Separator
        Token separation in input and output streams.
    log : logging.Logger
        Logger for displaying messages during the execution flow.
    args : list
        Extra arguments parsed from the command line.

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
