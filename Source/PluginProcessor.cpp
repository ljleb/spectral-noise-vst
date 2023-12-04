/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
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
    _buffer(2048, 0.0),
    _value_tree_state {
        *this, nullptr, "PARAMETERS", {}
    }
{
    for (size_t i = 0; i < _buffer.size(); ++i) {
        _buffer[i] = float(std::rand()) / RAND_MAX - 0.5;
    }
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

void SpectralNoiseAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

void SpectralNoiseAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
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
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    std::vector<size_t> buffer_indices(totalNumOutputChannels, _buffer_index);

    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto& buffer_index = buffer_indices[channel];
        size_t const num_samples = buffer.getNumSamples();
        size_t const copy_size = std::min(num_samples, _buffer.size() - buffer_index);
        std::copy_n(_buffer.data() + buffer_index, copy_size, channelData);
        buffer_index = (buffer_index + copy_size) % _buffer.size();
        std::copy_n(_buffer.data(), num_samples - copy_size, channelData + copy_size);
    }

    _buffer_index = buffer_indices[0];
}

bool SpectralNoiseAudioProcessor::hasEditor() const {
    return true;
}

juce::AudioProcessorEditor* SpectralNoiseAudioProcessor::createEditor() {
    return new SpectralNoiseAudioProcessorEditor(*this, _value_tree_state);
}

//==============================================================================
void SpectralNoiseAudioProcessor::getStateInformation(juce::MemoryBlock& destination) {
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    auto state = _value_tree_state.copyState();
    std::unique_ptr<juce::XmlElement> xml_state(state.createXml());
    copyXmlToBinary(*xml_state, destination);
}

void SpectralNoiseAudioProcessor::setStateInformation(const void* data, int size_in_bytes) {
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto xml_state = getXmlFromBinary(data, size_in_bytes);
    if (xml_state && xml_state->hasTagName(_value_tree_state.state.getType())) {
        _value_tree_state.replaceState(juce::ValueTree::fromXml(*xml_state));
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new SpectralNoiseAudioProcessor();
}
