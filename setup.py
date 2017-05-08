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

import distutils.command.build
from setuptools import setup, find_packages


def on_readthedocs():
    """Return True if building the online documentation on readthedocs"""
    return os.environ.get('READTHEDOCS', None) == 'True'


class WordsegBuild(distutils.command.build.build):
    """Compile the C++ code needed by wordseg"""
    # a list of C++ binaries to compile. We must have the Makefile
    # './wordseg/algos/TARGET/Makefile' that produces the executable
    # './wordseg/algos/TARGET/build/TARGET'
    targets = ['dpseg']

    def run(self):
        # call the usual build method
        distutils.command.build.build.run(self)

        if on_readthedocs():
            return

        # calling "make" and compile all the C++ targets
        for target in self.targets:
            build_dir = os.path.join('wordseg', 'algos', target, 'build')
            print('compiling C++ dependencies for', target, 'in', build_dir)

            if not os.path.exists(build_dir):
                os.makedirs(build_dir)

            subprocess.call(
                ['make'], cwd=os.path.join('wordseg', 'algos', target))

    @classmethod
    def bin_targets(cls):
        """Return the list of binaries to be installed with wordseg"""
        return ([] if on_readthedocs() else
                ['wordseg/algos/{}/build/{}'.format(t, t) for t in cls.targets])


setup(
    name='wordseg',
    version=open('VERSION').read().strip(),
    description='tools for text based word segmentation',
    long_description=open('README.md').read(),
    author='Alex Cristia, Mathieu Bernard, Elin Larsen',
    url='https://github.com/mmmaat/wordseg',
    license='GPL3',
    zip_safe=True,

    packages=find_packages(exclude=['test']),
    install_requires=(['six', 'joblib'] if on_readthedocs() else
                      ['six', 'joblib', 'numpy', 'pandas']),
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

    cmdclass={'build': WordsegBuild},
    data_files=[('bin', WordsegBuild.bin_targets())])
