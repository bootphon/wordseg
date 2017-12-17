#!/usr/bin/env bash

# Selecting speakers from cha files in prep to generating a phono
# format.
#
# IMPORTANT!! Includes data selection.
#
# Alex Cristia alecristia@gmail.com 2015-10-26. From
# https://github.com/alecristia/CDSwordSeg/blob/master/database_creation/scripts

######### VARIABLES
# Variables that have been passed by the user

# input cha file
CHAFILE=$1

# output file
SELFILE=$2
#########


echo "selecting speakers from $CHAFILE"

#********** A T T E N T I O N ***************#
# Modify this section to select the lines you want, for example here,
# we exclude speech by children and non-humans

# List of specific names we needed to add to eliminate speech from
# target child and non-adults:
# Bloom70/Peter/*.cha	        JEN	Child
# Bloom70/Peter/*.cha	        JEN	Sister
# Brent/w1-1005.cha	        MAG	Target_Child
# Brown/Adam/*.cha	        CEC	Cousin
# Brown/Adam/adam23.cha	        PAU	Brother
# Brown/Sarah/sarah019.cha	ANN	Cousin
# Brown/Sarah/sarah020.cha	JOA	Child
# Brown/Sarah/sarah046.cha	SAN	Playmate
# Clark/shem15.cha	        BRU	Child
# Clark/shem15.cha	        CAR	Child
# Clark/shem15.cha	        LEN	Child
# Clark/shem37.cha	        JEM	Child
# Cornell/moore189.cha	        JOS	Child
# Feldman/09.cha	        STV	Target_Child
# Feldman/*.cha 	        LAR	Child
# MacWhinney/08a.cha	        MAD	Child
# MacWhinney/*.cha	        MAR	Brother
# Sachs/n61na.cha	        JEN	Playmate
# Weist/Roman/rom*.cha	        SOP	Sister

iconv -f ISO-8859-1 "$CHAFILE" |
    grep '^*' |
    grep -v 'SI.\|BR.\|CHI\|TO.\|ENV\|BOY\|NON\|MAG\|JEN\|MAG\|CEC\|PAU\|ANN\|JOA\|SAN\|BRU\|CAR\|LEN\|JEM\|JOS\|STV\|LAR\|MAD\|MAR\|SOP' |   # leave this line uncommented to get rid of all child speakers
    # grep 'MOT' |   # leave this line uncommented to focus only on mother's speech
    iconv -t ISO-8859-1 >> $SELFILE
