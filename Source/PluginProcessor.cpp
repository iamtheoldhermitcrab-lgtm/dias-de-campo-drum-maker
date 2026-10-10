#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace ddc;

DrumMakerProcessor::DrumMakerProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    patterns[0] = gen.generate (drumStyles()[2], 32, .4f);
    audioPatterns = patterns;
}

juce::AudioProcessorValueTreeState::ParameterLayout DrumMakerProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pct = [] (float v, int) { return String (roundToInt (v * 100.f)); };
    auto f = [&] (const String& id, const String& name, float def)
    {
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.f, 1.f), def,
                                                      AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    };
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "style", 1 }, "Estilo", drumStyleNames(), 3));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 }, "Tono", noteNames(), 9));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "fills", 1 }, "Fills", StringArray { "No", "Cada 4", "Cada 8" }, 2));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "groove", 1 }, "Groove", StringArray { "Recto", "Shuffle", "Suelto" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "length", 1 }, "Longitud",
                                                   StringArray { String::fromUTF8 ("1 comp\xc3\xa1s"), String::fromUTF8 ("2 compases"), String::fromUTF8 ("4 compases") }, 1));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "kit", 1 }, "Kit", drumKitNames(), 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "slot", 1 }, String::fromUTF8 ("Patr\xc3\xb3n"),
                                                   StringArray { "A", "B", "C", "D", "E", "F", "G", "H" }, 0));
    // cada instrumento con su volumen, paneo, color, afinación y caída
    static const float panDef[numDrums] = { .5f, .5f, .52f, .4f, .58f, .58f, .38f, .45f, .65f, .35f };
    for (int i = 0; i < numDrums; ++i)
    {
        const String n = drumNames()[i];
        f ("lvl" + String (i), "Nivel " + n, .8f);
        f ("pan" + String (i), "Paneo " + n, panDef[i]);
        f ("col" + String (i), "Color " + n, .5f);
        f ("tune" + String (i), "Tono " + n, .5f);
        f ("dec" + String (i), "Decay " + n, .5f);
    }
    f ("density", "Densidad", .5f);
    f ("variation", "Variacion", .4f);
    f ("comp", String::fromUTF8 ("Compresi\xc3\xb3n"), .35f);
    f ("fillAmt", "Fills cantidad", .5f);
    f ("verb", "Reverb", .15f);
    f ("vol", "Volumen", .8f);
    f ("swing", "Swing", .1f);
    f ("humanize", "Humano", .1f);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "moodDest", 1 }, "Mood destino",
                                                   StringArray { "Densidad", "Tono", "Decay", "Color", String::fromUTF8 ("Compresi\xc3\xb3n"), "Reverb", "Volumen" }, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "moodAmt", 1 }, "Mood", NormalisableRange<float> (-1.f, 1.f), .5f,
                                                  AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "lfoShape", 1 }, "LFO forma",
                                                   StringArray { "Seno", String::fromUTF8 ("Tri\xc3\xa1ngulo"), "Sierra", "Cuadrada", "Aleatorio" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "lfoRate", 1 }, "LFO ritmo",
                                                   StringArray { "1/16", "1/8", "1/4", "1/2", String::fromUTF8 ("1 comp\xc3\xa1s"), "2 compases", "4 compases" }, 4));
    f ("lfoAmt", "LFO", 0.f);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "chain", 1 }, "Cadena", StringArray { "No", "A-B", "A-D", "A-H" }, 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "subMono", 1 }, "Sub mono", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "lockSound", 1 }, "Mantener sonido", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "lockPattern", 1 }, String::fromUTF8 ("Mantener patr\xc3\xb3n"), false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "lockFx", 1 }, "Mantener FX", false));
    return l;
}

bool DrumMakerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void DrumMakerProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    for (int i = 0; i < numDrums; ++i) voices[(size_t) i].prepare ((float) sampleRate, i);
    for (auto& q : queue) q.active = false;
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    sendBuf.setSize (2, juce::jmax (512, samplesPerBlock), false, true, true);
    compEnv = 0.f;
}

//==============================================================================
int DrumMakerProcessor::slotForStep (long long k) const
{
    static const int lens[] = { 1, 2, 4, 8 };
    const int chainLen = lens[juce::jlimit (0, 3, choice ("chain"))];
    if (chainLen == 1) return juce::jlimit (0, 7, choice ("slot"));
    const int L = getLength();
    const long long loop = (k >= 0 ? k : k - L + 1) / L;
    return (int) (((loop % chainLen) + chainLen) % chainLen);
}

// pone en cola los golpes de un paso: redoble, empuje, groove y humanizar
void DrumMakerProcessor::queueLaneStep (int lane, const DrumStep& st, int baseWait, int stepSamples, float velMul)
{
    if (st.prob < .999f && humanRnd.nextFloat() > st.prob) return;
    const float h = value ("humanize");
    const int n = juce::jlimit (1, 4, st.ratchet);
    for (int r = 0; r < n; ++r)
    {
        int wait = baseWait + r * stepSamples / n;
        float v = st.vel * velMul * (r == 0 ? 1.f : .78f + .1f * (float) r / (float) n);
        if (h > .001f)
        {
            wait += (int) (humanRnd.nextFloat() * 8.f * h * .001f * (float) sr);
            v = juce::jlimit (.05f, 1.f, v * (1.f + (humanRnd.nextFloat() - .5f) * .3f * h));
        }
        for (auto& q : queue)
            if (! q.active) { q.active = true; q.wait = juce::jmax (0, wait); q.lane = lane; q.vel = v; break; }
    }
}

