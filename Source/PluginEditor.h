#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

struct SusquashAudioProcessorEditor :
    public juce::AudioProcessorEditor
{
    SusquashAudioProcessorEditor (SusquashAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

    SusquashAudioProcessor& p;
    juce::Image bg;
    juce::Colour mainCol;

    juce::Label subTitle;
    Knob squash, gain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SusquashAudioProcessorEditor)
};
