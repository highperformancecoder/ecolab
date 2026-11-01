#MODELS=ecolab shadow newman jellyfish webworld netcomplexity
# averarching Makefile supporting building and clean target
MODELS=ecolab_model

# Declare all directory and clean targets as .PHONY to force execution
.PHONY: all clean $(MODELS) $(MODELS:=-clean)

all: $(MODELS:=)

$(MODELS:=):
	$(MAKE) -C $@

$(MODELS:=-clean):
	$(MAKE) -C $(@:-clean=) clean

clean: $(MODELS:=-clean)


