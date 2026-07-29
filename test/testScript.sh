#!/bin/bash

PROG=$1
shift
../src/qrat $PROG/${PROG}.qw -no-banner "$@" < /dev/null > $PROG/${PROG}.out 2>&1
diff $PROG/${PROG}.out $PROG/${PROG}.expected > /dev/null 2>&1
