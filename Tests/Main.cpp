// Offline smoke test for Finish Line: renders every factory preset through the real
// processor, checks the output for NaNs / overs / silence, exercises state save-load,
// the WAV bounce and takes a PNG snapshot of the editor.
#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace
{
struct Stats { float peak = 0.0f; double sumSq = 0.0; juce::int64 count = 0; bool bad = false; };

Stats run (FinishLineProcessor& proc, double sr, int note, double holdSeconds, double totalSeconds)
{
    Stats st;
    const int block = 512;
    juce::AudioBuffer<float> buffer (2, block);
    const int total = (int) (totalSeconds * sr), hold = (int) (holdSeconds * sr);
    bool on = false, off = false;

    for (int pos = 0; pos < total; pos += block)
    {
        juce::MidiBuffer midi;
        if (! on) { midi.addEvent (juce::MidiMessage::noteOn (1, note, 1.0f), 0); on = true; }
        if (! off && pos >= hold) { midi.addEvent (juce::MidiMessage::noteOff (1, note), 0); off = true; }

        buffer.clear();
        proc.processBlock (buffer, midi);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < block; ++i)
            {
                const float v = buffer.getSample (ch, i);
                if (! std::isfinite (v)) st.bad = true;
                st.peak = juce::jmax (st.peak, std::abs (v));
                st.sumSq += (double) v * v;
                ++st.count;
            }
    }

    return st;
}
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File outDir = argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1])
                                       : juce::File::getCurrentWorkingDirectory();
    outDir.createDirectory();

    int failures = 0;
    const double sr = 48000.0;

    FinishLineProcessor proc;
    proc.setPlayConfigDetails (0, 2, sr, 512);
    proc.prepareToPlay (sr, 512);

    const auto& presets = fl::factoryPresets();
    std::printf ("%-24s %-30s %8s %8s\n", "CATEGORY", "PRESET", "PEAK dB", "RMS dB");

    for (int i = 0; i < (int) presets.size(); ++i)
    {
        proc.prepareToPlay (sr, 512);
        proc.applyPreset (i);
        const auto st = run (proc, sr, 36, 1.5, 4.0);
        const float peakDb = juce::Decibels::gainToDecibels (st.peak, -120.0f);
        const float rmsDb = juce::Decibels::gainToDecibels ((float) std::sqrt (st.sumSq / (double) juce::jmax<juce::int64> (1, st.count)), -120.0f);
        const bool ok = ! st.bad && st.peak <= 1.0f && peakDb > -40.0f;
        if (! ok) ++failures;
        std::printf ("%-24s %-30s %8.1f %8.1f %s\n", presets[(size_t) i].category, presets[(size_t) i].name, peakDb, rmsDb, ok ? "" : "<-- FAIL");
    }

    // Individual layer notes, chromatic note, tape-stop note at other sample rates.
    for (double rate : { 44100.0, 96000.0, 192000.0 })
    {
        proc.setPlayConfigDetails (0, 2, rate, 512);
        proc.prepareToPlay (rate, 512);
        proc.applyPreset (0);
        for (int pid : { fl::P::sub_on, fl::P::imp_on, fl::P::syn_on, fl::P::ris_on, fl::P::flt_on })
            proc.setParam (pid, 1.0f);
        proc.setParam (fl::P::gate_rate, 2.0f);
        proc.setParam (fl::P::macro1, 1.0f);
        proc.setParam (fl::P::macro2, 1.0f);

        for (int note : { 36, 38, 40, 41, 43, 45, 64 })
        {
            const auto st = run (proc, rate, note, 0.5, 1.5);
            const bool ok = ! st.bad && st.peak <= 1.0f;
            if (! ok) ++failures;
            std::printf ("sr %6.0f note %3d peak %6.1f dB %s\n", rate, note, juce::Decibels::gainToDecibels (st.peak, -120.0f), ok ? "" : "<-- FAIL");
        }
    }

    proc.setPlayConfigDetails (0, 2, sr, 512);
    proc.prepareToPlay (sr, 512);

    // State round trip.
    {
        const int anvil = fl::findPreset ("Anvil Strike");
        proc.applyPreset (anvil);
        proc.setParam (fl::P::sub_start, 321.0f);
        juce::MemoryBlock state;
        proc.getStateInformation (state);
        proc.applyPreset (fl::findPreset ("Reese Classic"));
        proc.setStateInformation (state.getData(), (int) state.getSize());
        const bool ok = std::abs (proc.getParam (fl::P::sub_start) - 321.0f) < 0.5f && anvil >= 0 && proc.presetIndex == anvil;
        if (! ok) ++failures;
        std::printf ("state round trip: %s (%d bytes)\n", ok ? "ok" : "FAIL", (int) state.getSize());
    }

    // Randomiser respects locks.
    {
        proc.applyPreset (fl::findPreset ("Cinema Slam"));
        proc.setParam (fl::P::sub_lock, 1.0f);
        const float before = proc.getParam (fl::P::sub_start);
        const float impBefore = proc.getParam (fl::P::imp_attack);
        proc.randomize();
        const bool ok = proc.getParam (fl::P::sub_start) == before && proc.getParam (fl::P::imp_attack) != impBefore;
        if (! ok) ++failures;
        std::printf ("randomize + lock: %s\n", ok ? "ok" : "FAIL");
        const auto st = run (proc, sr, 36, 1.0, 3.0);
        if (st.bad || st.peak > 1.0f) ++failures;
        proc.setParam (fl::P::sub_lock, 0.0f);
    }

    // WAV bounce (drag-and-drop path).
    {
        proc.applyPreset (fl::findPreset ("Tension Riser - 2 Bars"));
        const auto wav = proc.renderToWav();
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (wav));
        const bool ok = reader != nullptr && reader->bitsPerSample == 24 && reader->lengthInSamples > 48000;
        if (! ok) ++failures;
        std::printf ("wav bounce: %s  %s  (%.2f s, %d-bit)\n", ok ? "ok" : "FAIL", wav.getFullPathName().toRawUTF8(),
                     reader ? (double) reader->lengthInSamples / reader->sampleRate : 0.0, reader ? (int) reader->bitsPerSample : 0);

        // The bounce doubles as a user-sample load test.
        const bool loaded = proc.loadUserSample (wav);
        if (! loaded) ++failures;
        std::printf ("user sample load: %s\n", loaded ? "ok" : "FAIL");
        proc.prepareToPlay (sr, 512); // drop any tails so only the user sample is heard
        proc.setParam (fl::P::imp_on, 1.0f);
        const auto st = run (proc, sr, 40, 0.5, 1.0);
        if (st.bad || st.peak < 0.01f) { ++failures; std::printf ("user sample playback FAIL\n"); }
    }

    // Randomiser stress: many random patches with every layer and effect engaged.
    {
        int bad = 0;
        for (int i = 0; i < 300; ++i)
        {
            proc.applyPreset (i % (int) presets.size());
            for (int pid : { fl::P::sub_on, fl::P::imp_on, fl::P::syn_on, fl::P::ris_on, fl::P::flt_on })
                proc.setParam (pid, 1.0f);
            proc.setParam (fl::P::imp_dly, 0.6f);
            proc.setParam (fl::P::dly_fb, 0.95f);
            proc.setParam (fl::P::flt_lfo_depth, (float) (i % 3) - 1.0f);
            proc.setParam (fl::P::flt_env, (float) (i % 5) * 0.5f - 1.0f);
            proc.setParam (fl::P::macro1, (float) (i % 4) / 3.0f);
            proc.setParam (fl::P::macro2, (float) (i % 7) / 6.0f);
            proc.setParam (fl::P::lim_mode, (float) (i & 1));
            proc.setParam (fl::P::tape_stop, (i % 6) == 0 ? 1.0f : 0.0f);
            proc.randomize();
            const auto st = run (proc, sr, (i % 2) ? 36 : 48 + (i % 30), 0.4, 0.8);
            if (st.bad || st.peak > 1.0f) ++bad;
        }
        failures += bad;
        std::printf ("randomizer stress (300 patches): %s\n", bad == 0 ? "ok" : "FAIL");
    }

    // Editor snapshot.
    {
        proc.applyPreset (0);
        proc.setParam (fl::P::syn_on, 1.0f);
        proc.setParam (fl::P::ris_on, 1.0f);
        proc.setParam (fl::P::ris_gate, 1.0f);
        run (proc, sr, 36, 0.3, 0.35);

        std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
        editor->setSize (FinishLineEditor::kW, FinishLineEditor::kH);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (200);
        const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        const auto png = outDir.getChildFile ("ui_snapshot.png");
        png.deleteFile();
        if (auto stream = png.createOutputStream())
        {
            juce::PNGImageFormat format;
            format.writeImageToStream (image, *stream);
        }
        std::printf ("ui snapshot: %s\n", png.getFullPathName().toRawUTF8());
    }

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