void DrumMakerProcessor::handleStep (long long k, int stepSamples, int sampleOffset)
{
    juce::ignoreUnused (sampleOffset);
    const int L = getLength();
    const int i = (int) (((k % L) + L) % L);
    playStep.store (i);
    absStep.store (k);
    const int slot = slotForStep (k);
    audioSlot = slot;
    playSlot.store (slot);
    const auto& p = audioPatterns[(size_t) slot];

    bool anySolo = false;
    for (auto& s : solo) anySolo |= s.load();

    // fill al final de cada 4 u 8 compases
    const int fillsMode = choice ("fills");
    const float fillAmt = value ("fillAmt");
    const int every = fillsMode == 1 ? 4 : 8;
    const int fillLen = fillAmt < .34f ? 4 : 8;
    const long long bar = (k >= 0 ? k : k - 15) / 16;
    const int inBar = (int) (((k % 16) + 16) % 16);
    const bool fillZone = fillsMode > 0 && fillAmt > .02f && ((bar + 1) % every == 0) && inBar >= 16 - fillLen;
    const int groove = choice ("groove");

    for (int lane = 0; lane < numDrums; ++lane)
    {
        if (mute[(size_t) lane].load() || (anySolo && ! solo[(size_t) lane].load())) continue;
        const int ll = laneLength (p, lane, L);
        // groove "suelto": cajas y palmas un pelín tarde, como un batería relajado
        const int lazy = (groove == 2 && (lane == snare || lane == clap || lane == rim)) ? stepSamples / 6 : 0;

        if (fillZone)
        {
            int rat = 1;
            const float fv = fillHit (lane, inBar - (16 - fillLen), fillLen, fillAmt, rat);
            if (fv >= 0.f)
            {
                if (fv > 0.f) { DrumStep fs; fs.vel = fv; fs.ratchet = rat; queueLaneStep (lane, fs, lazy, stepSamples, 1.f); }
                continue;
            }
        }
        // este paso (empuje hacia delante) ...
        {
            const int idx = (int) (((k % ll) + ll) % ll);
            const auto& st = p.s[(size_t) lane][(size_t) idx];
            if (drumAudible (st, densityNow) && st.nudge >= 0)
                queueLaneStep (lane, st, lazy + st.nudge * stepSamples / 12, stepSamples, 1.f);
        }
        // ... y los del paso siguiente que van adelantados
        {
            const long long k2 = k + 1;
            const int idx2 = (int) (((k2 % ll) + ll) % ll);
            const auto& st = p.s[(size_t) lane][(size_t) idx2];
            if (drumAudible (st, densityNow) && st.nudge < 0)
                queueLaneStep (lane, st, lazy + stepSamples + st.nudge * stepSamples / 12, stepSamples, 1.f);
        }
    }
}

void DrumMakerProcessor::triggerLane (int lane, float vel, int sampleOffset, bool sendMidi)
{
    const auto& kit = drumKits()[(size_t) juce::jlimit (0, (int) drumKits().size() - 1, choice ("kit"))];
    float tune = (laneValue ("tune", lane) - .5f) * 24.f + tuneMod;
    // el bombo y el tom se afinan a la nota del selector Tono
    if (lane == kick || lane == tom)
    {
        const float f = kit.p[lane].freq;
        const float m = 69.f + 12.f * std::log2 (f / 440.f);
        float d = (float) getKey() - std::fmod (m, 12.f);
        while (d > 6.f) d -= 12.f;
        while (d < -6.f) d += 12.f;
        tune += d;
    }
    const float decMul = std::pow (3.f, (laneValue ("dec", lane) - .5f) * 2.f) * decayMod;
    const float color = juce::jlimit (-1.f, 1.f, (laneValue ("col", lane) - .5f) * 2.f + colorMod);
    if (lane == closedHat) voices[openHat].choke();
    voices[(size_t) lane].trigger (vel, kit.p[lane], tune, decMul, color);
    hitCount[(size_t) lane].fetch_add (1);
    if (lane == kick) kickLevel.store (vel);
    if (sendMidi)
    {
        const int note = drumMidiNote (lane);
        if (noteOffLeft[(size_t) lane] > 0) midiOut.addEvent (juce::MidiMessage::noteOff (10, note), sampleOffset);
        midiOut.addEvent (juce::MidiMessage::noteOn (10, note, (juce::uint8) juce::jlimit (1, 127, juce::roundToInt (vel * 127.f))), sampleOffset);
        noteOffLeft[(size_t) lane] = (int) (sr * .04);
    }
}

void DrumMakerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (sendBuf.getNumSamples() < n) sendBuf.setSize (2, n, false, false, true);
    sendBuf.clear();

    {
        const juce::SpinLock::ScopedTryLockType lock (patternLock);
        if (lock.isLocked()) { audioPatterns = patterns; audioMoods = moods; }
    }

    const int slotNow = isRunning.load() ? audioSlot : juce::jlimit (0, 7, choice ("slot"));
    const auto& moodLane = audioMoods[(size_t) slotNow];
    const int moodDest = choice ("moodDest");
    const float moodAmt = value ("moodAmt"), lfoAmt = value ("lfoAmt");
    const int lfoShape = choice ("lfoShape");
    static const double lfoBeats[] = { .25, .5, 1., 2., 4., 8., 16. };
    const double lfoLen = lfoBeats[juce::jlimit (0, 6, choice ("lfoRate"))];
    const float vol = value ("vol"), comp = value ("comp"), verbK = value ("verb");
    float swing = value ("swing");
    if (choice ("groove") == 1) swing = juce::jmax (swing, .55f);   // shuffle: semicorcheas a ternario
    const bool lofi = drumKits()[(size_t) juce::jlimit (0, (int) drumKits().size() - 1, choice ("kit"))].lofi;

    double bpm = 124.0, ppq = 0.0;
    bool running = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (pos->getIsPlaying())
            {
                running = true;
                if (auto q = pos->getPpqPosition()) ppq = *q;
            }
        }
    if (! std::isfinite (bpm) || bpm < 20.0 || bpm > 999.0) bpm = 124.0;
    if (! std::isfinite (ppq) || std::abs (ppq) > 1.0e9) ppq = 0.0;
    lastBpm.store (bpm);
    if (! running && previewOn.load())
    {
        running = true;
        ppq = internalPpq;
        internalPpq += n * bpm / 60.0 / sr;
    }
    else if (! previewOn.load())
        internalPpq = 0.0;
    if (wasRunning && ! running)
    {
        playStep.store (-1); playSlot.store (-1); absStep.store (-1);
        for (auto& q : queue) q.active = false;
    }
    wasRunning = running;
    isRunning.store (running);

    // Mood: carril + LFO (se calcula una vez por bloque: la batería no necesita más)
    {
        const double beat = running ? ppq : (double) freeBeat;
        float laneV = 0.f;
        if (running)
        {
            const int Ls = getLength();
            const long long st = (long long) std::floor (beat * 4.0);
            laneV = moodLane[(size_t) (((st % Ls) + Ls) % Ls)];
        }
        float lfo = 0.f;
        if (lfoAmt > 1.0e-4f)
        {
            const double cyc = beat / lfoLen;
            const float ph = (float) (cyc - std::floor (cyc));
            switch (lfoShape)
            {
                case 1: lfo = 1.f - 4.f * std::abs (ph - .5f); break;
                case 2: lfo = 2.f * ph - 1.f; break;
                case 3: lfo = ph < .5f ? 1.f : -1.f; break;
                case 4: { juce::uint32 h = (juce::uint32) (long long) std::floor (cyc) * 2654435761u; h ^= h >> 13; h *= 0x5bd1e995u;
                          lfo = (float) ((h >> 8) & 0xffff) / 32767.5f - 1.f; break; }
                default: lfo = std::sin (juce::MathConstants<float>::twoPi * ph); break;
            }
        }
        const float target = moodAmt * (laneV + lfoAmt * lfo * .5f);
        smMod += (target - smMod) * .3f;
    }
    densityNow = value ("density");
    tuneMod = 0.f; decayMod = 1.f; colorMod = 0.f;
    float compNow = comp, verbNow = verbK, volNow = vol;
    if (std::abs (smMod) > 1.0e-4f)
    {
        auto mod = [this] (float v) { return juce::jlimit (0.f, 1.f, v + smMod); };
        switch (moodDest)
        {
            case 1: tuneMod = smMod * 12.f; break;
            case 2: decayMod = std::pow (3.f, smMod); break;
            case 3: colorMod = smMod; break;
            case 4: compNow = mod (comp); break;
            case 5: verbNow = mod (verbK); break;
            case 6: volNow = mod (vol); break;
            default: densityNow = mod (densityNow); break;
        }
    }

    // golpes pedidos desde los pads
    {
        const int req = padRequest.exchange (0);
        for (int l = 0; l < numDrums; ++l) if (req & (1 << l)) triggerLane (l, .85f, 0, false);
    }

    const double stepsPerSample = bpm / 60.0 * 4.0 / sr;
    const int stepSamples = (int) (1.0 / stepsPerSample);
    const double startPos = ppq * 4.0, endPos = startPos + n * stepsPerSample;
    struct Ev { int offset; long long k; };
    std::array<Ev, 128> evs {};
    size_t numEvs = 0;
    if (running)
    {
        const double sw = swing * .333;
        for (long long k = (long long) std::floor (startPos) - 1; k <= (long long) std::floor (endPos) + 1; ++k)
        {
            const double t = (double) k + ((k & 1) ? sw : 0.0);
            if (t >= startPos && t < endPos && numEvs < evs.size())
                evs[numEvs++] = { juce::jlimit (0, n - 1, (int) ((t - startPos) / stepsPerSample)), k };
        }
    }

    auto midiIt = midi.begin();
    size_t evIdx = 0;
    if (! smoothInit) { smVol = volNow; smComp = compNow; smVerb = verbNow; smoothInit = true; }
    const float sm = 1.f - std::exp (-1.f / (.02f * (float) sr));
    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    auto* SL = sendBuf.getWritePointer (0);
    auto* SR = sendBuf.getWritePointer (1);
    const bool subMono = value ("subMono") > .5f;
    const float hpA = std::exp (-juce::MathConstants<float>::twoPi * 120.f / (float) sr);
    // compresor del grupo: ataque 3 ms, relajación 120 ms
    const float atkC = std::exp (-1.f / (.003f * (float) sr)), relC = std::exp (-1.f / (.12f * (float) sr));
    // ganancias y envíos de cada fila (por bloque)
    std::array<float, numDrums> gL {}, gR {}, send {};
    static const float sendAmt[numDrums] = { 0.f, .6f, .7f, .5f, .25f, .45f, .5f, .6f, .35f, .6f };
    for (int l = 0; l < numDrums; ++l)
    {
        const float lv = laneValue ("lvl", l) * 1.25f, pan = laneValue ("pan", l);
        gL[(size_t) l] = lv * std::cos (pan * juce::MathConstants<float>::halfPi) * 1.4142f;
        gR[(size_t) l] = lv * std::sin (pan * juce::MathConstants<float>::halfPi) * 1.4142f;
        send[(size_t) l] = sendAmt[l];
    }
    float blockPeak = 0.f;

    for (int i = 0; i < n; ++i)
    {
        // teclado o controladora MIDI: notas GM o de C1 a A1 tocan las filas
        while (midiIt != midi.end() && (*midiIt).samplePosition <= i)
        {
            const auto m = (*midiIt).getMessage();
            if (m.isNoteOn())
            {
                int lane = -1;
                for (int l = 0; l < numDrums; ++l) if (drumMidiNote (l) == m.getNoteNumber()) lane = l;
                if (lane < 0 && m.getNoteNumber() >= 36 && m.getNoteNumber() < 36 + numDrums) lane = m.getNoteNumber() - 36;
                if (lane >= 0) triggerLane (lane, m.getFloatVelocity(), i, false);
            }
            ++midiIt;
        }
        while (evIdx < numEvs && evs[evIdx].offset <= i)
            handleStep (evs[evIdx++].k, stepSamples, i);
        for (auto& q : queue)
            if (q.active && --q.wait <= 0) { q.active = false; triggerLane (q.lane, q.vel, i, true); }
        for (int l = 0; l < numDrums; ++l)
            if (noteOffLeft[(size_t) l] > 0 && --noteOffLeft[(size_t) l] == 0)
                midiOut.addEvent (juce::MidiMessage::noteOff (10, drumMidiNote (l)), i);

        float l = 0.f, r = 0.f, sl = 0.f, sr2 = 0.f;
        for (int v = 0; v < numDrums; ++v)
        {
            auto& vc = voices[(size_t) v];
            if (! vc.isActive()) continue;
            const float x = vc.process();
            l += x * gL[(size_t) v]; r += x * gR[(size_t) v];
            sl += x * gL[(size_t) v] * send[(size_t) v]; sr2 += x * gR[(size_t) v] * send[(size_t) v];
        }
        l *= .5f; r *= .5f;

        // compresión en paralelo: pega el grupo sin quitarle pegada
        smComp += (compNow - smComp) * sm;
        {
            const float lev = juce::jmax (std::abs (l), std::abs (r));
            compEnv = lev > compEnv ? atkC * compEnv + (1.f - atkC) * lev : relC * compEnv + (1.f - relC) * lev;
            const float thr = .12f;
            const float gr = compEnv > thr ? std::pow (thr / compEnv, .75f) : 1.f;   // ratio 4:1
            const float wet = smComp * 1.6f;
            l = l + l * gr * 2.2f * wet;
            r = r + r * gr * 2.2f * wet;
            l /= 1.f + wet * .9f; r /= 1.f + wet * .9f;
        }
        // kit Lo-fi: menos bits y menos frecuencia de muestreo
        if (lofi)
        {
            if (++lofiCount >= 3) { lofiCount = 0; lofiHoldL = std::round (l * 64.f) / 64.f; lofiHoldR = std::round (r * 64.f) / 64.f; }
            l = l * .3f + lofiHoldL * .7f; r = r * .3f + lofiHoldR * .7f;
        }
        smVol += (volNow - smVol) * sm;
        l = std::tanh (l * smVol * 1.1f); r = std::tanh (r * smVol * 1.1f);

        if (subMono)
        {
            const float side = (l - r) * .5f, mid = (l + r) * .5f;
            sideHp1 = hpA * (sideHp1 + side - sideIn1); sideIn1 = side;
            sideHp2 = hpA * (sideHp2 + sideHp1 - sideIn2); sideIn2 = sideHp1;
            sideHp3 = hpA * (sideHp3 + sideHp2 - sideIn3); sideIn3 = sideHp2;
            sideHp4 = hpA * (sideHp4 + sideHp3 - sideIn4); sideIn4 = sideHp3;
            l = mid + sideHp4; r = mid - sideHp4;
        }
        if (R != nullptr) { L[i] = l; R[i] = r; } else L[i] = (l + r) * .5f;
        SL[i] = sl * .5f * smVol; SR[i] = sr2 * .5f * smVol;
        blockPeak = std::max (blockPeak, std::abs (l));
    }
    // reverb solo con lo que se manda (el bombo va seco)
    smVerb += (verbNow - smVerb) * .3f;
    if (smVerb > .005f)
    {
        juce::Reverb::Parameters rpm;
        rpm.roomSize = .6f; rpm.damping = .5f; rpm.width = 1.f;
        rpm.wetLevel = smVerb * .9f; rpm.dryLevel = 0.f;
        reverb.setParameters (rpm);
        reverb.processStereo (SL, SR, n);
        if (R != nullptr) { buffer.addFrom (0, 0, sendBuf, 0, 0, n); buffer.addFrom (1, 0, sendBuf, 1, 0, n); }
        else { buffer.addFrom (0, 0, sendBuf, 0, 0, n, .5f); buffer.addFrom (0, 0, sendBuf, 1, 0, n, .5f); }
    }
    outLevel.store (std::max (blockPeak, outLevel.load() * .8f));
    kickLevel.store (kickLevel.load() * .7f);
    beatPos.store (running ? ppq + n * bpm / 60.0 / sr : -1.0);
    freeBeat = (float) std::fmod ((double) freeBeat + n * bpm / 60.0 / sr, 64.0);
    midi.swapWith (midiOut);
    midiOut.clear();
}

