#TARGET 		= testbench fsynthTP3 lmh6521
FSYNTHSRC	= adf4351_regs.hpp fsynth.cpp fsynth.hpp ../langford_ioctl.h
VGASRC		= lmh6521_regs.hpp lmh6521.cpp

all: langford_rf_fsynth langford_rx_rf_bb_vga

langford_rf_fsynth: $(FSYNTHSRC)
	@echo -e 'Compiling RF Synthesizer register programmer.'
	g++ -ansi -Wall $(FSYNTHSRC) -o langford_rf_fsynth

langford_rx_rf_bb_vga: $(VGASRC)
	@echo -e 'Compiling Baseband VGA register programmer.'
	g++ -ansi -Wall $(VGASRC) -o langford_rx_rf_bb_vga

install: langford_rf_fsynth langford_rx_rf_bb_vga
	@echo -e 'Installing binaries to /usr/bin.'
	@sudo cp langford_rx_rf_bb_vga langford_rf_fsynth /usr/bin
	@sudo chmod a+rx /usr/bin/langford_rx_rf_bb_vga /usr/bin/langford_rf_fsynth

uninstall:
	@echo -e 'Removing langford utility binaries from /usr/bin.'
	@-sudo rm -rf /usr/bin/langford_rx_rf_bb_vga /usr/bin/langford_rf_fsynth

clean:
	@echo -e 'Cleaning installation directories.'
	@-sudo rm -rf langford_rf_fsynth langford_rx_rf_bb_vga

