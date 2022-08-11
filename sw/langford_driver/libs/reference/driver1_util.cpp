#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include "driver1_ioctl.h"

#include <boost/assign/list_of.hpp>
#include "dict.hpp"
#include "adf4350_regs.hpp"
#include "ranges.hpp"

const float LO_Freq = 156.25e6;
const float Samp_Freq = 200e6;

using namespace std;
using namespace boost::assign;
using namespace uhd;

void WriteLOSerialReg(int &DevFD, int &rc, unsigned int Content) {
	int		i;
	const unsigned long long	val0 = 0, val1 = 1;
	unsigned long long		val;

	rc |= ioctl(DevFD, IOCTL_SET_LO_LE, &val0);
	rc |= ioctl(DevFD, IOCTL_SET_LO_CE, &val1);
	for (i = 0; i < 32; i++) {
		rc |= ioctl(DevFD, IOCTL_SET_LO_CLK, &val0);
		val = (Content >> (31 - i)) & 0x00000001;
		rc |= ioctl(DevFD, IOCTL_SET_LO_DATA, &val);
		rc |= ioctl(DevFD, IOCTL_SET_LO_CLK, &val1);
	};
	rc |= ioctl(DevFD, IOCTL_SET_LO_CLK, &val0);
	rc |= ioctl(DevFD, IOCTL_SET_LO_LE, &val1);
	rc |= ioctl(DevFD, IOCTL_SET_LO_LE, &val0);
};


