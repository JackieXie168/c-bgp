XML_PATH= ../xml/nodes
DOC_SRCES= $(wildcard $(XML_PATH)/*.xml)
DOC_FILES= $(patsubst $(XML_PATH)/%,%,$(DOC_SRCES:.xml=.txt))

EXTRA_DIST = $(DOC_FILES)

install-data-local:
	$(INSTALL) -d $(docdir)/txt
	@(for f in $(DOC_FILES); do \
		$(INSTALL) -m 444 $$f $(docdir)/txt/; \
	done)

SUFFIXES= .xml .txt .groff

%.groff: $(XML_PATH)/%.xml
	xsltproc node2groff.xsl $< > $@

%.txt: %.groff
	groff $< -Tascii -mdoc > $@

gendoc: $(DOC_FILES:.txt=.groff) $(DOC_FILES)

verify:
	@(for f in $(DOC_SRCES); do \
	RESULT=`grep '<abstract></abstract>' $$f`; \
	if [ ! -z $$RESULT ]; then \
	echo "verify failed for $$f"; \
	fi \
	done)

