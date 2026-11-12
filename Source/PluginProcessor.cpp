#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace fl;

//==============================================================================
FinishLineProcessor::FinishLineProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (int i = 0; i < P::NumParams; ++i)
    {
        raw[(size_t) i] = apvts.getRawParameterValue (kDefs[i].id);
        par[(size_t) i] = apvts.getParameter (kDefs[i].id);
        jassert (raw[(size_t) i] != nullptr && par[(size_t) i] != nullptr);
    }

    formats.registerBasicFormats();
    engine.prepare (48000.0);
}

//==============================================================================
void FinishLineProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate);
    heldSynthNote = heldTapeNote = -1;
    keyState.reset();
}

bool FinishLineProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

int FinishLineProcessor::layersForNote (int note, float& transpose, bool& isTapeStop)
{
    transpose = 0.0f;
    isTapeStop = false;

    switch (note)
    {
        case 36: return kAllLayers;
        case 38: return kSub;
        case 40: return kImpact;
        case 41: return kSynth;
        case 43: return kRiser;
        case 45: isTapeStop = true; return 0;
        default: break;
    }

    if (note >= 48)
    {
        transpose = (float) (note - 60);
        return kAllLayers;
    }

    return 0;
}

void FinishLineProcessor::buildSnapshot (float* p) const
{
    float offs[P::NumParams] = {};
    bool any = false;

    static const int macroIds[2] = { P::macro1, P::macro2 };
    static const int firstSlot[2] = { P::m1_t1, P::m2_t1 };

    for (int m = 0; m < 2; ++m)
    {
        const float mv = raw[(size_t) macroIds[m]]->load();
        if (mv <= 0.0f) continue;

        for (int s = 0; s < 4; ++s)
        {
            const int t = (int) raw[(size_t) (firstSlot[m] + s * 2)]->load();
            const float a = raw[(size_t) (firstSlot[m] + s * 2 + 1)]->load();
            if (t > 0 && t < (int) std::size (kMacroTargets) && a != 0.0f)
            {
                offs[kMacroTargets[t]] += mv * a;
                any = true;
            }
        }
    }

    for (int i = 0; i < P::NumParams; ++i)
        p[i] = raw[(size_t) i]->load();

    if (any)
        for (int i = 0; i < P::NumParams; ++i)
            if (offs[i] != 0.0f)
            {
                const auto& range = par[(size_t) i]->getNormalisableRange();
                p[i] = range.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, range.convertTo0to1 (p[i]) + offs[i]));
            }
}

void FinishLineProcessor::uiTrigger (int mask, bool down)
{
    if (down) uiOn.fetch_or (mask);
    else      uiOff.fetch_or (mask);
}

