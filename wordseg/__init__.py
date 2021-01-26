"""The wordseg module"""

import datetime
import textwrap

__version__ = '0.8'


def url():
    """The URL to wordseg online documentation"""
    return 'https://docs.cognitive-ml.fr/wordseg'


def author():
    """The wordseg legal author"""
    return 'LSCP (EHESS, ENS, CNRS, PSL Research University)'


def version():
    """The wordseg version as a string"""
    return __version__


def version_long():
    """A long description with version and copyright"""
    year = datetime.date.today().year

    return textwrap.dedent(f'''\
    wordseg-{version()} - {url()}
    copyright 2015-{year} {author()}
    ''')
