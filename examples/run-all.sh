#!/bin/bash

CBGP=../src/cbgp

for f in *.cli; do
	$CBGP -c $f
	echo ""
done