//==============================================================================
void FinishLineProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    buffer.clear();
    if (buffer.getNumChannels() < 2 || n == 0)
        return;

    // Merge clicks on the on-screen keyboard into the MIDI stream (and show incoming notes on it).
    keyState.processNextMidiBuffer (midi, 0, n, true);

    float p[P::NumParams];
    buildSnapshot (p);

    HostInfo host;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm()) host.bpm = *bpm;
            if (auto ppq = pos->getPpqPosition()) host.ppq = *ppq;
            if (auto bar = pos->getPpqPositionOfLastBarStart()) host.ppqBar = *bar;
            if (auto sig = pos->getTimeSignature()) { host.num = sig->numerator; host.den = sig->denominator; }
            host.playing = pos->getIsPlaying();
        }

    hostBpm.store ((float) host.bpm);
    hostPlaying.store (host.playing);

    if (const int on = uiOn.exchange (0))
        engine.noteOn (on, 0.0f, 1.0f, p, host);
    if (const int off = uiOff.exchange (0))
        engine.noteOff (off);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    const double ppqPerSample = host.bpm / (60.0 * currentSampleRate);
    int cursor = 0;

    auto renderTo = [&] (int end)
    {
        end = juce::jlimit (cursor, n, end);
        if (end > cursor)
        {
            HostInfo h = host;
            h.ppq += (double) cursor * ppqPerSample;
            engine.process (L + cursor, R + cursor, end - cursor, p, h);
            cursor = end;
        }
    };

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        renderTo (meta.samplePosition);

        HostInfo h = host;
        h.ppq += (double) cursor * ppqPerSample;

        if (msg.isNoteOn())
        {
            float transpose = 0.0f;
            bool tapeNote = false;
            const int mask = layersForNote (msg.getNoteNumber(), transpose, tapeNote);

            if (tapeNote)
            {
                heldTapeNote = msg.getNoteNumber();
                engine.tapeEngage = true;
            }
            else if (mask != 0)
            {
                engine.noteOn (mask, transpose, msg.getFloatVelocity(), p, h);
                if (mask & kSynth) heldSynthNote = msg.getNoteNumber();
            }
        }
        else if (msg.isNoteOff())
        {
            if (msg.getNoteNumber() == heldTapeNote)
            {
                heldTapeNote = -1;
                engine.tapeEngage = false;
            }
            if (msg.getNoteNumber() == heldSynthNote)
            {
                heldSynthNote = -1;
                engine.noteOff (kSynth);
            }
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            heldSynthNote = heldTapeNote = -1;
            engine.tapeEngage = false;
            engine.noteOff (kSynth);
        }
    }

    renderTo (n);
    midi.clear();

    // ---- metering / scope feed
    float pkL = 0.0f, pkR = 0.0f;
    int w = scopeWrite.load (std::memory_order_relaxed);
    for (int i = 0; i < n; ++i)
    {
        pkL = juce::jmax (pkL, std::abs (L[i]));
        pkR = juce::jmax (pkR, std::abs (R[i]));
        scope[(size_t) w] = 0.5f * (L[i] + R[i]);
        w = (w + 1) & (kScopeSize - 1);
    }
    scopeWrite.store (w, std::memory_order_relaxed);

    meterL.store (juce::jmax (pkL, meterL.load() * 0.92f));
    meterR.store (juce::jmax (pkR, meterR.load() * 0.92f));
    gainReduction.store (juce::jmin (engine.gainReduction, gainReduction.load() * 0.9f + 0.1f));
    riserProgress.store (engine.riserProgress());
}

//==============================================================================
void FinishLineProcessor::setParam (int pid, float realValue)
{
    auto* prm = par[(size_t) pid];
    const float norm = prm->convertTo0to1 (realValue);
    if (std::abs (norm - prm->getValue()) > 1.0e-7f)
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (norm);
        prm->endChangeGesture();
    }
}

void FinishLineProcessor::applyPreset (int index)
{
    const auto& presets = factoryPresets();
    if (index < 0 || index >= (int) presets.size())
        return;

    using namespace P;
    static const int keep[] = { sub_lock, imp_lock, syn_lock, ris_lock, fx_lock, key, key_lock, master_gain };

    for (int i = 0; i < NumParams; ++i)
    {
        if (std::find (std::begin (keep), std::end (keep), i) != std::end (keep))
            continue;
        setParam (i, kDefs[i].df);
    }

    for (const auto& v : presets[(size_t) index].values)
        setParam (v.first, v.second);

    presetIndex = index;
    presetName = presets[(size_t) index].name;
}

