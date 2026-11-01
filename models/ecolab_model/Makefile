# root directory for ecolab include files and libraries
ECOLAB_HOME=../..

include $(ECOLAB_HOME)/include/Makefile

# GPU may not have double precision support
ifdef DPCPP
FLAGS+=-DUSE_FLOAT
endif
ifdef DPCPP_CUDA
FLAGS+=-DUSE_FLOAT
endif

#MODELS=ecolab shadow newman jellyfish webworld netcomplexity
MODELS=ecolab_model

all: $(MODELS:=.so)

# This rule uses a header file of object descriptors
$(MODELS:=.o): %.o: %.cc 

# how to build a model
$(MODELS:=.so): %.so: %.o $(ECOLAB_HOME)/lib/libecolab$(ECOLIBS_EXT).a
	$(LINK) $(FLAGS) -shared -Wl,-rpath $(ECOLAB_HOME)/lib $*.o $(LIBS) -o $@

ifneq ($(MAKECMDGOALS),clean)
include $(MODELS:=.d)
endif

clean:
	$(BASIC_CLEAN)
	rm -f $(MODELS)

