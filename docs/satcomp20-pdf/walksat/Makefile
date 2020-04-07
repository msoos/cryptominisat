TEX = pdflatex -shell-escape -interaction=nonstopmode -file-line-error
PRE =  $(TEX) -ini -job-name="preamble" "&pdflatex preamble.tex\dump"
BIB = bibtex

all: cms-walksat.pdf

view :
	    okular cms-walksat.pdf

cms-walksat.pdf : clean cms-walksat.tex cms-walksat.bbl cms-walksat.blg
	    $(TEX) cms-walksat.tex
	    $(TEX) cms-walksat.tex

cms-walksat.bbl cmsv5.blg : cms-walksat.bib cms-walksat.aux
	    $(BIB) cms-walksat

cms-walksat.aux : cms-walksat.tex
	    $(TEX) cms-walksat.tex

cms-walksat.bib : cms-walksat.tex
	    $(TEX) cms-walksat.tex

clean:
	rm -f *.log *.pdf *.blg *.bbl *.aux *.out *.backup
