# root directory for ecolab include files and libraries
ifeq ($(shell ls ../../include/ecolab.h),../../include/ecolab.h)
ECOLAB_HOME=../..
else
ifeq ($(shell ls $(HOME)/usr/ecolab/include/ecolab.h),$(HOME)/usr/ecolab/include/ecolab.h)
ECOLAB_HOME=$(HOME)/usr/ecolab
else
ECOLAB_HOME=/usr/local/ecolab
endif
endif

include $(ECOLAB_HOME)/include/Makefile

MODELS=stupid01 stupid02 stupid03 stupid04 stupid05 stupid06 stupid09 stupid10 stupid11 stupid12 stupid14 stupid15 stupid16 stupid-field

ifdef MPI
MODELS+=stupid-parallel
endif

#chmod command is to counteract AEGIS removing execute privelege from scripts
all: $(MODELS)
	-$(CHMOD) a+x *.tcl

ifeq ($(OS),Darwin)
all: $(MODELS:=.app)
endif

# This rule uses a header file of object descriptors
$(MODELS:=.o): %.o: %.cc 

# how to build a model executable
$(MODELS): %: %.o $(ECOLIBS)
	$(LINK) $(FLAGS) $*.o $(MODLINK) $(LIBS) -o $@

#make MacOS application bundles
$(MODELS:=.app): %.app: %
	$(ECOLAB_HOME)/utils/mkmacapp $<

include $(MODELS:=.d)

clean:
	$(BASIC_CLEAN)
	rm -f $(MODELS) $(MODELS:%=%_classdesc.h) *.cd
	rm -rf classdesc-lib classdesc-lib.cc classdesc.h classdesc.a
	rm -rf *.app *.log
	rm -f lakes/*.bin

