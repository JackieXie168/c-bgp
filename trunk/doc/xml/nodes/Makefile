XML_NODES=$(filter-out LINKS_%.xml, $(wildcard *.xml))

HTML_PATH=../../html
HTML_NODES=$(patsubst %.xml, %.html, $(XML_NODES))

TXT_PATH=../../txt
TXT_NODES=$(patsubst %.xml, %.txt, $(XML_NODES))

all: html txt

html: $(HTML_NODES)
	@echo "HTML documentation generated"

txt: $(TXT_NODES)
	@echo "TXT documentation generated"

.SUFFIXES: .xml .html .txt .groff

.PHONY: clean

.xml.html:
	xsltproc --stringparam filename "LINKS_$<" node2html.xsl $< > $(HTML_PATH)/nodes/$@

.xml.groff:
	xsltproc node2groff.xsl $< > $@

.groff.txt:
	groff $< -Tascii -m mdoc > $(TXT_PATH)/$@

clean:
	@rm -f $(patsubst %, $(HTML_PATH)/nodes/%, $(HTML_NODES))
	@rm -f $(patsubst %, $(TXT_PATH)/%, $(TXT_NODES))
	echo $(patsubst %, $(TXT_PATH)/%, $(TXT_NODES))

