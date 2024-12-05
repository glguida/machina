#!/bin/bash

set -x

STYLE=-nbad -bap -nbc -bbo -bl -bli2 -bls -ncdb -nce -cp1 -cs -di2 \
-ndj -nfc1 -nfca -hnl -i2 -ip5 -lp -pcs -nprs -psl -saf -sai \
-saw -nsc -nsob

for i in `find . | grep \\\.[ch]$ | grep -v contrib/ | grep -v tools/mig | grep -v libmach3/`; do echo $i; indent ${STYLE} $i; done
