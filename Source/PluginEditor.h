#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"
#include "ui/Look.h"

class FinishLineEditor : public juce::AudioProcessorEditor,
                         public juce::FileDragAndDropTarget,
                         private juce::Timer
{
public:
    static constexpr int kW = 1240, kH = 780;

    explicit FinishLineEditor (FinishLineProcessor&);
    ~FinishLineEditor() override;

    void resized() override;
    void paint (juce::Graphics&) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    void timerCallback() override;

    void buildTopBar();
    void buildMacros();
    void buildDisplay();
    void buildMaster();
    void buildLayers();
    void buildFx();
    void refreshPresetBox();
    void stepPreset (int delta);

    FinishLineProcessor& proc;
    ui::Look look;

    std::unique_ptr<juce::Component> content;
    juce::OwnedArray<juce::Component> owned;
    juce::Array<juce::Component*> animated;

    juce::ComboBox* presetBox = nullptr;
    juce::String shownPreset;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Component* display = nullptr;
    float reelAngle = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FinishLineEditor)
};
