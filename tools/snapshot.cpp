#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

// Renders the plugin editor at a given mood value to a PNG, with no window.
// Usage: MoodSnapshot <outfile.png> <mood 0..1>
int main (int argc, char** argv)
{
    if (argc < 3)
    {
        juce::Logger::outputDebugString ("usage: MoodSnapshot <out.png> <mood>");
        return 1;
    }

    juce::ScopedJuceInitialiser_GUI gui;

    const juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
    const float mood = juce::jlimit (0.0f, 1.0f, (float) atof (argv[2]));

    MoodMixerAudioProcessor proc;

    if (auto* p = proc.apvts.getParameter ("mood"))
        p->setValueNotifyingHost (mood);

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditorIfNeeded());
    editor->setBounds (0, 0, editor->getWidth(), editor->getHeight());

    const auto img = editor->createComponentSnapshot (editor->getLocalBounds());

    juce::PNGImageFormat png;
    out.deleteFile();
    juce::FileOutputStream os (out);
    if (os.openedOk())
        png.writeImageToStream (img, os);

    juce::Logger::outputDebugString ("wrote " + out.getFullPathName());
    return 0;
}