//==============================================================================
void DrumMakerProcessor::setParam (const char* id, float plainValue)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
        p->endChangeGesture();
    }
}

void DrumMakerProcessor::pushUndo()
{
    undoStack.push_back ({ curSlot(), patterns[(size_t) curSlot()] });
    if (undoStack.size() > 60) undoStack.erase (undoStack.begin());
    redoStack.clear();
}

void DrumMakerProcessor::setPattern (const DrumPattern& p, bool withUndo)
{
    if (withUndo) pushUndo();
    const juce::SpinLock::ScopedLockType lock (patternLock);
    patterns[(size_t) curSlot()] = p;
}

void DrumMakerProcessor::undo()
{
    if (undoStack.empty()) return;
    auto item = undoStack.back(); undoStack.pop_back();
    redoStack.push_back ({ item.slot, patterns[(size_t) item.slot] });
    if (item.slot != curSlot()) selectSlot (item.slot);
    const juce::SpinLock::ScopedLockType lock (patternLock);
    patterns[(size_t) item.slot] = item.p;
}

void DrumMakerProcessor::redo()
{
    if (redoStack.empty()) return;
    auto item = redoStack.back(); redoStack.pop_back();
    undoStack.push_back ({ item.slot, patterns[(size_t) item.slot] });
    if (item.slot != curSlot()) selectSlot (item.slot);
    const juce::SpinLock::ScopedLockType lock (patternLock);
    patterns[(size_t) item.slot] = item.p;
}

