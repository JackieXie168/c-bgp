DOC_SRCES= $(wildcard ../xml/nodes/*.xml)
DOC_FILES= $(DOC_SRCES:.xml=.txt)

DOC_INSTALL_FILES= $(wildcard *.txt)

EXTRA_DIST = $(DOC_INSTALL_FILES)

install-data-local:
	$(INSTALL) -d $(docdir)/txt
	@(for f in $(DOC_INSTALL_FILES); do \
		$(INSTALL) -m 444 $$f $(docdir)/txt/; \
	done)

SUFFIXES: .xml .txt

.xml.txt:
	@(target=`echo $@ | sed -e 's/.*\///g'`; \
	echo "generate $$target"; \
	xsltproc node2groff.xsl $< | groff -Tascii -mdoc > $$target)

gendoc: $(DOC_FILES)

