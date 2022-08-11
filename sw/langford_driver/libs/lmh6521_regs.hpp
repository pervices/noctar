/***********************************************************************
 * Defines and sets the LMH6521 High Performance Dual Differential DVGA
 * 
 * (c) 2013 Per Vices Corporation
 * See LICENSE.txt for license and copyright information.
 **********************************************************************/

#ifndef INCLUDED_LMH6521_REGS_HPP
#define INCLUDED_LMH6521_REGS_HPP

/* Specifications */
#define LMH6521_MAX_GAIN		31.5 /* dB */
#define LMH6521_MIN_GAIN		31.5 /* dB */
#define LMH6521_STEP_SIZE		0.5  /* dB */

#include <stdint.h>
#include <math.h>
#include <set>
#include <iostream>
#include <bitset>


class lmh6521_regs_t{
public:
    enum read_write_bit_t{
        READ_WRITE_BIT_WRITE = 0,
        READ_WRITE_BIT_READ = 1
    };
    read_write_bit_t read_write_bit;
    
    enum address_t{
        CHANNEL_A = 0,
        CHANNEL_B = 1
    };
    address_t address;

    enum enable_t{
        ENABLE_DISABLED = 0,
        ENABLE_ENABLED = 1
    };
    enable_t enable;
    
    int gain; //Gain setting in 0.5dB increments.
   
  //Output registers A and B to stdout.
    void show_regs(){
	std::cout << "\tRegister " << "=>\t" << std::hex <<  get_regs() << std::endl;
    };
    
    //Turn off channel
    void turn_off(int u_channel){
      address = (address_t)(u_channel);
      enable = ENABLE_DISABLED;
    };
    
    //Turn on channel
    void turn_on(int u_channel){
      address = u_channel == 0 ? CHANNEL_A : CHANNEL_B;
      enable = ENABLE_DISABLED;
    };
      
    //INITIALIZATION ROUTINE (Initialize in Safe State)
    //POWER OFF, LOWEST POWER SETTING.
    lmh6521_regs_t(){
        read_write_bit = READ_WRITE_BIT_READ;
	address = CHANNEL_A;
	enable = ENABLE_DISABLED;
	gain = 0;
    }

    //Generate the registers
    uint32_t get_regs(){
        int32_t reg = 0;
	//Start writing to the registers;
	reg |= (uint32_t(gain) & 0x3f) << 1;
	reg |= (uint32_t(enable) & 0x1) << 7;
	reg |= (uint32_t(address) & 0x1) << 8;
	reg |= (uint32_t(read_write_bit) & 0x1) << 15;
	return reg;
    };
  
    //Set the gain (from 0-31.5) on specified channel (0 or 1)
    double set_gain(double u_gain, int u_channel) {
        read_write_bit = READ_WRITE_BIT_WRITE;
	address = u_channel == 0 ? CHANNEL_A : CHANNEL_B;
	enable = ENABLE_ENABLED;
	gain = u_gain <= 0 ? 0 : int(u_gain); //Sanity check for min gain.
	gain = u_gain >= 31.5 ? 63 : int(u_gain*2) ; //Sanity check for max gain.
	return ((double) gain/2);
	};
	
    double read_channel(double u_channel) {
        read_write_bit = READ_WRITE_BIT_READ;
	address = u_channel == 0 ? CHANNEL_A : CHANNEL_B;
	//enable = ENABLE_ENABLED;
	//gain = u_gain <= 0 ? 0 : int(u_gain); //Sanity check for min gain.
	//gain = u_gain >= 31.5 ? 63 : int(u_gain*2) ; //Sanity check for max gain.
	return ((double) u_channel);
	};

};





#endif /* INCLUDED_ADF4350_REGS_HPP */
