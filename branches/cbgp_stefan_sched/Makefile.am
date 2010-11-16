SUBDIRS = src valid lib examples

EXTRA_DIST = \
	cbgp.spec

.PHONY: doc

DOC_SUBDIRS = src

doc:
	@(for subdir in $(DOC_SUBDIRS); do \
	(echo "$$subdir" ; cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) doc) \
	done)
