#pragma once
#include "Pattern.h"
#include "DrumSynth.h"

namespace ddc
{
//==============================================================================
// DRUM MAKER: patrón de batería (una fila por instrumento), estilos y generador

struct DrumStep
{
    float vel = 0.f;      // 0 = vacío; si no, la fuerza 0..1
    float need = 0.f;     // Densidad mínima para que suene (0 = siempre)
    int ratchet = 1;      // redoble: golpes dentro del paso (1..4)
    int nudge = 0;        // empuje: -3..3 (doceavos de paso; negativo = antes)
    float prob = 1.f;     // probabilidad de sonar en cada vuelta
    bool on() const { return vel > 0.f; }
};

struct DrumPattern
{
    std::array<std::array<DrumStep, maxSteps>, numDrums> s {};
    std::array<int, numDrums> len {};   // longitud de cada fila (0 = la del patrón): polimetría
};

inline int laneLength (const DrumPattern& p, int lane, int L) { const int l = p.len[(size_t) lane]; return l > 0 ? juce::jmin (l, L) : L; }

// ¿suena este golpe con esta densidad?
inline bool drumAudible (const DrumStep& st, float density) { return st.on() && st.need <= density + 1.0e-4f; }

//==============================================================================
// Guardar y cargar en el proyecto
inline juce::String drumPatternToString (const DrumPattern& p)
{
    juce::StringArray lanes;
    for (int l = 0; l < numDrums; ++l)
    {
        juce::StringArray hits;
        for (int i = 0; i < maxSteps; ++i)
        {
            const auto& st = p.s[(size_t) l][(size_t) i];
            if (! st.on()) continue;
            hits.add (juce::String (i) + "," + juce::String (juce::roundToInt (st.vel * 1000.f)) + "," + juce::String (juce::roundToInt (st.need * 1000.f))
                      + "," + juce::String (st.ratchet) + "," + juce::String (st.nudge) + "," + juce::String (juce::roundToInt (st.prob * 1000.f)));
        }
        lanes.add (juce::String (p.len[(size_t) l]) + "|" + hits.joinIntoString (";"));
    }
    return lanes.joinIntoString ("/");
}

inline bool drumPatternFromString (const juce::String& str, DrumPattern& out)
{
    auto lanes = juce::StringArray::fromTokens (str, "/", "");
    if (lanes.size() != numDrums) return false;
    DrumPattern p {};
    for (int l = 0; l < numDrums; ++l)
    {
        p.len[(size_t) l] = juce::jlimit (0, maxSteps, lanes[l].upToFirstOccurrenceOf ("|", false, false).getIntValue());
        for (auto& h : juce::StringArray::fromTokens (lanes[l].fromFirstOccurrenceOf ("|", false, false), ";", ""))
        {
            auto f = juce::StringArray::fromTokens (h, ",", "");
            if (f.size() != 6) continue;
            const int i = f[0].getIntValue();
            if (i < 0 || i >= maxSteps) continue;
            auto& st = p.s[(size_t) l][(size_t) i];
            st.vel = juce::jlimit (0.f, 1.f, f[1].getIntValue() / 1000.f);
            st.need = juce::jlimit (0.f, 1.f, f[2].getIntValue() / 1000.f);
            st.ratchet = juce::jlimit (1, 4, f[3].getIntValue());
            st.nudge = juce::jlimit (-3, 3, f[4].getIntValue());
            st.prob = juce::jlimit (0.f, 1.f, f[5].getIntValue() / 1000.f);
        }
    }
    out = p;
    return true;
}

//==============================================================================
// Estilos. Cada fila tiene varias figuras de un compás (16 semicorcheas):
//  '0' suena siempre, '1' '2' '3' aparecen al subir la Densidad, 'X' acento fijo, 'g' golpe fantasma (suave),
//  'r' redoble de 2 y 't' redoble de 3 (charles), '.' silencio
struct DrumStyle
{
    const char* name;
    std::array<std::vector<const char*>, numDrums> cells;
    std::vector<const char*> kits;
    int groove;                          // 0 recto, 1 shuffle, 2 suelto
    float swingMin, swingMax;
    float polyProb;                      // probabilidad de que la percusión vaya en otra longitud (polimetría)
};

inline const std::vector<DrumStyle>& drumStyles()
{
    static const std::vector<DrumStyle> s {
        { "Minimal", {{
            { "0...0...0...0...", "0...0...0...0.3." },
            { "................", "..........g....." },
            { "....0.......0...", "............0...", "....1.......0..2" },
            { "...2..1.....2...", ".2....1...2...1.", "......0.....1..." },
            { "2.1.2.1.2.1.2.1.", "3.0.3.0.3.0.3.1.", "..0...0...0...0." },
            { "..1...1...1...1.", "..2.......2....." },
            { "................" },
            { "................", "..............2." },
            { "3232323232323232", "3.2.3.2.3.2.3.2." },
            { "...1......1..2..", "..2..1....2..1..", ".1...2..1....2.." } }},
          { "Minimal", "Siete", "Lo-fi" }, 0, .10f, .30f, .55f },
        { "Rominimal", {{
            { "0...0...0...0...", "0...0...0..30..." },
            { "................" },
            { "....1.......1...", "............0..." },
            { ".1..2..1..2..1..", "...1..2...1..2.." },
            { "1213121312131213", "2r12121r21212r12", "1t12121312131t12" },
            { "..2...2...2...2." },
            { "................" },
            { "................" },
            { "3232323232323232" },
            { "..1..2..1..2..1.", ".2..1..2..1..2.." } }},
          { "Minimal", "Ocho-Ocho 808", "Lo-fi" }, 0, .15f, .35f, .7f },
        { "Minimal house", {{
            { "0...0...0...0..." },
            { "................", "..........g....g" },
            { "....0.......0..." },
            { "..2.....2..1....", "......2.....1..." },
            { "1.0.1.0.1.0.1.0.", "2.0.2.0.2.0.2.02" },
            { "..0...0...0...0." },
            { "................" },
            { "..............2.", "................" },
            { "3.2.3.2.3.2.3.2." },
            { "...1..2....1..2.", "..2....1..2....." } }},
          { "Nueve 909", "Minimal", "Siete" }, 0, .15f, .35f, .35f },
        { "Deep house", {{
            { "0...0...0...0..." },
            { "..........g....g", "................" },
            { "....0.......0..." },
            { "......2.....1...", "...2......1....." },
            { "2.1.2.1.2.1.2.1.", "2.0.2.0.2.0.2.1." },
            { "..0...0...0...0." },
            { "................", "0...1...0...1..." },
            { "................" },
            { "3212321232123212", "3.2.3.2.3.2.3.2." },
            { ".2....1..2....1.", "...2.....2......" } }},
          { "Ocho-Ocho 808", "Nueve 909", "Lo-fi" }, 2, .10f, .25f, .25f },
        { "Classic house", {{
            { "0...0...0...0..." },
            { "....1.......1...", "................" },
            { "....0.......0..." },
            { "................" },
            { "0.1.0.1.0.1.0.1." },
            { "..0...0...0...0." },
            { "0.2.1.2.0.2.1.2.", "0...0...0...0..." },
            { "............1.2.", "................" },
            { "2.3.2.3.2.3.2.3." },
            { "...2......2.....", "..1...1...1...1." } }},
          { "Nueve 909", "Siete" }, 0, .15f, .35f, 0.f },
        { "UK garage", {{
            { "0.....1...0.....", "0......2..0....2", "0.....0..1......" },
            { "....0.......0..g", "....1.......1..." },
            { "....0.......0..." },
            { "..1..2...1..2...", "...1..2....1..2." },
            { "0.1.0.10.1.0.1.0", "1.2.0.21.2.0.2.1" },
            { "..1.......1.....", "......1.......1." },
            { "................" },
            { "................" },
            { "3.2.3.2.3.2.3.2." },
            { "......2........1", "................" } }},
          { "Garage", "Nueve 909" }, 1, .45f, .70f, .2f },
        { "UK bassline", {{
            { "0...0...0...0..." },
            { "....1.......1..2" },
            { "....0.......0..." },
            { "...2.....2......" },
            { "0.10.1.10.10.1.1" },
            { "..0...0...0...0." },
            { "................" },
            { "................" },
            { "3232323232323232" },
            { "................", "..2.......2....." } }},
          { "Garage", "Nueve 909" }, 1, .25f, .50f, 0.f },
        { "Acid house", {{
            { "0...0...0...0..." },
            { "................" },
            { "....0.......0..." },
            { "..2.......2.....", "................" },
            { "0.1.0.1.0.1.0.1.", "0.10.1.20.1.0.12" },
            { "..0...0...0...0." },
            { "................" },
            { "................" },
            { "................" },
            { "................" } }},
          { "Nueve 909", "Siete" }, 0, .05f, .25f, 0.f } };
    return s;
}

inline juce::StringArray drumStyleNames()
{
    juce::StringArray s { "Todo" };
    for (auto& st : drumStyles()) s.add (st.name);
    return s;
}

inline int drumKitIndex (const char* name)
{
    const auto& k = drumKits();
    for (size_t i = 0; i < k.size(); ++i) if (juce::String (k[i].name) == name) return (int) i;
    return 0;
}

//==============================================================================
class DrumGenerator
{
public:
    juce::Random rnd;
    float r (float a, float b) { return a + rnd.nextFloat() * (b - a); }

