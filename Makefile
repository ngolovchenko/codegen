#
# Make all projects 
#

LIBS = cgihtml 
APPS = constdivmul delay

all:
	$(foreach module, $(LIBS) $(APPS), make -C $(module); )
clean:
	$(foreach module, $(LIBS) $(APPS), make clean -C $(module); )
depend:
	$(foreach module, $(LIBS) $(APPS), make depend -C $(module); )
test:
	$(foreach module, $(APPS), make test -C $(module); )
install:
	$(foreach module, $(APPS), make install -C $(module); )

.PHONY: all clean depend test install

