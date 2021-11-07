#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SusquashAudioProcessor::SusquashAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    apvts(*this, nullptr, "params", param::createParameters()),
    squash(apvts.getRawParameterValue(param::getID(param::ID::Squash))),
    gain(apvts.getRawParameterValue(param::getID(param::ID::Gain)))
#endif
{
}

SusquashAudioProcessor::~SusquashAudioProcessor()
{
}

//==============================================================================
const juce::String SusquashAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SusquashAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SusquashAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SusquashAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SusquashAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SusquashAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SusquashAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SusquashAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SusquashAudioProcessor::getProgramName (int index)
{
    return {};
}

void SusquashAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SusquashAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
}

void SusquashAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SusquashAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SusquashAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    //const auto totalNumInputChannels  = getTotalNumInputChannels();
    //const auto totalNumOutputChannels = getTotalNumOutputChannels();
    //for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    //    buffer.clear (i, 0, buffer.getNumSamples());

    const auto squashV = squash->load() * .01f;
    const auto gainV = juce::Decibels::decibelsToGain(gain->load());

    for (auto ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto samples = buffer.getWritePointer(ch);
        for (auto s = 0; s < buffer.getNumSamples(); ++s)
            samples[s] += squashV * (gainV * (samples[s] > 0 ? 1.f : samples[s] < 0 ? -1.f : 0.f) - samples[s]);
    }
}

//==============================================================================
bool SusquashAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SusquashAudioProcessor::createEditor()
{
    return new SusquashAudioProcessorEditor (*this);
}

//==============================================================================
void SusquashAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    std::unique_ptr<juce::XmlElement> xml(apvts.state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SusquashAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SusquashAudioProcessor();
}
