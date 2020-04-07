TEX = pdflatex -shell-escape -interaction=nonstopmode -file-line-error
PRE =  $(TEX) -ini -job-name="preamble" "&pdflatex preamble.tex\dump"
BIB = bibtex

all: cms-ccanr.pdf

view :
	    okular cms-ccanr.pdf

cms-ccanr.pdf : clean cms-ccanr.tex cms-ccanr.bbl cms-ccanr.blg
	    $(TEX) cms-ccanr.tex
	    $(TEX) cms-ccanr.tex

cms-ccanr.bbl cmsv5.blg : cms-ccanr.bib cms-ccanr.aux
	    $(BIB) cms-ccanr

cms-ccanr.aux : cms-ccanr.tex
	    $(TEX) cms-ccanr.tex

cms-ccanr.bib : cms-ccanr.tex
	    $(TEX) cms-ccanr.tex

clean:
	rm -f *.log *.pdf *.blg *.bbl *.aux *.out *.backup
