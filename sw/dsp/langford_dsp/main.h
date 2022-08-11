#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;


void	BuildTrigLUTs(int *CosLUT, int *SinLUT, const int LUTSize);
void	ComputePower(int *I, int *Q, int *E, const int NumSamples);
void	DecimateBy2(int *OldSamples, int *NewSamples, const int OldNumSamples);
void	DispHelp(const string ProgName);
void	DownConvert(int16_t *Sample, int *I, int *Q, const int NumSamples, const int *CosLUT, const int *SinLUT, const int DeltaPhi, const int LUTSize);
