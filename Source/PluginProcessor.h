#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Params.h"
#include "Presets.h"
#include "dsp/Engine.h"

class FinishLineProcessor : public juce::AudioProcessor
{
public:
    FinishLineProcessor();
    ~FinishLineProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Finish Line"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // MIDI map: C1 all | D1 sub | E1 impact | F1 synth | G1 riser | A1 tape stop | C2+ chromatic
    static int layersForNote (int note, float& transpose, bool& isTapeStop);

    // UI -> audio thread triggers (audition buttons).
    void uiTrigger (int mask, bool down);

    // Presets / randomiser (message thread).
    void applyPreset (int index);
    void randomize();
    void setParam (int pid, float realValue);
    float getParam (int pid) const { return raw[(size_t) pid]->load(); }
    juce::RangedAudioParameter* getParameterObject (int pid) const { return par[(size_t) pid]; }
    juce::File getUserPresetDirectory() const;
    void saveUserPreset (const juce::File&);
    bool loadUserPreset (const juce::File&);

    // User impact sample.
    bool loadUserSample (const juce::File&);
    std::shared_ptr<const fl::SampleData> getDisplaySample() const;

    // Offline bounce of the current sound -> 24-bit WAV in the temp folder.
    juce::File renderToWav();

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyState; // on-screen keyboard <-> MIDI stream

    static constexpr int kScopeSize = 4096;
    std::array<float, kScopeSize> scope {};
    std::atomic<int> scopeWrite { 0 };
    std::atomic<float> meterL { 0.0f }, meterR { 0.0f }, gainReduction { 1.0f };
    std::atomic<float> riserProgress { -1.0f }, hostBpm { 120.0f };
    std::atomic<bool> hostPlaying { false };

    juce::String presetName { "INIT" };
    int presetIndex = -1;
    float uiScale = 1.0f;

private:
    void buildSnapshot (float* p) const;
    juce::ValueTree makeState();
    void restoreState (const juce::ValueTree&);

    std::array<std::atomic<float>*, fl::P::NumParams> raw {};
    std::array<juce::RangedAudioParameter*, fl::P::NumParams> par {};

    fl::Engine engine;
    double currentSampleRate = 48000.0;

    std::atomic<int> uiOn { 0 }, uiOff { 0 };
    int heldSynthNote = -1, heldTapeNote = -1;

    juce::AudioFormatManager formats;
    juce::CriticalSection sampleLock;
    std::vector<std::shared_ptr<fl::SampleData>> userSamples; // kept alive for the audio thread
    std::shared_ptr<fl::SampleData> currentUserSample;
    juce::String userSamplePath;

    juce::Random random;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FinishLineProcessor)
};
