#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectralNoiseAudioProcessorEditor::SpectralNoiseAudioProcessorEditor(SpectralNoiseAudioProcessor& audio_processor, juce::AudioProcessorValueTreeState&):
    AudioProcessorEditor(&audio_processor),
    _audio_processor(audio_processor)
{
    setSize(400, 300);
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
