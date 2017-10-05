#!/usr/bin/env python
"""developing ag..."""

import logging
import os
import sys

from wordseg.utils import get_logger
from wordseg.prepare import prepare
from wordseg.algos import ag


input_file = './test/data/phonologic.txt.100'
grammar_file = './config/ag/Colloc0_enFestival.lt'

ag_args = '--log-level debug'

text = list(prepare(open(input_file, 'r').readlines()))

ag.segment(text, grammar_file,
           args=ag_args,
           log=get_logger(name='ag', level=logging.DEBUG))
