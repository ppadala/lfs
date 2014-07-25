all: lfs.pdf

lfs.pdf: lfs.tex
	latex lfs.tex
	bibtex lfs
	latex lfs.tex
	latex lfs.tex
	dvipdf lfs.dvi

clean:
	rm lfs.aux lfs.out lfs.log lfs.blg lfs.dvi lfs.pdf
