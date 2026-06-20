#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    constexpr int   kMaxFrames   = 60;   // upper bound when scanning for frames
    constexpr int   kEditorW     = 460;
    constexpr int   kEditorH     = 600;
    constexpr int   kKnobSize    = 210;

    // Where the face peaks (happiest). Reach it early, then a long slide into doubt.
    constexpr float kPeakMood    = 0.30f;

    // Skip the deadest-neutral frames so the knob starts already a touch happy.
    constexpr float kRiseStart   = 0.55f;

    const juce::Colour kBackground { 0xff15171c };
    const juce::Colour kPanel      { 0xff1f2229 };
    const juce::Colour kAccentLow  { 0xff3a6ea5 };  // calm blue  (start / normal)
    const juce::Colour kAccentGold { 0xfff5a623 };  // warm gold  (centre / happy)
    const juce::Colour kAccentWarn { 0xffe5484d };  // red        (too far / "are you sure?")
    const juce::Colour kText       { 0xffe7e9ee };
    const juce::Colour kTextDim    { 0xff8b909c };

    // Cool -> gold by the peak, then gold -> red toward the end.
    juce::Colour accentForMood (float mood)
    {
        mood = juce::jlimit (0.0f, 1.0f, mood);
        return mood <= kPeakMood
                   ? kAccentLow .interpolatedWith (kAccentGold, mood / kPeakMood)
                   : kAccentGold.interpolatedWith (kAccentWarn, (mood - kPeakMood) / (1.0f - kPeakMood));
    }
}

