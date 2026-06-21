#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    "Mood Mixer" – a gentle, mostly-placebo gift plugin.

    One big knob morphs a portrait from neutral/serious to happy and, at the same
    time, nudges the output gain from 0 dB (no change) up to a barely-there +0.5 dB.
    The visual feedback is the real payload: the listener *feels* like they fixed
    the mix while the audio stays essentially untouched.
*/
class MoodMixerAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    MoodMixerAudioProcessor();
    ~MoodMixerAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                          { return true; }

    //==============================================================================
    const juce::String getName() const override              { return JucePlugin_Name; }

    bool acceptsMidi() const override                        { return false; }
    bool producesMidi() const override                       { return false; }
    bool isMidiEffect() const override                       { return false; }
    double getTailLengthSeconds() const override             { return 0.0; }

    //==============================================================================
    int getNumPrograms() override                            { return 1; }
    int getCurrentProgram() override                         { return 0; }
    void setCurrentProgram (int) override                    {}
    const juce::String getProgramName (int) override         { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Flat 0 dB until the "Perfekt. Genau so." zone, then linear up to the end:
    //   knob 0.00 .. 0.28 -> 0.00 dB
    //   knob 0.28          -> 0.00 dB   (start of the ramp)
    //   knob 1.00          -> +0.50 dB
    static constexpr float maxBoostDb    = 0.50f;
    static constexpr float gainStartMood = 0.28f;

    static float moodToGainDb (float mood) noexcept
    {
        mood = juce::jlimit (0.0f, 1.0f, mood);
        if (mood <= gainStartMood)
            return 0.0f;
        return juce::jmap (mood, gainStartMood, 1.0f, 0.0f, maxBoostDb);
    }

    juce::AudioProcessorValueTreeState apvts;

    // 0.0 = neutral / "sad", 1.0 = full smile. Read by the editor for the image.
    float getMoodValue() const noexcept;

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::atomic<float>* moodParam = nullptr;
    juce::LinearSmoothedValue<float> smoothedGain { 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoodMixerAudioProcessor)
};
