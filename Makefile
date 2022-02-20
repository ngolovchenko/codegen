#
# Make all projects 
#

LIBS = cgihtml 
APPS = constdivmul delay keyword

all:
	$(foreach module, $(LIBS) $(APPS), make -C $(module); )
wasm:
	$(foreach module, $(LIBS) $(APPS), make -C $(module) wasm; )
clean:
	$(foreach module, $(LIBS) $(APPS), make clean -C $(module); )
depend:
	$(foreach module, $(LIBS) $(APPS), make depend -C $(module); )

.PHONY: all wasm clean depend

