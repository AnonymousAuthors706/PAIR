# makfile configuration
NAME            = tcb
OBJECTS         = tcb.o riot.o ../tcb.swatt hacl_mac.lib

# Choose GCC toolchain prefix ('msp430' for MSPGCC / 'msp430-elf' for GCC RedHat/TI)
MSPGCC_PFX      = msp430

# Select flags depending on selected toolchain
CFLAGS          = -D PFX_MSP430  -std=c99 -w -g -mcpu=430    -mmpy=none    -mivcnt=16 -DMODULE_CORE_THREAD_FLAGS -O0

# Toolchain executables
CC              = ${MSPGCC_PFX}-gcc
OBJCOPY		    = ${MSPGCC_PFX}-objcopy
OBJDUMP         = ${MSPGCC_PFX}-objdump
OBJSIZE         = ${MSPGCC_PFX}-size

SRCDIR          = ./core/src
INCLUDEDIR      = ./core/include/
LIBDIR          = ./core/lib/include
DEPFLAGS        = -MP -MD
SRC             = $(wildcard $(SRCDIR)/*.c)
INCLUDES        = -I$(INCLUDEDIR) -I${SRCDIR} -I$(LIBDIR) -I$(LIBDIR)/macros


SRC2 = $(wildcard $(SRCDIR)/*.c) $(wildcard $(INCLUDEDIR)/*.c)
OBJ = $(SRC:.c=.o)

.PHONY: all FORCE clean download download-jtag download-bsl dist

SRC_DIRS = ./core/lib ./core/src 

#all should be the first target. it's built when make is runwithout args
#all: srom_libc.a ${NAME}.o ${NAME}.elf ${NAME}.a43 ${NAME}.lst ${NAME}.size
all: hacl_mac.lib riot.o ${NAME}.o ${NAME}.elf ${NAME}.a43 ${NAME}.lst ${NAME}.size 

#additional rules for files
${NAME}.elf: ${OBJECTS}
	${CC} ${CFLAGS} -T pmem.def -o $@ ${OBJECTS}

${NAME}.a43: ${NAME}.elf
	${OBJCOPY} -O ihex $^ $@

${NAME}.lst: ${NAME}.elf
	${OBJDUMP} -dSt $^ >$@
	${OBJDUMP} -d $^ >bare.lst

${NAME}.size: ${NAME}.elf
	${OBJSIZE}  $^ >$@

${NAME}.o: ${APP}.c 
	cp ${APP}.c main.c
	${CC} ${CFLAGS} ${INCLUDES} -c -o $@ main.c

riot.o: 
	@set -e; \
	mkdir -p tmp_objs; \
	for file in $(shell find $(SRC_DIRS) -type f -name "*.c"); do \
	  obj=tmp_objs/$$(basename $$file .c).o; \
	  echo $(CC)" "$(CFLAGS)" "${INCLUDES}" -c "$$file" -o "$$obj; \
	  $(CC) $(CFLAGS) ${INCLUDES} -c $$file -o $$obj; \
	  $(OBJCOPY) --rename-section .text=.riot_body $$obj $$obj; \
	done; \
	ar rcs riot.o tmp_objs/*.o; \
	rm -rf tmp_objs

hacl_mac.lib: 
	cd hacl-c ; \
	${CC} ${CFLAGS} -c -o hacl.lib.o Hacl_HMAC_SHA2_256.c ; \
	ar crs ../$@ hacl.lib.o ; \
	rm hacl.lib.o 

clean:
	rm -f ${NAME} ${NAME}.a43 ${NAME}.lst ${NAME}.elf ${NAME}.size *.o pmem.def 

#project dependencies

