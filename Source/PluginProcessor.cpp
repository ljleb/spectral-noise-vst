#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <random>
#include <functional>
#include <cstdlib>

#define M_PI 3.1415926535897932384626433832795028841971693993751058209

juce::String const SpectralNoiseAudioProcessor::TILT_ID = "tilt";

SpectralNoiseAudioProcessor::SpectralNoiseAudioProcessor():
    #ifndef JucePlugin_PreferredChannelConfigurations
        AudioProcessor (BusesProperties()
            #if ! JucePlugin_IsMidiEffect
                #if ! JucePlugin_IsSynth
                    .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                #endif
                .withOutput("Output", juce::AudioChannelSet::stereo(), true)
            #endif
        ),
    #endif
    _value_tree_state {
        *this, nullptr, "PARAMETERS", {
            std::make_unique<juce::AudioParameterFloat>(
                TILT_ID,
                "Tilt",
                juce::NormalisableRange<float>(-10.f, 10.f, 0.0001f),
                0.f),
        }
    },
    _tilt(_value_tree_state.getRawParameterValue(TILT_ID)),
    _buffer_size(0),
    _buffer_index(0)
{
    _value_tree_state.getParameter(TILT_ID)->addListener(this);
}

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

double calculate_window_sample(double x) {
    return (std::cos(M_PI*(2.0*x - 1.0)) + 1.0) / 2.0;
}

void SpectralNoiseAudioProcessor::prepareToPlay(double sample_rate, int samples_per_block) {
    auto const buffer_capacity = size_t(std::ceil(sample_rate)); // buffer size param
    using BufferValueType = decltype(_buffer)::value_type;

    juce::dsp::FFT fft(int(std::ceil(std::log2(buffer_capacity))));
    std::vector<float> fft_buffer(fft.getSize() * 2);

    std::mt19937 generator(std::random_device{}()); // seed param
    std::uniform_real_distribution<BufferValueType> distribution(-1.0, 1.0);
    std::generate(fft_buffer.begin(), fft_buffer.end(), std::bind(distribution, generator));

    fft.performRealOnlyForwardTransform(&*fft_buffer.begin());
    for (size_t i = 0; i < fft_buffer.size() / 2; ++i) {
        auto& coefficient = reinterpret_cast<std::complex<BufferValueType>&>(fft_buffer[2*i]);
        auto const frequency = double(i) * sample_rate / double(buffer_capacity);

        if (frequency < 20) {
            coefficient = 0;
        } else {
            auto const pivot_frequency = 1000.0; // pivot param
            auto const tilt = _tilt->load();

            auto const octaves_from_pivot = std::log2(frequency / pivot_frequency);
            auto const scaling_db = tilt * octaves_from_pivot;
            auto const scaling_factor = std::pow(10.0, scaling_db / 20.0);
            coefficient *= float(scaling_factor);
        }
    }

    fft.performRealOnlyInverseTransform(&*fft_buffer.begin());
    _buffer.resize(buffer_capacity);
    std::copy_n(fft_buffer.cbegin(), _buffer.size(), _buffer.begin());
    
    for (auto& sample : _buffer) {
        sample = std::clamp(sample, -1.0f, 1.0f);
    }

    size_t const window_width = 2 * 64; // window width param
    auto const best_window_start = find_most_natural_subwindow(window_width);

    // apply subwindow evenly at the boundary
    decltype(_buffer) const buffer_copy(_buffer.cbegin(), _buffer.cend());
    for (size_t i = 0; i < window_width; ++i) {
        auto const window = calculate_window_sample(double(i) / double(window_width - 1));
        auto& original_sample = _buffer[(_buffer.size() - window_width / 2 + i) % _buffer.size()];
        auto& window_sample = _buffer[(best_window_start + i) % _buffer.size()];
        original_sample = (1 - window) * original_sample + window * window_sample;
    }

    _buffer_index = 0;
    _buffer_size = buffer_capacity;
}

void SpectralNoiseAudioProcessor::releaseResources() {}

size_t SpectralNoiseAudioProcessor::find_most_natural_subwindow(size_t window_width) {
    double max_similarity = 0.0;
    size_t best_index = 0;
    for (size_t i = 0; i < _buffer.size() - window_width; ++i) {
        double similarity = 0.0;
        for (size_t j = 0; j < window_width; ++j) {
            auto const weight = calculate_window_sample(2.0 * double(j) / double(window_width - 1));
            double const original_sample = _buffer[i + j];
            double const window_sample = _buffer[(_buffer.size() - window_width / 2 + j) % _buffer.size()];
            similarity += original_sample * window_sample * weight * 2.0 / window_width;
        }
        if (similarity > max_similarity) {
            max_similarity = similarity;
            best_index = i;
        }
    }

    return best_index;
}

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
    auto const num_channels = std::max(input_channels, output_channels);
    auto const num_samples = buffer.getNumSamples();

    for (size_t channel = 0; channel < output_channels; ++channel) {
        auto* channel_data = buffer.getWritePointer(channel);
        size_t buffer_index = _buffer_index;
        for (size_t i = 0; i < num_samples;) {
            // adjust _buffer_size to play tones
            // use multiple voices with slightly different pitches for unison
            // use many _buffer_indices for stereo width
            auto const copy_size = std::min<size_t>(num_samples - i, _buffer_size - buffer_index);
            std::copy_n(_buffer.cbegin() + buffer_index, copy_size, channel_data + i);
            buffer_index = (buffer_index + copy_size) % _buffer_size;

            i += copy_size;
        }
    }

    _buffer_index = (_buffer_index + num_samples) % _buffer_size;
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

void SpectralNoiseAudioProcessor::parameterValueChanged(int parameter_id, float value) {}

void SpectralNoiseAudioProcessor::parameterGestureChanged(int parameter_id, bool gesture_is_starting) {
    prepareToPlay(double(_buffer.size()), -1);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new SpectralNoiseAudioProcessor();
}