double SetLOFreq(int &DevFD, int rc, double TargetFreq) {
    cout << boost::format(
        "Local oscillator: target frequency %f Mhz"
    ) % (TargetFreq/1e6) << std::endl;

    //map prescaler setting to mininmum integer divider (N) values (pg.18 prescaler)
    static const uhd::dict<int, int> prescaler_to_min_int_div = map_list_of
        (0,23) //adf4350_regs_t::PRESCALER_4_5
        (1,75) //adf4350_regs_t::PRESCALER_8_9
    ;

    //map rf divider select output dividers to enums
    static const uhd::dict<int, adf4350_regs_t::rf_divider_select_t> rfdivsel_to_enum = map_list_of
        (1,  adf4350_regs_t::RF_DIVIDER_SELECT_DIV1)
        (2,  adf4350_regs_t::RF_DIVIDER_SELECT_DIV2)
        (4,  adf4350_regs_t::RF_DIVIDER_SELECT_DIV4)
        (8,  adf4350_regs_t::RF_DIVIDER_SELECT_DIV8)
        (16, adf4350_regs_t::RF_DIVIDER_SELECT_DIV16)
	(32, adf4350_regs_t::RF_DIVIDER_SELECT_DIV32)
	(64, adf4350_regs_t::RF_DIVIDER_SELECT_DIV64)
    ;

    double actual_freq, pfd_freq;
    //**************************************************************************
    //Set reference oscillator frequency here
    //**************************************************************************
    double ref_freq = LO_Freq;//this->get_iface()->get_clock_rate(unit);
    int R=0, BS=0, N=0, FRAC=0, MOD=0;
    int RFdiv = 1;
    adf4350_regs_t::reference_divide_by_2_t T     = adf4350_regs_t::REFERENCE_DIVIDE_BY_2_DISABLED;
    adf4350_regs_t::reference_doubler_t     D     = adf4350_regs_t::REFERENCE_DOUBLER_DISABLED;

    //Reference doubler for 50% duty cycle
    // if ref_freq < 12.5MHz enable regs.reference_divide_by_2
    if(ref_freq <= 12.5e6) D = adf4350_regs_t::REFERENCE_DOUBLER_ENABLED;

    //increase RF divider until acceptable VCO frequency
    //start with TargetFreq*2 because mixer has divide by 2
    double vco_freq = TargetFreq;
    while (vco_freq < 2.2e9) {
        vco_freq *= 2;
        RFdiv *= 2;
    }

    //use 8/9 prescaler for vco_freq > 3 GHz (pg.18 prescaler)
    adf4350_regs_t::prescaler_t prescaler = vco_freq > 3e9 ? adf4350_regs_t::PRESCALER_8_9 : adf4350_regs_t::PRESCALER_4_5;

    /*
     * The goal here is to loop though possible R dividers,
     * band select clock dividers, N (int) dividers, and FRAC
     * (frac) dividers.
     *
     * Calculate the N and F dividers for each set of values.
     * The loop exists when it meets all of the constraints.
     * The resulting loop values are loaded into the registers.
     *
     * from pg.21
     *
     * f_pfd = f_ref*(1+D)/(R*(1+T))
     * f_vco = (N + (FRAC/MOD))*f_pfd
     *    N = f_vco/f_pfd - FRAC/MOD = f_vco*((R*(T+1))/(f_ref*(1+D))) - FRAC/MOD
     * f_rf = f_vco/RFdiv)
     * f_actual = f_rf/2
     */
    for(R = 1; R <= 1023; R+=1){
        //PFD input frequency = f_ref/R ... ignoring Reference doubler/divide-by-2 (D & T)
        pfd_freq = ref_freq*(1+D)/(R*(1+T));

        //keep the PFD frequency at or below 25MHz (Loop Filter Bandwidth)
        if (pfd_freq > 31.25e6) continue;

        //ignore fractional part of tuning
        N = int(std::floor(vco_freq/pfd_freq));

        //keep N > minimum int divider requirement
        if (N < prescaler_to_min_int_div[prescaler]) continue;

        for(BS=1; BS <= 255; BS+=1){
            //keep the band select frequency at or below 125KHz
            //constraint on band select clock
            if (pfd_freq/BS > 125e3) continue;
            goto done_loop;
        }
    } done_loop:

    //Fractional-N calculation
    MOD = 4095; //max fractional accuracy
    FRAC = int((vco_freq/pfd_freq - N)*MOD);

    //Reference divide-by-2 for 50% duty cycle
    // if R even, move one divide by 2 to to regs.reference_divide_by_2
    if(R % 2 == 0){
        T = adf4350_regs_t::REFERENCE_DIVIDE_BY_2_ENABLED;
        R /= 2;
    }

    //actual frequency calculation
    actual_freq = double((N + (double(FRAC)/double(MOD)))*ref_freq*(1+int(D))/(R*(1+int(T)))/RFdiv);


    cout
        << boost::format("Local oscillator: ref=%0.2f, outdiv=%f, fbdiv=%f") % (ref_freq*(1+int(D))/(R*(1+int(T)))) % double(RFdiv*2) % double(N + double(FRAC)/double(MOD)) << std::endl

        << boost::format("Local oscillator: R=%d, BS=%d, N=%d, FRAC=%d, MOD=%d, T=%d, D=%d, RFdiv=%d"
            ) % R % BS % N % FRAC % MOD % T % D % RFdiv << std::endl
        << boost::format("Local oscillator (MHz): REQ=%0.2f, ACT=%0.2f, VCO=%0.2f, PFD=%0.2f, BAND=%0.2f"
            ) % (TargetFreq/1e6) % (actual_freq/1e6) % (vco_freq/1e6) % (pfd_freq/1e6) % (pfd_freq/BS/1e6) << std::endl;

    //load the register values
    adf4350_regs_t regs;

    regs.frac_12_bit = FRAC;
    regs.int_16_bit = N;
    regs.mod_12_bit = MOD;
    regs.prescaler = prescaler;
    regs.r_counter_10_bit = R;
    regs.reference_divide_by_2 = T;
    regs.reference_doubler = D;
    regs.band_select_clock_div = BS;
    UHD_ASSERT_THROW(rfdivsel_to_enum.has_key(RFdiv));
    regs.rf_divider_select = rfdivsel_to_enum[RFdiv];
    regs.aux_output_power = adf4350_regs_t::AUX_OUTPUT_POWER_M45DBM;
    regs.aux_output_enable = adf4350_regs_t::AUX_OUTPUT_ENABLE_DISABLED;
    regs.aux_output_select = adf4350_regs_t::AUX_OUTPUT_SELECT_DIVIDED;

//    if (unit == dboard_iface::UNIT_RX) {
        freq_range_t rx_lo_5dbm = list_of
            (range_t(0.05e9, 1.4e9))
        ;

        freq_range_t rx_lo_2dbm = list_of
            (range_t(1.4e9, 2.2e9))
        ;

        if (actual_freq == rx_lo_5dbm.clip(actual_freq)) regs.output_power = adf4350_regs_t::OUTPUT_POWER_5DBM;

        if (actual_freq == rx_lo_2dbm.clip(actual_freq)) regs.output_power = adf4350_regs_t::OUTPUT_POWER_2DBM;

/*    } else if (unit == dboard_iface::UNIT_TX) {
        freq_range_t tx_lo_5dbm = list_of
            (range_t(0.05e9, 1.7e9))
            (range_t(1.9e9, 2.2e9))
        ;

        freq_range_t tx_lo_m1dbm = list_of
            (range_t(1.7e9, 1.9e9))
        ;

        if (actual_freq == tx_lo_5dbm.clip(actual_freq)) regs.output_power = adf4350_regs_t::OUTPUT_POWER_5DBM;

        if (actual_freq == tx_lo_m1dbm.clip(actual_freq)) regs.output_power = adf4350_regs_t::OUTPUT_POWER_M1DBM;

    }
*/
    //write the registers
    //correct power-up sequence to write registers (5, 4, 3, 2, 1, 0)
    int addr;

    for(addr=5; addr>=0; addr--){
        cout << boost::format(
            "Local oscillator: (0x%02x): 0x%08x"
        ) % addr % regs.get_reg(addr) << std::endl;
/*
        this->get_iface()->write_spi(
            unit, spi_config_t::EDGE_RISE,
            regs.get_reg(addr), 32
        );
*/
		WriteLOSerialReg(DevFD, rc, regs.get_reg(addr));
    }

    //return the actual frequency
    cout << boost::format(
        "Local oscillator: actual frequency %f Mhz"
    ) % (actual_freq/1e6) << std::endl;
    return actual_freq;
};