void DrumMakerProcessor::copyCurrentToSlot (int target)
{
    target = juce::jlimit (0, 7, target);
    const auto src = patterns[(size_t) curSlot()];
    undoStack.push_back ({ target, patterns[(size_t) target] });
    redoStack.clear();
    {
        const juce::SpinLock::ScopedLockType lock (patternLock);
        patterns[(size_t) target] = src;
        moods[(size_t) target] = moods[(size_t) curSlot()];
    }
    selectSlot (target);
}

const DrumStyle& DrumMakerProcessor::currentStyle (bool pickIfAll)
{
    int si = choice ("style") - 1;
    if (si < 0)
    {
        si = 0;
        if (pickIfAll) si = gen.rnd.nextInt ((int) drumStyles().size());
        else for (size_t i = 0; i < drumStyles().size(); ++i) if (lastStyleUsed == drumStyles()[i].name) si = (int) i;
    }
    return drumStyles()[(size_t) si];
}

void DrumMakerProcessor::generate()
{
    const auto& st = currentStyle (true);
    lastStyleUsed = st.name;
    if (value ("lockPattern") < .5f)
    {
        setPattern (gen.generate (st, getLength(), value ("variation")), true);
        setParam ("swing", gen.r (st.swingMin, st.swingMax));
        setParam ("groove", (float) st.groove);
    }
    if (value ("lockSound") < .5f)
        setParam ("kit", (float) drumKitIndex (st.kits[(size_t) gen.rnd.nextInt ((int) st.kits.size())]));
    if (value ("lockFx") < .5f)
    {
        setParam ("comp", gen.r (.2f, .55f));
        setParam ("verb", gen.r (.08f, .3f));
    }
}

