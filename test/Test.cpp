// Pruebas del Drum Maker (se compila con -DDDC_BUILD_TEST=ON)
//   sin variables: patrones de cada estilo + nivel de cada estilo/kit
//   MEASURE=1     : nivel de cada instrumento en cada kit (para igualar volúmenes)
//   SHOT=archivo  : captura de la interfaz en PNG (SHOTSTEP=n: paso del campamento, SHOTT=n: reloj, SHOTSTYLE=n, SHOTLANE=n)
//   WAVDIR=dir    : guarda un WAV por estilo
//   CPU=1         : tiempo de proceso con todo sonando
//   EDIT=1        : prueba las herramientas de edición
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

struct FakeHead : juce::AudioPlayHead
{
    double ppq = 0, bpm = 124; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p; p.setBpm (bpm); p.setIsPlaying (playing); p.setPpqPosition (ppq); return p;
    }
};

static void setP (DrumMakerProcessor& pr, const juce::String& id, float plain)
{
    if (auto* p = pr.apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (plain));
}

static void printPatterns()
{
    ddc::DrumGenerator g; g.rnd.setSeed (1234);
    for (auto& st : ddc::drumStyles())
    {
        std::printf ("\n=== %s ===\n", st.name);
        auto p = g.generate (st, 32, .5f);
        for (int l = 0; l < ddc::numDrums; ++l)
        {
            std::printf ("%-9s", ddc::drumNames()[l].toRawUTF8());
            const int ll = ddc::laneLength (p, l, 32);
            for (int i = 0; i < 32; ++i)
            {
                const auto& s = p.s[(size_t) l][(size_t) i];
                char c = i >= ll ? ' ' : '.';
                if (s.on()) c = s.ratchet > 1 ? 'r' : (s.vel > .85f ? 'X' : (s.vel < .4f ? 'g' : 'x'));
                if (s.on() && s.need > .5f) c = (char) std::tolower (c) == 'x' ? 'o' : c;
                std::putchar (c);
                if (i % 16 == 15) std::putchar ('|');
            }
            std::printf (" len %d\n", ll);
        }
    }
}

