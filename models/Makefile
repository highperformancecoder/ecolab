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
FLAGS+=-DTHREE_D -UTK

include $(ECOLAB_HOME)/include/Makefile

ifdef DPCPP
# GPU may not have double precision support
FLAGS+=-DUSE_FLOAT
endif

#MODELS=ecolab shadow newman jellyfish webworld netcomplexity
MODELS=ecolab_model

#chmod command is to counteract AEGIS removing execute privelege from scripts
all: $(MODELS:=.so)
# Stupid_Project takes ages to build, so only do on regression runs.
#ifdef AEGIS
#	$(MAKE) -C Stupid_Project
#endif

ifeq ($(OS),Darwin)
all: $(MODELS:=.app)
endif

# This rule uses a header file of object descriptors
$(MODELS:=.o): %.o: %.cc 

# how to build a model
$(MODELS:=.so): %.so: %.o ../lib/libecolab$(ECOLIBS_EXT).a
	$(LINK) $(FLAGS) -shared -Wl,-rpath $(ECOLAB_HOME)/lib $*.o $(LIBS) -o $@

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

