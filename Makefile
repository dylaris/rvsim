CC 	    = clang
CFLAGS  = -ggdb -Wall -Wextra -O3
LDFLAGS =

all: rvemu

rvemu: one.c rvemu.c mmu.c machine.c
	$(CC) $(CFLAGS) -o rvemu one.c $(LDFLAGS)

clean:
	rm -f rvemu

help:
	@echo "===================================================="
	@echo "Available Targets: (builds the executable defaultly)"
	@echo "===================================================="
	@echo "help:  		Display this information"
	@echo "clean: 		Clean object files and executable file"
	@echo "===================================================="

.PHONY: all clean help
