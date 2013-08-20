XML_NODES=$(filter-out LINKS_%.xml, $(wildcard *.xml))

HTML_PATH=../../html
HTML_NODES=$(patsubst %.xml, %.html, $(XML_NODES))

TXT_PATH=../../txt
TXT_NODES=$(patsubst %.xml, %.txt, $(XML_NODES))

TEX_PATH=../../tex
TEX_NODES=$(patsubst %.xml, %.tex, $(XML_NODES))

all: html txt tex

html: $(HTML_NODES)

txt: $(TXT_NODES)

tex: $(TEX_NODES)
	OLD_PWD=$(shell pwd) ; \
	cd $(TEX_PATH) ; \
	for CMD in bgp net sim ; do \
		echo Generating "$$CMD"_commands.tex ; \
		rm "$$CMD"_commands.tex ; \
		ls -1 "nodes/$$CMD"_*.tex | awk '{ print "\\input{"$$0"}" }' >> "$$CMD"_commands.tex; \
	done ; \
	echo Generating other_commands.tex ; \
	rm -f other_commands.tex ; \
	ls -1 nodes/*.tex | grep -v -E '^nodes/(bgp|net|sim)' | awk '{ print "\\input{"$$0"}" }' >> other_commands.tex ; \
	cd $$OLD_PWD

#echo `ls -1 "$$CMD"_*.tex | awk '{ print "\input{"$0"}" }' >> "$$CMD"_commands.tex` ; \

.SUFFIXES: .xml .html .txt .groff .tex

.PHONY: clean

.xml.html:
	xsltproc --stringparam filename "../nodes/LINKS_$<" ../xsl/node2html.xsl $< > $(HTML_PATH)/nodes/$@

.xml.txt:
	xsltproc --stringparam filename "../nodes/LINKS_$<" ../xsl/node2groff.xsl $< | groff -Tascii -m mdoc - > $(TXT_PATH)/$@

#.xml.groff:
#	xsltproc --stringparam filename "LINKS_$<" ../xsl/node2groff.xsl $< > $@

#.groff.txt:
#	groff $< -Tascii -m mdoc > $(TXT_PATH)/$@

.xml.tex:
	xsltproc ../xsl/node2tex.xsl $< > $(TEX_PATH)/nodes/$@

clean:
	rm -f $(patsubst %, $(HTML_PATH)/nodes/%, $(HTML_NODES))
	rm -f $(patsubst %, $(TXT_PATH)/%, $(TXT_NODES))
	rm -f $(TEX_NODES)