void DrumMakerProcessor::regenerateSegment (int start, int len)
{
    const int L = getLength();
    start = juce::jlimit (0, L - 1, start);
    len = juce::jlimit (1, L - start, len);
    const auto& st = currentStyle (false);
    auto cur = patterns[(size_t) curSlot()];
    const auto fresh = gen.generate (st, L, juce::jmax (.5f, value ("variation")));
    for (int lane = 0; lane < numDrums; ++lane)
    {
        if (lane == kick && gen.rnd.nextFloat() < .7f) continue;   // el bombo casi siempre se queda
        if (cur.len[(size_t) lane] > 0) continue;                   // las filas en otra longitud no se tocan
        for (int i = start; i < start + len; ++i) cur.s[(size_t) lane][(size_t) i] = fresh.s[(size_t) lane][(size_t) i];
    }
    setPattern (cur, true);
}

void DrumMakerProcessor::regenerateLane (int lane)
{
    lane = juce::jlimit (0, numDrums - 1, lane);
    const auto& st = currentStyle (false);
    auto cur = patterns[(size_t) curSlot()];
    for (int c = 0; c < 8; ++c)
    {
        const auto fresh = gen.generate (st, getLength(), value ("variation"));
        bool differs = false;
        for (int i = 0; i < maxSteps; ++i) differs |= fresh.s[(size_t) lane][(size_t) i].on() != cur.s[(size_t) lane][(size_t) i].on();
        if (! differs && c < 7) continue;
        cur.s[(size_t) lane] = fresh.s[(size_t) lane];
        cur.len[(size_t) lane] = fresh.len[(size_t) lane];
        break;
    }
    setPattern (cur, true);
}

void DrumMakerProcessor::vary()
{
    // mueve, quita o añade unos pocos golpes (nunca toca el bombo a negras)
    auto p = patterns[(size_t) curSlot()];
    const int L = getLength();
    for (int c = 0; c < (L > 16 ? 4 : 2); ++c)
    {
        const int lane = 1 + gen.rnd.nextInt (numDrums - 1);
        const int i = gen.rnd.nextInt (L);
        auto& st = p.s[(size_t) lane][(size_t) i];
        if (st.on()) { if (gen.rnd.nextBool()) st = DrumStep(); else st.ratchet = st.ratchet == 1 ? 2 : 1; }
        else { st.vel = .4f + gen.rnd.nextFloat() * .35f; st.need = .3f; }
    }
    setPattern (p, true);
}

void DrumMakerProcessor::clearPattern() { setPattern (DrumPattern {}, true); }

void DrumMakerProcessor::editStep (int lane, int step, int tool, int dir)
{
    if (lane < 0 || lane >= numDrums || step < 0 || step >= maxSteps) return;
    auto p = patterns[(size_t) curSlot()];
    auto& st = p.s[(size_t) lane][(size_t) step];
    switch (tool)
    {
        case 0: if (st.on()) st = DrumStep(); else { st = DrumStep(); st.vel = step % 4 == 0 ? .9f : .75f; } break;
        case 1:
            if (dir > 0) st.vel = (st.on() && st.vel > .95f) ? .75f : 1.f;    // acento (o vuelta a normal)
            else         st.vel = (st.on() && st.vel < .35f) ? .75f : .28f;    // fantasma (o vuelta a normal)
            break;
        case 2: if (! st.on()) { st.vel = .7f; st.ratchet = 2; } else st.ratchet = ((st.ratchet - 1 + dir + 4) % 4) + 1; break;
        case 3:
        {
            static const float probs[] = { 1.f, .75f, .5f, .25f };
            if (! st.on()) { st.vel = .75f; st.prob = .5f; break; }
            int k = 0; for (int q = 0; q < 4; ++q) if (std::abs (probs[q] - st.prob) < .05f) k = q;
            st.prob = probs[(k + dir + 4) % 4];
            break;
        }
        case 4: if (st.on()) st.nudge = juce::jlimit (-3, 3, st.nudge + dir); break;
        default: break;
    }
    if (st.on() && (tool == 0 || ! patterns[(size_t) curSlot()].s[(size_t) lane][(size_t) step].on())) st.need = 0.f;   // lo puesto a mano suena siempre
    setPattern (p, true);
}

void DrumMakerProcessor::paintStep (int lane, int step, bool on)
{
    if (lane < 0 || lane >= numDrums || step < 0 || step >= maxSteps) return;
    const juce::SpinLock::ScopedLockType lock (patternLock);
    auto& st = patterns[(size_t) curSlot()].s[(size_t) lane][(size_t) step];
    if (on && ! st.on()) { st = DrumStep(); st.vel = step % 4 == 0 ? .9f : .75f; }
    else if (! on) st = DrumStep();
}

void DrumMakerProcessor::setStepVelocity (int lane, int step, float v, bool withUndo)
{
    if (lane < 0 || lane >= numDrums || step < 0 || step >= maxSteps) return;
    if (withUndo) pushUndo();
    const juce::SpinLock::ScopedLockType lock (patternLock);
    auto& st = patterns[(size_t) curSlot()].s[(size_t) lane][(size_t) step];
    if (st.on()) st.vel = juce::jlimit (.05f, 1.f, v);
}

void DrumMakerProcessor::setLaneLength (int lane, int len)
{
    auto p = patterns[(size_t) curSlot()];
    p.len[(size_t) juce::jlimit (0, numDrums - 1, lane)] = juce::jlimit (0, maxSteps, len);
    setPattern (p, true);
}