static void run (DrumMakerProcessor& pr, FakeHead& hd, int blocks, int bs, std::function<void (juce::AudioBuffer<float>&, juce::MidiBuffer&)> cb)
{
    for (int b = 0; b < blocks; ++b)
    {
        juce::AudioBuffer<float> buf (2, bs); buf.clear(); juce::MidiBuffer m;
        pr.processBlock (buf, m);
        cb (buf, m);
        hd.ppq += bs * hd.bpm / 60.0 / 44100.0;
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    if (auto* shot = std::getenv ("SHOT"))
    {
        DrumMakerProcessor pr;
        FakeHead hd; pr.setPlayHead (&hd);
        pr.prepareToPlay (44100, 512);
        if (auto* st = std::getenv ("SHOTSTEP"))
        {
            const int n = std::atoi (st);
            const juce::ScopedLock sl (pr.campLock);
            pr.campStep = juce::jlimit (0, 11, n);
            for (int i = 0; i < juce::jmin (12, n + 1); ++i) pr.campSeeds.add (1000 + i * 7919);
        }
        if (std::getenv ("SHOTLEN")) setP (pr, "length", (float) std::atoi (std::getenv ("SHOTLEN")));
        if (std::getenv ("SHOTSTYLE")) setP (pr, "style", (float) std::atoi (std::getenv ("SHOTSTYLE")));
        pr.generate();
        if (std::getenv ("SHOTLANE")) pr.selectedLane.store (std::atoi (std::getenv ("SHOTLANE")));
        // que suene un poco, para que se vea el paso y la gente saltando
        run (pr, hd, std::getenv ("SHOTPLAY") ? std::atoi (std::getenv ("SHOTPLAY")) : 30, 512, [] (auto&, auto&) {});
        std::unique_ptr<juce::AudioProcessorEditor> ed (pr.createEditor());
        ed->setSize (1330, 800);
        const int frames = std::getenv ("SHOTT") ? std::atoi (std::getenv ("SHOTT")) * 2 : 40;
        for (int i = 0; i < frames; ++i)
        {
            if (std::getenv ("SHOTLIVE")) run (pr, hd, 1, 512, [] (auto&, auto&) {});   // suena mientras se anima
            if (auto* rp = dynamic_cast<RadioPanel*> (ed->getChildComponent (0))) rp->tickForTest();
        }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.f);
        juce::File f (shot); f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat().writeImageToStream (img, os);
        std::printf ("captura %s\n", shot);
        return 0;
    }

    if (std::getenv ("MEASURE"))
    {
        // cada instrumento solo, negras durante 2 compases, en cada kit
        DrumMakerProcessor pr; FakeHead hd; pr.setPlayHead (&hd); pr.prepareToPlay (44100, 512);
        setP (pr, "comp", 0.f); setP (pr, "verb", 0.f); setP (pr, "humanize", 0.f); setP (pr, "swing", 0.f); setP (pr, "fills", 0.f);
        setP (pr, "length", 0.f);
        for (int k = 0; k < (int) ddc::drumKits().size(); ++k)
        {
            setP (pr, "kit", (float) k);
            std::printf ("--- %s\n", ddc::drumKits()[(size_t) k].name);
            for (int l = 0; l < ddc::numDrums; ++l)
            {
                ddc::DrumPattern p {};
                for (int i = 0; i < 16; i += 4) { auto& s = p.s[(size_t) l][(size_t) i]; s.vel = .8f; s.need = 0; s.ratchet = 1; s.prob = 1; }
                pr.setPattern (p, false);
                hd.ppq = 0;
                double acc = 0; int cnt = 0; float peak = 0;
                run (pr, hd, 200, 512, [&] (auto& buf, auto&)
                {
                    for (int i = 0; i < 512; ++i) { const float x = .5f * (buf.getSample (0, i) + buf.getSample (1, i)); acc += x * x; ++cnt; peak = std::max (peak, std::abs (x)); }
                });
                std::printf ("  %-9s rms %.4f peak %.3f\n", ddc::drumNames()[l].toRawUTF8(), std::sqrt (acc / cnt), peak);
            }
        }
        return 0;
    }

    if (std::getenv ("CPU"))
    {
        DrumMakerProcessor pr; FakeHead hd; pr.setPlayHead (&hd); pr.prepareToPlay (44100, 512);
        setP (pr, "density", 1.f);
        for (int st = 0; st < (int) ddc::drumStyles().size(); ++st)
        {
            setP (pr, "style", (float) st);
            pr.generate();
            setP (pr, "verb", .5f); setP (pr, "comp", .6f);
            hd.ppq = 0;
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            run (pr, hd, 860, 512, [] (auto&, auto&) {});   // 10 s
            const double ms = juce::Time::getMillisecondCounterHiRes() - t0;
            std::printf ("CPU %-16s %.1f %%\n", ddc::drumStyles()[(size_t) st].name, ms / 100.0);
        }
        return 0;
    }

    if (std::getenv ("EDIT"))
    {
        DrumMakerProcessor pr; pr.prepareToPlay (44100, 512);
        pr.clearPattern();
        pr.editStep (0, 0, 0);  std::printf ("poner: %d\n", (int) pr.getDrumPattern().s[0][0].on());
        pr.editStep (0, 0, 1);  std::printf ("acento: vel %.2f\n", pr.getDrumPattern().s[0][0].vel);
        pr.editStep (0, 0, 1, -1); std::printf ("fantasma: vel %.2f\n", pr.getDrumPattern().s[0][0].vel);
        pr.editStep (0, 0, 2);  std::printf ("redoble: %d\n", pr.getDrumPattern().s[0][0].ratchet);
        pr.editStep (0, 0, 3);  std::printf ("prob: %.2f\n", pr.getDrumPattern().s[0][0].prob);
        pr.editStep (0, 0, 4);  std::printf ("empuje: %d\n", pr.getDrumPattern().s[0][0].nudge);
        pr.editStep (0, 0, 4, -1); pr.editStep (0, 0, 4, -1); std::printf ("empuje atras: %d\n", pr.getDrumPattern().s[0][0].nudge);
        pr.setLaneLength (4, 12); std::printf ("largo fila 4: %d\n", pr.getDrumPattern().len[4]);
        pr.undo(); std::printf ("deshacer -> largo %d\n", pr.getDrumPattern().len[4]);
        pr.editStep (0, 0, 0);  std::printf ("quitar: %d\n", (int) pr.getDrumPattern().s[0][0].on());
        pr.generate(); pr.regenerateLane (1); pr.clearLane (2); pr.vary(); pr.regenerateSegment (0, 16);
        std::printf ("ok\n");
        return 0;
    }

    printPatterns();
    DrumMakerProcessor proc;
    FakeHead head; proc.setPlayHead (&head);
    const double sr = 44100; const int bs = 512;
    proc.prepareToPlay (sr, bs);
    const char* wavDir = std::getenv ("WAVDIR");
    for (int st = 0; st < (int) ddc::drumStyles().size(); ++st)
    {
        setP (proc, "style", (float) st);
        proc.generate();
        juce::AudioBuffer<float> all (2, (int) (sr * 4 * 60 / 124 * 8));
        all.clear();
        head.ppq = 0; int pos = 0, midiNotes = 0;
        while (pos + bs <= all.getNumSamples())
        {
            juce::AudioBuffer<float> b (2, bs); b.clear(); juce::MidiBuffer m;
            proc.processBlock (b, m);
            for (const auto ev : m) if (ev.getMessage().isNoteOn()) ++midiNotes;
            for (int c = 0; c < 2; ++c) all.copyFrom (c, pos, b, c, 0, bs);
            pos += bs; head.ppq += bs * head.bpm / 60.0 / sr;
        }
        const float peak = all.getMagnitude (0, 0, pos);
        bool nan = false; for (int i = 0; i < pos; ++i) if (! std::isfinite (all.getSample (0, i)) || ! std::isfinite (all.getSample (1, i))) nan = true;
        const auto kit = ddc::drumKits()[(size_t) (int) proc.apvts.getRawParameterValue ("kit")->load()].name;
        std::printf ("%-16s kit %-14s peak %.3f rms %.3f  midi %d %s\n", ddc::drumStyles()[(size_t) st].name, kit, peak, all.getRMSLevel (0, 0, pos), midiNotes, nan ? "NAN!" : "");
        if (wavDir != nullptr)
        {
            juce::File f (juce::String (wavDir) + "/" + juce::String (st).paddedLeft ('0', 2) + "_" + juce::String (ddc::drumStyles()[(size_t) st].name).replace (" ", "_") + ".wav");
            f.deleteFile();
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 16, {}, 0));
            w->writeFromAudioSampleBuffer (all, 0, pos);
        }
    }
    {
        juce::File mid ("/tmp/claude-0/drum_test.mid"), wv ("/tmp/claude-0/drum_test.wav");
        std::printf ("midi %d  wav %d\n", (int) proc.writeMidiFile (mid), (int) proc.renderWav (wv));
    }
    return 0;
}
