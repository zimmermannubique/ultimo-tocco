#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/** A large, friendly rotary knob look. */
class MoodKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MoodKnobLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
};

//==============================================================================
class MoodMixerAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit MoodMixerAudioProcessorEditor (MoodMixerAudioProcessor&);
    ~MoodMixerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void loadFrames();
    const juce::Image* frameForMood (float mood) const noexcept;

    MoodMixerAudioProcessor& processorRef;

    MoodKnobLookAndFeel knobLookAndFeel;

    juce::Slider moodKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> moodAttachment;

    // Two halves of the arc:
    //   riseFrames : normal -> happy            (knob 0.0 .. 0.5)
    //   doubtFrames: happy  -> "are you sure?"  (knob 0.5 .. 1.0)
    // If doubtFrames is empty, the rise frames are played back in reverse as a
    // placeholder until the real "second thoughts" frames are supplied.
    juce::Array<juce::Image> riseFrames;
    juce::Array<juce::Image> doubtFrames;
    float lastMood = -1.0f;

    juce::Rectangle<int> imageArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoodMixerAudioProcessorEditor)
};
