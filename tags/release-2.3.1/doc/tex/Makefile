SOURCES= \
	abstract.tex \
	bgp_commands.tex \
	cbgp-doc.tex \
	installation.tex \
	net_commands.tex \
	other_commands.tex \
	sim_commands.tex \
	user-interface.tex \
	version.tex
AUX_FILES= $(SOURCES:.tex=.aux)

pdf: $(SOURCES)
	pdflatex cbgp-doc.tex
	pdflatex cbgp-doc.tex

.PHONY: clean

clean:
	rm -f cbgp-doc.pdf cbgp-doc.aux cbgp-doc.log cbgp-doc.toc cbgp-doc.out $(AUX_FILES) 
