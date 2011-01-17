HTML_PATH=../html

SUBDIRS = nodes

all: html

html:
	xsltproc $(HTML_PATH)/index2html.xsl index.xml > $(HTML_PATH)/index.html
	xsltproc $(HTML_PATH)/toc2html.xsl toc.xml > $(HTML_PATH)/toc.html
	for subdir in $(SUBDIRS); do \
	  cd $$subdir && $(MAKE) html; \
	done
	@echo "HTML documentation generated"

.SUFFIXES: .xml .html

.PHONY: clean

clean:
	@rm -f $(HTML_PATH)/index.html $(HTML_PATH)/toc.html
	for subdir in $(SUBDIRS); do \
	  cd $$subdir && $(MAKE) clean; \
	done

