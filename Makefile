CC 	     = clang
CFLAGS   = -ggdb -Wall -Wextra -O3 -Iinc/
CLDFLAGS = -lm

EXE_CFLAGS  = $(CFLAGS)
EXE_LDFLAGS = $(CLDFLAGS)

LIB_CFLAGS  = $(CFLAGS) -fPIC
LIB_LDFLAGS = $(CLDFLAGS) -shared -Wl,--version-script=src/export.sym

all: rvsim

test: CFLAGS += -DTEST_TVM
test: rvsim

rvsim: src/one.c
	$(CC) $(EXE_CFLAGS) -o rvsim src/one.c $(EXE_LDFLAGS)

lib: src/api.c
	$(CC) $(LIB_CFLAGS) -o librvsim.so src/api.c $(LIB_LDFLAGS)

clean:
	rm -f rvsim librvsim.so

help:
	@echo "===================================================="
	@echo "Available Targets: (builds the exec and lib defaultly)"
	@echo "===================================================="
	@echo "help:  		Display this information"
	@echo "clean: 		Clean object files and executable file"
	@echo "lib: 		Generate shared library"
	@echo "test: 		Build with TEST_TVM"
	@echo "===================================================="

.PHONY: all clean help lib