void FinishLineProcessor::randomize()
{
    using namespace P;
    auto& r = random;
    auto rnd = [&r] (float a, float b) { return a + (b - a) * r.nextFloat(); };
    auto logRnd = [&r] (float a, float b) { return a * std::pow (b / a, r.nextFloat()); };
    auto pick = [&r] (int count) { return (float) r.nextInt (count); };

    if (getParam (sub_lock) < 0.5f)
    {
        setParam (sub_wave, pick (3));
        setParam (sub_start, logRnd (80.0f, 500.0f));
        setParam (sub_end, rnd (28.0f, 60.0f));
        setParam (sub_pdecay, logRnd (0.04f, 1.5f));
        setParam (sub_decay, logRnd (0.8f, 7.0f));
        setParam (sub_punch, rnd (0.0f, 0.9f));
        setParam (sub_harm, rnd (0.0f, 0.7f));
    }

    if (getParam (imp_lock) < 0.5f)
    {
        setParam (imp_sample, pick (kNumFactorySamples));
        setParam (imp_pitch, (float) juce::roundToInt (rnd (-12.0f, 7.0f)));
        setParam (imp_start, 0.0f);
        setParam (imp_reverse, r.nextFloat() < 0.15f ? 1.0f : 0.0f);
        setParam (imp_attack, rnd (-0.2f, 0.8f));
        setParam (imp_sustain, rnd (-0.5f, 0.6f));
        setParam (imp_rev, rnd (0.0f, 0.6f));
    }

    if (getParam (syn_lock) < 0.5f)
    {
        static const float pitches[] = { 0.0f, 0.0f, 0.0f, 12.0f, -12.0f, 7.0f };
        setParam (syn_wt, r.nextFloat());
        setParam (syn_uni, 1.0f + pick (8));
        setParam (syn_det, rnd (0.05f, 0.7f));
        setParam (syn_pitch, pitches[r.nextInt (6)]);
        setParam (syn_ntype, pick (3));
        setParam (syn_noise, r.nextBool() ? 0.0f : rnd (0.05f, 0.5f));
        setParam (syn_ftype, r.nextFloat() < 0.7f ? 0.0f : pick (3));
        setParam (syn_cut, logRnd (150.0f, 6000.0f));
        setParam (syn_res, rnd (0.0f, 0.6f));
        setParam (syn_a, logRnd (0.002f, 0.6f));
        setParam (syn_d, logRnd (0.05f, 1.5f));
        setParam (syn_s, rnd (0.4f, 1.0f));
        setParam (syn_r, logRnd (0.1f, 2.5f));
    }

    if (getParam (ris_lock) < 0.5f)
    {
        // Length and grid alignment are arrangement decisions, so they are left alone.
        setParam (ris_mode, r.nextFloat() < 0.6f ? 0.0f : pick (3));
        setParam (ris_curve, pick (2));
        setParam (ris_range, (float) juce::roundToInt (rnd (7.0f, 36.0f)));
        setParam (ris_tone, rnd (0.0f, 0.8f));
        setParam (ris_gate, r.nextFloat() < 0.7f ? 0.0f : 1.0f + pick (5));
        setParam (ris_rev, rnd (0.1f, 0.6f));
    }

    if (getParam (fx_lock) < 0.5f)
    {
        setParam (dist_type, pick (4));
        setParam (dist_drive, rnd (0.05f, 0.6f));
        setParam (rev_mode, pick (3));
        setParam (rev_size, rnd (0.2f, 0.9f));
        setParam (rev_duck, rnd (0.0f, 0.7f));
    }

    presetIndex = -1;
    presetName = "RANDOM " + juce::String (r.nextInt (9000) + 1000);
}

//==============================================================================
bool FinishLineProcessor::loadUserSample (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples < 16)
        return false;

    const int len = (int) juce::jmin<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 30.0));
    juce::AudioBuffer<float> buf (2, len);
    reader->read (&buf, 0, len, 0, true, true);
    if (reader->numChannels < 2)
        buf.copyFrom (1, 0, buf, 0, 0, len);

    auto s = std::make_shared<SampleData>();
    s->name = file.getFileNameWithoutExtension().toUpperCase();
    s->sr = reader->sampleRate;
    s->l.assign (buf.getReadPointer (0), buf.getReadPointer (0) + len);
    s->r.assign (buf.getReadPointer (1), buf.getReadPointer (1) + len);
    s->computePeaks();

    {
        const juce::ScopedLock sl (sampleLock);
        userSamples.push_back (s); // older ones stay alive: the audio thread may still be playing them
        currentUserSample = s;
        userSamplePath = file.getFullPathName();
    }

    engine.userSample.store (s.get());
    setParam (P::imp_sample, (float) kUserSampleIndex);
    return true;
}

std::shared_ptr<const SampleData> FinishLineProcessor::getDisplaySample() const
{
    const int idx = (int) getParam (P::imp_sample);
    const auto& bank = factoryBank();
    if (idx >= 0 && idx < (int) bank.size())
        return bank[(size_t) idx];

    const juce::ScopedLock sl (sampleLock);
    return currentUserSample;
}

