# prefix of where Ecolab software is to be installed
ifdef MXE
PREFIX=$(HOME)/usr/mxe/ecolab
else
PREFIX=$(HOME)/usr/ecolab
endif

# check if a user defined PREFIX is not good
TPREFIX=$(filter-out /usr /usr/local,$(PREFIX:%/=%))
ifeq ($(TPREFIX),)
$(error Cowardly refusing to install to $(PREFIX))
endif

# issue warning if a nonstandard location is specified
TPREFIX=$(filter /usr/ecolab /usr/local/ecolab $(HOME)/usr/ecolab $(HOME)/usr/mxe/ecolab,$(PREFIX:%/=%))
ifeq ($(TPREFIX),)
$(warning Nonstandard EcoLab install location $(PREFIX) specified)
endif

ECOLAB_HOME=$(shell pwd)
PATH:=$(PATH):$(ECOLAB_HOME)/utils

MCFG=include/Makefile.config

include include/Makefile

#undefine ECOLAB_LIB here so that ecolab_library is not set to the
#compiled location. INSTALLED_ECOLAB_LIB is for the generic ecolab
#binary
FLAGS+=-UECOLAB_LIB -DINSTALLED_ECOLAB_LIB=\"$(PREFIX)/include\"

ifdef AEGIS
# must build and test against c++11 now, as TR1 has goone!
CXXFLAGS+=-std=c++11
endif

# when upgrading MXE, this will need to be removed 
ifdef MXE
CXXFLAGS+=-std=c++11
endif


# for mingw, make should still be inoked with PREFIX=/mingw, or to root location 
# of all development libraries and includes, so that $(PREFIX)/include and
# $(PREFIX)/lib point to valid locations.
ifdef MSYSTEM
ifndef __MINGW32_VERSION
#the code uses this macro to check if compiling under mingw.
FLAGS+=-D__MINGW32_VERSION -DTK -DCAIRO
else
FLAGS+=-D__MINGW32_VERSION=$(__MINGW32_VERSION) -DTK -DCAIRO
endif
endif

ifeq ($(OS),Darwin)
CXXFLAGS+=-std=c++11
endif

VPATH=include src models

# objects to build ecolab.a
OBJS=src/automorph.o src/auxil.o src/arrays.o src/sparse_mat.o \
	src/findFirst.o src/graph.o src/netcomplexity.o src/naugraph.o \
	src/nautil.o src/nauty.o src/nautinv.o src/cachedDBM.o src/TCL_obj.o src/igraph.o \
	src/cairo_types.o src/cairo_base.o  src/cairoSurfaceImage.o src/plot.o src/tcl_arrays.o \
	src/tclgraph.o src/analysis.o src/random.o  src/ecolab.o

ifdef XDR
OBJS+=src/xdr_pack.o
endif

# build for Mac Aqua interface, requires special static build of Tk.
ifdef MAC_OSX_TK
CXXFLAGS+=-DMAC_OSX_TK 
OBJS+=src/getContext.o
LIBS+=-framework AppKit
endif

CDHDRS=ref.cd random.cd random_basic.cd TCL_obj_base.cd RESTProcess_base.cd signature.cd netcomplexity.cd graph.cd cachedDBM.cd sparse_mat.cd analysis.cd analysisBLT.cd analysisCairo.cd plot.cd cairoSurfaceImage.cd poly.cd polyRESTProcess.cd

ifdef UNURAN
CDHDRS+=random_unuran.cd
endif
ifdef GNUSL
CDHDRS+=random_gsl.cd
endif

# Utility programs
UTILS=utils/wrap utils/tac
SCRIPTS=utils/mkmacapp models/macrun

ELIBS=lib/libecolab$(ECOLIBS_EXT).a $(MODS:%=lib/%)

# toplevel version
include Makefile.version

# variant of $(VERSION) that has leading 0s stripped (for sonames)
SOVERSION=$(subst D0,D,$(subst D00,D,$(VERSION)))

#chmod command is to counteract AEGIS removing execute privelege from scripts
ifdef AEGIS
aegis-all: 
	$(MAKE) all
	$(MAKE) latex-docs 
	cd gml2pajek && $(MAKE)
	cd test; $(MAKE)
endif

all: all-without-models
	$(MAKE) models 
	-$(CHMOD) a+x models/*.tcl

all-without-models: ecolab-libs
	$(MAKE) bin/ecolab$(ECOLIBS_EXT)
	-$(CHMOD) a+x $(SCRIPTS)
# copy in the system built TCL library
ifdef MXE
	cp -r $(call search,lib*/tcl$(TCLVERSION)) include/tcl
	cp -r $(call search,lib*/tk$(TCLVERSION)) include/tk
