# set this variable to force use of gcc
CC=gcc
CPLUSPLUS=g++
# -I. should _not_ be necessary, but seems to be necessary on OpenSUSE
FLAGS=-g -I.

ifdef GCOV
FLAGS+=-fprofile-arcs -ftest-coverage
endif

ifdef AEGIS
FLAGS+=-Werror -Wall -Wno-unused-variable
endif

XDR=1

PREFIX=${HOME}/usr

OS=$(shell uname)

ifdef AEGIS  #keep a cached copy of sysincludes in home directory for AEGIS
SYSINCLUDES=${HOME}/usr/ecolab/sysincludes
else
SYSINCLUDES=sysincludes
endif

# Utility programs
EXES=classdesc insert-friend
# wildcard runs before functiondb.h is created on some systems
INCLUDES=$(wildcard *.h) functiondb.h unpack_base.h json_unpack_base.h

# canonicalise CYGWIN's OS name
ifeq ($(findstring CYGWIN,$(OS)),CYGWIN)
OS=CYGWIN
XDR=
# CYGWIN has a buggy fsetpos routine
FLAGS+=-DUSE_FSEEK
CPLUSPLUS=g++ -Wl,--enable-auto-import
endif

ifeq ($(findstring MINGW,$(OS)),MINGW)
OS=MINGW
XDR=
endif

FLAGS+=-I/usr/X11R6/include

ifdef XDR
FLAGS+=-DXDR_PACK -DHAVE_LONGLONG
endif

# Some compilers allow friend functions to refer to type without
# explicit template  arguments.
ifndef GCC
ifeq ($(OS),OSF1)
CC=cc
CPLUSPLUS=cxx  -D__USE_STD_IOSTREAM
FLAGS+=-DEXPLICIT_FRIEND_TEMPLATE_ARGS_REQUIRED=0
endif
ifeq ($(OS),IRIX64)
CC=cc
CPLUSPLUS=CC -LANG:std
FLAGS+=-DEXPLICIT_FRIEND_TEMPLATE_ARGS_REQUIRED=0
endif
endif

.SUFFIXES: .cc .c .o .h .d 

.cc.o: 
	$(CPLUSPLUS) -c $(FLAGS)  $<

.c.o: 
	$(CC) -c $(FLAGS) $<

ifdef AEGIS
all: aegis-all
else
all: build
endif

ifdef XDR
XDRPACK=xdr_pack.o
else
XDRPACK=
endif

build: $(EXES) $(XDRPACK) functiondb.h unpack_base.h json_unpack_base.h

unpack_base.h:
	-ln -s pack_base.h unpack_base.h

json_unpack_base.h:
	-ln -s json_pack_base.h json_unpack_base.h


aegis-all: build latex-docs
	echo $(LD_LIBRARY_PATH)
	-ln -s pack_base.h unpack_base.h
	if which mpicxx; then	cd mpi-examples && $(MAKE) CPLUSPLUS=mpicxx CC=mpicc; fi
	cd examples && $(MAKE) NOGUI=1
#	if which javac; then cd java && $(MAKE) && cd ../javaExamples && $(MAKE); fi
	cd test && $(MAKE)
	cd test/c++11 && $(MAKE)
#	-cd objc-examples && $(MAKE)

$(EXES): %: %.cc tokeninput.h
	$(CPLUSPLUS) $(FLAGS) $< -o $@

xdr_pack.o: xdr_pack.cc
	$(CPLUSPLUS) $(FLAGS) -DTR1 -c $<

functiondb.h: functiondb.sh
	-rm $@
	bash $< >$@

clean: 
	rm -f *.o  *~ "\#*\#" core *.exh *.exc *.d *,D *.exe
	rm -rf $(EXES) include-paths cxx_repository
	cd mpi-examples && $(MAKE) clean
	cd examples && $(MAKE) clean
	cd objc-examples && $(MAKE) clean
	cd java && $(MAKE) clean
	cd javaExamples && $(MAKE) clean
	cd test && $(MAKE) clean
	cd doc && rm -f *~ *.aux *.dvi *.log *.blg *.toc *.lof
	rm -rf ii_files */ii_files

# test compile Latex docs, if latex is on system
latex-docs:
	if which latex; then cd doc; rm -f *.out *.aux *.dvi *.log *.blg *.toc *.lof; latex -interaction=batchmode classdesc; fi

install: build
	-mkdir $(PREFIX)
	-mkdir $(PREFIX)/bin
	-mkdir $(PREFIX)/include
ifeq ($(OS),CYGWIN)
	cp -f $(EXES:%=%.exe)  $(PREFIX)/bin
else
	cp -f $(EXES)  $(PREFIX)/bin
endif
	cp -f fix-privates $(PREFIX)/bin
	cp -f $(INCLUDES) $(PREFIX)/include
#	-ln -s $(PREFIX)/include/pack_base.h $(PREFIX)/include/unpack_base.h
#	cp -r $(SYSINCLUDES) $(PREFIX)

sure: aegis-all
	-ln -s pack_base.h unpack_base.h
	-cd mpi-examples && $(MAKE) clean && $(MAKE) NOGUI=1
	-cd examples && $(MAKE) clean && $(MAKE) NOGUI=1
#	-cd objc-examples && $(MAKE)
	sh runtests "$(CPLUSPLUS)" test/00/*.sh

c++11-sure: clean 
	$(MAKE) CPLUSPLUS="g++ --std=c++11" classdesc
# cannot override CPLUSPLUS with mpi-examples 
	if which mpicxx; then	cd mpi-examples && $(MAKE); fi
	$(MAKE) CPLUSPLUS="g++ --std=c++11" sure



