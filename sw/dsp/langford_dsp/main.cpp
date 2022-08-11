#include "main.h"


int main(int argc, char **argv) {
	int	SampleRate;		//Sample rate of ADC
//	const int	DiscardSamples = 5e6;	//Number of samples to discard at the begining of a file read
//	const int	NumSamples = 20e6;		//Number of samples to read
//	const int	NumSamples = 20e3;		//Number of samples to read
	int		NumSamples;					//Number of samples to read
	int		CurrNumSamples;				//Number of samples after decimation
	FILE	*pInFile, *pOutFile;
	float	CentreFreq;
	int		DeltaPhi;
	int		ReqDecimation, Decimation;
	const int	LUTSize = 4096;
	int	*CosLUT, *SinLUT;
	int16_t	*Sample;
	int		*I, *Q, *Ip1, *Qp1, *E;
	int		i;
	int    Verbosity;
	int	   DiscardSamples;	//Number of samples to discard at the begining of a file read

    char *ProgramName = argv[0];

    //Check for correct arguments
	if ( (argc < 6) or (argc > 9) ) {
		DispHelp(ProgramName);
		return -EINVAL;
	};

    char *FileIn = argv[1];
    char *FileOut = argv[2];
	NumSamples = atof(argv[3]);
	CentreFreq = atof(argv[4]);
	ReqDecimation = atoi(argv[5]);

	//If 6 arguments, samplerate is 250MSPS, no discard, no verbosity
    if (argc = 6) {
		SampleRate = 250e6;
		DiscardSamples = 0;
		Verbosity = 0;
	}	//If 7 args, set sample rate, no discard, no verbosity
	else if (argc = 7) {
        SampleRate = atoi(argv[6]);
        DiscardSamples = 0;
		Verbosity = 0;
	} 	//If 8 args, set sample rate, discards, no verbosity
	else if (argc = 8) {
        SampleRate = atoi(argv[6]);
        DiscardSamples = atoi(argv[7]);
		Verbosity = 0;
	}
    else if (argc = 9) {
        SampleRate = atoi(argv[6]);
        DiscardSamples = atoi(argv[7]);
		Verbosity = atoi(argv[8]);
	}
	else    {
        cerr << ProgramName << ": Invalid Arguments.";
        return -EINVAL;
    };

    //Set the delta phase difference
    DeltaPhi = LUTSize * CentreFreq / SampleRate;

    //Determine Decimation
    Decimation = 1;
	ReqDecimation >>= 1;
	while (ReqDecimation) {
		Decimation <<= 1;
		ReqDecimation >>= 1;
	};

	//Set up
	CosLUT = new int[LUTSize];
	SinLUT = new int[LUTSize];
	if (!CosLUT || !SinLUT) {
		cerr << ProgramName << ": " << " Out of memory!" << endl;
		return -ENOMEM;
	};

	BuildTrigLUTs(CosLUT, SinLUT, LUTSize);

	//Read in data
	if (!(pInFile = fopen(FileIn, "rb"))) {
		cerr << ProgramName << ": " << " Cannot open " << FileIn << " for reading!" << endl;
		return -ENOENT;
	};
	if (!(Sample = new int16_t[(DiscardSamples > NumSamples ? DiscardSamples : NumSamples)])) {
		cerr << ProgramName << ": " << "Out of memory!" << endl;
		return -ENOMEM;
	};

    if (Verbosity) {
        cout << ProgramName << ": " << "Reading " << ((float)NumSamples) / 1e6 << " Msamples or " << ((float)NumSamples) / SampleRate * 1e3 << "ms of data" << endl;
        cout << ProgramName << ": " << "Sample Rate is " << SampleRate / 1e6 << "Msps" << endl;
        cout << ProgramName << ": " << "Nyquist frequency is " << SampleRate / 2e6 << "MHz" << endl;
        cout << ProgramName << ": " << "Discarding first " << DiscardSamples / 1e6 << "Msamples, processing the next " << NumSamples / 1e6 << "Msamples" << endl;
        cout << ProgramName << ": " << "Centre frequency for down conversion is " << CentreFreq / 1e6 << "MHz"<< endl;
        cout << ProgramName << ": " << "Phase increment is " << DeltaPhi << " out of " << LUTSize << " samples" << endl;
        cout << ProgramName << ": " << "Actual centre frequency for down conversion is " << ((float)DeltaPhi) * SampleRate / LUTSize / 1e6 << "MHz"<< endl;
        cout << ProgramName << ": " << "Requested decimation factor is " << ReqDecimation << endl;
        cout << ProgramName << ": " << "Actual decimation factor is " << Decimation << endl;
        cout << ProgramName << ": " << "Effective sampling rate is " << SampleRate / Decimation / 1e6 << "Msps" << endl;
        cout << ProgramName << ": " << "Output file will contain around " << NumSamples / Decimation / 1e6 << "Ms" << endl;
        cout << ProgramName << ": " << "Bandwidth is +/-" << SampleRate / Decimation / 2e6 << "MHz" << endl;
        cout << ProgramName << ": " << "Frequency range is " << (CentreFreq - SampleRate / Decimation / 2) / 1e6 << "MHz to " << (CentreFreq + SampleRate / Decimation / 2) / 1e6 << "MHz" << endl;
        cout << ProgramName << ": " << "Reading input file " << FileIn << "..." << endl;
    };

	fread(Sample, sizeof(int16_t), DiscardSamples, pInFile);
	fread(Sample, sizeof(int16_t), NumSamples, pInFile);
    fclose(pInFile);

	//Process data
	I = new int[NumSamples];
	Q = new int[NumSamples];
	Ip1 = new int[NumSamples];
	Qp1 = new int[NumSamples];
	E = new int[NumSamples];
	if (!I || !Q || !Ip1 || !Qp1 || !E) {
		cerr << ProgramName << ": " << "Out of memory!" << endl;
		return -ENOMEM;
	};

	//Down convert

	if (Verbosity) { cout << ProgramName << ": " << "Down converting..." << endl; };

	DownConvert(Sample, I, Q, NumSamples, CosLUT, SinLUT, DeltaPhi, LUTSize);

	//Perform filtering and decimation
	CurrNumSamples = NumSamples;
	Decimation >>= 1;
	while (Decimation) {
		int *tmp;

		if (Verbosity) { cout << ProgramName << ": " << "Applying half band filter and decimating by 2..." << endl; };
		DecimateBy2(I, Ip1, CurrNumSamples);
		DecimateBy2(Q, Qp1, CurrNumSamples);

		//This only shuffles allocated memory addresses around, it does not copy any values
		//Doing this saves us the trouble of memcpy, malloc and free every decimation cycle
		tmp = I;
		I = Ip1;
		Ip1 = tmp;

		tmp = Q;
		Q = Qp1;
		Qp1 = tmp;

		CurrNumSamples /= 2;
		Decimation >>= 1;
	};
/*
	//Compute power
	cout << ProgramName << ": " << "Computing power..." << endl;
	ComputePower(I, Q, E, CurrNumSamples);
*/
	//Write data back out
	if (Verbosity) { cout << ProgramName << ": " << "Writing results to " << FileOut << " ..." << endl; };

	if (!(pOutFile = fopen(FileOut, "wb"))) {
		cerr << ProgramName << ": " << "Cannot open " << FileOut << " for writing!" << endl;
		return -ENOENT;
	};

	for (i = 0; i < CurrNumSamples; i++) {
		float	fI, fQ;
//		fprintf(pOutFile, "%d\n", Sample[i]);
//		fprintf(pOutFile, "%d%+di\n", I[i], Q[i]);
//		fprintf(pOutFile, "%d\n", E[i]);
		fI = (float) I[i];
		fQ = (float) Q[i];
//      fQ = Q[i];
		fwrite(&fI, sizeof(float), 1, pOutFile);
		fwrite(&fQ, sizeof(float), 1, pOutFile);
	};
	fclose(pOutFile);

	//Clean up
	delete [] CosLUT;
	delete [] SinLUT;
	delete [] Sample;
	delete [] I;
	delete [] Q;
	delete [] Ip1;
	delete [] Qp1;
	delete [] E;

	return 0;
};