endif
# update Makefile.config with the configuration parameters used to build this
	echo TK=$(TK)>$(MCFG)
	echo ZLIB=$(ZLIB)>>$(MCFG)
	echo READLINE=$(READLINE)>>$(MCFG)
	echo XDR=$(XDR)>>$(MCFG)
	echo UNURAN=$(UNURAN)>>$(MCFG)
	echo PRNG=$(PRNG)>>$(MCFG)
	echo GNUSL=$(GNUSL)>>$(MCFG)
	echo PARMETIS=$(PARMETIS)>>$(MCFG)
	echo IGRAPH=$(IGRAPH)>>$(MCFG)
	echo SAUCY=$(SAUCY)>>$(MCFG)
	echo CAIRO=$(CAIRO)>>$(MCFG)
	echo PANGO=$(PANGO)>>$(MCFG)
	echo BLT=$(BLT)>>$(MCFG)
	echo BDB=$(BDB)>>$(MCFG)
	echo GDBM=$(GDBM) >>$(MCFG)
	echo GDBM_COMPAT=$(GDBM_COMPAT) >>$(MCFG)
	echo MPI=$(MPI)>>$(MCFG)
	echo PARALLEL=$(PARALLEL)>>$(MCFG)
	echo OPENMP=$(OPENMP)>>$(MCFG)
	echo GCC=$(GCC)>>$(MCFG)
	echo NOGUI=$(NOGUI)>>$(MCFG)
	echo AQUA=$(AQUA)>>$(MCFG)	
	echo MAC_OSX_TK=$(MAC_OSX_TK)>>$(MCFG)

ecolab-libs: lib bin $(CLASSDESC) include/nauty_sizes.h
	$(MAKE) $(UTILS) 
	$(MAKE) $(ELIBS) 

.PHONY: models classdesc

$(OBJS) $(MODS:%=src/%): $(CDHDRS:%=include/%)

# newarrays needs to be preexpanded ...
#include/newarrays.cd: include/newarrays.exh
#	classdesc -I $(CDINCLUDE) -I $(ECOLAB_HOME)/include $(ACTIONS) <$< >$@

include/newarrays.exh: include/newarrays.h utils/wrap
	gcc -E -P $(CONT_FLAG) $< |wrap >$@


models: $(ELIBS) 
	cd models; $(MAKE)

#how to build a utility executable
$(UTILS): %: %.cc 
	$(CPLUSPLUS) -g $(FLAGS) $< -o $@

GRAPHCODE_MAKE=$(MAKE) CC="$(CC)" CPLUSPLUS="$(CPLUSPLUS)" FLAGS="$(FLAGS) $(CXXFLAGS) $(OPT)"

lib: 
	-mkdir -p lib

bin: 
	-mkdir -p bin


lib/libecolab$(ECOLIBS_EXT).a: $(OBJS) $(LIBMODS)
# build graphcode objects
	-cd graphcode; $(GRAPHCODE_MAKE) MAP=vmap libgraphcode.a
	-cd graphcode; $(GRAPHCODE_MAKE) MAP=hmap libgraphcode.a
	-cp -f graphcode/*.h graphcode/vmap graphcode/hmap include
	ar r $@ graphcode/*.hmap graphcode/*.vmap $^
ifeq ($(OS),Darwin)
	ranlib $@
endif
ifdef DYNAMIC
	$(CPLUSPLUS) -shared -Wl,-soname,libecolab$(ECOLIBS_EXT).so.$(SOVERSION)  $^ graphcode/*.hmap graphcode/*.vmap -o lib/libecolab$(ECOLIBS_EXT).so.$(SOVERSION)
	cd lib; ln -sf libecolab$(ECOLIBS_EXT).so.$(SOVERSION) libecolab$(ECOLIBS_EXT).so
endif

$(MODS:%=lib/%): lib/%: src/%
	cp $< $@
ifeq ($(OS),IRIX64)
ifndef GCC
	cp -r src/ii_files lib
endif
endif

$(CLASSDESC):
	cd classdesc; $(MAKE) PREFIX=$(ECOLAB_HOME) XDR=$(XDR) install

src/xdr_pack.cc: classdesc/xdr_pack.cc
	-cp $< $@

#Include dependencies
# don't include dependencies is running libtest (t0008a)
ifndef LIBTEST
ifneq ($(MAKECMDGOALS),clean)
include $(OBJS:.o=.d) $(MODS:%.o=src/%.d)
endif
endif

.PHONY: clean
clean: 
	-$(BASIC_CLEAN) generate_nauty_sizes
	-cd src; $(BASIC_CLEAN) 
	-cd utils; $(BASIC_CLEAN)
	-cd include; $(BASIC_CLEAN) nauty_sizes.h unpack_base.h hashmap.h vmap hmap
	-cd include/Xecolab; $(BASIC_CLEAN)
	-rm -f $(patsubst classdesc/%,include/%,$(wildcard classdesc/*.h))
	-cd classdesc; $(MAKE) clean 
	-cd models; $(MAKE) ECOLAB_HOME=.. clean
	-cd models/Stupid_Project; $(MAKE) ECOLAB_HOME=../.. clean
	-cd test; $(MAKE) ECOLAB_HOME=.. clean
	-cd classdesc; $(MAKE) clean
	-cd graphcode; $(MAKE) clean
	-cd test; $(MAKE) clean
	-cd doc; $(BASIC_CLEAN) *.aux *.dvi *.log *.blg *.toc *.lof *.ps; rm -rf ecolab
	-rm -f $(UTILS)
	-rm -f lib/* bin/*
	-rm -rf classdesc-lib cxx_repository
	-rm -rf ii_files */ii_files
	-rm $(MCFG)

