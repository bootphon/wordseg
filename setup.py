#!/usr/bin/env python
"""Python setup script for the wordseg package"""

import codecs
import os
import pathlib
import platform
import subprocess
import sys

import setuptools
import setuptools.command.build_ext

import wordseg


class CMakeExtension(setuptools.Extension):
    def __init__(self, name, sourcedir=''):
        super().__init__(name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(setuptools.command.build_ext.build_ext):
    """CMakeExtension build command"""
    def run(self):
        try:
            subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError(
                "CMake must be installed to build the following extensions: " +
                ", ".join(e.name for e in self.extensions))

        for ext in self.extensions:
            build_dir = pathlib.Path(self.build_temp) / ext.name
            self.build_extension(ext, build_dir)

            if not os.path.isdir('build/bin'):
                os.makedirs('build/bin')
            self.copy_file(
                build_dir / ext.name, pathlib.Path('build/bin'))

    def build_extension(self, ext, build_dir):
        cmake_args = ['-DCMAKE_BUILD_TYPE=Release']

        if platform.system() == "Windows":
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args = ['--', '/m']
        else:
            build_args = ['--', '-j']

        if not os.path.exists(build_dir):
            os.makedirs(build_dir)

        print(f'CMake configure {ext.name}')
        try:
            subprocess.check_call(
                ['cmake', ext.sourcedir] + cmake_args,
                cwd=build_dir, env=os.environ)
        except subprocess.CalledProcessError:
            print(f'Error: CMake configure {ext.name} failed')

        print(f'CMake build {ext.name}')
        try:
            subprocess.check_call(
                ['cmake', '--build', '.'] + build_args,
                cwd=build_dir)
        except subprocess.CalledProcessError:
            print(f'Error: CMake build {ext.name} failed')


def data_files(directory):
    """Return a list of exemple configuration files bundled with `binary`"""
    return [os.path.join(directory, f) for f in os.listdir(directory)]


setuptools.setup(
    name='wordseg',
    version=wordseg.version(),
    author=wordseg.author(),
    license='GPL3',
    url=wordseg.url(),
    long_description=codecs.open('README.md', encoding='utf-8').read(),
    zip_safe=False,

    python_requires='>=3.7',

    setup_requires=[
        'pytest-runner'],

    tests_require=[
        'pytest>=5.0',
        'pytest-cov'],

    install_requires=[
        'six',
        'joblib',
        'scikit-learn'],

    packages=setuptools.find_packages(exclude='test'),

    entry_points={'console_scripts': [
        'wordseg-prep = wordseg.prepare:main',
        'wordseg-eval = wordseg.evaluate:main',
        'wordseg-stats = wordseg.statistics:main',
        'wordseg-syll = wordseg.syllabification:main',
        'wordseg-baseline = wordseg.algos.baseline:main',
        'wordseg-ag = wordseg.algos.ag:main',
        'wordseg-dibs = wordseg.algos.dibs:main',
        'wordseg-dpseg = wordseg.algos.dpseg:main',
        'wordseg-tp = wordseg.algos.tp:main',
        'wordseg-puddle = wordseg.algos.puddle:main']},

    data_files=[
        ('bin', ['build/bin/ag', 'build/bin/dpseg']),
        ('data/syllabification', data_files('data/syllabification')),
        ('data/ag', data_files('data/ag')),
        ('data/dpseg', data_files('data/dpseg'))],

    ext_modules=[
        CMakeExtension('dpseg', 'wordseg/algos/dpseg'),
        CMakeExtension('ag', 'wordseg/algos/ag')],
    cmdclass={'build_ext': CMakeBuild}
)
