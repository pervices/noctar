clear;
clf();

decimation = 16;
fs = 200e6 / decimation;

function [data_fft] = ReadFileFFT(FileName)
    data = evstr(mgetl(FileName, -1));
//    plot(data);
    data_fft = fft(data);
//    data_fft = 20*log10(abs(data_fft));
    data_fft = abs(data_fft);
endfunction

data_fft1 = ReadFileFFT('out.data');
data_fft_freqs1 = 0:fs/(length(data_fft1)-1):fs;

plot(data_fft_freqs1, data_fft1, 'b');
