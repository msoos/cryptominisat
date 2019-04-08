TEX = pdflatex -shell-escape -interaction=nonstopmode -file-line-error
PRE =  $(TEX) -ini -job-name="preamble" "&pdflatex preamble.tex\dump"
BIB = bibtex

all: cmsv56-yalsat.pdf

view :
	    okular cmsv56-yalsat.pdf

cmsv56-yalsat.pdf : clean cmsv56-yalsat.tex cmsv56-yalsat.bbl cmsv56-yalsat.blg
	    $(TEX) cmsv56-yalsat.tex
	    $(TEX) cmsv56-yalsat.tex

cmsv56-yalsat.bbl cmsv5.blg : cmsv56-yalsat.bib cmsv56-yalsat.aux
	    $(BIB) cmsv56-yalsat

cmsv56-yalsat.aux : cmsv56-yalsat.tex
	    $(TEX) cmsv56-yalsat.tex

cmsv56-yalsat.bib : cmsv56-yalsat.tex
	    $(TEX) cmsv56-yalsat.tex

clean:
	rm -f *.log *.pdf *.blg *.bbl *.aux *.out *.backup
