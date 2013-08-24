#!/bin/sh

groups="bgp net sim"

for g in $groups; do
	OUTPUT_FILE="$g""_commands.tex"
	echo "Generating $OUTPUT_FILE ...";
	FILES=`ls -1 "$g"_*.xml`;
	for f in $FILES; do
		echo "\input{"`echo $f | sed s/\.xml$// | sed s/_/\_/`"}";
	done > $OUTPUT_FILE;
done

OUTPUT_FILE="other_commands.tex"
echo "Generating $OUTPUT_FILE ...";
FILES=`ls -1 *.xml | grep -v -E '^LINKS_'`
for g in $groups; do
	FILES=`echo $FILES | tr ' ' '\n' | grep -v $g`;
done
for f in $FILES; do
	echo "\input{"`echo $f | sed s/\.xml$// | sed s/_/\_/`"}";
done > $OUTPUT_FILE;

