HTML_PATH=../html
TXT_PATH=../txt

SUBDIRS = nodes

all: html txt

html:
	xsltproc index2html.xsl index.xml > $(HTML_PATH)/index.html
	xsltproc toc2html.xsl toc.xml > $(HTML_PATH)/toc.html
	for subdir in $(SUBDIRS); do \
	  cd $$subdir && $(MAKE) html; \
	done
	@echo "HTML documentation generated"

txt:
	for subdir in $(SUBDIRE); do \
	  cd $$subdir && $(MAKE) txt; \
	done
	@echo "TXT documentation generated"

.SUFFIXES: .xml .html

.PHONY: clean

clean:
	@rm -f $(HTML_PATH)/index.html $(HTML_PATH)/toc.html
	for subdir in $(SUBDIRS); do \
	  cd $$subdir && $(MAKE) clean; \
	done