void BuildTrigLUTs(int *CosLUT, int *SinLUT, const int LUTSize) {
	int		i;

	for (i = 0; i < LUTSize; i++) {
		CosLUT[i] = (int16_t) 2048 * cos(2 * M_PI * i / LUTSize + M_PI / 2);
		SinLUT[i] = (int16_t) 2048 * sin(2 * M_PI * i / LUTSize + M_PI / 2);
	};
};


void ComputePower(int *I, int *Q, int *E, const int NumSamples) {
	int		i;

	for (i = 0; i < NumSamples; i++) {
		E[i] = sqrt(I[i] * I[i] + Q[i] + Q[i]);
	};
};


void DecimateBy2(int *OldSamples, int *NewSamples, const int OldNumSamples) {
	int		i;

	/* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
	   Command line: /www/usr/fisher/helpers/mkfilter -Ch -1.0000000000e+00 -Lp -o 10 -a 2.5000000000e-01 0.0000000000e+00 -l */
	float xv[11], yv[11];
	for (i = 0; i < 11; i++) {
		xv[i] = yv[i] = 0;
	};

	for (i = 0; i < OldNumSamples; i++) {
		xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; xv[8] = xv[9]; xv[9] = xv[10];
        xv[10] = OldSamples[i] / 3.125116703e+03;
        yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; yv[8] = yv[9]; yv[9] = yv[10];
        yv[10] =   (xv[0] + xv[10]) + 10 * (xv[1] + xv[9]) + 45 * (xv[2] + xv[8])
                     + 120 * (xv[3] + xv[7]) + 210 * (xv[4] + xv[6]) + 252 * xv[5]
                     + ( -0.2381749476 * yv[0]) + (  1.2030513372 * yv[1])
                     + ( -3.5333054455 * yv[2]) + (  7.3107119587 * yv[3])
                     + (-11.5735533520 * yv[4]) + ( 14.4976786330 * yv[5])
                     + (-14.5697155560 * yv[6]) + ( 11.6333850960 * yv[7])
                     + ( -7.2908812246 * yv[8]) + (  3.2331357374 * yv[9]);
        NewSamples[i / 2] = yv[10];
	};
};


