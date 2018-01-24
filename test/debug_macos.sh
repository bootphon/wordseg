#!/bin/bash
#
# This script is executed on travis only for MacOS, used to debug
# dpseg and AG.

head -10 ./data/tagged.txt | wordseg-prep | wordseg-dpseg -f 1 -vv
