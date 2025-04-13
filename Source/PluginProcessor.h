#pragma once

#include <JuceHeader.h>
#include "SpectralNoiseSampler.h"

class SpectralNoiseAudioProcessor  : public juce::AudioProcessor, public juce::AudioProcessorParameter::Listener {
    std::array<SpectralNoiseSampler, 2> _noise_samplers;
    std::vector<size_t> _notes_counts;

    juce::AudioProcessorValueTreeState _value_tree_state;
    std::atomic<float>* _tilt;

public:
    static juce::String const TILT_ID;

    SpectralNoiseAudioProcessor();
    ~SpectralNoiseAudioProcessor() override;

    void prepareToPlay(double, int) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout&) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override;

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    void parameterValueChanged(int, float) override;
    void parameterGestureChanged(int, bool) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralNoiseAudioProcessor)
};
