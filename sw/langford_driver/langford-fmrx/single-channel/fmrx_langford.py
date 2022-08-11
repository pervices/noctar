#!/usr/bin/env python
##################################################
# Gnuradio Python Flow Graph
# Title: Langford FM Reciever
# Author: Victor Wollesen
# Description: Langford FM Reciever
# Generated: Tue Aug  6 15:55:51 2013
##################################################

from gnuradio import analog
from gnuradio import audio
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import filter
from gnuradio import gr
from gnuradio import wxgui
from gnuradio.eng_option import eng_option
from gnuradio.fft import window
from gnuradio.filter import firdes
from gnuradio.wxgui import forms
from gnuradio.wxgui import waterfallsink2
from grc_gnuradio import wxgui as grc_wxgui
from optparse import OptionParser
import wx

class fmrx_langford(grc_wxgui.top_block_gui):

    def __init__(self, audio_sample_rate=48000, source_decim=16):
        grc_wxgui.top_block_gui.__init__(self, title="Langford FM Reciever")
        _icon_path = "/usr/share/icons/hicolor/32x32/apps/gnuradio-grc.png"
        self.SetIcon(wx.Icon(_icon_path, wx.BITMAP_TYPE_ANY))

        ##################################################
        # Parameters
        ##################################################
        self.audio_sample_rate = audio_sample_rate
        self.source_decim = source_decim

        ##################################################
        # Variables
        ##################################################
        self.adc_rate = adc_rate = 250000000
        self.source_rate = source_rate = adc_rate/source_decim
        self.chan_width = chan_width = 200e3
        self.filt_decim_A1 = filt_decim_A1 = int(round(source_rate/chan_width))
        self.chan_sr_in_demod_A1 = chan_sr_in_demod_A1 = int(round(adc_rate/(source_decim*filt_decim_A1)))
        self.aud_decim_A1 = aud_decim_A1 = int(round(chan_sr_in_demod_A1/audio_sample_rate))
        self.freq = freq = 97.5e6
        self.chan_sr_out_A1 = chan_sr_out_A1 = int(round(chan_sr_in_demod_A1/aud_decim_A1))
        self.chan_1 = chan_1 = 99.9e6
        self.vol_1 = vol_1 = 0
        self.variable_static_text_1 = variable_static_text_1 = chan_sr_out_A1
        self.demod_sr_C1 = demod_sr_C1 = chan_sr_in_demod_A1
        self.chan_xlate_A = chan_xlate_A = freq-chan_1

        ##################################################
        # Blocks
        ##################################################
        _vol_1_sizer = wx.BoxSizer(wx.VERTICAL)
        self._vol_1_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_vol_1_sizer,
        	value=self.vol_1,
        	callback=self.set_vol_1,
        	label="Channel 1 Volume",
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._vol_1_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_vol_1_sizer,
        	value=self.vol_1,
        	callback=self.set_vol_1,
        	minimum=0,
        	maximum=21,
        	num_steps=21,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.GridAdd(_vol_1_sizer, 3, 1, 1, 1)
        self._freq_static_text = forms.static_text(
        	parent=self.GetWin(),
        	value=self.freq,
        	callback=self.set_freq,
        	label="WB Centre Freq",
        	converter=forms.float_converter(),
        )
        self.GridAdd(self._freq_static_text, 0, 0, 1, 1)
        self.wxgui_waterfallsink2_0 = waterfallsink2.waterfall_sink_c(
        	self.GetWin(),
        	baseband_freq=freq,
        	dynamic_range=50,
        	ref_level=60,
        	ref_scale=2.0,
        	sample_rate=int(round(adc_rate/source_decim)),
        	fft_size=512,
        	fft_rate=8,
        	average=False,
        	avg_alpha=None,
        	title="WideBand Spectrum (FM Band)",
        )
        self.GridAdd(self.wxgui_waterfallsink2_0.win, 1, 0, 4, 1)
        self._variable_static_text_1_static_text = forms.static_text(
        	parent=self.GetWin(),
        	value=self.variable_static_text_1,
        	callback=self.set_variable_static_text_1,
        	label="Aud SR (1)",
        	converter=forms.int_converter(),
        )
        self.Add(self._variable_static_text_1_static_text)
        self.freq_xlating_fir_filter_xxx_0 = filter.freq_xlating_fir_filter_ccc(filt_decim_A1, (firdes.low_pass (1,int(round(adc_rate/source_decim)),75e3,25e3)), chan_xlate_A, int(round(adc_rate/source_decim)))
        self._demod_sr_C1_static_text = forms.static_text(
        	parent=self.GetWin(),
        	value=self.demod_sr_C1,
        	callback=self.set_demod_sr_C1,
        	label="Demod SR (1)",
        	converter=forms.int_converter(),
        )
        self.Add(self._demod_sr_C1_static_text)
        _chan_width_sizer = wx.BoxSizer(wx.VERTICAL)
        self._chan_width_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_chan_width_sizer,
        	value=self.chan_width,
        	callback=self.set_chan_width,
        	label="Channel Width",
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._chan_width_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_chan_width_sizer,
        	value=self.chan_width,
        	callback=self.set_chan_width,
        	minimum=50e3,
        	maximum=1e6,
        	num_steps=100,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.GridAdd(_chan_width_sizer, 1, 1, 1, 1)
        _chan_1_sizer = wx.BoxSizer(wx.VERTICAL)
        self._chan_1_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_chan_1_sizer,
        	value=self.chan_1,
        	callback=self.set_chan_1,
        	label="Channel 1",
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._chan_1_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_chan_1_sizer,
        	value=self.chan_1,
        	callback=self.set_chan_1,
        	minimum=87.9e6,
        	maximum=107.9e6,
        	num_steps=100,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.GridAdd(_chan_1_sizer, 2, 1, 1, 1)
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_vff((vol_1, ))
        self.blocks_interleaved_short_to_complex_0 = blocks.interleaved_short_to_complex(False)
        self.blocks_file_source_0 = blocks.file_source(gr.sizeof_short*1, "/dev/langford", False)
        self.audio_sink = audio.sink(audio_sample_rate, "plughw:0,0", True)
        self.analog_fm_demod_cf_0 = analog.fm_demod_cf(
        	channel_rate=chan_sr_in_demod_A1,
        	audio_decim=aud_decim_A1,
        	deviation=75000,
        	audio_pass=15000,
        	audio_stop=16000,
        	gain=1.0,
        	tau=75e-6,
        )

        ##################################################
        # Connections
        ##################################################
        self.connect((self.blocks_interleaved_short_to_complex_0, 0), (self.wxgui_waterfallsink2_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.audio_sink, 0))
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.audio_sink, 1))
        self.connect((self.blocks_interleaved_short_to_complex_0, 0), (self.freq_xlating_fir_filter_xxx_0, 0))
        self.connect((self.blocks_file_source_0, 0), (self.blocks_interleaved_short_to_complex_0, 0))
        self.connect((self.freq_xlating_fir_filter_xxx_0, 0), (self.analog_fm_demod_cf_0, 0))
        self.connect((self.analog_fm_demod_cf_0, 0), (self.blocks_multiply_const_vxx_0, 0))


