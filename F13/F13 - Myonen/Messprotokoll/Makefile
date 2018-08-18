# Makefile for building latex documents (c) Darko Veberic
# $Id: Makefile 3551 2015-01-24 11:03:56Z veberic $

USEPDFLATEX := 1
USEBIBTEX := 0

TEXFILES := $(shell grep -le '^\s*\\document\(class\|style\)' *.tex)
TARGETS := $(basename $(TEXFILES))
DEPENDENCIES := $(wildcard pictures/* *.bib) $(filter-out $(TEXFILES), $(wildcard *.tex Aufgaben/*.tex tikz/*.tex))

##########################################################
SHELL := bash
PDFS := $(addsuffix .pdf,$(TARGETS))
DEPENDENCIES += $(wildcard *.sty *.bib)

LATEX := $(if $(filter 1, $(USEPDFLATEX)), pdflatex, latex)

COLORIZE := awk 'BEGIN{red="\033[31;1m";yel="\033[34m";byel="\033[34;1m";end="\033[0m"}/Rerun /||/rerunfilecheck[^.]/{e=e$$0"\n";print red""$$0""end;next}/^LaTeX Warning:/||/natbib Warning:/{w=w$$0"\n";print yel""$$0""end;next}{print}END{if(w||e)print "\n"red"Warning/error summary:\n"byel""w""end;if(e){print red""e""end;exit 1}}'

define basic_latex_rule
  for i in 1 2 3 ; do $(LATEX) $< | $(COLORIZE) && break ; done
endef

ifeq ($(USEBIBTEX), 1)
define latex_rule
  $(LATEX) $< ; bibtex $*
  $(call basic_latex_rule)
endef
else
define latex_rule
  $(call basic_latex_rule)
endef
endif

.PHONY: all clean

all: $(PDFS)

# dependencies
$(PDFS): $(DEPENDENCIES)

clean:
	- $(RM) *{.tex~,.dvi,.log,.aux,.toc,.nav,.out,.snm,.bbl,.blg,.spl,.lof,Notes.bib,.maf,.fff,.lot,.ttt}
	- for f in $(TARGETS) ; do $(RM) $$f{.ps,.pdf} ; done

# general rules
ifeq ($(USEPDFLATEX), 1)
%.pdf: %.tex $(DEPENDENCIES)
	$(call latex_rule)

%.ps: %.pdf
	pdf2ps $<
else
%.dvi: %.tex $(DEPENDENCIES)
	$(call latex_rule)

%.pdf: %.dvi
	dvipdf $<

%.ps: %.dvi
	dvips $< -o
endif

# embed pdf fonts
%.embedded_fonts.pdf: %.pdf
	gs -dSAFER -dNOPLATFONTS -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -sPAPERSIZE=a4 -dCompatibilityLevel=1.4 -dPDFSETTINGS=/printer -dCompatibilityLevel=1.4 -dMaxSubsetPct=100 -dSubsetFonts=true -dEmbedAllFonts=true -sOutputFile=$@ -f $<
