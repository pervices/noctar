#include <iostream>
#include "adf4351_regs.hpp"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <bitset>
#include <string>

using namespace std;


int main(void) {
	int		Pass, Fail;

	Pass = Fail = 0;

	std::cout << "Library Testbench\n" << std::endl;

	{
		std::string	strBin;
		// Starting with something simple
		std::cout << "Testing Integer to binary conversion" << std::endl;
		strBin = bitset<8>((int) 123).to_string();
		if (strBin == "01111011") {
			std::cout << "Pass";
			Pass++;
		} else {
			std::cout << "FAIL";
			Fail++;
		};
		std::cout << "\t123 => " << strBin << std::endl;
		
		// Initialization ADF4351 Registers in something safe...
		std::cout << "\nTesting Default Safe State Initialization of ADF435{0,1} Registers." << std::endl;

		//Known Safe Values
		std::string adf4351_saferegs[6] = {
		  "00000000000010111000000000000000", //Register 0
		  "00000000000000001000000000010001", //Register 1
		  "00000000000000000100000001110010", //Register 2
		  "00000000000000000000000000000011", //Register 3
		  "00000000000000000001100000000100", //Register 4
		  "00000000010110000000000000000101" //Register 5
		  };
		
		//Initialize registers
		adf4351_regs_t adf4351_regs;
		for (int i=0;i<=5;i++){
		  int regcontents = (int) adf4351_regs.get_reg(i);
		  strBin = bitset<32>(regcontents).to_string();
		  //std::cout << i << flush;
		  if (strBin == adf4351_saferegs[i]) {
			std::cout << "Pass";
			Pass++;
		  } else {
			std::cout << "FAIL";
			Fail++;
		  };
		  std::cout << "\tRegister " << i << "=>\t" << strBin << " == " << hex << regcontents << std::endl;
		};
		
		//Debug outputs;
		//adf4351_regs.show_regs();

	}

	std::cout << Pass << " tests passed, " << Fail << " tests failed." << std::endl;

	return Fail;
};
