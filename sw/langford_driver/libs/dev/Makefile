#TARGET 		= testbench fsynthTP3 lmh6521
TESTSRC		= testbench.cpp adf4351_regs.hpp
FSYNTHTP3HPSRC	= adf4351_regs.hpp fsynthTP3_HP.cpp fsynthTP3_HP.hpp ../../langford_ioctl.h
FSYNTHADF4351SRC	= adf4351_regs.hpp fsynthADF4351.cpp fsynthADF4351.hpp ../../langford_ioctl.h

all: testbench fsynthTP3_HP fsynthADF4351

testbench: $(TESTSRC)
	g++ -ansi -Wall $(TESTSRC) -o testbench

fsynthTP3_HP: $(FSYNTHTP3HPSRC)
	g++ -ansi -Wall $(FSYNTHTP3HPSRC) -o fsynthTP3_HP

fsynthADF4351: $(FSYNTHADF4351SRC)
	g++ -ansi -Wall $(FSYNTHADF4351SRC) -o fsynthADF4351

clean:
	rm -rf testbench fsynthADF4351 fsynthTP3_HP 