    static void cellToSteps (const char* cell, int bar, DrumPattern& p, int lane, juce::Random& rng)
    {
        for (int i = 0; i < 16 && cell[i] != 0; ++i)
        {
            const char c = cell[i];
            if (c == '.') continue;
            auto& st = p.s[(size_t) lane][(size_t) (bar * 16 + i)];
            float base = i % 4 == 0 ? .9f : (i % 2 == 0 ? .74f : .56f);
            st = DrumStep();
            st.ratchet = 1;
            switch (c)
            {
                case '0': st.need = 0.f; break;
                case '1': st.need = .3f; break;
                case '2': st.need = .55f; break;
                case '3': st.need = .8f; break;
                case 'X': st.need = 0.f; base = 1.f; break;
                case 'g': st.need = .55f; base = .35f; break;
                case 'r': st.need = .3f; st.ratchet = 2; break;
                case 't': st.need = .55f; st.ratchet = 3; break;
                default: break;
            }
            st.vel = juce::jlimit (.1f, 1.f, base + (rng.nextFloat() - .5f) * .14f);
        }
    }

    DrumPattern generate (const DrumStyle& st, int L, float variation)
    {
        DrumPattern p {};
        const int bars = juce::jmax (1, L / 16);
        for (int lane = 0; lane < numDrums; ++lane)
        {
            const auto& cells = st.cells[(size_t) lane];
            if (cells.empty()) continue;
            const char* main = cells[(size_t) rnd.nextInt ((int) cells.size())];
            for (int b = 0; b < bars; ++b)
            {
                // con Variación, algunos compases usan otra figura de la misma fila
                const char* c = (b > 0 && rnd.nextFloat() < variation * .5f) ? cells[(size_t) rnd.nextInt ((int) cells.size())] : main;
                cellToSteps (c, b, p, lane, rnd);
            }
            // pequeñas variaciones: un golpe que se mueve o un fantasma de más (nunca en el bombo)
            if (lane != kick)
                for (int b = 1; b < bars; ++b)
                    if (rnd.nextFloat() < variation * .6f)
                    {
                        const int i = b * 16 + rnd.nextInt (16);
                        auto& sx = p.s[(size_t) lane][(size_t) i];
                        if (! sx.on()) { sx.vel = .35f + rnd.nextFloat() * .2f; sx.need = .55f; }
                    }
        }
        // polimetría en la percusión y el rim (sobre todo en el minimal)
        if (rnd.nextFloat() < st.polyProb)
        {
            static const int lens[] = { 12, 10, 6, 5 };
            const int pl = lens[rnd.nextInt (4)];
            p.len[perc] = pl;
            // la figura se recorta a esa longitud y se repite
            for (int i = pl; i < maxSteps; ++i) p.s[perc][(size_t) i] = DrumStep();
            if (rnd.nextFloat() < .5f) { p.len[rim] = 12; for (int i = 12; i < maxSteps; ++i) p.s[rim][(size_t) i] = DrumStep(); }
        }
        return p;
    }
};

//==============================================================================
// Fill al final de cada 4 u 8 compases: caja y tom en semicorcheas que van subiendo, y el charles redobla
// devuelve la fuerza del golpe (0 = nada) para esa fila en ese paso del fill, y el redoble
inline float fillHit (int lane, int posInFill, int fillLen, float amount, int& ratchet)
{
    ratchet = 1;
    const float u = (float) posInFill / (float) juce::jmax (1, fillLen - 1);
    switch (lane)
    {
        case snare: if (posInFill % 2 == 0 || amount > .6f) return .45f + .55f * u; return 0.f;
        case tom:   if (posInFill % 2 == 1 && amount > .3f) return .5f + .4f * (1.f - u); return 0.f;
        case clap:  return posInFill == fillLen - 1 ? 1.f : 0.f;
        case closedHat: if (amount > .5f) { ratchet = posInFill >= fillLen - 2 ? 3 : 2; return .6f; } return -1.f;   // -1: deja el normal
        default: return -1.f;
    }
}
} // namespace ddc
