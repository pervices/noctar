/***********************************************************************
 * The header extends adf4351_regs_t to work and initialize 
 * the Per Vices Phi transciever version TP3.
 * 
 * (c) 2013 Per Vices Corporation
 * See LICENSE.txt for license and copyright information.
 **********************************************************************/

#include "adf4351_regs.hpp"

class fsynth_t : public adf4351_regs_t {
public:
  //This is the initialization routine specific to revision TP3.
  //Pfd = 31.25MHz (as per header file), 156.25MHz reference, DIV=5.
  fsynth_t() {
	//Initialize to 555.000MHz
        frac_12_bit = 1;
        int_16_bit = 71;
        mod_12_bit = 26;
        phase_12_bit = 1;
        prescaler = PRESCALER_4_5;
	phase_adj = PHASE_ADJ_OFF;
        counter_reset = COUNTER_RESET_DISABLED;
        cp_three_state = CP_THREE_STATE_DISABLED;
        power_down = POWER_DOWN_DISABLED;
        pd_polarity = PD_POLARITY_POSITIVE;
        ldp = LDP_10NS;
        ldf = LDF_FRAC_N; //If in Int-N mode, set to LDF_INT_N
        charge_pump_current = CHARGE_PUMP_CURRENT_1_88MA;
        double_buffer = DOUBLE_BUFFER_DISABLED;
        r_counter_10_bit = 2;
        reference_divide_by_2 = REFERENCE_DIVIDE_BY_2_ENABLED;
        reference_doubler = REFERENCE_DOUBLER_DISABLED;
        muxout = MUXOUT_ANALOG_LD;
        low_noise_and_spur = LOW_NOISE_AND_SPUR_LOW_SPUR;
        clock_divider_12_bit = 75; // Nominal lock time for ADF4351 is 250us in normal mode (125us in fast lock mode)
				   // Setting clock divider per tsync > worst case locktime (275us), 
				   // tsync = 300 = Clk_div_value * MOD * t_pfd
        clock_div_mode = CLOCK_DIV_MODE_CLOCK_DIVIDER_OFF;
        cycle_slip_reduction = CYCLE_SLIP_REDUCTION_DISABLED;
	charge_cancel = CHARGE_CANCEL_DISABLED; //Can be used if in Int-N. Not recommended for Frac-N.
	antibacklash_pulse = ANTIBACKLASH_PULSE_6NS; //Can be 3NS (Reduced Spurs + Phase noise) if in Int-N. Not recommended for Frac-N.
	band_sel_clk_mode = BAND_SEL_CLK_MODE_LOW; //HIGH is suitable for high PFD frequencies + fast lock (only ADF4351) LOW has 50 compatability.
        output_power = OUTPUT_POWER_5DBM;
        rf_output_enable = RF_OUTPUT_ENABLE_ENABLED;
        aux_output_power = AUX_OUTPUT_POWER_M4DBM;
        aux_output_enable = AUX_OUTPUT_ENABLE_DISABLED;
        aux_output_select = AUX_OUTPUT_SELECT_DIVIDED;
        mute_till_lock_detect = MUTE_TILL_LOCK_DETECT_MUTE_DISABLED;
        vco_power_down = VCO_POWER_DOWN_VCO_POWERED_UP;
        band_select_clock_div = 250;
        rf_divider_select = RF_DIVIDER_SELECT_DIV4;
        feedback_select = FEEDBACK_SELECT_FUNDAMENTAL; //DIVIDED for higher output power.
        ld_pin_mode = LD_PIN_MODE_DLD;
    // Nothing.
  };
};
