#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <memory>

SpectralNoiseAudioProcessorEditor::SpectralNoiseAudioProcessorEditor(SpectralNoiseAudioProcessor& audio_processor, juce::AudioProcessorValueTreeState& value_tree_state):
    AudioProcessorEditor(&audio_processor),
    _audio_processor(audio_processor)
{
    for (auto const& parameter_id : {
        SpectralNoiseAudioProcessor::TILT_ID,
    }) {
        _slider_packs.emplace_back(
            std::make_unique<SliderPack>(
                *this,
                value_tree_state,
                parameter_id,
                static_cast<unsigned int>(_slider_packs.size())));
    }

    for (auto const& parameter_id: std::initializer_list<juce::String>{
    }) {
        _button_packs.emplace_back(
            std::make_unique<ButtonPack>(
                *this,
                value_tree_state,
                parameter_id,
                static_cast<unsigned int>(_button_packs.size())));
    }

    setResizable(false, false);
    setSize(static_cast<unsigned int>((_slider_packs.size() + 1) * 100), 140);
}

SpectralNoiseAudioProcessorEditor::SliderPack::SliderPack(
    SpectralNoiseAudioProcessorEditor& editor,
    juce::AudioProcessorValueTreeState& value_tree_state,
    juce::StringRef const parameter_identifier,
    unsigned int slider_position
):
    _slider(parameter_identifier),
    _attachment(value_tree_state, parameter_identifier, _slider)
{
    _slider.setBounds(100 * (slider_position + 1), 25, 105, 105);
    _slider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 64, 20);
    _slider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    editor.addAndMakeVisible(_slider);

    auto const& slider_parameter = value_tree_state.getParameter(_slider.getName());
    _label.attachToComponent(&_slider, false);
    _label.setJustificationType(juce::Justification::centred);
    _label.setFont(juce::Font(16.f));
    _label.setText(slider_parameter->getName(20), juce::NotificationType::dontSendNotification);
    editor.addAndMakeVisible(_label);
}

SpectralNoiseAudioProcessorEditor::ButtonPack::ButtonPack(
    SpectralNoiseAudioProcessorEditor& editor,
    juce::AudioProcessorValueTreeState& value_tree_state,
    juce::StringRef const parameter_identifier,
    unsigned int button_position
):
    _attachment(value_tree_state, parameter_identifier, _button)
{
    auto const& parameter = value_tree_state.getParameter(parameter_identifier);
    _button.setBounds(0, 25 * button_position, 105, 25);
    _button.setButtonText(parameter->getName(20));
    editor.addAndMakeVisible(_button);
}

SpectralNoiseAudioProcessorEditor::~SpectralNoiseAudioProcessorEditor() {
}

void SpectralNoiseAudioProcessorEditor::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void SpectralNoiseAudioProcessorEditor::resized() {
}
