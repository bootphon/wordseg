#!/usr/bin/env python


import codecs
import logging
import os

from wordseg.algos import ag


def preparation(tags_file, directory):
    from wordseg.prepare import prepare, gold

    tags = codecs.open(tags_file, 'r', encoding='utf8').readlines()[:10]
    prepared = list(prepare(tags))

    fprep = os.path.join(directory, 'input.txt')
    codecs.open(fprep, 'w', encoding='utf8').write('\n'.join(prepared) + '\n')

    fgold = os.path.join(directory, 'gold.txt')
    codecs.open(fgold, 'w', encoding='utf8').write('\n'.join(gold(tags)) + '\n')

    fgrammar = os.path.join(directory, 'grammar.lt')
    phones = sorted(set(p for line in prepared for p in line.split()))
    codecs.open(fgrammar, 'w', encoding='utf8').write(
        ag.build_colloc0_grammar(phones))

    return fprep, fgold, fgrammar


def run_ag(finput, foutput, fgrammar):
    args = '-n 100 -d 100'

    text = codecs.open(finput, 'r', encoding='utf8').readlines()
    ag._run_ag_single(text, foutput, fgrammar, args, log_level=logging.DEBUG)


def postprocess(output_files, segment_category, ignore_first_parses):
    for foutput in output_files:
        trees = [ag._yield_trees(
            codecs.open(foutput, 'r', encoding='utf8'),
            ignore_firsts=ignore_first_parses)]
        print(len(trees))

        # words = ag._tree2words(codecs.open(foutput, 'r', encoding='utf8'))
        # print('\n'.join(words))


def main():
    ftags = '/home/mathieu/dev/wordseg-ag-mem/test/data/tagged.txt'
    directory = '/home/mathieu/dev/wordseg-ag-mem/dev'
    foutput = os.path.join(directory, 'output.txt')

    fprep, fgold, fgrammar = preparation(ftags, directory)

    try:
        run_ag(fprep, foutput, fgrammar)
    except RuntimeError as e:
        print('fail with exception:', str(e))

    output = postprocess([foutput], 'Colloc0', 0)



if __name__ == '__main__':
    main()
