#pragma once
#include <JuceHeader.h>
#include "Pattern.h"
#include "Drum.h"

class DrumMakerProcessor : public juce::AudioProcessor
{
public:
    DrumMakerProcessor();
    ~DrumMakerProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Dias de Campo Drum Maker"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

    ddc::DrumPattern getDrumPattern() const { return patterns[(size_t) curSlot()]; }
    ddc::Lane getMood() const { return moods[(size_t) curSlot()]; }
    void setMoodValue (int step, float v);

    // edición de la rejilla (tool: 0 poner/quitar, 1 acento, 2 redoble, 3 probabilidad, 4 empuje)
    void editStep (int lane, int step, int tool, int dir = 1);
    void beginPaint() { pushUndo(); }
    void paintStep (int lane, int step, bool on);
    void setStepVelocity (int lane, int step, float v, bool withUndo);
    void setLaneLength (int lane, int len);
    void clearLane (int lane);
    void regenerateLane (int lane);

    std::atomic<int> selectedLane { 0 };
    std::array<std::atomic<bool>, ddc::numDrums> mute {}, solo {};
    std::array<std::atomic<int>, ddc::numDrums> hitCount {};   // sube con cada golpe (para los pads y la escena)
    std::atomic<long long> absStep { -1 };                      // paso absoluto que suena (la polimetría lo necesita)
    std::atomic<int> padRequest { 0 };                           // golpes pedidos desde los pads (un bit por fila)
    void auditionLane (int lane) { padRequest.fetch_or (1 << juce::jlimit (0, ddc::numDrums - 1, lane)); }
    juce::String laneParamId (const char* what, int lane) const { return juce::String (what) + juce::String (lane); }

    bool renderWav (const juce::File& f) const;
    juce::String suggestedWavName() const { return suggestedFileName().upToLastOccurrenceOf (".", false, false) + ".wav"; }
    std::atomic<int> editorW { 0 };

    juce::CriticalSection campLock;
    int campStep = 0;
    juce::Array<int> campSeeds;
    int curSlot() const { return choice ("slot"); }
    void selectSlot (int s) { setParam ("slot", (float) juce::jlimit (0, 7, s)); }
    void copyCurrentToSlot (int s);
    bool slotHasNotes (int s) const
    {
        for (auto& l : patterns[(size_t) s].s) for (auto& st : l) if (st.on()) return true;
        return false;
    }
    std::atomic<int> transpose { 0 };
    std::atomic<float> outLevel { 0.f }, kickLevel { 0.f };
    std::atomic<double> beatPos { 0.0 };
    void setPattern (const ddc::DrumPattern& p, bool withUndo);
    void generate();
    void vary();
    void regenerateSegment (int start, int len);
    void clearPattern();
    void undo();
    void redo();
    bool canUndo() const { return ! undoStack.empty(); }
    bool canRedo() const { return ! redoStack.empty(); }
    bool writeMidiFile (const juce::File& f) const;

    static juce::File userPresetFolder();
    juce::Array<juce::File> userPresets() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& f);
    juce::String currentUserPreset;
    juce::String suggestedFileName() const;

    int getLength() const   { const int c = choice ("length"); return c == 2 ? 64 : (c == 1 ? 32 : 16); }
    int getKey() const      { return choice ("key"); }
    float getDensity() const { return value ("density"); }

    std::atomic<int> playStep { -1 };
    std::atomic<bool> isRunning { false };
    std::atomic<int> playSlot { -1 };
    std::atomic<bool> previewOn { false };
    std::atomic<double> lastBpm { 124.0 };
    juce::String lastStyleUsed;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    int choice (const char* id) const { return (int) apvts.getRawParameterValue (id)->load(); }
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    float laneValue (const char* what, int lane) const { return apvts.getRawParameterValue (laneParamId (what, lane))->load(); }
    void setParam (const char* id, float plainValue);
    void pushUndo();
    const ddc::DrumStyle& currentStyle (bool pickIfAll);
    void handleStep (long long k, int stepSamples, int sampleOffset);
    void queueLaneStep (int lane, const ddc::DrumStep& st, int baseWait, int stepSamples, float velMul);
    int slotForStep (long long k) const;
    void triggerLane (int lane, float vel, int sampleOffset, bool sendMidi);

    struct Hit { bool active = false; int wait = 0, lane = 0; float vel = 0; };
    std::array<Hit, 128> queue {};
    juce::Random humanRnd;

    std::array<ddc::DrumVoice, ddc::numDrums> voices;
    juce::MidiBuffer midiOut;
    std::array<int, ddc::numDrums> noteOffLeft {};

    float sideHp1 = 0, sideHp2 = 0, sideIn1 = 0, sideIn2 = 0, sideHp3 = 0, sideHp4 = 0, sideIn3 = 0, sideIn4 = 0;
    int audioSlot = 0;
    float densityNow = .5f, tuneMod = 0.f, decayMod = 1.f, colorMod = 0.f;

    std::array<ddc::DrumPattern, 8> patterns {};
    std::array<ddc::DrumPattern, 8> audioPatterns {};
    std::array<ddc::Lane, 8> moods {};
    std::array<ddc::Lane, 8> audioMoods {};
    float smMod = 0, freeBeat = 0;
    juce::SpinLock patternLock;
    struct UndoItem { int slot; ddc::DrumPattern p; };
    std::vector<UndoItem> undoStack, redoStack;
    ddc::DrumGenerator gen;

    juce::Reverb reverb;
    juce::AudioBuffer<float> sendBuf;
    float compEnv = 0.f, smVol = 0.f, smComp = 0.f, smVerb = 0.f, lofiHoldL = 0, lofiHoldR = 0;
    int lofiCount = 0;
    double sr = 44100.0;
    double internalPpq = 0.0;
    bool wasRunning = false, smoothInit = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumMakerProcessor)
};
