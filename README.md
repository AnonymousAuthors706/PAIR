### Requirements / Recommended setup

1- Xilinx Vivado (version 2021.1 or higher)

2- Python 3.8.10 or higher

3- We evaluated PAIR prototype on 64-bit Ubuntu 20.04 OS WSL2

### Setup

1- Clone this Repository

2- `cd` into `scripts` and run `sudo make install`

3- Install Xilinx Vivado: https://www.xilinx.com/support/download.html

4- Verify required python package: `SymPy`. 

### Create a Vivado Project for PAIR

1- Start Vivado. On the upper left select: File -> New Project

2- Follow the wizard, select a project name and location. In project type, select RTL Project and click Next.

3- In the "Add Sources" window, select Add Files and add all .v and .mem files contained in the following directories of this reposiroty:

        /acfi_hw
        /msp_bin
        /openmsp430/fpga
        /openmsp430/msp_core
        /openmsp430/msp_memory
        /openmsp430/msp_periph
       
and select Next.

Note that /msp_bin contains the pmem.mem and smem.mem binaries, generated in step [Building PAIR Software].

4- In the "Add Constraints" window, select add files and add the file

        openmsp430/contraints_fpga/Basys-3-Master.xdc

and select Next.

        Note: this file needs to be modified accordingly if you are running PAIR in a different FPGA.

5- In the "Default Part" window select "Boards", search for Basys3, select it, and click Next.

        Note: if you don't see Basys3 as an option you may need to download Basys3 to your Vivado installation.

6- Select "Finish". This will conclude the creation of a Vivado Project for PAIR.

Now we need to configure the project for systhesis.

7- In the PROJECT MANAGER "Sources" window, search for openMSP430_fpga (openMSP430_fpga.v) file, right click it and select "Set as Top".
This will make openMSP430_fpga.v the top module in the project hierarchy. Now its name should appear in bold letters.

8- In the same "Sources" window, search for openMSP430_defines.v file, right click it and select Set File Type and, from the dropdown menu select "Verilog Header".

9- After adding `*.v` and `*.mem` files to the project, open a terminal window and `cd` into `scripts`.

10- Run `make riot_scheduler APP=[TEST_SUBPATH_IN_RIOT_DIR]` to compile software for the basic test. This will update the `*.mem` files. For example, to compile cover from beebs in the current directory, use `make riot_scheduler APP=./beeb-tests/cover`. 

### Basic Test

1- Now we are ready to synthesize openmsp430 with PAIR hardware. On the left menu of the PROJECT MANAGER, click "Run Synthesis", and select execution parameters (e.g., number of CPUs used for synthesis) according to your PC's capabilities. This step takes 2-10 minutes.

2- If synthesis succeeds, a window to "Run Implementation" will appear. Do not "Run Implementation" for the basic test, and close this prompt window.

3- In Vivado, click "Add Sources" (Alt-A), then select "Add or create simulation sources", click "Add Files", and select everything inside `openmsp430/simulation`.

4- Now, navigate to the "Sources" window in Vivado. Search for `tb_openMSP430_fpga`, and in the "Simulation Sources" tab, right-click `tb_openMSP430_fpga.v` and set its file type as the top module.

5- Go back to the Vivado window, and in the "Flow Navigator" tab (on the left-most part of Vivado's window), click "Run Simulation," then "Run Behavioral Simulation."

6- On the newly opened simulation window, select 8ms as the time for the simulation to run. Then press "Shift+F2" to run.

7- Run until the program finishes (e.g., PC pauses). 

### Running SymPy

1- cd into `rta` folder

2- Run `python3 sympy_proof.py

3- Printed to the console will be the simplification to obtain the degradation constraint 

### Running syringe example

1- Follow same steps from `Basic test` except use `make riot_scheduler syringe_full` in step 10

### Verification

1- run `make verify-install`

2- run `make verify-run MODULE=availability_module`
