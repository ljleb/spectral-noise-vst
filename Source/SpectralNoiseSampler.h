#pragma once

#include <vector>
#include <random>
#include <complex>
#include "fftw-3.3/api/fftw3.h"

class SpectralNoiseSampler
{
	std::vector<float> _buffer;
	std::vector<std::complex<float>> _fourrier_buffer;
	fftwf_plan _fft_plan;

	size_t _index;
	float _db_per_octave;

public:
	SpectralNoiseSampler();
	~SpectralNoiseSampler();
	void set_buffer_size(size_t buffer_size);
	void set_db_per_octave(float db_per_octave);
	void resample_noise();
	float next_sample();
};
