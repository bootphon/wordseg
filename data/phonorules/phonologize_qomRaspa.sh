#!/bin/sh
# Wrapper to take a single cleaned up transcript and phonologize it
# Alex Cristia alecristia@gmail.com 2015-10-26
# Modified by Laia Fibla laia.fibla.reixachs@gmail.com 2016-09-28 adapted to arg spanish

# Convert to a readable language 
#LC_CTYPE=C

#########VARIABLES
#Variables to modify
LANGUAGE=$1 	#right now, only options are qom and raspanish (rioplatense argentinian spanish) -- NOTICE, IN SMALL CAPS
ORTHO=$2 	#path to the file to be phonologized

#########
if [ "$LANGUAGE" = "qom" ]
   then
  echo "recognized $LANGUAGE"
  tr '[:upper:]' '[:lower:]' < "$ORTHO"  | # change uppercase letters to lowercase letters
  sed 's/ch/C/g' |
  sed 's/sh/S/g' |
  sed 's/ñ/N/g' |
  tr "'" "Q"  |
  iconv -f ISO-8859-1 > "$ORTHO"_phon


elif [ "$LANGUAGE" = "raspanish" ]
   then
  echo "recognized $LANGUAGE"
  tr '[:upper:]' '[:lower:]' < "$ORTHO"  | # change uppercase letters to lowercase letters
  sed 's/x/ks/g' |
  sed 's/ch/T/g' | # for clarity
  sed 's/ñ/N/g' |
  sed 's/j/x/g' |
  sed 's/v/b/g' | # rioplatense argentinian does not have this contrast
  sed 's/z/s/g' | # idem
  sed 's/c/k/g' |
  sed 's/rr/R/g' | # rioplatense argentinian contrasts r and rr
  sed 's/ r/ R/g' | # idem
  sed 's/y /i /g' | # word final y pronounced as i
  sed 's/y$/i/g' | # word final y pronounced as i
  sed 's/y/S/g' | 
  sed 's/ll/S/g' | 
  sed 's/qu/k/g' |
  sed 's/á/a/g' |
  sed 's/é/e/g' |
  sed 's/í/i/g' |
  sed 's/ó/o/g' |
  sed 's/ú/u/g' |
  sed 's/ü/u/g' |
  sed 's/h//g' | 
  iconv -f ISO-8859-1 > "$ORTHO"_phon


else
	echo "Adapt the script to a new language"
	echo "I don't know $LANGUAGE"

fi


echo "finished"
