# Makefile for generating derived BDF and PCF font files -- Markus Kuhn

# use bash or ksh if the "${i%.bdf}" substitution fails with your shell
SHELL=/bin/bash
#SHELL=/bin/ksh

# Location of the mapping files from ftp://ftp.unicode.org/Public/MAPPINGS/
MAPPINGS=../MAPPINGS
# Location of the ISO10646-1 source BDF files
SRCBDF=..
# BDF remapping tool
UCS2ANY=../ucs2any.pl +d
# BDF truncation tool
BDFTRUNCATE=../bdftruncate.pl

FONTS=5x7 5x8 6x9 6x10 6x12 6x13 7x13 7x14 8x13 9x15 9x18 10x20 \
      6x13B 6x13O 7x13B 7x13O 7x14B 8x13B 8x13O 9x15B 9x18B
BIGFONTS=12x13ja 18x18ja 18x18ko

all: pcfs

# Make a small essential selection of 8-bit encoding variants
bdfs:
	for i in $(FONTS) ; do \
	  $(BDFTRUNCATE) U+3200 <$(SRCBDF)/$$i.bdf >$$i.bdf ; \
	  $(UCS2ANY) $(SRCBDF)/$$i.bdf \
	    $(MAPPINGS)/8859-1.TXT  ISO8859-1 ; \
	done
	for i in 6x13 6x13B 6x13O 7x13 7x13B 7x13O 8x13 8x13B 8x13O ; do \
	  $(UCS2ANY) $(SRCBDF)/$$i.bdf \
	    $(MAPPINGS)/8859-2.TXT  ISO8859-2  \
	    $(MAPPINGS)/8859-5.TXT  ISO8859-5  \
	    $(MAPPINGS)/8859-7.TXT  ISO8859-7  \
	    $(MAPPINGS)/8859-9.TXT  ISO8859-9  \
	    $(MAPPINGS)/8859-13.TXT ISO8859-13 \
	    $(MAPPINGS)/8859-15.TXT ISO8859-15 ; \
	done
	for i in 6x13 7x13 8x13 ; do \
	  $(UCS2ANY) $(SRCBDF)/$$i.bdf \
	    $(MAPPINGS)/8859-3.TXT  ISO8859-3  \
	    $(MAPPINGS)/8859-4.TXT  ISO8859-4  \
	    $(MAPPINGS)/8859-8.TXT  ISO8859-8  \
	    $(MAPPINGS)/8859-10.TXT ISO8859-10 \
	    $(MAPPINGS)/KOI8-R.TXT KOI8-R ; \
	done
	for i in 7x14 ; do \
	   $(UCS2ANY) $(SRCBDF)/$$i.bdf \
	    $(MAPPINGS)/JIS0201.TXT JISX0201.1976-0 ; \
	done
	-for i in $(BIGFONTS) ; do \
	  cp $(SRCBDF)/$$i.bdf . ; \
	done

# Make all possible 8-bit encoding variants
all-bdfs:
	for i in $(FONTS) ; do \
	  $(BDFTRUNCATE) U+3200 <$(SRCBDF)/$$i.bdf >$$i.bdf ; \
	  $(UCS2ANY) $(SRCBDF)/$$i.bdf \
	    $(MAPPINGS)/8859-1.TXT  ISO8859-1  \
	    $(MAPPINGS)/8859-2.TXT  ISO8859-2  \
	    $(MAPPINGS)/8859-3.TXT  ISO8859-3  \
	    $(MAPPINGS)/8859-4.TXT  ISO8859-4  \
	    $(MAPPINGS)/8859-5.TXT  ISO8859-5  \
	    $(MAPPINGS)/8859-7.TXT  ISO8859-7  \
	    $(MAPPINGS)/8859-8.TXT  ISO8859-8  \
	    $(MAPPINGS)/8859-9.TXT  ISO8859-9  \
	    $(MAPPINGS)/8859-10.TXT ISO8859-10 \
	    $(MAPPINGS)/8859-13.TXT ISO8859-13 \
	    $(MAPPINGS)/8859-14.TXT ISO8859-14 \
	    $(MAPPINGS)/8859-15.TXT ISO8859-15 \
	    $(MAPPINGS)/KOI8-R.TXT KOI8-R ; \
	done
	for i in 7x14 ; do \
	  $(UCS2ANY) $(SRCBDF)/$$i.bdf \
	    $(MAPPINGS)/JIS0201.TXT JISX0201.1976-0 ; \
	done
	-for i in $(BIGFONTS) ; do \
	  cp $(SRCBDF)/$$i.bdf . ; \
	done

pcfs: bdfs
	for i in *.bdf ; do \
	  bdftopcf $$i >$${i%.bdf}.pcf ; \
	done
	gzip -9 *.pcf

all-pcfs: all-bdfs
	for i in *.bdf ; do \
	  bdftopcf $$i >$${i%.bdf}.pcf ; \
	done
	gzip -9 *.pcf

ucs-fixed.tar.gz: pcfs
	mkfontdir
	tar cvf ucs-fixed.tar README fonts.alias *.bdf *.pcf.gz
	gzip -9 ucs-fixed.tar

distribution: ucs-fixed.tar.gz
	rcsdiff -u RCS/*
	mv ucs-fixed.tar.gz \
	  $(HOME)/.www/download/ucs-fixed-`date -u +%Y%m%d`.tar.gz
	rm -f $(HOME)/.www/download/ucs-fixed.tar.gz
	ln -s ucs-fixed-`date -u +%Y%m%d`.tar.gz \
	  $(HOME)/.www/download/ucs-fixed.tar.gz

install: pcfs
	mkdir -p $(HOME)/local/lib/X11/fonts/
	mv *.pcf.gz $(HOME)/local/lib/X11/fonts/
	cd $(HOME)/local/lib/X11/fonts/ ; mkfontdir
	xset +fp $(HOME)/local/lib/X11/fonts

clean:
	rm -f fonts.dir *.bdf *.pcf *.gz *~ list1 list2

list:
	ls *.bdf | sort -n | \
	perl -pe 's/^(.*).bdf$$/\tFontObj($$1) \\/;' >list1
	ls *.bdf | sort -n | \
	perl -pe 's/^(.*).bdf$$/FontTarget($$1)/;' >list2
