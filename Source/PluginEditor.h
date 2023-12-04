#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SpectralNoiseAudioProcessorEditor  : public juce::AudioProcessorEditor
{
    SpectralNoiseAudioProcessor& _audio_processor;

public:
    SpectralNoiseAudioProcessorEditor(SpectralNoiseAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~SpectralNoiseAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralNoiseAudioProcessorEditor)
};
