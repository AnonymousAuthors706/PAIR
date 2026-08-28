CC = msp430-gcc
CFLAGS = -D PFX_MSP430 -std=c99 -g -mcpu=430    -mmpy=none    -mivcnt=16
SRCDIR = ./src
INCLUDEDIR = ./include/
LIBDIR = ./lib/include
CPUDIR=../cpu/msp430
DEPFLAGS = -MP -MD
SRC = $(wildcard $(SRCDIR)/*.c)
OUT = sched_program


# Find all .h header files in the include directories
INCLUDES = -I$(INCLUDEDIR)  -I${LIBDIR} -I$(LIBDIR)/macros -I${CPUDIR} -I${CPUDIR}/include
$(info Found source files: $(INCLUDES))

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(OUT) $(SRC) $(DEPFLAGS)

clean:
	rm -f $(OUT)
