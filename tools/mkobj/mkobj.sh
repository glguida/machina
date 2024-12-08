#!/bin/bash

OUTPATH=../../kern

function genobj {
    TYPE=$1; shift;
    UPPERCASE=$(echo -n $TYPE | tr '[:lower:]' '[:upper:]')
    OUTPUT_FILE=$1; shift;
    m4 -D TYPE=$TYPE -DUPPERCASE=$UPPERCASE  obj.h.in > $OUTPUT_FILE
    echo "Generated file: $OUTPUT_FILE"
}

genobj port ${OUTPATH}/portref.h
genobj vmobj ${OUTPATH}/vmobjref.h
genobj task ${OUTPATH}/taskref.h
genobj thread ${OUTPATH}/threadref.h
