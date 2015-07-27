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

# compile 3 D jellyfish version
FLAGS+=-DTHREE_D

include $(ECOLAB_HOME)/include/Makefile

MODELS=ecolab shadow newman jellyfish webworld netcomplexity

#chmod command is to counteract AEGIS removing execute privelege from scripts
all: $(MODELS)
	-$(CHMOD) a+x *.tcl

ifeq ($(OS),Darwin)
all: $(MODELS:=.app)
endif

# This rule uses a header file of object descriptors
$(MODELS:=.o): %.o: %.cc 
#	$(CPLUSPLUS) -c $(FLAGS)  $<

# how to build a model executable
$(MODELS): %: %.o $(ECOLIBS)
	$(LINK) $(FLAGS) $*.o $(MODLINK) $(LIBS) -o $@

#make MacOS application bundles
$(MODELS:=.app): %.app: %
	$(ECOLAB_HOME)/utils/mkmacapp $<

ifneq ($(MAKECMDGOALS),clean)
include $(MODELS:=.d)
endif

clean:
	$(BASIC_CLEAN)
	rm -f $(MODELS) *.cd
	rm -rf *.app
	rm -f lakes/*.bin

