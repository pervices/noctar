#!/usr/bin/env python
##################################################
# Gnuradio Python Flow Graph
# Title: Langford Tx Dc
# Generated: Tue Oct  1 22:51:36 2013
##################################################

from gnuradio import analog
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import wxgui
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from gnuradio.wxgui import forms
from gnuradio.wxgui import scopesink2
from grc_gnuradio import wxgui as grc_wxgui
from optparse import OptionParser
import wx

class langford_tx_dc(grc_wxgui.top_block_gui):

    def __init__(self):
        grc_wxgui.top_block_gui.__init__(self, title="Langford Tx Dc")
        _icon_path = "/usr/share/icons/hicolor/32x32/apps/gnuradio-grc.png"
        self.SetIcon(wx.Icon(_icon_path, wx.BITMAP_TYPE_ANY))

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 250e6/8
        self.offset = offset = 0
        self.freq = freq = 7.5e6
        self.amp = amp = 15e3

        ##################################################
        # Blocks
        ##################################################
        _offset_sizer = wx.BoxSizer(wx.VERTICAL)
        self._offset_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_offset_sizer,
        	value=self.offset,
        	callback=self.set_offset,
        	label='offset',
        	converter=forms.int_converter(),
        	proportion=0,
        )
        self._offset_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_offset_sizer,
        	value=self.offset,
        	callback=self.set_offset,
        	minimum=0,
        	maximum=32765,
        	num_steps=100,
        	style=wx.SL_HORIZONTAL,
        	cast=int,
        	proportion=1,
        )
        self.Add(_offset_sizer)
        _freq_sizer = wx.BoxSizer(wx.VERTICAL)
        self._freq_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_freq_sizer,
        	value=self.freq,
        	callback=self.set_freq,
        	label='freq',
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._freq_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_freq_sizer,
        	value=self.freq,
        	callback=self.set_freq,
        	minimum=0,
        	maximum=samp_rate/2,
        	num_steps=100,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.Add(_freq_sizer)
        _amp_sizer = wx.BoxSizer(wx.VERTICAL)
        self._amp_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_amp_sizer,
        	value=self.amp,
        	callback=self.set_amp,
        	label='amp',
        	converter=forms.int_converter(),
        	proportion=0,
        )
        self._amp_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_amp_sizer,
        	value=self.amp,
        	callback=self.set_amp,
        	minimum=0,
        	maximum=33000,
        	num_steps=1000,
        	style=wx.SL_HORIZONTAL,
        	cast=int,
        	proportion=1,
        )
        self.Add(_amp_sizer)
        self.wxgui_scopesink2_0_0 = scopesink2.scope_sink_f(
        	self.GetWin(),
        	title="Scope Plot",
        	sample_rate=samp_rate,
        	v_scale=0,
        	v_offset=0,
        	t_scale=0,
        	ac_couple=False,
        	xy_mode=False,
        	num_inputs=1,
        	trig_mode=wxgui.TRIG_MODE_AUTO,
        	y_axis_label="Counts",
        )
        self.Add(self.wxgui_scopesink2_0_0.win)
        self.wxgui_scopesink2_0 = scopesink2.scope_sink_c(
        	self.GetWin(),
        	title="Scope Plot",
        	sample_rate=samp_rate,
        	v_scale=0,
        	v_offset=0,
        	t_scale=0,
        	ac_couple=False,
        	xy_mode=False,
        	num_inputs=1,
        	trig_mode=wxgui.TRIG_MODE_AUTO,
        	y_axis_label="Counts",
        )
        self.Add(self.wxgui_scopesink2_0.win)
        self.blocks_short_to_float_0 = blocks.short_to_float(1, 1)
        self.blocks_interleaved_short_to_complex_0 = blocks.interleaved_short_to_complex(False)
        self.blocks_interleave_0 = blocks.interleave(gr.sizeof_short*1)
        self.analog_sig_source_x_0 = analog.sig_source_s(samp_rate, analog.GR_SIN_WAVE, freq, amp, offset)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.blocks_interleaved_short_to_complex_0, 0), (self.wxgui_scopesink2_0, 0))
        self.connect((self.blocks_interleave_0, 0), (self.blocks_interleaved_short_to_complex_0, 0))
        self.connect((self.analog_sig_source_x_0, 0), (self.blocks_interleave_0, 0))
        self.connect((self.analog_sig_source_x_0, 0), (self.blocks_interleave_0, 1))
        self.connect((self.blocks_interleave_0, 0), (self.blocks_short_to_float_0, 0))
        self.connect((self.blocks_short_to_float_0, 0), (self.wxgui_scopesink2_0_0, 0))


# QT sink close method reimplementation

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.analog_sig_source_x_0.set_sampling_freq(self.samp_rate)
        self.wxgui_scopesink2_0.set_sample_rate(self.samp_rate)
        self.wxgui_scopesink2_0_0.set_sample_rate(self.samp_rate)

    def get_offset(self):
        return self.offset

    def set_offset(self, offset):
        self.offset = offset
        self._offset_slider.set_value(self.offset)
        self._offset_text_box.set_value(self.offset)
        self.analog_sig_source_x_0.set_offset(self.offset)

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        self._freq_slider.set_value(self.freq)
        self._freq_text_box.set_value(self.freq)
        self.analog_sig_source_x_0.set_frequency(self.freq)

    def get_amp(self):
        return self.amp

    def set_amp(self, amp):
        self.amp = amp
        self._amp_slider.set_value(self.amp)
        self._amp_text_box.set_value(self.amp)
        self.analog_sig_source_x_0.set_amplitude(self.amp)

if __name__ == '__main__':
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    (options, args) = parser.parse_args()
    tb = langford_tx_dc()
    tb.Start(True)
    tb.Wait()