int main(int argc, char **argv) {
	string	cmd_str = "";
	unsigned long long	val, ulltmp;
	int		DevFD;
	int		rc = 0;
	unsigned int tmp;

	if (argc != 4) {
		cerr << argv[0] << " device_name -x val" << endl;
		return 1;
	};

	DevFD = open(argv[1], O_RDWR);
	if (!DevFD) {
		cerr << "Cannot open " << argv[1] << endl;
		return 1;
	};

	cmd_str = argv[2];
	val = strtoul(argv[3], 0, 0);
	cout << "Got command: " << cmd_str << endl;
	cout << "Got value: " << val << " = 0x" << std::hex << val << std::dec << endl;

	if (cmd_str == "-g") {
		//Gain
		rc |= ioctl(DevFD, IOCTL_GET_RFDDCEN, &ulltmp);
		if (ulltmp) {
			cout << "RF stage enabled, setting RF gain" << endl;
			rc |= ioctl(DevFD, IOCTL_SET_AT0, &val);
			rc |= ioctl(DevFD, IOCTL_SET_AT1, &val);
			rc |= ioctl(DevFD, IOCTL_SET_AT2, &val);
			rc |= ioctl(DevFD, IOCTL_SET_AT3, &val);
		} else {
			cout << "RF stage disabled, setting DC gain" << endl;
			rc |= ioctl(DevFD, IOCTL_SET_DCGAIN, &val);
		};
	} else if (cmd_str == "-c") {
		//DC HILO
		rc |= ioctl(DevFD, IOCTL_SET_DCHILO, &val);
	} else if (cmd_str == "-m") {
		//Enable DC stage (val = 0 is base band, val = 1 is RF)
		//Disable gain stages appropriately
		if (val) {
			//RF stage
			val = 0;
			rc |= ioctl(DevFD, IOCTL_SET_DCGAIN, &val);
			rc |= ioctl(DevFD, IOCTL_SET_DCEN, &val);
			rc |= ioctl(DevFD, IOCTL_SET_DCHILO, &val);
			val = 1;
		} else {
			//Direct sampling
			rc |= ioctl(DevFD, IOCTL_SET_AT0, &val);
			rc |= ioctl(DevFD, IOCTL_SET_AT1, &val);
			rc |= ioctl(DevFD, IOCTL_SET_AT2, &val);
			rc |= ioctl(DevFD, IOCTL_SET_AT3, &val);
		};
		//The following pins are set to 1 when RF IQ sampling is enabled
		rc |= ioctl(DevFD, IOCTL_SET_RFDDCEN, &val);
		//The following pins are set to 1 when base band direct sampling is enabled
		val = !val;
		rc |= ioctl(DevFD, IOCTL_SET_BBSB, &val);
		rc |= ioctl(DevFD, IOCTL_SET_RFSB, &val);
		rc |= ioctl(DevFD, IOCTL_SET_DCEN, &val);
	} else if (cmd_str == "-d") {
		//Decimation
		if (val <= 1) {
			cerr << "Decimation factor must be larger than 1" << endl;
			return 1;
		} else if (val <= 2) {
			tmp = 0;
		} else if (val <= 4) {
			tmp = 1;
		} else if (val <= 8) {
			tmp = 3;
		} else if (val <= 16) {
			tmp = 7;
		} else if (val <= 32) {
			tmp = 15;
		} else if (val <= 64) {
			tmp = 31;
		} else if (val <= 128) {
			tmp = 63;
		};
		cout << "Effective sampling rate after decimation is " << Samp_Freq / (tmp + 1) / 2e6 << "Msps" << endl;
		val = tmp;
		rc |= ioctl(DevFD, IOCTL_SET_DECEN, &val);
	} else if (cmd_str == "-f") {
		rc |= ioctl(DevFD, IOCTL_GET_RFDDCEN, &ulltmp);
		if (ulltmp) {
			//RF DDC enabled, set RF LO freq
			cout << "RF stage enabled, setting RF LO frequency first" << endl;
			if (val < LO_Freq * 5.5) {
				//Cannot get LO frequency low enough, we will need to use the DDC more
				if (LO_Freq * 5.5 - val > Samp_Freq / 4) {
					//Difference in frequency is too large: DDC cannot make up for the difference
					cout << "Frequency is too low with RF stage enabled. Lowest frequency possible with RF stage enabled is " << LO_Freq * 5.5 - Samp_Freq / 4 << "Hz" << endl;
				}
				SetLOFreq(DevFD, rc, LO_Freq * 5.5);
				val = LO_Freq * 5.5 - val;
			} else {
				//Set the LO frequency as close as possible
				val = round(val - SetLOFreq(DevFD, rc, val));
			};
		};
		cout << "Setting DDC frequency to " << val << endl;
		tmp = ((float) val) * (((int64_t) 1) << 32) / (Samp_Freq / 2);
		cout << "Phase inc: " << tmp << " = 0x" << std::hex << tmp << std::dec << endl;
		cout << "Actual frequency: " << (Samp_Freq / 2 * tmp / (((int64_t) 1) << 32)) << endl;
		val = tmp;
		rc |= ioctl(DevFD, IOCTL_SET_PHASEINC, &val);
		val = (ulltmp ? 0 : 1);
		rc |= ioctl(DevFD, IOCTL_GET_REVDDCF, &val);
	} else if (cmd_str == "-l")  {
		//Write raw LO registers
		WriteLOSerialReg(DevFD, rc, val);
	} else if (cmd_str == "-s")  {
		//Print status
		rc |= ioctl(DevFD, IOCTL_GET_LO_MUXOUT, &val);
		cout << "LO mux out = " << val << endl;
		rc |= ioctl(DevFD, IOCTL_GET_LO_LD, &val);
		cout << "LO PLL lock detect = " << val << endl;
	} else {
		cerr << "Unknown command: " << cmd_str << endl;
		return 1;
	};

	close(DevFD);

	cout << "Return code from value change: " << rc << endl;

	return 0;
};