void DrumMakerProcessor::clearLane (int lane)
{
    auto p = patterns[(size_t) curSlot()];
    for (auto& st : p.s[(size_t) juce::jlimit (0, numDrums - 1, lane)]) st = DrumStep();
    setPattern (p, true);
}

void DrumMakerProcessor::setMoodValue (int step, float v)
{
    if (step < 0 || step >= maxSteps) return;
    const juce::SpinLock::ScopedLockType lock (patternLock);
    moods[(size_t) curSlot()][(size_t) step] = juce::jlimit (0.f, 1.f, v);
}

//==============================================================================
namespace
{
struct RenderHead : juce::AudioPlayHead
{
    double ppq = 0, bpm = 124;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p; p.setBpm (bpm); p.setIsPlaying (true); p.setPpqPosition (ppq); return p;
    }
};
}

bool DrumMakerProcessor::renderWav (const juce::File& file) const
{
    const double rsr = 44100.0;
    DrumMakerProcessor copy;
    juce::MemoryBlock mb;
    const_cast<DrumMakerProcessor*> (this)->getStateInformation (mb);
    copy.setStateInformation (mb.getData(), (int) mb.getSize());
    for (int l = 0; l < numDrums; ++l) { copy.mute[(size_t) l].store (mute[(size_t) l].load()); copy.solo[(size_t) l].store (solo[(size_t) l].load()); }
    RenderHead head; head.bpm = juce::jlimit (40.0, 250.0, lastBpm.load());
    copy.setPlayHead (&head);
    copy.prepareToPlay (rsr, 512);
    const double beatsOut = 16.0;
    const double spBeat = 60.0 / head.bpm * rsr;
    const int pre = juce::roundToInt (4.0 * spBeat), total = juce::roundToInt (beatsOut * spBeat);
    juce::AudioBuffer<float> out (2, total);
    out.clear();
    head.ppq = -4.0;
    int done = -pre;
    while (done < total)
    {
        int bs = 512;
        if (done < 0) bs = juce::jmin (bs, -done);
        else bs = juce::jmin (bs, total - done);
        juce::AudioBuffer<float> b (2, bs);
        juce::MidiBuffer m;
        copy.processBlock (b, m);
        if (done >= 0) for (int c = 0; c < 2; ++c) out.copyFrom (c, done, b, c, 0, bs);
        done += bs;
        head.ppq += bs / spBeat;
    }
    file.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (file), rsr, 2, 24, {}, 0));
    if (w == nullptr) return false;
    return w->writeFromAudioSampleBuffer (out, 0, total);
}

juce::String DrumMakerProcessor::suggestedFileName() const
{
    const auto style = lastStyleUsed.isNotEmpty() ? lastStyleUsed.replace (" ", "") : juce::String ("drums");
    return "diasdecampo_drums_" + style + "_" + juce::String (juce::roundToInt (lastBpm.load())) + "bpm.mid";
}

// El MIDI lleva 4 compases (así se oye la polimetría), con las notas de un Drum Rack (bombo en C1...)
bool DrumMakerProcessor::writeMidiFile (const juce::File& file) const
{
    constexpr int tps = 24;
    const int L = getLength();
    const int total = 64;
    const int sw = juce::roundToInt (tps * .333 * value ("swing"));
    const auto p = patterns[(size_t) curSlot()];
    const float density = value ("density");
    bool anySolo = false;
    for (auto& s : solo) anySolo |= s.load();
    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::tempoMetaEvent (juce::roundToInt (60000000.0 / lastBpm.load())), 0);
    seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
    for (int lane = 0; lane < numDrums; ++lane)
    {
        if (mute[(size_t) lane].load() || (anySolo && ! solo[(size_t) lane].load())) continue;
        const int ll = laneLength (p, lane, L);
        for (int k = 0; k < total; ++k)
        {
            const auto& st = p.s[(size_t) lane][(size_t) (k % ll)];
            if (! drumAudible (st, density)) continue;
            for (int r = 0; r < st.ratchet; ++r)
            {
                const int on = juce::jmax (0, k * tps + ((k & 1) ? sw : 0) + st.nudge * tps / 12 + r * tps / st.ratchet);
                const int vel = juce::jlimit (1, 127, juce::roundToInt (st.vel * (r == 0 ? 1.f : .8f) * 127.f));
                seq.addEvent (juce::MidiMessage::noteOn (10, drumMidiNote (lane), (juce::uint8) vel), on);
                seq.addEvent (juce::MidiMessage::noteOff (10, drumMidiNote (lane)), on + juce::jmax (2, tps / st.ratchet - 2));
            }
        }
    }
    seq.addEvent (juce::MidiMessage::endOfTrack(), total * tps);
    seq.updateMatchedPairs();
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (96);
    mf.addTrack (seq);
    file.deleteFile();
    juce::FileOutputStream out (file);
    if (! out.openedOk()) return false;
    return mf.writeTo (out);
}

//==============================================================================
juce::File DrumMakerProcessor::userPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Dias de Campo").getChildFile ("Drum Maker Presets");
}

