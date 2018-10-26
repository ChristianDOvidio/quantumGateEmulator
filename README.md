# quantumQateEmulator
A web accessible, VHDL based quantum gate emulator that runs on the DE10 platform

The code MUST be run from the DE10-nano kit.

In order to use the emulation, the following source code must be available:
* Archived Quantum Gate Emulator Quartus project ("Quantum_Gate_Emulation.qar")
* Website source code (web server + html files)
* Compiled C executables (or .c source files + build script) – `write_input.c`, `write_gate.c`, `read_output.c`

# Dependencies:
* Python 3.5+
* Flask

# Steps:
1. Connect the DE10-nano to the internet by Ethernet cable
2. Restore the archived Quartus project and compile
3. Move the website source code and the C executables to the uSD card on the DE10-nano
4. To compile the C executables from the source (if they aren’t already compiled), use the “build_app.sh” script :
`<./build_app.sh source_file.c>`
5. Make sure the C executables are in a folder named “c” in the website directory. The “c” folder must be at the same directory level as the `quantum.py`, `static` and `templates` folders 
6. Make sure that the C executables, as well as the “c” directory, have appropriate permissions - 
`<chmod 777 c>` and `<chmod 777 c/*> 7`. 
7. Connect a serial terminal to the board - 
`<screen /dev/ttyUSB* 115200>`
8. Obtain the IP address of the board - 
`<ifconfig | grep inet>`
9. Program the DE10-nano with the output file of the compiled project using the Quartus II software. When the software prompts you to choose the device version, choose the first option, which is “5CSEBA6” 10. Run the web server with the following command - 
`python quantum.py`
11. Connect to the website in browser. The URL is `IP_ADDRESS:10000` (Note that this can only be accessed on the same network that the DE10-nano is connected to)
