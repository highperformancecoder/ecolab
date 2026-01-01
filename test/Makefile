# root directory for ecolab include files and libraries
ifeq ($(shell ls ../include/ecolab.h),../include/ecolab.h)
ECOLAB_HOME=$(shell pwd)/..
else
ifeq ($(shell ls $(HOME)/usr/ecolab/include/ecolab.h),$(HOME)/usr/ecolab/include/ecolab.h)
ECOLAB_HOME=$(HOME)/usr/ecolab
else
ECOLAB_HOME=/usr/local/ecolab
endif
endif

include $(ECOLAB_HOME)/include/Makefile
# enable assertions
FLAGS+=-g -UNDEBUG

ifdef GDBM_COMPAT
FLAGS+=-DUSE_DBM
GDBM=-lgdbm_compat
endif
GDBM+=-lgdbm

#MODELS=testrefassign testref testgraph test_boostgraph testCacheDBM testCairo accessor vector-pair
MODELS=testParallel testCacheDBM 
# vector-pair - doesn't currently compile - see ticket #161
#EXTRAMODELS=test_tcl_stl.exe tcl-arrays.exe complex_tcl_args.exe
#EXES=test_omp_rw_lock testCheckpointableFile test_netcomplexity test_boostgraph test_ecostrstream posTest testarrays pangoTest testParallel
EXES=test_omp_rw_lock testCheckpointableFile test_netcomplexity test_ecostrstream posTest testarrays pangoTest 
SPECIALS=testNDBM testBDB

LIBS+=-lboost_system

ifeq ($(CC),gcc)
OMPOPT=-fopenmp
endif
ifeq ($(CC),icc)
OMPOPT=-openmp
endif

ifdef AEGIS
CXXFLAGS+=-std=c++11
endif

#chmod command is to counteract AEGIS removing execute privelege from scripts
all: $(MODELS:=.so) $(EXES) $(SPECIALS)
	-$(CHMOD) a+x *.tcl

ifeq ($(OS),Darwin)
all: $(MODELS:=.app)
endif

# This rule uses a header file of object descriptors
$(MODELS:=.o): %.o: %.cc 
	$(CPLUSPLUS) -c -g $(FLAGS) $(CXXFLAGS) $<

# how to build a model 
$(MODELS:=.so): %.so: %.o
	$(LINK) $(FLAGS) -shared -Wl,-rpath $(ECOLAB_HOME)/lib $*.o $(LIBS) -o $@

# temporary workaround until directories of same name go
#$(EXTRAMODELS): %.exe: %.o
#	$(LINK) $(FLAGS) $*.o $(MODLINK) $(LIBS) -o $@

#make MacOS application bundles
$(MODELS:=.app): %.app: %
	$(ECOLAB_HOME)/utils/mkmacapp $<

ifneq ($(MAKECMDGOALS),clean)
include $(MODELS:=.d) $(EXTRAMODELS:%.exe=%.d) $(EXES:=.d) 
endif

test_omp_rw_lock: test_omp_rw_lock.cc
	-$(CPLUSPLUS) $(OMPOPT) $(FLAGS) $(CXXFLAGS) -o $@ $<

testCheckpointableFile: testCheckpointableFile.o
	$(LINK)  $(FLAGS) -o $@  $< $(LIBS)

test_netcomplexity: test_netcomplexity.o
	$(LINK)  $(FLAGS) -o $@  $< $(LIBS)

test_ecostrstream: test_ecostrstream.o
	$(LINK)  $(FLAGS) -o $@  $< $(LIBS)

callTclArgsMethod: callTclArgsMethod.o
	$(LINK)  $(FLAGS) -o $@  $< $(LIBS)

posTest: posTest.o
	$(LINK)  $(FLAGS) -o $@  $< $(LIBS)

pangoTest: pangoTest.o
	$(LINK)  $(FLAGS) -o $@  $< $(LIBS)

testNDBM: cachedDBM.o
	$(CPLUSPLUS) $(FLAGS) $(CXXFLAGS) -UBDB -DUSE_DBM $< ../src/cachedDBM.cc $(LIBS) $(GDBM) -o $@

testBDB: cachedDBM.o
	$(CPLUSPLUS) $(FLAGS) $(CXXFLAGS) -DBDB $< ../src/cachedDBM.cc $(LIBS) -ldb -o $@

#Test Utilities
testarrays: testarrays.o 
	$(LINK) $(FLAGS) $^ $(LIBS) -lm  -o $@

clean:
	$(BASIC_CLEAN) $(EXES) $(SPECIALS) $(MODELS) *.cd

