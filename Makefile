
CFLAGS	= -O2 -g0 -Wall -pedantic

HEADERS = 
SOURCES = main.cc

#CFLAGS	+= $(patsubst %, -I%, $(INCLUDE))
CFLAGS	+= $(patsubst %, -include %, $(HEADERS))

all: fincore

fincore: $(patsubst %,_obj/%.o, $(SOURCES))
	$(CXX) -o $@ $^ -lc

_obj/%.cc.o : source/%.cc
	$(CXX) -std=c++17 -c ${CFLAGS} -o $@ $<

clean:
	@rm -f _obj/*

retab:
	@find . -type f -regex '.*\.\(cc\|h\)' | \
		while IFS="" read i; do \
			expand -t4 "$$i" > "$$i-"; \
			mv "$$i-" "$$i"; \
		done


.PHONY: clean

