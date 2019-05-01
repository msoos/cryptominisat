TEX = pdflatex -shell-escape -interaction=nonstopmode -file-line-error
PRE =  $(TEX) -ini -job-name="preamble" "&pdflatex preamble.tex\dump"
BIB = bibtex

all: cmsv56-walksat.pdf

view :
	    okular cmsv56-walksat.pdf

cmsv56-walksat.pdf : clean cmsv56-walksat.tex cmsv56-walksat.bbl cmsv56-walksat.blg
	    $(TEX) cmsv56-walksat.tex
	    $(TEX) cmsv56-walksat.tex

cmsv56-walksat.bbl cmsv5.blg : cmsv56-walksat.bib cmsv56-walksat.aux
	    $(BIB) cmsv56-walksat

cmsv56-walksat.aux : cmsv56-walksat.tex
	    $(TEX) cmsv56-walksat.tex

cmsv56-walksat.bib : cmsv56-walksat.tex
	    $(TEX) cmsv56-walksat.tex

clean:
	rm -f *.log *.pdf *.blg *.bbl *.aux *.out *.backup
