#!/usr/bin/env python
#
# Copyright 2015, 2016 Mathieu Bernard
#
# This file is part of wordseg: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# wordseg is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with wordseg. If not, see <http://www.gnu.org/licenses/>.

"""Setup script for the wordseg package"""

import os
import subprocess
import sys

from setuptools import setup, find_packages


def on_readthedocs():
    """Return True if building the online documentation on readthedocs"""
    return os.environ.get('READTHEDOCS', None)


PACKAGE_NAME = 'wordseg'
PACKAGE_VERSION = open('VERSION').read().strip()

# A list of python packages required by wordseg
REQUIREMENTS = ['joblib'] if on_readthedocs() else ['joblib', 'numpy', 'pandas']

# a list of C++ binaries to compile. We must have the Makefile
# './wordseg/algos/TARGET/Makefile' that produces the executable
# './wordseg/algos/TARGET/build/TARGET'
CPP_TARGETS = [] if on_readthedocs() else ['dpseg']

# the list of binaries to be installed with wordseg
BIN_TARGETS = ['wordseg/algos/{}/build/{}'.format(t, t) for t in CPP_TARGETS]

# calling "make" and compile all the C++ targets just defined
for target in CPP_TARGETS:
    build_dir = os.path.join('wordseg', 'algos', target, 'build')
    print('compiling C++ dependencies for', target, 'in', build_dir)

    if not os.path.exists(build_dir):
        os.makedirs(build_dir)

    subprocess.call(['make'], cwd=os.path.join('wordseg', 'algos', target))


setup(
    name=PACKAGE_NAME,
    version=PACKAGE_VERSION,
    description='A collection of tools for text based word segmentation',
    long_description=open('README.md').read(),
    author='Alex Cristia, Mathieu Bernard, Elin Larsen',
    url='https://github.com/mmmaat/wordseg',
    license='GPL3',
    zip_safe=True,

    packages=find_packages(exclude=['test']),
    install_requires=REQUIREMENTS,
    setup_requires=['pytest-runner'],
    tests_require=['pytest'],

    entry_points={'console_scripts': [
        'wordseg-prep = wordseg.prepare:main',
        'wordseg-gold = wordseg.gold:main',
        'wordseg-eval = wordseg.evaluate:main',
        'wordseg-stats = wordseg.stats:main',
        'wordseg-dibs = wordseg.algos.dibs:main',
        'wordseg-dpseg = wordseg.algos.dpseg:main',
        'wordseg-tp = wordseg.algos.tp:main',
        'wordseg-puddle = wordseg.algos.puddle:main']},

    data_files=[('bin', BIN_TARGETS)])
