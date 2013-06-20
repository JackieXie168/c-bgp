XML_NODES=$(filter-out LINKS_%.xml, $(wildcard *.xml))

HTML_PATH=../../html
HTML_NODES=$(patsubst %.xml, %.html, $(XML_NODES))

TXT_PATH=../../txt
TXT_NODES=$(patsubst %.xml, %.txt, $(XML_NODES))

TEX_PATH=./
TEX_NODES=$(patsubst %.xml, %.tex, $(XML_NODES))

all: html txt tex

html: $(HTML_NODES)

txt: $(TXT_NODES)

tex: $(TEX_NODES)

#.SUFFIXES: .xml .html .txt .groff
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
	xsltproc ../xsl/node2tex.xsl $< > $@

clean:
	rm -f $(patsubst %, $(HTML_PATH)/nodes/%, $(HTML_NODES))
	rm -f $(patsubst %, $(TXT_PATH)/%, $(TXT_NODES))
	rm -f $(TEX_NODES)

