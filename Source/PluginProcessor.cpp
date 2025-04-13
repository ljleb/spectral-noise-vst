#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <random>
#include <functional>
#include <cstdlib>
#include "fftw-3.3/api/fftw3.h"

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
                juce::NormalisableRange<float>(-12.f, 12.f, 0.0001f),
                -6.f),
        }
    },
    _tilt(_value_tree_state.getRawParameterValue(TILT_ID))
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

void SpectralNoiseAudioProcessor::prepareToPlay(double sample_rate, int samples_per_block) {
    for (auto& noise_sampler : _noise_samplers) {
        noise_sampler.set_buffer_size(std::ceil(sample_rate));
        noise_sampler.set_db_per_octave(_tilt->load());
        noise_sampler.resample_noise();
    }
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

static juce::MidiBuffer filterMidiMessagesForChannel(const juce::MidiBuffer& input, int channel)
{
    juce::MidiBuffer output;
    for (auto metadata : input)
    {
        auto message = metadata.getMessage();
        if (message.getChannel() == channel) {
            output.addEvent(message, metadata.samplePosition);
        }
    }
    return output;
}

void SpectralNoiseAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_messages) {
    juce::ScopedNoDenormals noDenormals;
    auto const input_channels = getTotalNumInputChannels();
    auto const output_channels = getTotalNumOutputChannels();
    auto const num_channels = std::max(input_channels, output_channels);
    auto const num_samples = buffer.getNumSamples();

    _notes_counts.resize(output_channels, 0);

    for (size_t channel = 0; channel < output_channels; ++channel) {
        auto* channel_data = buffer.getWritePointer(channel);
        size_t& notes_count = _notes_counts[channel];

        for (size_t i = 0; i < num_samples; ++i) {
            for (auto metadata : midi_messages) {
                auto message = metadata.getMessage();
                if (size_t(message.getTimeStamp()) == i) {
                    if (message.isNoteOn()) {
                        ++notes_count;
                    }
                    if (message.isNoteOff()) {
                        --notes_count;
                    }
                }
            }
            float weight = float(bool(notes_count));
            // adjust _buffer_size to play tones
            // use multiple voices with slightly different pitches for unison
            // use many _buffer_indices for stereo width
            channel_data[i] = _noise_samplers[channel].next_sample() * weight;
        }
    }
}

bool SpectralNoiseAudioProcessor::hasEditor() const {
    return true;
}

juce::AudioProcessorEditor* SpectralNoiseAudioProcessor::createEditor() {
    return new SpectralNoiseAudioProcessorEditor(*this, _value_tree_state);
}

void SpectralNoiseAudioProcessor::getStateInformation(juce::MemoryBlock& destination) {
    auto state = _value_tree_state.copyState();
    auto xml_state = state.createXml();
    copyXmlToBinary(*xml_state, destination);
}

void SpectralNoiseAudioProcessor::setStateInformation(const void* data, int size_in_bytes) {
    auto xml_state = getXmlFromBinary(data, size_in_bytes);
    if (xml_state && xml_state->hasTagName(_value_tree_state.state.getType())) {
        _value_tree_state.replaceState(juce::ValueTree::fromXml(*xml_state));
    }
}

void SpectralNoiseAudioProcessor::parameterValueChanged(int parameter_id, float value) {
    for (auto& noise_sampler : _noise_samplers) {
        noise_sampler.set_db_per_octave(_tilt->load());
        noise_sampler.resample_noise();
    }
}

void SpectralNoiseAudioProcessor::parameterGestureChanged(int parameter_id, bool gesture_is_starting) {
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new SpectralNoiseAudioProcessor();
}
