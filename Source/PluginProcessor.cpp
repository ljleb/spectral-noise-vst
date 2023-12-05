#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <random>
#include <functional>
#include <cstdlib>

#define M_PI 3.1415926535897932384626433832795028841971693993751058209

SpectralNoiseAudioProcessor::SpectralNoiseAudioProcessor():
    #ifndef JucePlugin_PreferredChannelConfigurations
        AudioProcessor (BusesProperties()
            #if ! JucePlugin_IsMidiEffect
                #if ! JucePlugin_IsSynth
                    .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                #endif
                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
            #endif
        ),
    #endif
    _value_tree_state {
        *this, nullptr, "PARAMETERS", {}
    },
    _buffer_index(0)
{}

SpectralNoiseAudioProcessor::~SpectralNoiseAudioProcessor()
{}

const juce::String SpectralNoiseAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool SpectralNoiseAudioProcessor::acceptsMidi() const {
    return JucePlugin_WantsMidiInput;
}

bool SpectralNoiseAudioProcessor::producesMidi() const {
    return JucePlugin_ProducesMidiOutput;
}

bool SpectralNoiseAudioProcessor::isMidiEffect() const {
    return JucePlugin_IsMidiEffect;
}

double SpectralNoiseAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int SpectralNoiseAudioProcessor::getNumPrograms() {
    return 1;
}

int SpectralNoiseAudioProcessor::getCurrentProgram() {
    return 0;
}

void SpectralNoiseAudioProcessor::setCurrentProgram(int index) {}

const juce::String SpectralNoiseAudioProcessor::getProgramName(int index) {
    return {};
}

void SpectralNoiseAudioProcessor::changeProgramName(int index, const juce::String& new_name) {}

void SpectralNoiseAudioProcessor::prepareToPlay(double sample_rate, int samples_per_block) {
    auto const buffer_size = size_t(std::ceil(sample_rate)) * 20;
    using BufferValueType = decltype(_buffer)::value_type;

    juce::dsp::FFT fft(int(std::ceil(std::log2(buffer_size))));
    _raw_buffer.resize(fft.getSize() * 2);

    std::mt19937 generator;
    std::uniform_real_distribution<BufferValueType> distribution(-1.0, 1.0);
    std::generate(_raw_buffer.begin(), _raw_buffer.end(), std::bind(distribution, generator));

    decltype(_raw_buffer) fft_buffer(_raw_buffer.cbegin(), _raw_buffer.cend());
    fft.performRealOnlyForwardTransform(fft_buffer.data());
    for (size_t i = 0; i < fft_buffer.size() / 2; ++i) {
        auto& coefficient = reinterpret_cast<std::complex<BufferValueType>&>(fft_buffer[2*i]);
        auto const frequency = double(i) * sample_rate / double(buffer_size);

        if (frequency < 20) {
            coefficient = 0;
        } else {
            auto const pivot_frequency = 1000.0;
            auto const scaling_dbo = -4.5;

            auto const octaves_from_pivot = std::log2(frequency / pivot_frequency);
            auto const scaling_db = scaling_dbo * octaves_from_pivot;
            auto const scaling_factor = std::pow(10.0, scaling_db / 20.0);
            coefficient *= float(scaling_factor);
        }
    }

    fft.performRealOnlyInverseTransform(fft_buffer.data());
    _buffer.resize(buffer_size);
    std::copy_n(fft_buffer.begin(), _buffer.size(), _buffer.begin());
    //for (size_t i = 0; i < _buffer.size(); ++i) {
    //    auto const x = double(i) / (_buffer.size() - 1);
    //    auto const f_x = (std::cos(M_PI * (2.0 * x - 1.0)) + 1.0) / 2.0;
    //    auto const g_x = 0; // (std::cos(M_PI * (4.0 * x - 1.0)) + 1.0) / 8.0;
    //    _buffer[i] *= f_x + g_x;
    //}
    //decltype(_buffer) buffer_copy(_buffer);
    //for (size_t i = 0; i < _buffer.size(); ++i) {
    //    _buffer[i] += buffer_copy[(i + _buffer.size() / 2) % _buffer.size()];
    //}
    _buffer_index = 0;
}

void SpectralNoiseAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SpectralNoiseAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    #if JucePlugin_IsMidiEffect
        juce::ignoreUnused (layouts);
        return true;
    #else
        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
            layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()
        ) {
            return false;
        }
        #if ! JucePlugin_IsSynth
            if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()) {
                return false;
            }
        #endif
        return true;
    #endif
}
#endif

void SpectralNoiseAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_messages) {
    juce::ScopedNoDenormals noDenormals;
    auto const input_channels = getTotalNumInputChannels();
    auto const output_channels = getTotalNumOutputChannels();
    auto const num_samples = buffer.getNumSamples();

    for (size_t channel = 0; channel < output_channels; ++channel) {
        auto* channel_data = buffer.getWritePointer(channel);
        auto const copy_size = std::min<size_t>(num_samples, _buffer.size() - _buffer_index);
        std::copy_n(_buffer.cbegin() + _buffer_index, copy_size, channel_data);
        std::copy_n(_buffer.cbegin(), num_samples - copy_size, channel_data + copy_size);
    }

    _buffer_index = (_buffer_index + num_samples) % _buffer.size();
}

bool SpectralNoiseAudioProcessor::hasEditor() const {
    return true;
}

juce::AudioProcessorEditor* SpectralNoiseAudioProcessor::createEditor() {
    return new SpectralNoiseAudioProcessorEditor(*this, _value_tree_state);
}

void SpectralNoiseAudioProcessor::getStateInformation(juce::MemoryBlock& destination) {
    auto state = _value_tree_state.copyState();
    std::unique_ptr<juce::XmlElement> xml_state(state.createXml());
    copyXmlToBinary(*xml_state, destination);
}

void SpectralNoiseAudioProcessor::setStateInformation(const void* data, int size_in_bytes) {
    auto xml_state = getXmlFromBinary(data, size_in_bytes);
    if (xml_state && xml_state->hasTagName(_value_tree_state.state.getType())) {
        _value_tree_state.replaceState(juce::ValueTree::fromXml(*xml_state));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new SpectralNoiseAudioProcessor();
}
