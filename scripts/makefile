install:
	apt-get install bison pkg-config gawk clang flex gcc-msp430 iverilog tcl-dev

demo :
	rm -f ../logs/*.cflog
	sed -i 's/#define IS_SIM  SIM/#define IS_SIM  NOT_SIM/g' ../tcb/wrapper.c 
	sh build/run "demo_prv" true

demo_sim:
	rm -f ../logs/*.cflog
	sed -i 's/#define IS_SIM  NOT_SIM/#define IS_SIM  SIM/g' ../tcb/wrapper.c 
	sh build/run "demo_prv" true

riot_scheduler:
	rm -f ../logs/*.cflog
	sh build/run "riot_scheduler" true
# 	cd ../demo_vrf && python3 vrf_offline.py

simple-app:
	rm -f ../logs/*.cflog
	sh build/run "simple-app" true

beebs:
	cp ../beebs/tests/${BEEBS}.c ../beebs/main.c
	sh build/run "beebs" true
	rm ../beebs/main.c

verify-install:
	cd verif-tools ; \
	tar -xvzf verilog2smv-1.1.3.tar.gz ; \
	cd verilog2smv-1.1.3 ; \
	sh build.sh ; \
	cp verilog2smv.sh ../

verify-run:
	sh verif-tools/run ${MODULE}

clean :
	rm -rf simv *.mem pmem.inc stimulus.v tmp-build
	
syringe_full:
	cp ./build/syringe-full.x ./build/linker.msp430.x
	cp ../riot_scheduler/Makefile.syringe-full ../riot_scheduler/makefile
	sh build/run "riot_scheduler" true	
	cp ../riot_scheduler/Makefile.test ../riot_scheduler/makefile
	cp ./build/default.x ./build/linker.msp430.x 