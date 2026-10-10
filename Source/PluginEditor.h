#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Look.h"
#include "LogoData.h"

// Rejilla de pasos de la batería (una fila por instrumento). Se llama PianoRoll para compartir el resto del panel.
class PianoRoll : public juce::Component
{
public:
    PianoRoll (DrumMakerProcessor& p, int& toolRef) : proc (p), tool (toolRef) { setOpaque (true); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void followNotes() {}
    std::function<void()> onLaneSelected;

private:
    juce::Rectangle<int> grid() const { return getLocalBounds().withTrimmedLeft (78).withTrimmedBottom (18).withTrimmedRight (6).withTrimmedTop (2); }
    bool cellAt (juce::Point<int> pos, int& lane, int& step) const;
    int laneAtLabel (juce::Point<int> pos) const;
    int segmentLength() const { return proc.getLength() <= 16 ? 8 : 16; }
    juce::Rectangle<int> diceRect (int segStart) const;
    int segmentAt (juce::Point<int> pos) const;
    void laneMenu (int lane);
    int hotSegment = -1;
    bool painting = false, paintOn = true;
    int lastPaintLane = -1, lastPaintStep = -1;
    DrumMakerProcessor& proc;
    int& tool;
};

// Carril bajo la rejilla: fuerza de cada golpe del instrumento elegido, o el secuenciador Mood
class LaneEditor : public juce::Component
{
public:
    explicit LaneEditor (DrumMakerProcessor& p) : proc (p) { setOpaque (true); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    int mode = 0;   // 0 = velocity, 1 = mood

private:
    juce::Rectangle<int> grid() const { return getLocalBounds().withTrimmedLeft (78).withTrimmedRight (6).reduced (0, 4); }
    void apply (juce::Point<int> pos, bool erase);
    DrumMakerProcessor& proc;
    int lastCol = -1;
    float lastVal = 0;
    bool undoDone = false;
};

// Botón para arrastrar el MIDI o el WAV a Ableton (clic sin arrastrar: guardar archivo)
class ExportDragArea : public juce::Component, public juce::SettableTooltipClient
{
public:
    ExportDragArea (DrumMakerProcessor& p, bool isWav) : proc (p), wav (isWav) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); }

private:
    bool writeTo (const juce::File& f) const { return wav ? proc.renderWav (f) : proc.writeMidiFile (f); }
    juce::String fileName() const { return wav ? proc.suggestedWavName() : proc.suggestedFileName(); }
    DrumMakerProcessor& proc;
    bool wav;
    bool dragging = false, didDrag = false, down = false;
    std::unique_ptr<juce::FileChooser> chooser;
};

// Pads de batería: uno por instrumento, se iluminan al sonar. Clic: suena y se elige; M y S: silenciar y solo.
class ChordPads : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ChordPads (DrumMakerProcessor& p) : proc (p) { setOpaque (true); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void update();
    std::function<void()> onLaneSelected;
    static constexpr int numPads = ddc::numDrums;
private:
    juce::Rectangle<int> padRect (int i) const;
    juce::Rectangle<int> muteRect (int i) const { auto r = padRect (i); return { r.getRight() - 25, r.getY() + 6, 10, 10 }; }
    juce::Rectangle<int> soloRect (int i) const { auto r = padRect (i); return { r.getRight() - 13, r.getY() + 6, 10, 10 }; }
    DrumMakerProcessor& proc;
    std::array<int, ddc::numDrums> lastHits {}, glow {};
    int lastSel = -1, lastMs = -1;
};

