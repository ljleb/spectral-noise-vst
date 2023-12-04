#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <cstdlib>

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
    }
{
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
    _buffer.resize(std::ceil(sample_rate), 0.0);
    for (size_t i = 0; i < _buffer.size(); ++i) {
        _buffer[i] = float(std::rand()) / RAND_MAX - 0.5;
    }
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
        auto* channelData = buffer.getWritePointer(channel);
        auto const copy_size = std::min<size_t>(num_samples, _buffer.size() - _buffer_index);
        std::copy_n(_buffer.data() + _buffer_index, copy_size, channelData);
        std::copy_n(_buffer.data() + copy_size, num_samples - copy_size, channelData + copy_size);
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
