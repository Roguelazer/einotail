SOURCES := $(wildcard *.c *.h)
TARGETS := einotail
PUPPET_TARGETS := einotail-32 einotail-64

CFLAGS += -Wall -ggdb -Wextra -pedantic -std=c99 

all: $(TARGETS)

.PHONY : force
force: clean all

.PHONY : clean 
clean:
	rm -f $(TARGETS) $(PUPPET_TARGETS) $(TEST_TARGETS) *.o
	rm -rf build

einotail-32: $(SOURCES)
	$(CC) -o $@ $(CFLAGS) -m32 $(filter %.c,$^)

einotail-64: $(SOURCES)
	$(CC) -o $@ $(CFLAGS) -m64 $(filter %.c,$^)

dep:
	sh ./automake.sh

#### AUTO ####
einotail.o: einotail.c
#### END AUTO ####
