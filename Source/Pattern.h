#pragma once
#include <JuceHeader.h>
#include <array>
#include <map>

namespace ddc
{
constexpr int maxSteps = 64;

struct Step
{
    bool on = false;
    int n = 0;      // semitonos respecto a la tónica
    int len = 1;    // duración en semicorcheas
    bool acc = false;
    bool sl = false;
    float v = .8f;     // velocity 0..1
    float need = 0.f;  // densidad mínima para que suene (0 = siempre)
    int ch = 1;        // tipo de acorde (voicing, ver Stab.h)
    int inv = -1;      // inversión: -1 = automática (enlace de voces), 0..3 = a mano
};

using Pattern = std::array<Step, maxSteps>;
using Lane = std::array<float, maxSteps>;   // carril Mood: un valor 0..1 por paso

// rango de notas editable (semitonos respecto a la tónica): cuatro octavas
constexpr int minSemi = -19, maxSemi = 31;

// patrón que suena con la densidad actual: las notas con need > densidad se ocultan
inline Pattern effectivePattern (const Pattern& p, float density)
{
    Pattern e = p;
    for (auto& s : e) if (s.on && s.need > density + 1.0e-4f) s.on = false;
    return e;
}

//==============================================================================
inline const juce::StringArray& noteNames()
{
    static juce::StringArray n { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return n;
}

inline const juce::StringArray& scaleNames()
{
    static juce::StringArray s { juce::String::fromUTF8 ("Menor"), juce::String::fromUTF8 ("Mayor"),
                                 juce::String::fromUTF8 ("D\xc3\xb3rica"), juce::String::fromUTF8 ("Frigia"),
                                 juce::String::fromUTF8 ("Menor arm\xc3\xb3nica"), juce::String::fromUTF8 ("Pentat\xc3\xb3nica menor") };
    return s;
}

inline const std::vector<int>& scaleIntervals (int idx)
{
    static const std::vector<std::vector<int>> s {
        { 0, 2, 3, 5, 7, 8, 10 }, { 0, 2, 4, 5, 7, 9, 11 }, { 0, 2, 3, 5, 7, 9, 10 },
        { 0, 1, 3, 5, 7, 8, 10 }, { 0, 2, 3, 5, 7, 8, 11 }, { 0, 3, 5, 7, 10 } };
    return s[(size_t) juce::jlimit (0, 5, idx)];
}

inline bool inScale (int semi, int scale)
{
    const int pc = ((semi % 12) + 12) % 12;
    for (int i : scaleIntervals (scale)) if (i == pc) return true;
    return false;
}

// Filas del piano roll (de arriba abajo): todo el rango editable
inline std::vector<int> rowSemis (int scale)
{
    std::vector<int> r;
    for (int s = maxSemi; s >= minSemi; --s) if (inScale (s, scale)) r.push_back (s);
    return r;
}

// filas que usa el generador (la zona central, dos octavas)
inline std::vector<int> genRows (int scale)
{
    std::vector<int> r;
    for (int s = 14; s >= -5; --s) if (inScale (s, scale)) r.push_back (s);
    return r;
}

inline int snapToScale (int semi, int scale)
{
    if (inScale (semi, scale)) return semi;
    for (int d = 1; d < 6; ++d)
    {
        if (inScale (semi - d, scale)) return semi - d;
        if (inScale (semi + d, scale)) return semi + d;
    }
    return semi;
}

inline int baseMidi (int key, int oct) { return (key <= 4 ? 36 + key : 24 + key) + 12 * oct; }

//==============================================================================
// Presets de sonido
struct Preset
{
    const char* name;
    int engine;          // 0 = oscilador, 1 = FM de 2 operadores, 2 = FM de 4 operadores, 3 = analógico
    int wave;            // 0 seno, 1 triángulo, 2 sierra, 3 cuadrada, 4 órgano, 5 reese
    float ratio, idx, idxEnd, idxT;
    float sus, pAmt, pT;
    int transpose;
    float sub, glide, cut, reso, env, decay, drive;
    int fm = -1;         // patch FM de 4 operadores (engine 2)
    int an = -1;         // patch analógico (engine 3)
    float dly = .25f, verb = .2f;   // delay y reverb de partida
};

//==============================================================================
// Motor analógico: oscilador con sierra, pulso (con PWM), sub-oscilador y ruido,
// segundo oscilador opcional, filtro de 24 dB, paso alto, dos ADSR, LFO, chorus y deriva analógica
struct AnalogPatch
{
    int family = 0;                    // 0 = Mono (estilo SH-101), 1 = Coro (estilo Juno-106)
    float saw = 1, pulse = 0, pw = .5f, pwm = 0;
    int subMode = 0;                   // 0 = cuadrada -1 octava, 1 = cuadrada -2 octavas, 2 = pulso estrecho -2 octavas
    float noise = 0;
    float osc2 = 0, osc2Semi = 0, osc2Cents = 0;
    float hpf = 0;                     // paso alto 0..1
    float fA = .002f, fD = .3f, fS = .2f, fR = .08f, keyTrack = .3f;
    float aA = .002f, aD = .4f, aS = .8f, aR = .06f;
    bool gateAmp = false;              // volumen tipo puerta (como el modo GATE del Juno)
    float lfoRate = .5f, lfoPwm = 0, lfoPitch = 0, lfoFilter = 0;
    int chorus = 0;                    // 0 = apagado, 1 = chorus I, 2 = chorus II
    float drift = 3.f;                 // deriva de afinación en cents
    float sat = 1.f;                   // saturación a la entrada del filtro
    // --- ampliaciones de la fase 3 ---
    int osc2Wave = 0;                  // 0 = sierra, 1 = pulso
    float osc2Pw = .5f;
    float xmod = 0;                    // el oscilador 2 modula la fase del 1 (modulación cruzada / FM)
    bool sync = false;                 // el oscilador 2 se sincroniza con el 1
    float osc2EnvSemi = 0;             // la envolvente del filtro sube el tono del oscilador 2 (barridos de sync)
    bool slope12 = false;              // filtro de 12 dB en vez de 24
    float sine = 0;                    // seno en el oscilador 1
    float organ = 0;                   // órgano aditivo
    float string = 0;                  // cuerda pulsada (Karplus-Strong)
    float stringT60 = 1.5f, stringBright = .5f, thump = 0;
};

inline const std::vector<AnalogPatch>& analogPatches()
{
    static const std::vector<AnalogPatch> p {
        // --- Mono (estilo SH-101) ---
        { .family = 0, .saw = 0, .pulse = .8f, .pw = .5f, .subMode = 0, .fD = .25f, .fS = .2f, .aD = .4f, .aS = .8f },                                   // 0 Mono Bass
        { .family = 0, .saw = .8f, .subMode = 1, .fD = .18f, .fS = 0, .aD = .3f, .aS = .6f, .sat = 1.3f },                                                  // 1 Mono Rubber
        { .family = 0, .saw = 1, .fD = .2f, .fS = 0, .keyTrack = .5f, .aD = .5f, .aS = .7f, .drift = 2, .sat = 1.6f },                                      // 2 Mono Acid
        { .family = 0, .saw = 0, .pulse = .3f, .subMode = 1, .fD = .4f, .fS = .5f, .aD = .6f, .aS = .9f },                                                  // 3 Mono Sub
        { .family = 0, .saw = 1, .fD = .35f, .fS = .2f, .aD = .5f, .aS = .7f, .sat = 1.4f },                                                                // 4 Mono Resonant
        { .family = 0, .saw = 0, .pulse = .9f, .pw = .3f, .subMode = 2, .fD = .08f, .fS = 0, .aD = .12f, .aS = 0 },                                         // 5 Mono Sequence
        { .family = 0, .saw = .7f, .pulse = .4f, .pw = .5f, .fD = .1f, .fS = 0, .aD = .25f, .aS = 0 },                                                      // 6 Mono Pluck
        { .family = 0, .saw = 0, .pulse = .8f, .pw = .45f, .pwm = .2f, .fD = .3f, .fS = .3f, .gateAmp = true, .lfoRate = .5f },                              // 7 Mono House
        { .family = 0, .saw = 1, .pulse = .3f, .noise = .08f, .fD = .25f, .fS = .25f, .aD = .5f, .aS = .8f, .sat = 1.8f },                                  // 8 Mono Rave
        // --- Coro (estilo Juno-106) ---
        { .family = 1, .saw = .8f, .pulse = .5f, .pw = .5f, .fD = .35f, .fS = .3f, .gateAmp = true, .chorus = 1, .drift = 1 },                              // 9  Coro Bass
        { .family = 1, .saw = 1, .pulse = .7f, .pw = .4f, .fD = .5f, .fS = .5f, .gateAmp = true, .chorus = 1, .drift = 1, .sat = 1.3f },                    // 10 Coro Solid
        { .family = 1, .saw = 0, .pulse = 1, .pw = .3f, .fD = .2f, .fS = .1f, .aD = .4f, .aS = .6f, .chorus = 1, .drift = 1 },                             // 11 Coro Rubber
        { .family = 1, .saw = 1, .pulse = 1, .pw = .5f, .pwm = .45f, .noise = .03f, .fD = .4f, .fS = .4f, .gateAmp = true, .lfoRate = .7f, .chorus = 2, .drift = 1, .sat = 1.5f }, // 12 Coro Mega
        { .family = 1, .saw = 1, .fD = .4f, .fS = .4f, .aD = .6f, .aS = .8f, .chorus = 1, .drift = 4 },                                                     // 13 Coro Analog
        { .family = 1, .saw = 0, .pulse = .4f, .pw = .5f, .fD = .4f, .fS = .5f, .gateAmp = true, .drift = 1 },                                              // 14 Coro Sub
        { .family = 1, .saw = .15f, .hpf = 0, .fD = .5f, .fS = .5f, .aA = .005f, .aD = .6f, .aS = .7f, .chorus = 1, .drift = 1 },                          // 15 Coro Deep Sub
        // --- Ocho (estilo Jupiter-8): dos osciladores ---
        { .family = 2, .saw = 1, .osc2 = .8f, .osc2Cents = 7, .fD = .3f, .fS = .25f, .aS = .8f, .drift = 3 },                                               // 16 Ocho Bass
        { .family = 2, .saw = 1, .osc2 = .7f, .osc2Cents = -6, .fD = .4f, .fS = .4f, .aS = .8f, .drift = 4, .osc2Wave = 1, .osc2Pw = .4f },               // 17 Ocho Analog
        { .family = 2, .saw = 1, .osc2 = .7f, .osc2Semi = -12, .osc2Cents = 5, .fD = .4f, .fS = .35f, .aS = .85f, .drift = 3, .sat = 1.6f },              // 18 Ocho Ladder
        { .family = 2, .saw = 0, .pulse = .5f, .osc2 = .8f, .osc2Semi = -12, .fD = .5f, .fS = .5f, .aS = .9f, .drift = 2, .osc2Wave = 1 },                 // 19 Ocho Sub
        { .family = 2, .saw = 1, .osc2 = .8f, .osc2Cents = 10, .fD = .35f, .fS = .4f, .aS = .85f, .drift = 3, .sat = 1.4f, .xmod = .15f },                // 20 Ocho Solid
        // --- Seis (estilo Polysix): un oscilador con sub y ensemble ---
        { .family = 3, .saw = 1, .subMode = 0, .fD = .3f, .fS = .3f, .aS = .8f },                                                                          // 21 Seis Bass
        { .family = 3, .saw = 0, .pulse = .8f, .pw = .4f, .pwm = .3f, .fD = .35f, .fS = .35f, .aS = .8f, .lfoRate = .6f, .chorus = 3 },                    // 22 Seis Analog
        { .family = 3, .saw = 0, .pulse = 1, .pw = .3f, .subMode = 1, .fD = .18f, .fS = .05f, .aD = .35f, .aS = .6f },                                    // 23 Seis Rubber
        { .family = 3, .saw = .5f, .fD = .5f, .fS = .5f, .aS = .9f },                                                                                       // 24 Seis Deep
        { .family = 3, .saw = 1, .pulse = .5f, .pw = .45f, .fD = .3f, .fS = .3f, .aS = .8f, .chorus = 3 },                                                 // 25 Seis Synth
        // --- Tres (estilo JX-3P) ---
        { .family = 4, .saw = 1, .osc2 = .7f, .osc2Cents = 8, .fD = .3f, .fS = .3f, .gateAmp = true, .chorus = 1, .drift = 1, .osc2Wave = 1 },            // 26 Tres Bass
        // --- Norte (estilo Nord Lead): analógico virtual ---
        { .family = 5, .saw = 1, .osc2 = .8f, .osc2Cents = 9, .fD = .3f, .fS = .3f, .aS = .85f, .drift = 1, .sat = 1.3f },                                // 27 Norte Bass
        { .family = 5, .saw = 0, .osc2Semi = 12, .fD = .4f, .fS = .5f, .aS = .9f, .drift = 1, .xmod = .9f, .sine = 1 },                                      // 28 Norte FM
        { .family = 5, .saw = 1, .noise = .05f, .osc2 = 1, .osc2Semi = 7, .fD = .25f, .fS = .3f, .aS = .85f, .drift = 1, .sat = 1.8f, .sync = true, .osc2EnvSemi = 24 }, // 29 Norte Rave
        { .family = 5, .saw = .8f, .pulse = .5f, .fD = .08f, .fS = 0, .aD = .25f, .aS = 0, .drift = 1 },                                                   // 30 Norte Pluck
        { .family = 5, .saw = .5f, .fD = .5f, .fS = .5f, .aS = .9f, .drift = 1, .sat = 1.4f, .sine = 1.3f },                                                             // 31 Norte Deep
        { .family = 5, .saw = 1, .fD = .3f, .fS = .15f, .aS = .8f, .drift = 1, .sat = 1.8f, .slope12 = true },                                                          // 32 Norte Resonant
        { .family = 5, .saw = 0, .pulse = 1, .pw = .35f, .fD = .2f, .fS = .1f, .aD = .35f, .aS = .6f, .drift = 1 },                                       // 33 Norte Rubber
        // --- Uno (aproximación al M1 con síntesis) ---
        { .family = 6, .saw = .45f, .fD = .15f, .fS = .4f, .aD = .5f, .aS = .7f, .drift = 0, .sat = 1.5f, .organ = 1.4f },                                                // 34 Uno Bass
        { .family = 6, .saw = 0, .fD = .5f, .fS = .8f, .aA = .001f, .aD = 2.f, .aS = .9f, .aR = .12f, .drift = 0, .string = 1, .stringT60 = 4.5f, .stringBright = .35f, .thump = .5f }, // 35 Uno Finger
        { .family = 6, .saw = 0, .fD = .5f, .fS = .8f, .aA = .001f, .aD = 2.f, .aS = .9f, .aR = .1f, .drift = 0, .string = 1, .stringT60 = 5.5f, .stringBright = .7f, .thump = .2f },  // 36 Uno Guitar
        { .family = 6, .saw = 0, .fD = .2f, .fS = .5f, .gateAmp = true, .drift = 0, .organ = 1 },                                                           // 37 Uno House
        { .family = 6, .saw = .7f, .pulse = .5f, .fD = .25f, .fS = .2f, .aS = .8f, .drift = 0, .organ = .2f },                                            // 38 Uno Synth
        // ---------- stabs ----------
        { .family = 1, .saw = .7f, .pulse = .5f, .pw = .5f, .fD = .25f, .fS = .15f, .aD = .4f, .aS = .5f, .aR = .25f, .chorus = 1, .drift = 1 },              // 39 Coro Chord
        { .family = 1, .saw = 1, .pulse = .3f, .fD = .16f, .fS = .04f, .aD = .3f, .aS = .2f, .aR = .3f, .chorus = 2, .drift = 1 },                           // 40 Coro Dub
        { .family = 1, .saw = .8f, .pulse = .6f, .pwm = .3f, .fA = .02f, .fD = .6f, .fS = .5f, .aA = .01f, .aD = .8f, .aS = .7f, .aR = .5f, .lfoRate = .5f, .chorus = 2, .drift = 1 }, // 41 Coro Pad
        { .family = 2, .saw = 1, .osc2 = .8f, .osc2Cents = 8, .fA = .03f, .fD = .4f, .fS = .45f, .aS = .8f, .aR = .2f, .drift = 3 },                          // 42 Ocho Brass
        { .family = 2, .saw = 1, .osc2 = .7f, .osc2Cents = -7, .fD = .15f, .fS = .1f, .aD = .35f, .aS = .3f, .aR = .2f, .drift = 3, .sat = 1.3f },           // 43 Ocho Stab
        { .family = 3, .saw = 1, .fD = .3f, .fS = .25f, .aD = .5f, .aS = .6f, .aR = .3f, .chorus = 3 },                                                        // 44 Seis Chord
        { .family = 3, .saw = 0, .pulse = .9f, .pw = .35f, .fD = .1f, .fS = 0, .aD = .25f, .aS = 0, .aR = .2f, .chorus = 3 },                                // 45 Seis Pluck
        { .family = 5, .saw = 1, .noise = .03f, .osc2 = .9f, .osc2Cents = 12, .fD = .2f, .fS = .3f, .aS = .7f, .aR = .15f, .drift = 1, .sat = 1.8f },         // 46 Norte Rave
        { .family = 5, .saw = 1, .fD = .12f, .fS = 0, .aD = .3f, .aS = .2f, .aR = .2f, .drift = 1, .sat = 1.4f, .slope12 = true },                           // 47 Norte Filter
        { .family = 6, .saw = 0, .fD = .2f, .fS = .6f, .aR = .08f, .gateAmp = true, .drift = 0, .organ = 1 },                                                 // 48 Uno Organ
        { .family = 6, .saw = .1f, .fD = .5f, .fS = .8f, .aA = .001f, .aD = 1.5f, .aS = .8f, .aR = .3f, .drift = 0, .sat = 1.2f, .string = 1, .stringT60 = 2.5f, .stringBright = .85f, .thump = .1f }, // 49 Uno Piano
        { .family = 6, .saw = 1, .osc2 = .8f, .osc2Cents = 10, .fA = .05f, .fD = .6f, .fS = .7f, .aA = .03f, .aD = 1.f, .aS = .9f, .aR = .4f, .chorus = 3, .drift = 2 }, // 50 Uno Strings
        // ---------- pads y teclados del Chord Maker ----------
        { .family = 1, .saw = .8f, .pulse = .45f, .pwm = .35f, .fA = .4f, .fD = 1.4f, .fS = .55f, .fR = .8f, .aA = .35f, .aD = 1.5f, .aS = .9f, .aR = .9f, .lfoRate = .35f, .chorus = 2, .drift = 1 },   // 51 Coro Warm Pad
        { .family = 2, .saw = 1, .osc2 = .85f, .osc2Cents = 9, .fA = .25f, .fD = 1.2f, .fS = .65f, .fR = .6f, .aA = .28f, .aD = 1.f, .aS = .9f, .aR = .8f, .chorus = 3, .drift = 3 },              // 52 Ocho Strings
        { .family = 3, .saw = 1, .fA = .1f, .fD = .9f, .fS = .6f, .aA = .12f, .aD = 1.f, .aS = .9f, .aR = .7f, .chorus = 3 },                                                                           // 53 Seis Ensemble
        { .family = 5, .saw = 0, .osc2 = .35f, .osc2Semi = 12, .fD = 1.2f, .fS = .5f, .aA = .02f, .aD = 1.6f, .aS = .45f, .aR = 1.1f, .chorus = 1, .drift = 1, .xmod = .35f, .sine = 1 },      // 54 Norte Glass
        { .family = 6, .saw = .3f, .pulse = .6f, .pw = .3f, .pwm = .5f, .hpf = .35f, .fA = .3f, .fD = 1.f, .fS = .45f, .aA = .4f, .aD = 1.f, .aS = .9f, .aR = 1.f, .lfoRate = 4.5f, .chorus = 2, .drift = 1 }, // 55 Uno Choir
        { .family = 6, .saw = .12f, .fD = .6f, .fS = .8f, .aA = .001f, .aD = 2.5f, .aS = .7f, .aR = .45f, .drift = 0, .sat = 1.2f, .string = 1, .stringT60 = 3.5f, .stringBright = .9f, .thump = .12f } }; // 56 Uno House Piano
    return p;
}

inline juce::String presetFamily (int presetIdx);

//==============================================================================
// FM de 4 operadores al estilo TX81Z
// Operadores: A (con realimentación), B, C, D. Algoritmos tipo OPM/OPZ:
// 0: A>B>C>D   1: (A+B)>C>D   2: (A+(B>C))>D   3: ((A>B)+C)>D
// 4: (A>B)+(C>D)   5: A>(B,C,D)   6: (A>B)+C+D   7: A+B+C+D
struct FMOp
{
    float ratio, detune;     // relación de frecuencia y desafinación en cents
    int level;               // 0..99
    int wave;                // 1..8
    float a, d1, d1l, d2, r; // envolvente: ataque, caída 1, nivel tras caída 1, caída 2, relajación (segundos)
    float vel;               // sensibilidad a la velocidad 0..1
};

struct FMPatch
{
    int alg, fb;             // algoritmo 0..7, realimentación 0..7
    FMOp op[4];
    float lofi;              // grano del conversor 0..1
};

inline const std::vector<FMPatch>& fmPatches()
{
    static const std::vector<FMPatch> p {
        // 0 FM Bass: dos pilas, cuerpo redondo con un poco de brillo al principio
        { 4, 4, { { 1.f, 0, 85, 1, .001f, .30f, .35f, 1.5f, .06f, .3f },
                  { 1.f, 0, 99, 1, .001f, .90f, .80f, 3.0f, .06f, .2f },
                  { 2.f, 3, 78, 1, .001f, .12f, .10f, 0.6f, .05f, .5f },
                  { 1.f, -3, 90, 1, .001f, .70f, .65f, 2.5f, .06f, .2f } }, .2f },
        // 1 Lately Bass: ataque brillante y percusivo que se apaga rápido, cuerpo con algo de órgano
        { 4, 6, { { 1.f, 0, 90, 1, .001f, .16f, .22f, 0.9f, .05f, .5f },
                  { 1.f, 0, 99, 1, .001f, .55f, .60f, 2.0f, .06f, .3f },
                  { 3.f, 0, 86, 1, .001f, .06f, .00f, 0.0f, .03f, .7f },
                  { 1.f, 4, 92, 1, .001f, .35f, .45f, 1.6f, .06f, .3f } }, .45f },
        // 2 Solid FM Bass: cadena en serie, gordo y sostenido
        { 0, 4, { { 1.f, 0, 62, 1, .001f, .80f, .70f, 3.0f, .08f, .2f },
                  { 1.f, 0, 72, 1, .001f, .30f, .45f, 2.0f, .08f, .4f },
                  { 1.f, 2, 78, 1, .001f, .20f, .55f, 2.0f, .08f, .3f },
                  { 1.f, 0, 99, 1, .001f, .90f, .85f, 4.0f, .07f, .2f } }, .2f },
        // 3 Deep FM Bass: portadora una octava por debajo, oscuro y profundo
        { 4, 2, { { 1.f, 0, 58, 1, .002f, .60f, .40f, 2.0f, .08f, .2f },
                  { 1.f, 0, 92, 1, .002f, 1.2f, .85f, 4.0f, .09f, .1f },
                  { 2.f, 0, 50, 1, .001f, .10f, .05f, 0.8f, .06f, .4f },
                  { .5f, 0, 95, 1, .002f, 1.5f, .90f, 5.0f, .09f, .1f } }, .1f },
        // 4 Punchy FM Bass: golpe de ataque muy marcado
        { 1, 7, { { 1.f, 0, 88, 1, .001f, .05f, .05f, 0.5f, .04f, .8f },
                  { 5.f, 0, 78, 1, .001f, .025f, .00f, 0.0f, .03f, .8f },
                  { 1.f, 0, 76, 1, .001f, .12f, .30f, 1.0f, .05f, .5f },
                  { 1.f, 0, 99, 1, .001f, .40f, .60f, 2.0f, .06f, .3f } }, .3f },
        // 5 Synth Bass FM: la realimentación da una sierra, sostenido como un sinte analógico
        { 3, 7, { { 1.f, 0, 84, 1, .001f, .50f, .55f, 2.0f, .07f, .2f },
                  { 1.f, 0, 80, 1, .001f, .40f, .60f, 2.0f, .07f, .3f },
                  { 2.f, 0, 55, 1, .001f, .15f, .10f, 0.8f, .05f, .5f },
                  { 1.f, 0, 99, 1, .001f, .80f, .75f, 3.0f, .07f, .2f } }, .2f },
        // 6 Bass 7 (estilo DX7): dos pilas, una con portadora una octava abajo
        { 4, 5, { { 1.f, 0, 84, 1, .001f, .25f, .30f, 1.5f, .06f, .4f },
                  { 1.f, 0, 99, 1, .001f, .80f, .75f, 3.0f, .07f, .2f },
                  { 1.f, 7, 86, 1, .001f, .50f, .40f, 2.0f, .06f, .4f },
                  { .5f, 0, 82, 1, .001f, 1.0f, .80f, 3.5f, .07f, .2f } }, .1f },
        // 7 Bass 81 (estilo TX81Z): formas de onda alternativas y más grano
        { 0, 5, { { 1.f, 0, 70, 5, .001f, .30f, .40f, 1.5f, .06f, .4f },
                  { 1.f, 0, 76, 3, .001f, .20f, .35f, 1.5f, .06f, .5f },
                  { .5f, 0, 74, 1, .001f, .50f, .60f, 2.5f, .06f, .3f },
                  { 1.f, 0, 99, 2, .001f, .60f, .70f, 3.0f, .06f, .2f } }, .7f },
        // 8 FM piano eléctrico: púa metálica corta sobre un cuerpo redondo
        { 4, 0, { { 14.f, 0, 68, 1, .001f, .06f, 0.f, 0.f, .10f, .8f },
                  { 1.f, 0, 99, 1, .001f, 1.2f, .40f, 2.5f, .30f, .3f },
                  { 1.f, 0, 74, 1, .001f, .80f, .20f, 1.5f, .20f, .6f },
                  { 1.f, 3, 95, 1, .001f, 1.5f, .50f, 3.0f, .30f, .3f } }, .1f },
        // 9 FM stab: golpe brillante tipo metal
        { 1, 5, { { 1.f, 0, 85, 1, .001f, .15f, .25f, .8f, .10f, .5f },
                  { 3.f, 0, 72, 1, .001f, .06f, 0.f, 0.f, .05f, .7f },
                  { 1.f, 0, 90, 1, .001f, .40f, .50f, 1.5f, .15f, .3f },
                  { 1.f, 0, 99, 1, .001f, .60f, .60f, 2.0f, .15f, .2f } }, .3f },
        // 10 FM campana
        { 4, 0, { { 3.5f, 0, 80, 1, .001f, .30f, .10f, 1.5f, .30f, .5f },
                  { 1.f, 0, 99, 1, .001f, 1.5f, .30f, 3.0f, .50f, .2f },
                  { 7.f, 0, 66, 1, .001f, .10f, 0.f, 0.f, .20f, .6f },
                  { 2.f, 0, 80, 1, .001f, 1.0f, .20f, 2.0f, .40f, .3f } }, .1f },
        // 11 FM órgano: cuatro portadoras
        { 7, 0, { { .5f, 0, 85, 1, .002f, 1.f, .95f, 0.f, .06f, .1f },
                  { 1.f, 0, 99, 1, .002f, 1.f, .95f, 0.f, .06f, .1f },
                  { 2.f, 0, 82, 1, .002f, 1.f, .90f, 0.f, .06f, .1f },
                  { 3.f, 0, 72, 3, .002f, .3f, .60f, 0.f, .06f, .2f } }, .5f } };
    return p;
}

inline const std::vector<Preset>& presets()
{
    //  nombre             motor              sus    pAmt pT  tr   sub   glide cut   reso  env   decay drive  fm  an  delay reverb
    static const std::vector<Preset> p {
        { "Coro Pad",       3, 0, 0, 0, 0, 0, 1.f, 0, .02f, 0, 0.f, 0.f, .48f, .15f, .30f, .55f, .10f, -1, 41, .10f, .45f },
        { "Coro Warm Pad",  3, 0, 0, 0, 0, 0, 1.f, 0, .02f, 0, 0.f, 0.f, .42f, .10f, .35f, .50f, .10f, -1, 51, .05f, .50f },
        { "Coro Chord",     3, 0, 0, 0, 0, 0, 1.f, 0, .02f, 0, 0.f, 0.f, .45f, .20f, .45f, .50f, .15f, -1, 39, .15f, .35f },
        { "Ocho Strings",   3, 0, 0, 0, 0, 0, 1.f, 0, .02f, 0, 0.f, 0.f, .50f, .10f, .30f, .50f, .10f, -1, 52, .05f, .50f },
        { "Ocho Brass",     3, 0, 0, 0, 0, 0, 1.f, 0, .02f, 0, 0.f, 0.f, .45f, .20f, .45f, .50f, .25f, -1, 42, .10f, .35f },
        { "Seis Ensemble",  3, 0, 0, 0, 0, 0, 1.f, 0, .02f, 0, 0.f, 0.f, .50f, .10f, .35f, .50f, .10f, -1, 53, .05f, .45f },
        { "Seis Chord",     3, 0, 0, 0, 0, 0, 1.f, 0, .02f, 0, 0.f, 0.f, .44f, .20f, .45f, .50f, .15f, -1, 44, .15f, .35f },
        { "Norte Glass",    3, 0, 0, 0, 0, 0, 1.f, 0, .02f, 0, 0.f, 0.f, .60f, .10f, .30f, .50f, .10f, -1, 54, .15f, .50f },
        { "Uno Piano",      3, 0, 0, 0, 0, 0, 1.f, 0, .02f, 0, 0.f, 0.f, .75f, .00f, .15f, .50f, .15f, -1, 49, .10f, .35f },
        { "Uno House Piano",3, 0, 0, 0, 0, 0, 1.f, 0, .02f, 0, 0.f, 0.f, .80f, .00f, .15f, .50f, .15f, -1, 56, .10f, .30f },
        { "Uno Strings",    3, 0, 0, 0, 0, 0, 1.f, 0, .02f, 0, 0.f, 0.f, .50f, .10f, .30f, .50f, .10f, -1, 50, .05f, .50f },
        { "Uno Organ",      3, 0, 0, 0, 0, 0, 1.f, 0, .02f, 0, 0.f, 0.f, .60f, .10f, .25f, .50f, .25f, -1, 48, .10f, .30f },
        { "Uno Choir",      3, 0, 0, 0, 0, 0, 1.f, 0, .02f, 0, 0.f, 0.f, .55f, .05f, .25f, .50f, .05f, -1, 55, .05f, .55f },
        { "FM Rhodes",      2, 0, 0, 0, 0, 0, 1.f, 0, .02f, 0, 0.f, 0.f, .90f, .05f, .45f, .55f, .10f, 8, -1, .15f, .35f },
        { "FM Bell",        2, 0, 0, 0, 0, 0, 1.f, 0, .02f, 0, 0.f, 0.f, .92f, .05f, .50f, .55f, .10f, 10, -1, .20f, .45f },
        { "FM Organ",       2, 0, 0, 0, 0, 0, 1.f, 0, .02f, 0, 0.f, 0.f, .85f, .05f, .50f, .50f, .20f, 11, -1, .10f, .30f },
        { "Organ",          0, 4, 0, 0, 0, 0,  .80f, 0, .02f, 0, 0.f, 0.f, .65f, .05f, .10f, .60f, .20f, -1, -1, .10f, .30f },
        { "Saw Pad",        0, 5, 0, 0, 0, 0,  .85f, 0, .02f, 0, 0.f, 0.f, .40f, .10f, .25f, .70f, .10f, -1, -1, .05f, .50f } };
    return p;
}

inline juce::StringArray presetNames()
{
    juce::StringArray s;
    for (auto& p : presets()) s.add (p.name);
    return s;
}

inline juce::String presetFamily (int presetIdx)
{
    const auto& pr = presets()[(size_t) juce::jlimit (0, (int) presets().size() - 1, presetIdx)];
    if (pr.engine == 2) return "FM (estilo DX7 / TX81Z)";
    if (pr.engine == 3)
    {
        static const char* fam[] = { "Mono (estilo SH-101)", "Coro (estilo Juno-106)", "Ocho (estilo Jupiter-8)", "Seis (estilo Polysix)",
                                     "Tres (estilo JX-3P)", "Norte (estilo Nord Lead)", "Uno (aproximaci\xc3\xb3n al M1)" };
        return juce::String::fromUTF8 (fam[juce::jlimit (0, 6, analogPatches()[(size_t) pr.an].family)]);
    }
    return juce::String::fromUTF8 ("B\xc3\xa1sicos");
}

inline int presetIndex (const char* name)
{
    auto& p = presets();
    for (size_t i = 0; i < p.size(); ++i) if (juce::String (p[i].name) == name) return (int) i;
    return 0;
}

//==============================================================================
// Estilos de generación
// Cada estilo define figuras rítmicas de medio compás ('x' = nota, '-' = mantener, '.' = silencio),
// cómo elige las notas, progresiones de acordes, cuánto evita el bombo y su densidad típica.
struct Style
{
    const char* name;
    std::vector<const char*> cells;          // figuras de 8 semicorcheas
    int pitchMode;                           // 0 pedal minimal, 1 rolling, 2 minimal house, 3 deep, 4 garage, 5 bassline, 6 acid
    std::vector<std::array<int, 4>> progressions;
    float staticProb;                        // probabilidad de quedarse en un solo acorde
    bool avoidKick;                          // evita las negras donde cae el bombo
    float density;                           // notas típicas por medio compás
    float acc, slide;
    bool legato;
    std::vector<const char*> presetsFor;
    float swingMin, swingMax, driveMin, driveMax, pumpMin, pumpMax;
};

inline const std::vector<Style>& styles()
{
    static const std::vector<Style> s {
        { "Minimal",   // microhouse juguetón, mucho espacio
          { "..x...x.", "..x.x...", "...x..x.", "..x..x..", ".x....x.", "..x-..x.", "......x.", "..x.....", "...x...x", "..x...xx" },
          0, { { 0, 0, 0, 5 }, { 0, 0, -2, 0 }, { 0, 0, 0, 3 } }, .7f, false, 2.2f, .3f, .04f, false,
          { "Sub Pluck","Short Bass","Rubber Bass","FM Pluck","Pluck Bass","Punchy FM Bass","Mono Pluck","Mono Sequence","Mono Bass","Coro Sub","Norte Pluck","Seis Rubber","Uno Finger" }, .10f,.30f, 0.f,.30f, .10f,.35f },
        { "Minimal rolling",   // rominimal hipnótico: semicorcheas rodando entre bombos
          { ".xxx.xxx", "..xx.xxx", ".xx..xx.", ".xxx..xx", "..x..xxx", ".x.x.xxx", "..xx..xx", ".xx-.xxx" },
          1, { { 0, 0, 0, 5 }, { 0, 0, -2, -2 }, { 0, 0, 3, 0 } }, .75f, true, 5.f, .35f, .03f, false,
          { "Short Bass","Sub Pluck","Rubber Bass","Bouncy Bass","Mono Sequence","Mono Rubber","Mono Sub","Tres Bass","Seis Bass" }, .15f,.35f, .05f,.35f, .15f,.40f },
        { "Minimal house",   // groove contundente y sincopado
          { "..x.x.x.", "x.x...x.", "..xx..x.", "..x..xx.", "x..x..x.", "..x-.xx.", ".x.x..x.", "..x...xx", "x.x.x..x" },
          2, { { 0, 0, 5, 5 }, { 0, -2, 0, -2 }, { 0, 5, 0, 7 }, { 0, 0, 3, -2 } }, .55f, false, 3.2f, .3f, .08f, false,
          { "Mono Bass","Mono House","Coro Bass","Seis Bass","Tres Bass","Norte Bass","Punchy FM Bass","Short Bass","Ocho Bass","Uno Synth","Rubber Bass" }, .15f,.35f, .10f,.40f, .20f,.45f },
        { "Deep house",   // notas largas, acordes que se mueven, colores de séptima y novena
          { "x--..x--", "x---..x-", "..x---x.", "x--x--x-", "x-----x.", "...x--x-", "x--...x.", "x-.x-.x-" },
          3, { { 0, -4, -2, 0 }, { 0, 5, -2, 3 }, { 0, 3, 5, -2 }, { 0, -2, -4, -5 }, { 0, 5, 0, -4 } }, .1f, false, 2.3f, .15f, .1f, true,
          { "Lately Bass","FM Bass","Sub Pluck","Deep FM Bass","Bass 7","Coro Deep Sub","Coro Analog","Coro Bass","Seis Deep","Ocho Sub","Uno Finger","Norte Deep","Uno Bass" }, .10f,.25f, 0.f,.25f, .15f,.40f },
        { "UK garage",   // 2-step: huecos, síncopas y notas que saltan
          { "x..x..x.", "...x.x..", "x.....x.", "..x..x.x", "x-.x...x", "...x--x.", "x..x.x..", "x--..x.x" },
          4, { { 0, -4, -2, 0 }, { 0, 0, 5, -2 }, { 0, 3, -2, 5 } }, .3f, true, 2.8f, .3f, .12f, false,
          { "UK Garage Bass","Lately Bass","FM Bass","Rubber Bass","UK House Bass","Solid FM Bass","Bass 81","Punchy FM Bass","Synth Bass FM","Coro Solid","Coro Rubber","Mono House","Uno House","Uno Bass","Uno Synth","Ocho Solid","Norte FM" }, .45f,.70f, .20f,.50f, .30f,.60f },
        { "UK bassline",   // contratiempos que rebotan y saltos de octava
          { "..x...x.", "..xx..xx", ".xx..xx.", "..x.x.x.", "x.x.x.x.", "..x-..xx", ".x.x.x.x" },
          5, { { 0, 0, -4, -2 }, { 0, 5, 0, 7 }, { 0, 0, 3, -2 } }, .5f, true, 3.5f, .3f, .3f, false,
          { "Reese Bass","UK House Bass","Donk Bass","Bouncy Bass","Resonant Bass","Mono Rave","Coro Mega","Mono Resonant","Norte Rave","Ocho Ladder","Norte Resonant","Seis Synth" }, .25f,.50f, .35f,.70f, .30f,.60f },
        { "Acid house",   // semicorcheas con slides y acentos tipo 303
          { "xxxxxxxx", "xx.xxx.x", "x.xxx.xx", "xxx.xx.x", "x-xxx-xx", "xx.x.xxx", "x.x.xxxx" },
          6, { { 0, 0, 0, 0 } }, 1.f, false, 6.5f, .4f, .28f, false,
          { "Mono Acid","Mono Resonant","Resonant Bass","Norte Resonant","Ocho Ladder","Mono Rave" }, .05f,.25f, .30f,.60f, .10f,.30f } };
    return s;
}

inline juce::StringArray styleNames()
{
    juce::StringArray s { "Todo" };
    for (auto& st : styles()) s.add (st.name);
    return s;
}

//==============================================================================
class Generator
{
public:
    juce::Random rnd;

    float r (float a, float b) { return a + rnd.nextFloat() * (b - a); }

    int pickNote (const std::vector<int>& rows, float spread, bool rootish, const std::map<int, float>& boost)
    {
        static const std::map<int, float> W { {0,10.f},{12,3.f},{7,2.5f},{-5,2.5f},{10,1.6f},{-2,1.6f},{3,1.3f},{5,1.1f},{2,.9f},{8,.8f},{-4,.8f},{14,.6f} };
        std::vector<float> ws;
        float total = 0;
        for (int s : rows)
        {
            auto it = W.find (s);
            float w = it != W.end() ? it->second : .5f;
            if (s != 0) w *= spread;
            auto b = boost.find (s);
            if (b != boost.end()) w *= b->second;
            if (rootish && s == 0) w *= 3.f;
            ws.push_back (w);
            total += w;
        }
        float t = rnd.nextFloat() * total;
        for (size_t i = 0; i < rows.size(); ++i) { t -= ws[i]; if (t <= 0) return rows[i]; }
        return 0;
    }

    static int noteAt (const Pattern& p, int c)
    {
        for (int j = c; j >= 0; --j)
            if (p[(size_t) j].on) return (j + p[(size_t) j].len > c) ? j : -1;
        return -1;
    }

    void mutate (Pattern& p, int from, int to, int count, const std::vector<int>& rows, float spread)
    {
        for (int k = 0; k < count; ++k)
        {
            const int i = from + rnd.nextInt (to - from);
            auto& s = p[(size_t) i];
            if (s.on)
            {
                const float x = rnd.nextFloat();
                if (x < .45f) s.n = pickNote (rows, spread, false, {});
                else if (x < .6f) s.acc = ! s.acc;
                else if (x < .72f) s.sl = ! s.sl;
                else s = Step();
            }
            else if (noteAt (p, i) < 0)
            {
                s = { true, pickNote (rows, spread, false, {}), 1, rnd.nextFloat() < .25f, false };
            }
        }
    }

    //==============================================================================
    // Generador por frases: motivo -> variaciones -> frase, siguiendo acordes, y elige el mejor de muchos intentos
    Pattern generate (const Style& st, int scale, int L, float densityKnob, float variation)
    {
        Pattern best {};
        float bestScore = -1.0e9f;
        for (int c = 0; c < 48; ++c)
        {
            auto p = candidate (st, scale, L, densityKnob, variation);
            const float sc = score (p, st, L, densityKnob, variation);
            if (sc > bestScore) { bestScore = sc; best = p; }
        }
        return best;
    }

    // Genera la línea como siempre (con la Densidad actual, la misma calidad que antes) y la prepara para el dial en directo:
    // - al bajar la Densidad desaparecen primero los adornos de esa línea (al final quedan la tónica y los tiempos fuertes)
    // - al subirla aparecen notas de relleno sacadas de la propia línea (repeticiones y octavas en los huecos)
    Pattern generateLayered (const Style& st, int scale, int L, float density, float variation)
    {
        auto p = generate (st, scale, L, density, variation);
        const float d = juce::jlimit (0.f, 1.f, density);

        struct Item { int idx; float imp; };
        std::vector<Item> items;
        for (int i = 0; i < L; ++i)
        {
            auto& s = p[(size_t) i];
            if (! s.on) continue;
            float imp = 0;
            if (i % 16 == 0) imp += 5.f;
            if (i % 4 == 0) imp += 2.5f;
            else if (i % 2 == 0) imp += 1.f;
            if (((s.n % 12) + 12) % 12 == 0) imp += 2.f;
            if (st.avoidKick && i % 4 == 2) imp += 2.f;   // en estilos que esquivan el bombo, el contratiempo manda
            imp += (float) s.len * .4f + (s.acc ? .8f : 0.f) + rnd.nextFloat() * .9f;
            items.push_back ({ i, imp });
            s.v = juce::jlimit (.35f, 1.f, (i % 4 == 0 ? .85f : .74f) + (rnd.nextFloat() - .5f) * .14f);
        }
        std::sort (items.begin(), items.end(), [] (const Item& a, const Item& b) { return a.imp > b.imp; });
        const float N = (float) juce::jmax<size_t> (1, items.size());
        // todas las notas de la línea suenan con la densidad actual; el tercio más importante suena siempre
        for (size_t r = 0; r < items.size(); ++r)
            p[(size_t) items[r].idx].need = d * juce::jlimit (0.f, 1.f, 1.5f * ((float) r + .5f) / N - .5f);

        if (d > .97f || items.empty()) return p;

        // huecos libres (no tapados por una nota larga) para el relleno
        std::array<bool, maxSteps> covered {};
        for (int i = 0; i < L; ++i)
            if (p[(size_t) i].on)
                for (int k = i; k < juce::jmin (L, i + juce::jmax (1, p[(size_t) i].len)); ++k) covered[(size_t) k] = true;

        std::vector<Item> extras;
        int prev = -1;
        for (int i = 0; i < L; ++i)
        {
            if (p[(size_t) i].on) { prev = i; continue; }
            if (covered[(size_t) i] || prev < 0) continue;
            float imp = rnd.nextFloat();
            if (i % 2 == 1) imp += .6f;                        // las semicorcheas sueltas dan el "rolling" típico
            if (st.avoidKick && i % 4 == 0) imp -= 1.5f;       // no pisar el bombo
            extras.push_back ({ i, imp });
        }
        std::sort (extras.begin(), extras.end(), [] (const Item& a, const Item& b) { return a.imp > b.imp; });
        const int M = juce::jmin ((int) extras.size(), juce::jmax (1, juce::roundToInt (N * (1.f - d) * .9f)));
        for (int r = 0; r < M; ++r)
        {
            const int i = extras[(size_t) r].idx;
            int j = i - 1;
            while (j >= 0 && ! p[(size_t) j].on) --j;
            if (j < 0) continue;
            const auto& src = p[(size_t) j];
            Step s;
            s.on = true; s.len = 1; s.acc = false; s.sl = false;
            s.n = src.n;
            if (rnd.nextFloat() < .3f) s.n = src.n + (src.n >= 7 ? -12 : 12);   // a veces la octava
            if (s.n < minSemi || s.n > maxSemi) s.n = src.n;
            s.v = juce::jlimit (.35f, 1.f, .6f + (rnd.nextFloat() - .5f) * .12f);   // relleno algo más suave
            s.need = d + (1.f - d) * ((float) r + .5f) / (float) M;
            p[(size_t) i] = s;
        }
        return p;
    }

private:
    struct Half { const char* cell; std::array<int, 8> iv; }; // medio compás con sus intervalos

    // elige una figura cercana a la densidad deseada
    const char* pickCell (const Style& st, float target)
    {
        std::vector<float> w;
        float total = 0;
        for (auto* c : st.cells)
        {
            int n = 0; for (int i = 0; i < 8; ++i) n += c[i] == 'x';
            const float d = std::abs ((float) n - target);
            const float ww = std::exp (-d * 1.1f);
            w.push_back (ww); total += ww;
        }
        float t = rnd.nextFloat() * total;
        for (size_t i = 0; i < w.size(); ++i) { t -= w[i]; if (t <= 0) return st.cells[i]; }
        return st.cells.back();
    }

    // elige una nota (intervalo respecto a la raíz del acorde) según el estilo y la posición
    int pickInterval (const Style& st, int stepInBar, bool firstOfMotif, int prev)
    {
        auto choose = [this] (std::initializer_list<std::pair<int, float>> opts)
        {
            float total = 0; for (auto& o : opts) total += o.second;
            float t = rnd.nextFloat() * total;
            for (auto& o : opts) { t -= o.second; if (t <= 0) return o.first; }
            return opts.begin()->first;
        };
        const bool strong = stepInBar % 4 == 0 || firstOfMotif;
        switch (st.pitchMode)
        {
            case 0: return strong ? 0 : choose ({ { 0, 8 }, { 12, 1.2f }, { 7, 1 }, { 10, .8f }, { -5, .6f }, { 3, .4f } });
            case 1: return firstOfMotif ? 0 : choose ({ { 0, 10 }, { 12, 1.3f }, { 7, 1 }, { 3, .9f }, { 10, .7f }, { 5, .4f } });
            case 2: return strong ? choose ({ { 0, 6 }, { 12, 1 } }) : choose ({ { 0, 5 }, { 12, 2 }, { 7, 1.4f }, { 10, 1.2f }, { 3, .8f }, { 5, .6f }, { -2, .5f } });
            case 3:
            {
                if (firstOfMotif) return 0;
                // tonos del acorde con preferencia por moverse poco desde la nota anterior
                static const int tones[] = { 0, 7, 10, 3, 12, 2, 5, -5, -2 };
                static const float base[] = { 5, 3, 2, 2, 1.5f, 1, .8f, 1.2f, 1 };
                float total = 0, w[9];
                for (int i = 0; i < 9; ++i) { w[i] = base[i] * std::exp (-std::abs (tones[i] - prev) / 6.f); total += w[i]; }
                float t = rnd.nextFloat() * total;
                for (int i = 0; i < 9; ++i) { t -= w[i]; if (t <= 0) return tones[i]; }
                return 0;
            }
            case 4: return strong ? 0 : choose ({ { 0, 5 }, { 12, 1.8f }, { 7, 1.4f }, { 10, 1.4f }, { 5, .7f }, { 3, .6f } });
            case 5:
            {
                // rebote raíz / octava
                if (prev == 0 && rnd.nextFloat() < .55f) return 12;
                if (prev == 12 && rnd.nextFloat() < .6f) return 0;
                return choose ({ { 0, 5 }, { 12, 3 }, { 7, 1 }, { 10, 1 }, { -2, .5f } });
            }
            default: return firstOfMotif ? 0 : choose ({ { 0, 6 }, { 12, 2.5f }, { 3, 1.2f }, { 7, 1.4f }, { 10, 1.2f }, { 5, .8f }, { -2, .8f }, { 14, .5f } });
        }
    }

    Half makeHalf (const Style& st, float target, int halfIndex, int& prev)
    {
        Half h { pickCell (st, target), {} };
        bool first = halfIndex == 0;
        for (int i = 0; i < 8; ++i)
            if (h.cell[i] == 'x')
            {
                h.iv[(size_t) i] = pickInterval (st, halfIndex * 8 + i, first, prev);
                prev = h.iv[(size_t) i];
                first = false;
            }
        return h;
    }

    // variación de medio compás: cambia una o dos notas, o la figura
    Half varyHalf (const Style& st, const Half& src, float target, int halfIndex, int prev, float amount)
    {
        Half h = src;
        if (rnd.nextFloat() < amount * .6f) return makeHalf (st, target, halfIndex, prev);
        const int changes = 1 + (rnd.nextFloat() < amount ? 1 : 0);
        for (int c = 0; c < changes; ++c)
        {
            const int i = rnd.nextInt (8);
            if (h.cell[i] == 'x') h.iv[(size_t) i] = pickInterval (st, halfIndex * 8 + i, false, prev);
        }
        return h;
    }

    static int fold (int semi) { while (semi > 14) semi -= 12; while (semi < -5) semi += 12; return semi; }

    Pattern candidate (const Style& st, int scale, int L, float densityKnob, float variation)
    {
        const float target = st.density * (.5f + densityKnob);
        const int bars = L / 16;

        // progresión
        std::array<int, 4> prog { 0, 0, 0, 0 };
        if (rnd.nextFloat() >= st.staticProb) prog = st.progressions[(size_t) rnd.nextInt ((int) st.progressions.size())];

        // motivo de un compás (dos mitades)
        int prev = 0;
        const Half a1 = makeHalf (st, target, 0, prev);
        const Half a2 = rnd.nextFloat() < .5f - variation * .3f ? a1 : makeHalf (st, target, 1, prev);

        // forma de la frase según la variación: 0 = A, 1 = A', 2 = B
        std::array<int, 4> form;
        if (variation < .33f) form = { 0, 0, 0, 1 };
        else if (variation < .66f) form = { 0, 1, 0, 2 };
        else form = { 0, 2, 1, 2 };
        if (bars == 2) form = { 0, variation < .33f ? 1 : 2, 0, 0 };

        Pattern p {};
        for (int b = 0; b < bars; ++b)
        {
            Half h1 = a1, h2 = a2;
            if (form[(size_t) b] == 1) h2 = varyHalf (st, a2, target, 1, prev, variation);
            if (form[(size_t) b] == 2) { h1 = varyHalf (st, a1, target, 0, prev, variation * .5f); h2 = makeHalf (st, target, 1, prev); }
            const int root = prog[(size_t) (bars == 4 ? b : (bars == 2 ? b * 2 : 0))];
            for (int half = 0; half < 2; ++half)
            {
                const auto& h = half == 0 ? h1 : h2;
                for (int i = 0; i < 8; ++i)
                {
                    if (h.cell[i] != 'x') continue;
                    const int step = b * 16 + half * 8 + i;
                    int len = 1;
                    while (i + len < 8 && h.cell[i + len] == '-') ++len;
                    p[(size_t) step] = { true, fold (snapToScale (root + h.iv[(size_t) i], scale)), juce::jmin (4, len), false, false };
                }
            }
        }

        // nota de paso hacia el siguiente acorde
        if (bars > 1)
            for (int b = 0; b < bars; ++b)
            {
                const int nextRoot = prog[(size_t) (bars == 4 ? (b + 1) % 4 : ((b + 1) % 2) * 2)];
                if (st.pitchMode == 0 || st.pitchMode == 1 || rnd.nextFloat() > .6f) continue;
                for (int s = b * 16 + 15; s >= b * 16 + 12; --s)
                    if (p[(size_t) s].on)
                    {
                        const int dir = rnd.nextBool() ? 1 : -1;
                        int t = nextRoot + dir * 2;
                        p[(size_t) s].n = fold (snapToScale (t, scale));
                        break;
                    }
            }

        // duración, acentos y slides según el estilo
        for (int i = 0; i < L; ++i)
        {
            auto& s = p[(size_t) i];
            if (! s.on) continue;
            int gap = 1; while (i + gap < L && ! p[(size_t) (i + gap)].on && gap < 4) ++gap;
            if (st.legato && s.len < gap && rnd.nextFloat() < .7f) s.len = gap;
            s.len = juce::jmin (s.len, gap);
            const bool offbeat = i % 4 != 0;
            s.acc = rnd.nextFloat() < st.acc * (offbeat ? 1.2f : .6f);
            const int nx = i + s.len;
            if (nx < L && p[(size_t) nx].on)
            {
                const int jump = std::abs (p[(size_t) nx].n - s.n);
                float sl = st.slide;
                if (st.pitchMode == 5 && jump == 12) sl *= 2.f;
                s.sl = jump > 0 && rnd.nextFloat() < sl;
            }
        }
        return p;
    }

    float score (const Pattern& p, const Style& st, int L, float densityKnob, float variation)
    {
        float sc = 0;
        int notes = 0, onKick = 0, bigLeaps = 0, offbeats = 0, prevN = 0;
        bool havePrev = false;
        for (int i = 0; i < L; ++i)
        {
            const auto& s = p[(size_t) i];
            if (! s.on) continue;
            ++notes;
            if (i % 4 == 0) ++onKick;
            if (i % 2 == 1) ++offbeats;
            if (havePrev)
            {
                const int j = std::abs (s.n - prevN);
                if (j > 7 && ! (j == 12 && (st.pitchMode == 5 || st.pitchMode == 2 || st.pitchMode == 6))) ++bigLeaps;
            }
            prevN = s.n; havePrev = true;
        }
        const float target = st.density * (.5f + densityKnob) * (L / 8.f);
        sc -= std::abs ((float) notes - target) * 1.5f;
        sc -= (float) bigLeaps * 2.f;
        if (st.avoidKick) sc -= (float) onKick * 1.2f;
        // en los estilos que no evitan el bombo, la tónica al principio ayuda
        if (! st.avoidKick && p[0].on && p[0].n == 0) sc += 3.f;
        // algo de síncopa (semicorcheas pares) da groove al house
        sc += juce::jmin (3.f, (float) offbeats * .4f) * (st.pitchMode == 3 ? .3f : 1.f);
        // la tónica debe aparecer con frecuencia
        int roots = 0; for (int i = 0; i < L; ++i) if (p[(size_t) i].on && ((p[(size_t) i].n % 12) + 12) % 12 == 0) ++roots;
        sc += juce::jmin (4.f, roots * .5f);
        // pequeño factor aleatorio para no repetir siempre la misma solución
        sc += rnd.nextFloat() * (1.f + variation * 2.f);
        return sc;
    }
};

//==============================================================================
inline juce::String patternToString (const Pattern& p)
{
    juce::StringArray parts;
    for (auto& s : p)
        parts.add (juce::String (s.on ? 1 : 0) + "," + juce::String (s.n) + "," + juce::String (s.len) + ","
                   + juce::String (s.acc ? 1 : 0) + "," + juce::String (s.sl ? 1 : 0) + ","
                   + juce::String (juce::roundToInt (s.v * 1000.f)) + "," + juce::String (juce::roundToInt (s.need * 1000.f)) + ","
                   + juce::String (s.ch) + "," + juce::String (s.inv));
    return parts.joinIntoString (";");
}

inline bool patternFromString (const juce::String& str, Pattern& out)
{
    auto parts = juce::StringArray::fromTokens (str, ";", "");
    if (parts.size() != 32 && parts.size() != maxSteps) return false;   // acepta proyectos guardados con 2 compases
    Pattern p {};
    for (int i = 0; i < parts.size(); ++i)
    {
        auto f = juce::StringArray::fromTokens (parts[i], ",", "");
        if (f.size() < 5 || f.size() > 9 || f.size() == 6) return false;
        Step st;
        st.on = f[0].getIntValue() != 0; st.n = juce::jlimit (minSemi, maxSemi, f[1].getIntValue());
        st.len = juce::jlimit (1, maxSteps, f[2].getIntValue()); st.acc = f[3].getIntValue() != 0; st.sl = f[4].getIntValue() != 0;
        if (f.size() >= 8) st.ch = juce::jlimit (0, 7, f[7].getIntValue());
        if (f.size() >= 9) st.inv = juce::jlimit (-1, 3, f[8].getIntValue());
        if (f.size() >= 7) { st.v = juce::jlimit (0.f, 1.f, f[5].getIntValue() / 1000.f); st.need = juce::jlimit (0.f, 1.f, f[6].getIntValue() / 1000.f); }
        p[(size_t) i] = st;
    }
    out = p;
    return true;
}
inline juce::String laneToString (const Lane& l)
{
    juce::StringArray parts;
    for (float v : l) parts.add (juce::String (juce::roundToInt (v * 1000.f)));
    return parts.joinIntoString (",");
}

inline bool laneFromString (const juce::String& str, Lane& out)
{
    auto parts = juce::StringArray::fromTokens (str, ",", "");
    if (parts.size() != maxSteps) return false;
    for (int i = 0; i < maxSteps; ++i) out[(size_t) i] = juce::jlimit (0.f, 1.f, parts[i].getIntValue() / 1000.f);
    return true;
}
} // namespace ddc
