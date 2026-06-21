#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr const char* kMoodParamId = "mood";
}

//==============================================================================
MoodMixerAudioProcessor::MoodMixerAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    moodParam = apvts.getRawParameterValue (kMoodParamId);
}

MoodMixerAudioProcessor::~MoodMixerAudioProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout MoodMixerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kMoodParamId, 1 },
        "Stimmung",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel ("vibe")
            .withStringFromValueFunction ([] (float v, int)
            {
                return juce::String (juce::roundToInt (v * 100.0f)) + "%";
            })));

    return layout;
}

float MoodMixerAudioProcessor::getMoodValue() const noexcept
{
    return moodParam != nullptr ? moodParam->load() : 0.0f;
}

//==============================================================================
void MoodMixerAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    smoothedGain.reset (sampleRate, 0.05);

    const float mood = getMoodValue();
    smoothedGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (moodToGainDb (mood)));
}

void MoodMixerAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MoodMixerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (mainOut != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

void MoodMixerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    // Map mood [0, 1] -> [0 dB .. +0.75 dB] via the shared curve. Deliberately tiny.
    const float mood = getMoodValue();
    smoothedGain.setTargetValue (juce::Decibels::decibelsToGain (moodToGainDb (mood)));

    const int numSamples = buffer.getNumSamples();

    for (int n = 0; n < numSamples; ++n)
    {
        const float g = smoothedGain.getNextValue();

        for (int ch = 0; ch < totalNumInputChannels; ++ch)
            buffer.getWritePointer (ch)[n] *= g;
    }
}

//==============================================================================
juce::AudioProcessorEditor* MoodMixerAudioProcessor::createEditor()
{
    return new MoodMixerAudioProcessorEditor (*this);
}

//==============================================================================
void MoodMixerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }
}

void MoodMixerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MoodMixerAudioProcessor();
}