juce::Array<juce::File> DrumMakerProcessor::userPresets() const
{
    auto files = userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.ddcdrum");
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileNameWithoutExtension().compareIgnoreCase (b.getFileNameWithoutExtension()) < 0; });
    return files;
}

bool DrumMakerProcessor::saveUserPreset (const juce::String& rawName)
{
    const auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty()) return false;
    auto folder = userPresetFolder();
    if (! folder.isDirectory() && ! folder.createDirectory()) return false;
    juce::MemoryBlock mb;
    getStateInformation (mb);
    auto xml = getXmlFromBinary (mb.getData(), (int) mb.getSize());
    if (xml == nullptr) return false;
    for (auto* att : { "edW", "campStep", "campSeeds" }) xml->removeAttribute (att);
    xml->setAttribute ("ddcPresetName", name);
    if (! xml->writeTo (folder.getChildFile (name + ".ddcdrum"))) return false;
    currentUserPreset = name;
    return true;
}

bool DrumMakerProcessor::loadUserPreset (const juce::File& f)
{
    auto xml = juce::XmlDocument::parse (f);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType().toString())) return false;
    {
        const juce::ScopedLock sl (campLock);
        juce::StringArray seeds;
        for (int v : campSeeds) seeds.add (juce::String (v));
        xml->setAttribute ("campStep", campStep);
        xml->setAttribute ("campSeeds", seeds.joinIntoString (","));
    }
    if (editorW.load() > 0) xml->setAttribute ("edW", editorW.load());
    xml->removeAttribute ("ddcPresetName");
    juce::MemoryBlock mb;
    copyXmlToBinary (*xml, mb);
    setStateInformation (mb.getData(), (int) mb.getSize());
    currentUserPreset = f.getFileNameWithoutExtension();
    return true;
}

void DrumMakerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    for (int i = 0; i < 8; ++i)
    {
        state.setProperty ("drums" + juce::String (i), drumPatternToString (patterns[(size_t) i]), nullptr);
        state.setProperty ("mood" + juce::String (i), laneToString (moods[(size_t) i]), nullptr);
    }
    juce::String ms;
    for (int l = 0; l < numDrums; ++l) ms << (mute[(size_t) l].load() ? "1" : "0") << (solo[(size_t) l].load() ? "1" : "0");
    state.setProperty ("muteSolo", ms, nullptr);
    state.setProperty ("selLane", selectedLane.load(), nullptr);
    if (editorW.load() > 0) state.setProperty ("edW", editorW.load(), nullptr);
    {
        const juce::ScopedLock sl (campLock);
        juce::StringArray seeds;
        for (int v : campSeeds) seeds.add (juce::String (v));
        state.setProperty ("campStep", campStep, nullptr);
        state.setProperty ("campSeeds", seeds.joinIntoString (","), nullptr);
    }
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void DrumMakerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        if (tree.isValid() && tree.hasType (apvts.state.getType()))
        {
            std::array<DrumPattern, 8> loaded {};
            bool any = false;
            for (int i = 0; i < 8; ++i)
            {
                DrumPattern p {};
                if (drumPatternFromString (tree.getProperty ("drums" + juce::String (i)).toString(), p)) { loaded[(size_t) i] = p; any = true; }
                tree.removeProperty ("drums" + juce::String (i), nullptr);
            }
            std::array<Lane, 8> loadedMoods {};
            for (int i = 0; i < 8; ++i)
            {
                laneFromString (tree.getProperty ("mood" + juce::String (i)).toString(), loadedMoods[(size_t) i]);
                tree.removeProperty ("mood" + juce::String (i), nullptr);
            }
            const auto ms = tree.getProperty ("muteSolo").toString();
            if (ms.length() == numDrums * 2)
                for (int l = 0; l < numDrums; ++l) { mute[(size_t) l].store (ms[l * 2] == '1'); solo[(size_t) l].store (ms[l * 2 + 1] == '1'); }
            tree.removeProperty ("muteSolo", nullptr);
            selectedLane.store (juce::jlimit (0, numDrums - 1, (int) tree.getProperty ("selLane", 0)));
            tree.removeProperty ("selLane", nullptr);
            if (tree.hasProperty ("edW")) editorW.store ((int) tree.getProperty ("edW"));
            tree.removeProperty ("edW", nullptr);
            {
                const juce::ScopedLock sl (campLock);
                campStep = juce::jlimit (0, 11, (int) tree.getProperty ("campStep", 0));
                campSeeds.clear();
                for (auto& t : juce::StringArray::fromTokens (tree.getProperty ("campSeeds").toString(), ",", ""))
                    if (t.isNotEmpty() && campSeeds.size() < 12) campSeeds.add (t.getIntValue());
            }
            tree.removeProperty ("campStep", nullptr);
            tree.removeProperty ("campSeeds", nullptr);
            {
                const juce::SpinLock::ScopedLockType lock (patternLock);
                moods = loadedMoods;
            }
            apvts.replaceState (tree);
            if (any)
            {
                const juce::SpinLock::ScopedLockType lock (patternLock);
                patterns = loaded;
            }
        }
    }
}

juce::AudioProcessorEditor* DrumMakerProcessor::createEditor() { return new DrumMakerEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DrumMakerProcessor(); }