//==============================================================================
MoodKnobLookAndFeel::MoodKnobLookAndFeel()
{
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void MoodKnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float rotaryStartAngle,
                                            float rotaryEndAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (10.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto centre = bounds.getCentre();
    const auto angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const float trackThickness = radius * 0.16f;
    const auto trackRadius = radius - trackThickness * 0.5f;

    // Background track.
    {
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, trackRadius, trackRadius, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (kPanel.brighter (0.12f));
        g.strokePath (track, juce::PathStrokeType (trackThickness, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // Value arc, coloured from calm blue to warm gold.
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, trackRadius, trackRadius, 0.0f,
                             rotaryStartAngle, angle, true);
        g.setColour (accentForMood (sliderPos));
        g.strokePath (value, juce::PathStrokeType (trackThickness, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // Knob body.
    const auto knobRadius = radius - trackThickness * 1.6f;
    {
        juce::ColourGradient grad (kPanel.brighter (0.25f), centre.x, centre.y - knobRadius,
                                   kPanel.darker (0.4f),    centre.x, centre.y + knobRadius, false);
        g.setGradientFill (grad);
        g.fillEllipse (juce::Rectangle<float> (knobRadius * 2.0f, knobRadius * 2.0f).withCentre (centre));

        g.setColour (kBackground.darker (0.3f));
        g.drawEllipse (juce::Rectangle<float> (knobRadius * 2.0f, knobRadius * 2.0f).withCentre (centre), 2.0f);
    }

    // Pointer.
    {
        const float pointerLength = knobRadius * 0.72f;
        const float pointerThick  = juce::jmax (3.0f, radius * 0.05f);
        juce::Path pointer;
        pointer.addRoundedRectangle (-pointerThick * 0.5f, -pointerLength,
                                     pointerThick, pointerLength, pointerThick * 0.5f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
        g.setColour (kText);
        g.fillPath (pointer);
    }
}

//==============================================================================
MoodMixerAudioProcessorEditor::MoodMixerAudioProcessorEditor (MoodMixerAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    loadFrames();

    moodKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    moodKnob.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                  juce::MathConstants<float>::pi * 2.8f,
                                  true);
    moodKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    moodKnob.setLookAndFeel (&knobLookAndFeel);
    moodKnob.setDoubleClickReturnValue (true, 0.0);
    addAndMakeVisible (moodKnob);

    moodAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "mood", moodKnob);

    setSize (kEditorW, kEditorH);
    startTimerHz (30);
}

MoodMixerAudioProcessorEditor::~MoodMixerAudioProcessorEditor()
{
    moodKnob.setLookAndFeel (nullptr);
}

//==============================================================================
void MoodMixerAudioProcessorEditor::loadFrames()
{
    auto loadSequence = [] (juce::StringRef prefix, juce::Array<juce::Image>& dest)
    {
        for (int i = 0; i < kMaxFrames; ++i)
        {
            const auto name = prefix + juce::String (i).paddedLeft ('0', 2) + "_png";

            int dataSize = 0;
            if (const char* data = BinaryData::getNamedResource (name.toRawUTF8(), dataSize))
            {
                auto img = juce::ImageFileFormat::loadFrom (data, (size_t) dataSize);
                if (img.isValid())
                    dest.add (img);
            }
        }
    };

    // "mood_" is the existing normal -> happy gradient (the rising half).
    loadSequence ("rise_",  riseFrames);
    if (riseFrames.isEmpty())
        loadSequence ("mood_", riseFrames);

    // Optional second-thoughts half: happy -> "are you sure?".
    loadSequence ("doubt_", doubtFrames);
}

const juce::Image* MoodMixerAudioProcessorEditor::frameForMood (float mood) const noexcept
{
    if (riseFrames.isEmpty())
        return nullptr;

    auto pick = [] (const juce::Array<juce::Image>& seq, float t, float startFrac = 0.0f) -> const juce::Image*
    {
        const int   last = seq.size() - 1;
        const float u    = startFrac + t * (1.0f - startFrac);
        return &seq.getReference (juce::jlimit (0, last, juce::roundToInt (u * (float) last)));
    };

    if (mood <= kPeakMood)
        return pick (riseFrames, mood / kPeakMood, kRiseStart); // already-happy -> happy

    const float t = (mood - kPeakMood) / (1.0f - kPeakMood);
    if (! doubtFrames.isEmpty())
        return pick (doubtFrames, t);                          // happy -> doubting

    return pick (riseFrames, 1.0f - t);                        // placeholder: play rise in reverse
}

//==============================================================================
void MoodMixerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBackground);

    const auto bounds = getLocalBounds();
    const float mood  = processorRef.getMoodValue();

    // Header — the title is long, so stack it: big line + "di Maestro Marco".
    auto header = bounds.reduced (24, 0).removeFromTop (58);

    // Italian tagline on the right, vertically centred.
    g.setColour (kTextDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::italic));
    g.drawText ("the last plugin in your chain...", header,
                juce::Justification::centredRight, false);

    auto titleArea = header;
    g.setColour (kText);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("L'ULTIMO TOCCO", titleArea.removeFromTop (32),
                juce::Justification::bottomLeft, false);

    g.setColour (kText);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold | juce::Font::italic));
    g.drawText ("di Maestro Marco", titleArea, juce::Justification::topLeft, false);

    // Image panel.
    {
        auto panel = imageArea.toFloat();
        g.setColour (kPanel);
        g.fillRoundedRectangle (panel, 12.0f);

        if (const juce::Image* frame = frameForMood (mood))
        {
            const auto inner = panel.reduced (6.0f);
            juce::Path clip;
            clip.addRoundedRectangle (inner, 8.0f);
            g.saveState();
            g.reduceClipRegion (clip);
            g.drawImage (*frame, inner,
                         juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
            g.restoreState();
        }
        else
        {
            g.setColour (kTextDim);
            g.drawText ("no image", panel, juce::Justification::centred, false);
        }

        g.setColour (kBackground.brighter (0.08f));
        g.drawRoundedRectangle (panel, 12.0f, 1.5f);
    }

    // Mood caption between image and knob.
    {
        auto captionArea = juce::Rectangle<int> (bounds.getX(), imageArea.getBottom() + 6,
                                                 bounds.getWidth(), 28);
        const juce::String caption = mood < 0.28f ? "hmm, it's missing something?"
                                   : mood < 0.52f ? "perfect. that's the one."
                                   : mood < 0.76f ? "...wait, is that a bit much?"
                                                  : "are you sure about this?";
        g.setColour (kTextDim);
        g.setFont (juce::FontOptions (14.0f));
        g.drawText (caption, captionArea, juce::Justification::centred, false);
    }

}

void MoodMixerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (24);
    bounds.removeFromTop (58); // header

    const int imgW = bounds.getWidth();
    const int imgH = juce::roundToInt (imgW * 0.625f); // 360x224 -> ~0.622 aspect
    imageArea = bounds.removeFromTop (imgH);

    bounds.removeFromTop (34); // caption space

    const int knobSize = juce::jmin (bounds.getWidth(), kKnobSize);
    auto knobArea = juce::Rectangle<int> (knobSize, knobSize)
                        .withCentre ({ getWidth() / 2, bounds.getY() + knobSize / 2 });
    moodKnob.setBounds (knobArea);
}

//==============================================================================
void MoodMixerAudioProcessorEditor::timerCallback()
{
    const float mood = processorRef.getMoodValue();

    // Repaint only when the knob has actually moved (covers host automation too).
    if (std::abs (mood - lastMood) > 1.0e-4f)
    {
        lastMood = mood;
        repaint();
    }
}
