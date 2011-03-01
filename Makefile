SOURCES := $(wildcard *.c *.h)
TARGETS := ytail
PUPPET_TARGETS := ytail-32 ytail-64

CFLAGS += -Wall -ggdb -Wextra -pedantic -std=c99 

all: $(TARGETS)

.PHONY : force
force: clean all

.PHONY : clean 
clean:
	rm -f $(TARGETS) $(PUPPET_TARGETS) $(TEST_TARGETS) *.o
	rm -rf build

ytail-32: $(SOURCES)
	$(CC) -o $@ $(CFLAGS) -m32 $(filter %.c,$^)

ytail-64: $(SOURCES)
	$(CC) -o $@ $(CFLAGS) -m64 $(filter %.c,$^)

dep:
	sh ./automake.sh

#### AUTO ####
ytail.o: ytail.c
#### END AUTO ####