doc/ecolab/ecolab.html: doc/*.tex
	(cd doc; ./Makedoc)

distrib: doc/ecolab/ecolab.html
	rm -rf ecolab/*	        
	cp -r doc $(DIST) ecolab

# test compile Latex docs, if latex is on system
latex-docs:
	if which latex; then cd doc; rm -f *.aux *.dvi *.log *.blg *.toc *.lof *.out; latex -interaction=batchmode ecolab; fi

# bin/ecolab is a no model ecolab binary that only works from installed location
bin/ecolab$(ECOLIBS_EXT): src/ecolab.o src/tclmain.o lib/libecolab$(ECOLIBS_EXT).a
	$(LINK) $(FLAGS) src/ecolab.o src/tclmain.o  $(LIBS) -o $@
	-find . \( -name "*.cc" -o -name "*.h" \) -print |etags -

.PHONY: install
install: all-without-models
# recompile src/ecolab so as to pick up new INSTALLED_ECOLAB_LIB based on PREFIX
	rm src/ecolab.o
	$(MAKE) bin/ecolab$(ECOLIBS_EXT)
# if installing on top of a different version, remove directory completely
	if [ -f $(PREFIX)/include/version.h ] && ! diff -q $(PREFIX)/include/version.h include/version.h; then rm -rf $(PREFIX); fi
	mkdir -p $(PREFIX)
	cp -r include $(PREFIX)
# ensure cd files are more up-to-date than their sources.
	touch $(PREFIX)/include/*.cd
# fix up unpack_base.h
	(cd $(PREFIX)/include; ln -f -s pack_base.h unpack_base.h)
	cp -r lib bin $(PREFIX)
	cp models/histogram.tcl $(PREFIX)/bin
	chmod a+x $(PREFIX)/bin/*.tcl
ifeq ($(OS),Darwin)
	ranlib $(PREFIX)/lib/*.a
endif
	cp -r utils $(PREFIX)
	@if [ -z "$(TPREFIX)" ]; then echo "Warning: nonstandard install location $(PREFIX) specified"; fi

UNURAN_LIB=$(firstword $(call search,lib*/libunuran.a))

$(ECOLAB_HOME)/$(MCFG):
	@rm -f $(MCFG)
# absolute dependecies
	@if [ -z "$(call search,lib*/tclConfig.sh)" ]; then \
	   echo "Error: Cannot find TCL - please install TCL"; fi
# optionals
	@if [ -n "$(call search,lib*/tkConfig.sh)" ]; then echo TK=1>>$(MCFG); fi
	@if [ -n "$(call search,include/zlib.h)" ]; then echo ZLIB=1>>$(MCFG); fi
	@if [ -n "$(call search,include/readline/readline.h)" ]; then echo READLINE=1>>$(MCFG); fi
	@if [ -n "$(call search,include/rpc/xdr.h)" ]; then echo XDR=1>>$(MCFG); fi
# if unuran installed chose that, otherwise chose GNUSL if installed
	@if [ -n "$(call search,include/unuran.h)" ]; then echo UNURAN=1>>$(MCFG); fi
	@if nm $(UNURAN_LIB)|grep prng_new >/dev/null; then \
	    echo PRNG=1>>$(MCFG); fi
	@if [ -n "$(call search,include/gsl/gsl_version.h)" ]; then echo GNUSL=1>>$(MCFG); fi
	@if [ -n "$(call search,include/parmetis.h)" ]; then echo PARMETIS=1>>$(MCFG); fi
	@if [ -n "$(call search,include/igraph/igraph.h)" ]; then echo IGRAPH=1>>$(MCFG); fi
	@if [ -n "$(call search,include/saucy.h)" ]; then echo SAUCY=1>>$(MCFG); fi
	@if $(PKG_CONFIG) --exists cairo; then echo CAIRO=1>>$(MCFG); \
	elif [ -n "$(call search,include/blt.h)" ]; then echo BLT=1>>$(MCFG); fi
