"""lx.py -- Mark Johnson, 24th Febuary 2005

lx contains utility functions for the other programs
in this directory."""

import csv, os, os.path

def incr(d, k, inc=1):
    """incr adds inc to the value of d[k] if d[k] is defined,
    or sets d[k] to inc if d[k] is undefined.

    d is the dictionary being incremented.
    k is the dictionary key whose value is incremented.
    inc is the size of the increment (default 1)."""
    if k in d:
        d[k] += inc
    else:
        d[k] = inc

def incr2(d, k1, k2, inc=1):
    """incr2 adds inc to the value of d[k1][k2] if d[k1][k2] is defined,
    or sets d[k1][k2] to inc if d[k1][k2] is undefined.

    d is the dictionary of dictionaries being incremented.
    k1, k2 are the dictionary keys whose value is incremented.
    inc is the size of the increment (default 1)."""
    if k1 in d:
        dk1 = d[k1]
        if k2 in dk1:
            dk1[k2] += inc
        else:
            dk1[k2] = inc
    else:
        d[k1] = {k2:inc}

def incr3(d, k1, k2, k3, inc=1):
    """incr3 adds inc to the value of d[k1][k2][k3] if it is defined,
    otherwise it sets d[k1][k2][k3] to inc.

    d is the dictionary of dictionaries being incremented.
    k1, k2, k3 are the dictionary keys whose value is incremented.
    inc is the size of the increment (default 1). """
    if k1 in d:
        dk1 = d[k1]
        if k2 in dk1:
            dk1k2 = dk1[k2]
            if k3 in dk1k2:
                dk1k2[k3] += inc
            else:
                dk1k2[k3] = inc
        else:
            dk1[k2] = {k3:inc}
    else:
        d[k1] = {k2:{k3:inc}}

def second(xs):
    """second() returns the second element in a sequence.
    This is mainly usefule as the value of the key argument
    to sort and sorted."""
    return xs[1]


def count_elements(xs, dct=None):
    """Given a sequence xs of elements, return a dictionary dct of
    mapping elements to the number of times they appear in items.  If
    dct is not None, use dct as this dictionary."""
    if dct==None:
        dct = {}
    for item in xs:
        incr(dct, item)
    return dct


# Finding all files that meet a condition

def findfiles(topdir, file_re):
    """Returns a list of filenames below dir whose names match filenameregex."""
    filenames = []
    files = os.listdir(topdir)
    for file in files:
        if file_re.match(file):
            filenames.append(os.path.join(topdir, file))
    return filenames


def writecsvfile(filename, data, header=None):
    """writecsvfile writes data to a file in a format that can be
    easily imported to a spreadsheet.  Specifically, it writes data to
    csvfilename.csv, with header at the top.  If Header != None, it
    also checks that each tuple in data has same length as header.
    CSV stands for Comma Separated Values, and CSV files are generally
    readable by spreadsheet programs like Excel."""
    outf = csv.writer(file(filename+".csv", "wb"))
    if header != None:
        outf.writerow(header)
    for row in data:
        outf.writerow(row)
        if header != None and len(header) != len(row):
            print("## Error in zipf:writecsv(): header = %s, row = %s" % (header,row))
