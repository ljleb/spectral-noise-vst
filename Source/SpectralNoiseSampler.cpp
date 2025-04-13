#include "SpectralNoiseSampler.h"
#include <cmath>
#include <functional>
#include <random>
#include <cstdlib>
#include <complex>
#include "fftw-3.3/api/fftw3.h"

SpectralNoiseSampler::SpectralNoiseSampler():
	_index(0),
	_db_per_octave(0)
{
    _fft_plan = fftwf_plan_dft_c2r_1d(_buffer.size(), reinterpret_cast<fftwf_complex*>(_fourrier_buffer.data()), _buffer.data(), FFTW_MEASURE);
}

SpectralNoiseSampler::~SpectralNoiseSampler() {
    fftwf_destroy_plan(_fft_plan);
}

void SpectralNoiseSampler::set_buffer_size(size_t buffer_size) {
    buffer_size = buffer_size + buffer_size % 2;  // ensure buffer size is even
    fftwf_destroy_plan(_fft_plan);
    _buffer.resize(buffer_size);
    _fourrier_buffer.resize(buffer_size/2 + 1);
    _fft_plan = fftwf_plan_dft_c2r_1d(_buffer.size(), (fftwf_complex*)_fourrier_buffer.data(), _buffer.data(), FFTW_MEASURE);
}

void SpectralNoiseSampler::set_db_per_octave(float db_per_octave) {
	_db_per_octave = db_per_octave;
}

void SpectralNoiseSampler::resample_noise() {
    if (_buffer.empty()) {
        return;
    }

    // generate gaussian spectral noise with expected norm of 1
    std::mt19937 generator(std::random_device{}());
    //std::normal_distribution<float> distribution(0.f, 0.5f); // 2.f / M_PI
    std::uniform_real_distribution<float> distribution(-1.f, 1.f);
    std::generate(
        reinterpret_cast<float*>(&*_fourrier_buffer.begin()),
        reinterpret_cast<float*>(&*_fourrier_buffer.end() + 1),
        std::bind(distribution, generator)
    );

    auto const min_frequency = 20;
    auto const normalization = std::sqrt(float(_buffer.size()));
    for (size_t frequency = 0; frequency < _fourrier_buffer.size(); ++frequency) {
        auto& coefficient = reinterpret_cast<std::complex<float>&>(_fourrier_buffer[frequency]);

        if (frequency < min_frequency) {
            coefficient = 0;
        }
        else {
            auto const pivot_frequency = 1000.0;

            auto const octaves_from_pivot = std::log2(frequency / pivot_frequency);
            auto const scaling_db = _db_per_octave * octaves_from_pivot;
            auto const scaling_factor = std::pow(10.0, scaling_db / 20.0);
            coefficient *= float(scaling_factor);
        }
    }

    fftwf_execute(_fft_plan);

    float root_sum = 0;
    for (const float& sample: _buffer) {
        root_sum += sample * sample;
    }
    const float root_mean_square = std::sqrt(root_sum / _buffer.size());
    for (float& sample: _buffer) {
        sample *= 64 / (normalization * root_mean_square);
    }

    _index = 0;
}

float SpectralNoiseSampler::next_sample() {
    if (_buffer.empty()) {
        return 0;
    }
    if (_index >= _buffer.size()) {
        resample_noise();
    }
    return _buffer[_index++];
}
