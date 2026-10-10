#pragma once
#include <JuceHeader.h>
#include <cmath>

namespace ddc
{
//==============================================================================
// DRUM MAKER: baterías sintetizadas (sin samples), al estilo de las cajas de ritmo clásicas
//==============================================================================
constexpr int numDrums = 10;
enum DrumId { kick, snare, clap, rim, closedHat, openHat, ride, tom, shaker, perc };

inline const juce::StringArray& drumNames()
{
    static juce::StringArray n { "BOMBO", "CAJA", "PALMAS", "RIM", "CH CERR", "CH ABIER", "RIDE", "TOM", "SHAKER", "PERC" };
    return n;
}
// notas MIDI estándar (General MIDI): así el MIDI exportado funciona directamente en un Drum Rack de Ableton
inline int drumMidiNote (int lane)
{
    static const int n[numDrums] = { 36, 38, 39, 37, 42, 46, 51, 45, 70, 56 };
    return n[juce::jlimit (0, numDrums - 1, lane)];
}

// Cómo suena cada instrumento en un kit
struct DrumPatch
{
    float freq;      // frecuencia base (Hz)
    float decay;     // caída principal (s)
    float pitch;     // barrido de tono: cuántas veces más agudo empieza (1 = nada)
    float pitchT;    // tiempo del barrido (s)
    float noise;     // cantidad de ruido
    float tone;      // frecuencia del filtro del ruido o del metal (Hz)
    float click;     // golpe del principio
    float drive;     // saturación
    float level;     // volumen
    int variant;     // variante (percusión: 0 cencerro, 1 conga, 2 clave, 3 bloque)
};

struct DrumKit { const char* name; DrumPatch p[numDrums]; bool lofi; };

inline const std::vector<DrumKit>& drumKits()
{
    //            freq   decay pitch pitchT noise tone   click drive level var
    static const std::vector<DrumKit> k {
        { "Nueve 909", {
            { 52.f,  .42f, 4.2f, .028f, 0.f,  0.f,    .55f, 1.6f, 1.00f, 0 },   // bombo: barrido rápido y golpe marcado
            { 185.f, .14f, 1.25f,.02f,  .75f, 4200.f, .2f,  1.2f, .80f, 0 },    // caja
            { 0.f,   .22f, 1.f,  0.f,   1.f,  1150.f, 0.f,  1.f,  .85f, 0 },    // palmas
            { 1650.f,.035f,1.f,  0.f,   .2f,  3000.f, 0.f,  1.f,  .55f, 0 },    // rim
            { 1.f,   .055f,1.f,  0.f,   .45f, 9000.f, 0.f,  1.f, 0.51f, 0 },    // charles cerrado (metal + ruido, como la 909)
            { 1.f,   .38f, 1.f,  0.f,   .45f, 8500.f, 0.f,  1.f, 0.49f, 0 },    // charles abierto
            { 1.6f,  1.1f, 1.f,  0.f,   .15f, 6000.f, 0.f,  1.f, 0.46f, 0 },    // ride
            { 130.f, .28f, 1.7f, .06f,  .05f, 0.f,    .2f,  1.1f, .70f, 0 },    // tom
            { 0.f,   .07f, 1.f,  0.f,   1.f,  7000.f, 0.f,  1.f,  .38f, 0 },    // shaker
            { 540.f, .22f, 1.f,  0.f,   0.f,  2200.f, 0.f,  1.f,  .40f, 0 } }, false },   // cencerro
        { "Ocho-Ocho 808", {
            { 46.f,  .85f, 2.2f, .045f, 0.f,  0.f,    .25f, 1.15f,1.00f, 0 },   // bombo largo y redondo
            { 165.f, .16f, 1.15f,.015f, .55f, 3500.f, .1f,  1.f,  .75f, 0 },
            { 0.f,   .26f, 1.f,  0.f,   1.f,  1000.f, 0.f,  1.f,  .85f, 0 },
            { 1750.f,.03f, 1.f,  0.f,   0.f,  3000.f, 0.f,  1.f,  .55f, 0 },
            { 1.f,   .045f,1.f,  0.f,   0.f,  8000.f, 0.f,  1.f, 0.80f, 0 },    // metal puro, como la 808
            { 1.f,   .45f, 1.f,  0.f,   0.f,  7500.f, 0.f,  1.f, 0.88f, 0 },
            { 1.45f, .9f,  1.f,  0.f,   0.f,  5000.f, 0.f,  1.f, 0.42f, 0 },
            { 110.f, .32f, 1.35f,.08f,  0.f,  0.f,    .1f,  1.f,  .70f, 1 },
            { 0.f,   .06f, 1.f,  0.f,   1.f,  6500.f, 0.f,  1.f,  .38f, 0 },
            { 540.f, .28f, 1.f,  0.f,   0.f,  2000.f, 0.f,  1.f,  .42f, 0 } }, false },
        { "Siete", {
            { 58.f,  .3f,  3.f,  .02f,  0.f,  0.f,    .45f, 1.4f, 1.00f, 0 },   // más seco, ochentero
            { 200.f, .12f, 1.2f, .015f, .85f, 5000.f, .25f, 1.2f, .78f, 0 },
            { 0.f,   .18f, 1.f,  0.f,   1.f,  1300.f, 0.f,  1.f,  .85f, 0 },
            { 1550.f,.03f, 1.f,  0.f,   .3f,  3500.f, 0.f,  1.f,  .55f, 0 },
            { 1.15f, .05f, 1.f,  0.f,   .7f,  10000.f,0.f,  1.f, 0.44f, 0 },
            { 1.15f, .3f,  1.f,  0.f,   .7f,  9500.f, 0.f,  1.f, 0.33f, 0 },
            { 1.7f,  .8f,  1.f,  0.f,   .3f,  7000.f, 0.f,  1.f, 0.36f, 0 },
            { 150.f, .22f, 1.5f, .05f,  .1f,  0.f,    .2f,  1.f,  .68f, 0 },
            { 0.f,   .065f,1.f,  0.f,   1.f,  8000.f, 0.f,  1.f,  .38f, 0 },
            { 2500.f,.04f, 1.f,  0.f,   0.f,  3000.f, 0.f,  1.f,  .40f, 2 } }, false },   // clave
        { "Minimal", {
            { 55.f,  .26f, 3.5f, .012f, 0.f,  0.f,    .8f,  1.3f, 1.00f, 0 },   // bombo corto con clic
            { 230.f, .08f, 1.1f, .01f,  .6f,  6000.f, .3f,  1.f,  .70f, 0 },
            { 0.f,   .12f, 1.f,  0.f,   1.f,  1500.f, 0.f,  1.f,  .80f, 0 },
            { 1900.f,.022f,1.f,  0.f,   .15f, 4000.f, 0.f,  1.f,  .60f, 0 },
            { 1.2f,  .03f, 1.f,  0.f,   .3f,  11000.f,0.f,  1.f, 0.71f, 0 },
            { 1.2f,  .2f,  1.f,  0.f,   .3f,  10000.f,0.f,  1.f, 0.69f, 0 },
            { 1.9f,  .6f,  1.f,  0.f,   .1f,  8000.f, 0.f,  1.f, 0.45f, 0 },
            { 180.f, .12f, 1.4f, .03f,  0.f,  0.f,    .3f,  1.f,  .62f, 0 },
            { 0.f,   .045f,1.f,  0.f,   1.f,  9000.f, 0.f,  1.f,  .40f, 0 },
            { 900.f, .06f, 1.f,  0.f,   .2f,  2500.f, 0.f,  1.f,  .45f, 3 } }, false },   // bloque de madera
        { "Garage", {
            { 50.f,  .5f,  3.2f, .03f,  0.f,  0.f,    .4f,  1.5f, 1.00f, 0 },
            { 190.f, .18f, 1.3f, .02f,  .7f,  4500.f, .2f,  1.3f, .85f, 0 },   // caja con cuerpo
            { 0.f,   .24f, 1.f,  0.f,   1.f,  1250.f, 0.f,  1.f,  .85f, 0 },
            { 1700.f,.04f, 1.f,  0.f,   .25f, 3200.f, 0.f,  1.f,  .60f, 0 },
            { 1.1f,  .05f, 1.f,  0.f,   .6f,  10500.f,0.f,  1.f, 0.44f, 0 },   // charles brillantes
            { 1.1f,  .32f, 1.f,  0.f,   .6f,  10000.f,0.f,  1.f, 0.39f, 0 },
            { 1.6f,  1.f,  1.f,  0.f,   .2f,  6500.f, 0.f,  1.f, 0.46f, 0 },
            { 140.f, .25f, 1.6f, .05f,  .05f, 0.f,    .2f,  1.f,  .66f, 0 },
            { 0.f,   .08f, 1.f,  0.f,   1.f,  7500.f, 0.f,  1.f,  .40f, 0 },
            { 340.f, .18f, 1.2f, .02f,  0.f,  0.f,    .1f,  1.f,  .50f, 1 } }, false },   // conga
        { "Lo-fi", {
            { 50.f,  .45f, 3.f,  .03f,  0.f,  0.f,    .4f,  2.2f, 1.00f, 0 },
            { 175.f, .15f, 1.2f, .02f,  .7f,  3000.f, .2f,  1.8f, .80f, 0 },
            { 0.f,   .2f,  1.f,  0.f,   1.f,  1000.f, 0.f,  1.f,  .85f, 0 },
            { 1500.f,.04f, 1.f,  0.f,   .2f,  2500.f, 0.f,  1.f,  .55f, 0 },
            { 1.f,   .06f, 1.f,  0.f,   .5f,  7000.f, 0.f,  1.f, 0.57f, 0 },
            { 1.f,   .35f, 1.f,  0.f,   .5f,  6500.f, 0.f,  1.f, 0.50f, 0 },
            { 1.5f,  .9f,  1.f,  0.f,   .2f,  5000.f, 0.f,  1.f, 0.45f, 0 },
            { 120.f, .3f,  1.5f, .06f,  .05f, 0.f,    .2f,  1.2f, .68f, 0 },
            { 0.f,   .07f, 1.f,  0.f,   1.f,  5500.f, 0.f,  1.f,  .38f, 0 },
            { 340.f, .2f,  1.2f, .02f,  0.f,  0.f,    .1f,  1.f,  .50f, 1 } }, true } };    // sucio: menos bits y menos frecuencia
    return k;
}

inline juce::StringArray drumKitNames()
{
    juce::StringArray s;
    for (auto& k : drumKits()) s.add (k.name);
    return s;
}

//==============================================================================
// Filtro de estado variable (TPT): paso bajo, banda y alto a la vez
struct Svf
{
    float ic1 = 0, ic2 = 0, g = 0, k = 1, a1 = 0, a2 = 0, a3 = 0;
    void set (float fc, float q, float sr)
    {
        g = std::tan (juce::MathConstants<float>::pi * juce::jlimit (20.f, sr * .45f, fc) / sr);
        k = 1.f / juce::jmax (.1f, q);
        a1 = 1.f / (1.f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    void process (float x, float& lp, float& bp, float& hp)
    {
        const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.f * v1 - ic1; ic2 = 2.f * v2 - ic2;
        lp = v2; bp = v1; hp = x - k * v1 - v2;
    }
    void reset() { ic1 = ic2 = 0; }
};

//==============================================================================
// Una voz de batería: cada fila tiene la suya (mono, como en las cajas de ritmo)
class DrumVoice
{
public:
    void prepare (float sampleRate, int laneIndex) { sr = sampleRate; lane = laneIndex; f1.reset(); f2.reset(); amp = 0; active = false; seed = 0x1234567u + (juce::uint32) laneIndex * 7919u; }

    // tune: -12..+12 semitonos; decayMul: 0,3x..3x; color: -1..1 (más oscuro / más brillante y sucio)
    void trigger (float vel, const DrumPatch& p, float tune, float decayMul, float color)
    {
        patch = p;
        v = std::pow (juce::jlimit (0.f, 1.f, vel), 1.3f) * p.level;
        const float tr = std::pow (2.f, tune / 12.f);
        freq = p.freq * tr;
        col = color;
        const float dec = juce::jmax (.008f, p.decay * decayMul);
        kAmp = std::exp (-1.f / (dec * sr / 4.6f));          // -40 dB en "dec" segundos
        kPitch = p.pitchT > 0 ? std::exp (-1.f / (p.pitchT * sr)) : 0.f;
        pitchEnv = 1.f;
        kNoise = std::exp (-1.f / (juce::jmax (.005f, dec * (lane == snare ? .9f : 1.f)) * sr / 4.6f));
        clickEnv = 1.f;
        t = 0;
        amp = 1.f; noiseAmp = 1.f; choked = false;
        // la fase no se reinicia en el metal (como los osciladores libres de la 808); en el bombo sí, para que pegue igual siempre
        if (lane == kick || lane == tom) ph1 = .25f;
        const float bright = std::pow (2.f, col * 1.2f);
        if (lane == snare)      { f1.set (p.tone * bright, .7f, sr); }
        else if (lane == clap)  { f1.set (p.tone * bright, 1.4f, sr); }
        else if (lane == closedHat || lane == openHat || lane == ride) { f1.set (p.tone * bright, 1.2f, sr); f2.set (p.tone * .75f * bright, .7f, sr); }
        else if (lane == shaker) { f1.set (p.tone * bright, .8f, sr); }
        else if (lane == rim)   { f1.set (p.tone * bright, .9f, sr); }
        else if (lane == perc)  { f1.set (p.tone * bright, 2.f, sr); }
        active = true;
    }

    // los charles cerrados cortan al abierto, como en las máquinas de verdad
    void choke() { choked = true; }

    float process()
    {
        if (! active) return 0.f;
        float y = 0.f;
        const float dt = 1.f / sr;
        ++t;
        if (choked) { amp *= .985f; noiseAmp *= .985f; }
        switch (lane)
        {
            case kick:
            case tom:
            {
                const float f = freq * (1.f + (patch.pitch - 1.f) * pitchEnv);
                pitchEnv *= kPitch;
                ph1 += f * dt; ph1 -= std::floor (ph1);
                y = std::sin (juce::MathConstants<float>::twoPi * ph1) * amp;
                if (patch.click > 0 && clickEnv > 1.0e-3f)
                {
                    const float n = rnd() * clickEnv * patch.click * (lane == kick ? .9f : .4f);
                    y += n * (1.f + col * .6f);
                    clickEnv *= std::exp (-1.f / (.0025f * sr));
                }
                const float drv = patch.drive * (1.f + juce::jmax (0.f, col) * 1.5f);
                if (drv > 1.01f) y = std::tanh (y * drv) / std::tanh (drv);
                amp *= kAmp;
                break;
            }
            case snare:
            {
                const float f = freq * (1.f + (patch.pitch - 1.f) * pitchEnv);
                pitchEnv *= kPitch;
                ph1 += f * dt; ph1 -= std::floor (ph1);
                ph2 += f * 1.62f * dt; ph2 -= std::floor (ph2);
                const float body = (std::sin (juce::MathConstants<float>::twoPi * ph1) * .6f + std::sin (juce::MathConstants<float>::twoPi * ph2) * .35f) * amp * amp;
                float lp, bp, hp; f1.process (rnd(), lp, bp, hp);
                const float n = (hp * .7f + bp * .5f) * noiseAmp * patch.noise * (1.f + col * .4f);
                y = body * (1.f - patch.noise * .5f) + n;
                if (patch.drive > 1.01f) y = std::tanh (y * patch.drive) / std::tanh (patch.drive);
                amp *= std::exp (-1.f / (.05f * sr));
                noiseAmp *= kNoise;
                if (noiseAmp < 1.0e-4f && amp < 1.0e-4f) active = false;
                return y * v;
            }
            case clap:
            {
                // tres golpes muy seguidos y luego la cola: la "palmada de grupo" de las cajas de ritmo
                const float ms = (float) t / sr * 1000.f;
                float e;
                if (ms < 30.f) { const float local = std::fmod (ms, 10.f); e = std::exp (-local / 2.2f); }
                else e = noiseAmp;
                float lp, bp, hp; f1.process (rnd(), lp, bp, hp);
                y = bp * 2.2f * e;
                if (ms >= 30.f) noiseAmp *= kNoise;
                if (ms > 30.f && noiseAmp < 1.0e-4f) active = false;
                return y * v;
            }
            case rim:
            {
                ph1 += freq * dt; ph1 -= std::floor (ph1);
                ph2 += freq * .29f * dt; ph2 -= std::floor (ph2);
                const float tri = 1.f - 4.f * std::abs (ph2 - .5f);
                float lp, bp, hp; f1.process (std::sin (juce::MathConstants<float>::twoPi * ph1) + tri * .6f + rnd() * patch.noise, lp, bp, hp);
                y = (bp * 1.5f + hp * .3f) * amp;
                amp *= kAmp;
                break;
            }
            case closedHat:
            case openHat:
            case ride:
            {
                // seis osciladores cuadrados desafinados (las frecuencias "metálicas" de la 808) por un paso banda
                static const float mf[6] = { 205.3f, 304.4f, 369.6f, 522.7f, 540.f, 800.f };
                float m = 0.f;
                for (int i = 0; i < 6; ++i)
                {
                    mph[i] += mf[i] * freq * dt; mph[i] -= std::floor (mph[i]);
                    m += mph[i] < .5f ? 1.f : -1.f;
                }
                m = m / 6.f * (1.f - patch.noise) + rnd() * patch.noise;
                float lp, bp, hp; f1.process (m, lp, bp, hp);
                float lp2, bp2, hp2; f2.process (bp, lp2, bp2, hp2);
                y = hp2 * 1.8f * amp;
                amp *= kAmp;
                break;
            }
            case shaker:
            {
                // sube en unos milisegundos (el grano moviéndose) y cae
                const float ms = (float) t / sr * 1000.f;
                const float att = juce::jmin (1.f, ms / 6.f);
                float lp, bp, hp; f1.process (rnd(), lp, bp, hp);
                y = hp * att * amp * 1.2f;
                amp *= kAmp;
                break;
            }
            case perc:
            default:
            {
                if (patch.variant == 0)   // cencerro: dos cuadradas por un paso banda
                {
                    ph1 += freq * dt; ph1 -= std::floor (ph1);
                    ph2 += freq * 1.48f * dt; ph2 -= std::floor (ph2);
                    float lp, bp, hp; f1.process ((ph1 < .5f ? 1.f : -1.f) + (ph2 < .5f ? 1.f : -1.f), lp, bp, hp);
                    y = bp * .9f * amp;
                }
                else if (patch.variant == 1)   // conga
                {
                    const float f = freq * (1.f + (patch.pitch - 1.f) * pitchEnv);
                    pitchEnv *= kPitch;
                    ph1 += f * dt; ph1 -= std::floor (ph1);
                    y = std::sin (juce::MathConstants<float>::twoPi * ph1) * amp;
                    if (clickEnv > 1.0e-3f) { y += rnd() * clickEnv * .2f; clickEnv *= std::exp (-1.f / (.002f * sr)); }
                }
                else   // clave y bloque de madera
                {
                    ph1 += freq * dt; ph1 -= std::floor (ph1);
                    y = std::sin (juce::MathConstants<float>::twoPi * ph1) * amp;
                    if (patch.noise > 0) { float lp, bp, hp; f1.process (rnd(), lp, bp, hp); y += bp * patch.noise * amp; }
                }
                amp *= kAmp;
                break;
            }
        }
        if (amp < 1.0e-4f) active = false;
        return y * v;
    }

    bool isActive() const { return active; }

private:
    float rnd() { seed = seed * 1664525u + 1013904223u; return (float) (seed >> 8) / 8388608.f - 1.f; }
    DrumPatch patch {};
    Svf f1, f2;
    float sr = 44100.f, v = 0, freq = 100, col = 0;
    float amp = 0, noiseAmp = 0, kAmp = 0, kNoise = 0, pitchEnv = 0, kPitch = 0, clickEnv = 0;
    float ph1 = 0, ph2 = 0, mph[6] { .1f, .37f, .62f, .05f, .81f, .44f };
    int lane = 0, t = 0;
    bool active = false, choked = false;
    juce::uint32 seed = 0x1234567u;
};
} // namespace ddc