# select Berkley DB by default, 
	@if [ -n "$(call search,include/db4/db.h)" -o -n "$(call search,include/db.h)" ]; then \
	  echo BDB=1>>$(MCFG); \
        else \
	  if [ -n "$(call search,lib*/libgdbm.so)" -o -n "$(call search,lib*/*/libgdbm.so)" -o -n "$(call search,lib*/libgdbm.a)" -o -n "$(call search,lib*/*/libgdbm.a)" ]; then \
	 	echo GDBM=1 >>$(MCFG) ; \
	  fi \
	fi
	@if $(PKG_CONFIG) --exists pangocairo; then echo PANGO=1>>$(MCFG); fi
# check whether the new ndbmcompatibility library is present	
	@if [ -n "$(call search,lib*/libgdbm_compat.so)" -o -n "$(call search,lib*/*/libgdbm_compat.so)" -o -n "$(call search,lib*/libgdbm_compat.a)" -o -n "$(call search,lib*/*/libgdbm_compat.a)" ]; then \
	 	echo GDBM_COMPAT=1 >>$(MCFG) ; \
	fi
# record value of following configuration variables 
	echo MPI=$(MPI)>>$(MCFG)
	echo PARALLEL=$(PARALLEL)>>$(MCFG)
	echo OPENMP=$(OPENMP)>>$(MCFG)
	echo GCC=$(GCC)>>$(MCFG)
	echo NOGUI=$(NOGUI)>>$(MCFG)
	echo AQUA=$(AQUA)>>$(MCFG)

generate_nauty_sizes: generate_nauty_sizes.c
	$(CC) $(FLAGS) $(OPT) $< -o $@

src/getContext.o: src/getContext.cc
	g++ -ObjC++ -DMAC_OSX_TK -I/opt/local/include -Iinclude -c $< -o $@

include/nauty_sizes.h: generate_nauty_sizes
ifdef MXE
# MXE is a 32 bit environment
	echo "#define SIZEOF_INT 4" >$@
	echo "#define SIZEOF_LONG 4" >>$@
	echo "#define SIZEOF_LONG_LONG 8" >>$@
else
	rm -f $@
	./generate_nauty_sizes >$@
endif

sure: all 
	-$(MAKE) $(TESTS)
	-cd test; $(MAKE)
	-cd test/test_tcl_stl; $(MAKE) 
	-cd test/tcl-arrays; $(MAKE) 
	-cd test/complex_tcl_args; $(MAKE) 
	sh runtests test/00/*.sh

# install documentation on SourceForge
DOCPREFIX=web.sf.net:/home/project-web/ecolab/htdocs/doc
install-doc:
	doxygen
	-cd doc; sh Makedoc
	rsync -e ssh -r -z --progress --delete doc/ecolab $(DOCPREFIX)
	rsync -e ssh -r -z --progress --delete doxygen/ecolab $(DOCPREFIX)/doxygen

# exhaustively test different preprocessor options
compileTest:
	$(MAKE) clean; $(MAKE) DEBUG=1 GCC=1 MEMDEBUG=1 TIMECMDS=1 AEGIS=1
# note that icc currently cannot build the test directory
	$(MAKE) clean; $(MAKE) UNURAN= PANGO= AEGIS=1
	$(MAKE) clean; $(MAKE) UNURAN= GNUSL=  AEGIS=1
	$(MAKE) clean; env LD_LIBRARY_PATH=/usr/lib64/mpi/gcc/openmpi/lib64:$(LD_LIBRARY_PATH) $(MAKE) BLT= READLINE= MPI=1  AEGIS=1
	$(MAKE) clean; $(MAKE) TK= BDB= IGRAPH= PARALLEL=1 CAIRO= ZLIB= 
	$(MAKE) clean; $(MAKE) NOGUI= PARMETIS= SAUCY=  AEGIS=1
	$(MAKE) clean; $(MAKE) MXE=1 DEBUG=1 
	$(MAKE) clean; $(MAKE) BDB= GDBM= 
	$(MAKE) clean; $(MAKE) DYNAMIC=1 GCC=1
	$(MAKE) clean; $(MAKE) CPLUSPLUS=clang++
	$(MAKE) clean; $(MAKE) TIMER=1
	$(MAKE) clean; $(MAKE) CAIRO=1 TK=    #see ticket #139
	$(MAKE) clean; $(MAKE) ICC=1
# note, this currently fails
#	$(MAKE) clean; $(MAKE) DYNAMIC=1

dist:
	sh makeDist.sh