// Piloto que parpadea en cada tiempo
class TempoLed : public juce::Component
{
public:
    explicit TempoLed (DrumMakerProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;
    void update() { const bool o = isOn(); if (o != lastOn) { lastOn = o; repaint(); } }
private:
    bool isOn() const { const double b = proc.beatPos.load(); return b >= 0 && (b - std::floor (b)) < .14; }
    DrumMakerProcessor& proc;
    bool lastOn = false;
};

// Selector de presets del usuario: vuelve a leer la carpeta justo antes de abrirse
class PresetCombo : public juce::ComboBox
{
public:
    std::function<void()> onBeforePopup;
    void showPopup() override { if (onBeforePopup) onBeforePopup(); juce::ComboBox::showPopup(); }
};

// Ventanita para poner nombre al preset (dentro del propio plugin, así nunca queda escondida detrás de Ableton)
class SaveDialog : public juce::Component
{
public:
    SaveDialog();
    void paint (juce::Graphics&) override;
    void resized() override;
    void open (const juce::String& suggestion);
    void close() { setVisible (false); }
    std::function<void (const juce::String&)> onSave;
    juce::Rectangle<int> box() const { return getLocalBounds().withSizeKeepingCentre (440, 180); }
private:
    juce::TextEditor name;
    juce::TextButton ok { "GUARDAR" }, cancel { "CANCELAR" };
};

// Toda la radio, dibujada a tamaño fijo; el editor la escala al tamaño de la ventana
class RadioPanel : public juce::Component, private juce::Timer
{
public:
    explicit RadioPanel (DrumMakerProcessor&);
    ~RadioPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void paintStatic (juce::Graphics&);

    static constexpr int baseW = 1330, baseH = 800;
    void tickForTest() { timerCallback(); }   // solo para el programa de pruebas

private:
    void timerCallback() override;
    void addCombo (juce::ComboBox& box, const char* paramId, const juce::String& label, bool lane = false);
    void setLaneMode (int m);
    void advanceCamp();
    void rebuildCrowd (int newestBorn);
    void redrawBadge();
    void afterPatternChange() { roll.followNotes(); roll.repaint(); lane.repaint(); }

    DrumMakerProcessor& proc;
    ddc::PixelLook look;
    int tool = 0;

    juce::TextButton previewBtn { "PREVIEW" }, genBtn { "GENERAR" }, varyBtn { "Variar" },
                     undoBtn, redoBtn, clearBtn { "Limpiar" };
    juce::TooltipWindow tooltips { this, 600 };
    juce::TextButton toolBtns[5];
    juce::TextButton lockBtns[3];
    juce::TextButton slotBtns[8];
    juce::TextButton laneTabs[2];
    int lastPresetSeen = -1;   // último sonido que vino del procesador (no del selector)
    juce::Rectangle<int> slotR;
    juce::ComboBox styleBox, keyBox, fillsBox, modeBox, lenBox, presetBox;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> laneAtt;
    void attachLaneKnobs();
    int attachedLane = -1;
    int lastKickHits = 0;
    juce::ComboBox moodDestBox, lfoShapeBox, lfoRateBox, chainBox;
    juce::TextButton subMonoBtn;
    int lastPlaySlot = -2;
    PresetCombo userBox;
    juce::TextButton saveBtn { "GUARDAR" };
    SaveDialog saveDlg;
    juce::Array<juce::File> userFiles;
    void refreshUserPresets();
    juce::Slider knobs[13];
    juce::Slider moodKnob, lfoKnob;
    juce::StringArray comboLabels;
    std::vector<juce::ComboBox*> combos;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SA>> sliderAtt;
    std::vector<std::unique_ptr<CA>> comboAtt;
    std::vector<std::unique_ptr<BA>> buttonAtt;

    PianoRoll roll;
    LaneEditor lane;
    ChordPads pads;
    TempoLed led;
    ExportDragArea dragMidi, dragWav;

    juce::Image badge, logoImg;
    int frame = 0, ticks = 0, lastSlot = -1, sceneT = 0;
    float sceneLevel = 0.f, todShown = 0.f;
    int campStepShown = 0;
    std::vector<ddc::Camper> crowd;
    juce::Image bg;            // fondo estático ya dibujado
    float bgScale = 0.f;
    int bgLaneMode = -1, lastPlayStep = -2;
    float lastDensity = -1.f;
    ddc::Actor actor;
    juce::Random rng;

    juce::Rectangle<int> bodyR, statusR, logoR, badgeR, selR, lockR, toolR, rollR, laneHeadR, laneR, knobR, speakL, modR, plaqueR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadioPanel)
};

class DrumMakerEditor : public juce::AudioProcessorEditor
{
public:
    explicit DrumMakerEditor (DrumMakerProcessor&);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (ddc::col::outline); }

private:
    DrumMakerProcessor& proc;
    RadioPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumMakerEditor)
};