//==============================================================================
juce::File FinishLineProcessor::renderToWav()
{
    const double sr = currentSampleRate > 0.0 ? currentSampleRate : 48000.0;

    float p[P::NumParams];
    buildSnapshot (p);
    p[P::tape_stop] = 0.0f;

    HostInfo host;
    host.bpm = juce::jlimit (20.0, 400.0, (double) hostBpm.load());
    host.playing = false;

    auto eng = std::make_unique<Engine>();
    eng->prepare (sr);
    eng->userSample.store (engine.userSample.load());
    eng->noteOn (kAllLayers, 0.0f, 1.0f, p, host);

    static const double bars[] = { 1.0, 2.0, 4.0, 8.0 };
    const double barSeconds = 4.0 * 60.0 / host.bpm;
    const double holdSeconds = p[P::ris_on] > 0.5f ? bars[juce::jlimit (0, 3, (int) p[P::ris_len])] * barSeconds : barSeconds;
    const int hold = (int) (holdSeconds * sr);
    const int maxLen = hold + (int) (20.0 * sr);
    const int block = 512;

    juce::AudioBuffer<float> out (2, maxLen);
    out.clear();

    int pos = 0, lastLoud = 0;
    bool released = false;

    while (pos < maxLen)
    {
        if (! released && pos >= hold)
        {
            eng->noteOff (kAllLayers);
            released = true;
        }

        const int n = juce::jmin (block, maxLen - pos);
        eng->process (out.getWritePointer (0, pos), out.getWritePointer (1, pos), n, p, host);
        if (out.getMagnitude (pos, n) > 0.0005f)
            lastLoud = pos + n;
        pos += n;

        if (released && pos - lastLoud > (int) (0.4 * sr))
            break;
    }

    const int length = juce::jlimit (block, maxLen, lastLoud + (int) (0.05 * sr));
    out.applyGainRamp (juce::jmax (0, length - 256), juce::jmin (256, length), 1.0f, 0.0f);

    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("FinishLine");
    dir.createDirectory();
    auto name = juce::File::createLegalFileName (presetName.isEmpty() ? juce::String ("Finish Line") : presetName);
    auto file = dir.getChildFile ("FL_" + name + "_" + juce::String ((int) std::round (host.bpm)) + "bpm.wav");
    file.deleteFile();

    juce::WavAudioFormat wav;
    if (auto stream = std::unique_ptr<juce::FileOutputStream> (file.createOutputStream()))
    {
        if (auto* writer = wav.createWriterFor (stream.get(), sr, 2, 24, {}, 0))
        {
            stream.release(); // the writer owns the stream now
            std::unique_ptr<juce::AudioFormatWriter> w (writer);
            w->writeFromAudioSampleBuffer (out, 0, length);
        }
    }

    return file;
}

//==============================================================================
juce::ValueTree FinishLineProcessor::makeState()
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("presetIndex", presetIndex, nullptr);
    state.setProperty ("uiScale", uiScale, nullptr);
    {
        const juce::ScopedLock sl (sampleLock);
        state.setProperty ("userSample", userSamplePath, nullptr);
    }
    return state;
}

void FinishLineProcessor::restoreState (const juce::ValueTree& state)
{
    if (! state.isValid())
        return;

    apvts.replaceState (state.createCopy());
    presetName = state.getProperty ("presetName", "INIT").toString();
    presetIndex = (int) state.getProperty ("presetIndex", -1);
    uiScale = juce::jlimit (0.75f, 2.0f, (float) state.getProperty ("uiScale", 1.0f));

    const auto path = state.getProperty ("userSample", "").toString();
    if (path.isNotEmpty() && juce::File::isAbsolutePath (path) && juce::File (path).existsAsFile())
    {
        const float selected = getParam (P::imp_sample);
        loadUserSample (juce::File (path));
        setParam (P::imp_sample, selected);
    }
}

void FinishLineProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = makeState().createXml())
        copyXmlToBinary (*xml, destData);
}

void FinishLineProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        restoreState (juce::ValueTree::fromXml (*xml));
}

juce::File FinishLineProcessor::getUserPresetDirectory() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("desNotto").getChildFile ("Finish Line").getChildFile ("Presets");
    dir.createDirectory();
    return dir;
}

void FinishLineProcessor::saveUserPreset (const juce::File& file)
{
    presetName = file.getFileNameWithoutExtension();
    presetIndex = -1;
    if (auto xml = makeState().createXml())
        xml->writeTo (file);
}

bool FinishLineProcessor::loadUserPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return false;

    const float scale = uiScale;
    restoreState (juce::ValueTree::fromXml (*xml));
    uiScale = scale;
    presetName = file.getFileNameWithoutExtension();
    presetIndex = -1;
    return true;
}

//==============================================================================
juce::AudioProcessorEditor* FinishLineProcessor::createEditor()
{
    return new FinishLineEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FinishLineProcessor();
}
