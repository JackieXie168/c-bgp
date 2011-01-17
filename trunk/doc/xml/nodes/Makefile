HTML_PATH=../../html
XML_NODES=$(filter-out LINKS_%.xml, $(wildcard *.xml))
HTML_NODES=$(patsubst %.xml, %.html, $(XML_NODES))

all: html

html: $(HTML_NODES)
	@echo "HTML documentation generated"

.SUFFIXES: .xml .html

.PHONY: clean

.xml.html:
	xsltproc --stringparam filename "LINKS_$<" node2html.xsl $< > $(HTML_PATH)/nodes/$@

clean:
	@rm -f $(patsubst %, $(HTML_PATH)/nodes/%, $(HTML_NODES))

