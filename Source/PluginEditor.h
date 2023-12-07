#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SpectralNoiseAudioProcessorEditor  : public juce::AudioProcessorEditor {
    SpectralNoiseAudioProcessor& _audio_processor;

    class SliderPack {
        juce::Slider _slider;
        juce::Label _label;
        juce::AudioProcessorValueTreeState::SliderAttachment _attachment;

    public:
        SliderPack(
            SpectralNoiseAudioProcessorEditor& editor,
            juce::AudioProcessorValueTreeState& value_tree_state,
            juce::StringRef const parameter_identifier,
            unsigned int slider_position);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliderPack)
    };

    class ButtonPack {
        juce::ToggleButton _button;
        juce::AudioProcessorValueTreeState::ButtonAttachment _attachment;

    public:
        ButtonPack(
            SpectralNoiseAudioProcessorEditor& editor,
            juce::AudioProcessorValueTreeState& value_tree_state,
            juce::StringRef const parameter_identifier,
            unsigned int slider_position);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonPack)
    };

    std::vector<std::unique_ptr<SliderPack>> _slider_packs;
    std::vector<std::unique_ptr<ButtonPack>> _button_packs;

public:
    SpectralNoiseAudioProcessorEditor(SpectralNoiseAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~SpectralNoiseAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralNoiseAudioProcessorEditor)
};