void DispHelp(const string ProgName) {
	cout << "Usage: " << ProgName << " INPUT OUTPUT NSAMP CENTREFREQ DECIMATION (SAMPRATE) (DISCARD) (VERBOSITY)" << endl;
	cout << endl;
    cout << "INPUT - Character device to read form" << endl;
	cout << "OUTPUT - Output text file with array of IQ pairs in a+b*%%i form (Scilab)" << endl;
	cout << "NSAMP - Number of samples to acquire prior to decimation" << endl;
	cout << "CENTREFREQ - Frequency for digital down conversion" << endl;
	cout << "DEC - Decimation factor, only exponents of 2 are supported" << endl;
	cout << "SAMPRATE - (optional) SampleRate of source (default 250e6 SPS)" << endl;
	cout << "DISCARD - (optional) Number of samples to ignore at start of sample (default 0)" << endl;
	cout << "VERBOSITY - (optional) Verbosity, 0 to mute, 1 for verbose (default 0)" << endl;
};


void DownConvert(int16_t *Sample, int *I, int *Q, const int NumSamples, const int *CosLUT, const int *SinLUT, const int DeltaPhi, const int LUTSize) {
	int		i;
	int		Phi = 0;

	//I and Q are swapped (cos and sin are swapped) for downconversion
	for (i = 0; i < NumSamples; i++) {
		Phi = (Phi + DeltaPhi) % LUTSize;
		I[i] = Sample[i] * SinLUT[Phi];
		Q[i] = Sample[i] * CosLUT[Phi];
	};
};