# QT sink close method reimplementation

    def get_audio_sample_rate(self):
        return self.audio_sample_rate

    def set_audio_sample_rate(self, audio_sample_rate):
        self.audio_sample_rate = audio_sample_rate
        self.set_aud_decim_A1(int(round(self.chan_sr_in_demod_A1/self.audio_sample_rate)))

    def get_source_decim(self):
        return self.source_decim

    def set_source_decim(self, source_decim):
        self.source_decim = source_decim
        self.set_chan_sr_in_demod_A1(int(round(self.adc_rate/(self.source_decim*self.filt_decim_A1))))
        self.set_source_rate(self.adc_rate/self.source_decim)
        self.wxgui_waterfallsink2_0.set_sample_rate(int(round(self.adc_rate/self.source_decim)))
        self.freq_xlating_fir_filter_xxx_0.set_taps((firdes.low_pass (1,int(round(self.adc_rate/self.source_decim)),75e3,25e3)))

    def get_adc_rate(self):
        return self.adc_rate

    def set_adc_rate(self, adc_rate):
        self.adc_rate = adc_rate
        self.set_chan_sr_in_demod_A1(int(round(self.adc_rate/(self.source_decim*self.filt_decim_A1))))
        self.set_source_rate(self.adc_rate/self.source_decim)
        self.wxgui_waterfallsink2_0.set_sample_rate(int(round(self.adc_rate/self.source_decim)))
        self.freq_xlating_fir_filter_xxx_0.set_taps((firdes.low_pass (1,int(round(self.adc_rate/self.source_decim)),75e3,25e3)))

    def get_source_rate(self):
        return self.source_rate

    def set_source_rate(self, source_rate):
        self.source_rate = source_rate
        self.set_filt_decim_A1(int(round(self.source_rate/self.chan_width)))

    def get_chan_width(self):
        return self.chan_width

    def set_chan_width(self, chan_width):
        self.chan_width = chan_width
        self.set_filt_decim_A1(int(round(self.source_rate/self.chan_width)))
        self._chan_width_slider.set_value(self.chan_width)
        self._chan_width_text_box.set_value(self.chan_width)

    def get_filt_decim_A1(self):
        return self.filt_decim_A1

    def set_filt_decim_A1(self, filt_decim_A1):
        self.filt_decim_A1 = filt_decim_A1
        self.set_chan_sr_in_demod_A1(int(round(self.adc_rate/(self.source_decim*self.filt_decim_A1))))

    def get_chan_sr_in_demod_A1(self):
        return self.chan_sr_in_demod_A1

    def set_chan_sr_in_demod_A1(self, chan_sr_in_demod_A1):
        self.chan_sr_in_demod_A1 = chan_sr_in_demod_A1
        self.set_aud_decim_A1(int(round(self.chan_sr_in_demod_A1/self.audio_sample_rate)))
        self.set_chan_sr_out_A1(int(round(self.chan_sr_in_demod_A1/self.aud_decim_A1)))
        self.set_demod_sr_C1(self.chan_sr_in_demod_A1)

    def get_aud_decim_A1(self):
        return self.aud_decim_A1

    def set_aud_decim_A1(self, aud_decim_A1):
        self.aud_decim_A1 = aud_decim_A1
        self.set_chan_sr_out_A1(int(round(self.chan_sr_in_demod_A1/self.aud_decim_A1)))

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        self.set_chan_xlate_A(self.freq-self.chan_1)
        self._freq_static_text.set_value(self.freq)
        self.wxgui_waterfallsink2_0.set_baseband_freq(self.freq)

    def get_chan_sr_out_A1(self):
        return self.chan_sr_out_A1

    def set_chan_sr_out_A1(self, chan_sr_out_A1):
        self.chan_sr_out_A1 = chan_sr_out_A1
        self.set_variable_static_text_1(self.chan_sr_out_A1)

    def get_chan_1(self):
        return self.chan_1

    def set_chan_1(self, chan_1):
        self.chan_1 = chan_1
        self.set_chan_xlate_A(self.freq-self.chan_1)
        self._chan_1_slider.set_value(self.chan_1)
        self._chan_1_text_box.set_value(self.chan_1)

    def get_vol_1(self):
        return self.vol_1

    def set_vol_1(self, vol_1):
        self.vol_1 = vol_1
        self._vol_1_slider.set_value(self.vol_1)
        self._vol_1_text_box.set_value(self.vol_1)
        self.blocks_multiply_const_vxx_0.set_k((self.vol_1, ))

    def get_variable_static_text_1(self):
        return self.variable_static_text_1

    def set_variable_static_text_1(self, variable_static_text_1):
        self.variable_static_text_1 = variable_static_text_1
        self._variable_static_text_1_static_text.set_value(self.variable_static_text_1)

    def get_demod_sr_C1(self):
        return self.demod_sr_C1

    def set_demod_sr_C1(self, demod_sr_C1):
        self.demod_sr_C1 = demod_sr_C1
        self._demod_sr_C1_static_text.set_value(self.demod_sr_C1)

    def get_chan_xlate_A(self):
        return self.chan_xlate_A

    def set_chan_xlate_A(self, chan_xlate_A):
        self.chan_xlate_A = chan_xlate_A
        self.freq_xlating_fir_filter_xxx_0.set_center_freq(self.chan_xlate_A)

if __name__ == '__main__':
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    parser.add_option("", "--audio-sample-rate", dest="audio_sample_rate", type="intx", default=48000,
        help="Set Audio Sample Rate [default=%default]")
    parser.add_option("-d", "--source-decim", dest="source_decim", type="intx", default=16,
        help="Set source decimation [default=%default]")
    (options, args) = parser.parse_args()
    tb = fmrx_langford(audio_sample_rate=options.audio_sample_rate, source_decim=options.source_decim)
    tb.Start(True)
    tb.Wait()

