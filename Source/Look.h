#pragma once
#include <JuceHeader.h>
#include "BinaryData.h"

namespace ddc
{
namespace col
{
    // Chord Maker: cuerpo azul, panel de arriba oscuro y pantallas violetas
    const juce::Colour outer      { 0xffe9e7e1 };
    const juce::Colour khaki      { 0xff7a2232 };   // burdeos
    const juce::Colour khakiLight { 0xffa8495a };
    const juce::Colour khakiDark  { 0xff4a1420 };
    const juce::Colour outline    { 0xff1c070c };
    const juce::Colour cream      { 0xfff3ebdd };
    const juce::Colour labelInk   { 0xff1c070c };
    const juce::Colour display    { 0xff0c1626 };   // fondo de las pantallas (azul noche)
    const juce::Colour amber      { 0xfff2a33a };
    const juce::Colour amberHi    { 0xffffe2a6 };
    const juce::Colour teal       { 0xff2aa5a0 };
    const juce::Colour red        { 0xffd46a55 };
    const juce::Colour screen     { 0xff2c3a2a };
    const juce::Colour band       { 0xff1a2a18 };
    const juce::Colour charcoal   { 0xff2c2830 };   // tapa de arriba del sampler
    const juce::Colour charcoalHi { 0xff4a4450 };
    const juce::Colour charcoalLo { 0xff1c1a20 };
    const juce::Colour panelInk   { 0xffecdcbe };   // letras crema sobre la tapa oscura
    // nombres antiguos que aún usa la escena
    const juce::Colour bg = display, panel = display, panel2 = display, line = khakiDark, line2 = khakiDark,
                       ink = cream, muted = khakiLight, note = amber, noteHi = amberHi, slide = teal;
}

struct Fonts
{
    juce::Typeface::Ptr title, body;
    Fonts()
    {
        title = juce::Typeface::createSystemTypefaceFor (BinaryData::PressStart2PRegular_ttf, BinaryData::PressStart2PRegular_ttfSize);
        body  = juce::Typeface::createSystemTypefaceFor (BinaryData::PixelifySans_ttf, BinaryData::PixelifySans_ttfSize);
    }
    juce::Font t (float h) const { return juce::Font (juce::FontOptions (title).withHeight (h)); }
    juce::Font b (float h) const { return juce::Font (juce::FontOptions (body).withHeight (h)); }
};

inline const Fonts& fonts() { static Fonts f; return f; }

// Texto para pantallas LCD: mayúsculas y sin tildes, como en los aparatos de 8 bits
inline juce::String lcdText (const juce::String& s)
{
    auto t = s.toUpperCase();
    const char* from[] = { "\xc3\x81", "\xc3\x89", "\xc3\x8d", "\xc3\x93", "\xc3\x9a", "\xc3\x91", "\xc3\x9c",
                           "\xc3\xa1", "\xc3\xa9", "\xc3\xad", "\xc3\xb3", "\xc3\xba", "\xc3\xb1", "\xc3\xbc" };
    const char* to[]   = { "A", "E", "I", "O", "U", "N", "U", "A", "E", "I", "O", "U", "N", "U" };
    for (int i = 0; i < 14; ++i) t = t.replace (juce::String::fromUTF8 (from[i]), to[i]);
    return t;
}

// Líneas de escaneo sobre una pantalla
inline void scanlines (juce::Graphics& g, juce::Rectangle<int> r, float alpha = .14f)
{
    g.setColour (juce::Colours::black.withAlpha (alpha));
    for (int y = r.getY(); y < r.getBottom(); y += 3) g.fillRect (r.getX(), y, r.getWidth(), 1);
}

// Iconos de 7x7 en pixel art para los botones
inline void pixelIcon (juce::Graphics& g, const juce::String& name, int x, int y, int sc, juce::Colour c)
{
    static const std::map<juce::String, std::array<const char*, 7>> icons {
        { "dice",  { "XXXXXXX", "X.....X", "X.X.X.X", "X.....X", "X.X.X.X", "X.....X", "XXXXXXX" } },
        { "wave",  { ".......", ".XX....", "X..X...", "X..X..X", "....X.X", ".....X.", "......." } },
        { "trash", { "..XXX..", "XXXXXXX", ".X.X.X.", ".X.X.X.", ".X.X.X.", ".X.X.X.", ".XXXXX." } },
        { "down",  { "...X...", "...X...", "...X...", "X..X..X", ".X.X.X.", "..XXX..", "...X..." } } };
    auto it = icons.find (name);
    if (it == icons.end()) return;
    g.setColour (c);
    for (int j = 0; j < 7; ++j)
        for (int i = 0; i < 7; ++i)
            if (it->second[(size_t) j][i] == 'X') g.fillRect (x + i * sc, y + j * sc, sc, sc);
}

// Borde de píxel con las esquinas recortadas
inline void pixelBox (juce::Graphics& g, juce::Rectangle<int> r, juce::Colour fill, juce::Colour border, int px = 3)
{
    g.setColour (fill);
    g.fillRect (r);
    g.setColour (border);
    g.fillRect (r.getX(), r.getY() - px, r.getWidth(), px);
    g.fillRect (r.getX(), r.getBottom(), r.getWidth(), px);
    g.fillRect (r.getX() - px, r.getY(), px, r.getHeight());
    g.fillRect (r.getRight(), r.getY(), px, r.getHeight());
}

//==============================================================================
// Personaje que sale de una tienda al generar
struct Actor
{
    bool active = false;
    int tent = 0;      // 0 = tienda naranja, 1 = tienda turquesa
    int action = 0;    // 0 baila, 1 fuma, 2 saluda, 3 salta, 4 se calienta en la hoguera
    int f = 0;         // fotograma (10 por segundo)
    juce::uint32 shirt = 0, pants = 0, hair = 0, skin = 0;

    static constexpr int walkFrames = 12, actFrames = 42;
    int totalFrames() const { return walkFrames * 2 + actFrames; }

    void start (juce::Random& r)
    {
        static const juce::uint32 shirts[] = { 0xffff5a1f, 0xff4fe3c1, 0xffffb000, 0xffe8e2cc, 0xff9a7cff, 0xffff7aa8, 0xff7ad3ff };
        static const juce::uint32 pantsC[] = { 0xff2a3a6a, 0xff4a3020, 0xff3a3a3a, 0xff1f4a3a };
        static const juce::uint32 hairs[]  = { 0xff2a1a12, 0xfff2d16b, 0xff9a3a1a, 0xff141018, 0xffd9d9d9, 0xffff7aa8 };
        static const juce::uint32 skins[]  = { 0xffffdbac, 0xfff1c27d, 0xffc68642, 0xff8d5524 };
        active = true; f = 0;
        tent = r.nextInt (2);
        action = r.nextInt (5);
        shirt = shirts[r.nextInt (7)]; pants = pantsC[r.nextInt (4)];
        hair = hairs[r.nextInt (6)]; skin = skins[r.nextInt (4)];
    }
    void advance() { if (active && ++f >= totalFrames()) active = false; }
};

//==============================================================================
// Campista que se queda en el claro después de salir de su tienda
struct Camper
{
    enum Action { dance, smoke, drink, sit, jump, sleep, couple, tentLove, numActions };
    int slot = 0, action = dance, tent = 0, born = -100000;
    juce::uint32 shirt = 0, pants = 0, hair = 0, skin = 0, shirt2 = 0, hair2 = 0, skin2 = 0;

    // todo sale de una semilla, así basta con guardar un número por campista en el proyecto
    static Camper fromSeed (int seed, int index, bool tentLoveFree0, bool tentLoveFree1)
    {
        static const juce::uint32 shirts[] = { 0xffff5a1f, 0xff4fe3c1, 0xffffb000, 0xffe8e2cc, 0xff9a7cff, 0xffff7aa8, 0xff7ad3ff, 0xffc8ff7a };
        static const juce::uint32 pantsC[] = { 0xff2a3a6a, 0xff4a3020, 0xff3a3a3a, 0xff1f4a3a };
        static const juce::uint32 hairs[]  = { 0xff2a1a12, 0xfff2d16b, 0xff9a3a1a, 0xff141018, 0xffd9d9d9, 0xffff7aa8 };
        static const juce::uint32 skins[]  = { 0xffffdbac, 0xfff1c27d, 0xffc68642, 0xff8d5524 };
        juce::Random r ((juce::int64) seed * 977 + 13);
        Camper c;
        c.slot = index;
        c.tent = r.nextInt (2);
        // reparto: más gente bailando, alguna pareja y, si queda una tienda libre, una tienda "ocupada"
        static const int pool[] = { dance, dance, dance, smoke, smoke, drink, drink, sit, jump, sleep, couple, couple, tentLove };
        c.action = pool[r.nextInt ((int) (sizeof (pool) / sizeof (pool[0])))];
        if (c.action == tentLove)
        {
            if (tentLoveFree0) c.tent = 0;
            else if (tentLoveFree1) c.tent = 1;
            else c.action = couple;
        }
        c.shirt = shirts[r.nextInt (8)]; c.pants = pantsC[r.nextInt (4)];
        c.hair = hairs[r.nextInt (6)];   c.skin = skins[r.nextInt (4)];
        c.shirt2 = shirts[r.nextInt (8)]; c.hair2 = hairs[r.nextInt (6)]; c.skin2 = skins[r.nextInt (4)];
        return c;
    }
};

// Lo que la escena necesita saber del plugin para moverse con la música
struct SceneInfo
{
    int t = 0;            // reloj de la animación (15 por segundo)
    float level = 0.f;    // volumen del bajo, 0..1
    double beat = -1.0;   // posición en tiempos (negativa = parado)
    float tod = 0.f;      // hora: 0 = noche cerrada ... 1 = mañana
    const std::vector<Camper>* crowd = nullptr;
};

class Scene
{
public:
    // Dibuja el bosque en una imagen de baja resolución. El dibujo mide 108 px de alto;
    // offY recorta la parte de arriba del cielo para que quepa en la imagen.
    static void draw (juce::Image& img, int offY, const SceneInfo& info)
    {
        juce::Graphics g (img);
        g.addTransform (juce::AffineTransform::translation (0.f, (float) -offY));
        Scene s (g);
        s.info = info;
        s.W = img.getWidth();
        s.top = offY;
        s.paintScene();
    }

    // sitios del claro donde se quedan los campistas (en el orden en que se van llenando)
    static constexpr int numSlots = 12;
    static juce::Point<int> slotPos (int i)
    {
        // delante de la carpa, en la pista
        static const int xy[numSlots][2] = { { 80, 99 }, { 112, 99 }, { 88, 101 }, { 104, 100 }, { 72, 102 }, { 120, 102 },
                                             { 60, 101 }, { 132, 101 }, { 96, 104 }, { 48, 103 }, { 144, 103 }, { 158, 102 } };
        return { xy[i % numSlots][0], xy[i % numSlots][1] };
    }

private:
    explicit Scene (juce::Graphics& gr) : g (gr) {}
    juce::Graphics& g;
    juce::uint32 seed = 7;
    SceneInfo info;
    int W = 192, top = 0;

    float rnd()
    {
        seed = seed * 1664525u + 1013904223u;
        return (float) ((seed >> 8) & 0xffffff) / 16777216.f;
    }
    void px (float x, float y, int w, int h, juce::uint32 c)
    {
        g.setColour (juce::Colour (c));
        g.fillRect (juce::roundToInt (x), juce::roundToInt (y), w, h);
    }
    void pxc (float x, float y, int w, int h, juce::Colour c)
    {
        g.setColour (c);
        g.fillRect (juce::roundToInt (x), juce::roundToInt (y), w, h);
    }
    void pxa (float x, float y, int w, int h, juce::uint32 c, float alpha)
    {
        g.setColour (juce::Colour (c).withMultipliedAlpha (juce::jlimit (0.f, 1.f, alpha)));
        g.fillRect (juce::roundToInt (x), juce::roundToInt (y), w, h);
    }
    static juce::Colour mix (juce::uint32 a, juce::uint32 b, float k) { return juce::Colour (a).interpolatedWith (juce::Colour (b), juce::jlimit (0.f, 1.f, k)); }
    static float smooth (float e0, float e1, float x) { const float t = juce::jlimit (0.f, 1.f, (x - e0) / (e1 - e0)); return t * t * (3.f - 2.f * t); }

    void pine (int x, int base, int h, juce::Colour c, juce::Colour trunk)
    {
        const int tier = juce::jmax (3, h / 3);
        for (int y = 0; y < h; ++y)
        {
            const int t = y / tier, w = (int) ((y % tier) * .7f) + t * 2;
            pxc ((float) (x - w), (float) (base - h - 2 + y), w * 2 + 1, 1, c);
        }
        pxc ((float) (x - 1), (float) (base - 2), 3, 2, trunk);
    }
    void tent (int x, int base, int w, int h, juce::uint32 light, juce::uint32 dark, juce::uint32 door)
    {
        for (int y = 0; y < h; ++y)
        {
            const int half = juce::roundToInt ((y + 1) * w / (2.f * h));
            px ((float) (x - half), (float) (base - h + y), half, 1, light);
            px ((float) x, (float) (base - h + y), half + 1, 1, dark);
        }
        const int dh = (int) (h * .55f);
        for (int y = 0; y < dh; ++y)
        {
            const int half = juce::roundToInt ((y + 1) * (w * .22f) / dh);
            px ((float) (x - half), (float) (base - dh + y), half * 2 + 1, 1, door);
        }
        px ((float) x, (float) (base - h - 2), 1, 2, dark);
    }
    void heart (float x, float y, float alpha)
    {
        pxa (x, y, 1, 1, 0xffff5a8a, alpha); pxa (x + 2, y, 1, 1, 0xffff5a8a, alpha);
        pxa (x - 0, y + 1, 3, 1, 0xffff5a8a, alpha); pxa (x + 1, y + 2, 1, 1, 0xffff5a8a, alpha);
    }

    // una persona de pie: brazos de atrás y de delante con su postura
    enum Arm { down, up, mouth, forward, wave2, cup };
    void figure (int x, int yb, int dir, Arm armBack, Arm armFront, bool legsTogether,
                 juce::uint32 shirt, juce::uint32 pants, juce::uint32 hair, juce::uint32 skin)
    {
        if (legsTogether) { px ((float) (x - 1), (float) (yb - 3), 3, 2, pants); px ((float) (x - 1), (float) (yb - 1), 3, 1, 0xff141018); }
        else { px ((float) (x - 1), (float) (yb - 3), 1, 2, pants); px ((float) (x + 1), (float) (yb - 3), 1, 2, pants);
               px ((float) (x - 1), (float) (yb - 1), 1, 1, 0xff141018); px ((float) (x + 1), (float) (yb - 1), 1, 1, 0xff141018); }
        px ((float) (x - 1), (float) (yb - 7), 3, 4, shirt);
        px ((float) (x - 1), (float) (yb - 10), 3, 3, skin);
        px ((float) (x - 1), (float) (yb - 11), 3, 1, hair);
        px ((float) (x - dir), (float) (yb - 10), 1, 1, hair);
        px ((float) (x + dir), (float) (yb - 9), 1, 1, 0xff141018);
        auto arm = [&] (Arm a, int side)
        {
            const int ax = x + side * 2;
            switch (a)
            {
                case down:    px ((float) ax, (float) (yb - 7), 1, 2, shirt); px ((float) ax, (float) (yb - 5), 1, 1, skin); break;
                case up:      px ((float) ax, (float) (yb - 10), 1, 3, shirt); px ((float) ax, (float) (yb - 11), 1, 1, skin); break;
                case wave2:   px ((float) ax, (float) (yb - 9), 1, 2, shirt); px ((float) (ax + side), (float) (yb - 10), 1, 1, skin); break;
                case forward: px ((float) ax, (float) (yb - 7), 1, 1, shirt); px ((float) (ax + side), (float) (yb - 7), 1, 1, skin); break;
                case mouth:   px ((float) ax, (float) (yb - 8), 1, 1, shirt); px ((float) (x + side), (float) (yb - 8), 1, 1, skin); break;
                case cup:     px ((float) ax, (float) (yb - 9), 1, 2, shirt); px ((float) (x + side), (float) (yb - 10), 1, 1, skin);
                              px ((float) (x + side * 2), (float) (yb - 11), 1, 2, 0xffffe2a6); break;
            }
        };
        arm (armBack, -dir);
        arm (armFront, dir);
    }

    void camper (const Camper& c, int t, bool playing, int beatNo, float pulse)
    {
        const auto slot = slotPos (c.slot);
        if (c.action == Camper::tentLove) return;   // se dibuja en la propia tienda
        const int age = t - c.born, walk = 18;
        const int doorX = c.tent == 0 ? tentX0 : tentX1, doorY = c.tent == 0 ? tentB0 : tentB1;
        int x = slot.x, yb = slot.y;
        const int dir = x < 96 ? 1 : -1;          // miran hacia la hoguera
        if (age < 3) return;                     // aún dentro de la tienda
        if (age < walk)
        {
            const float p = (age - 3) / (float) (walk - 3);
            x = doorX + juce::roundToInt ((slot.x - doorX) * p);
            yb = doorY + juce::roundToInt ((slot.y - doorY) * p);
            figure (x, yb, slot.x > doorX ? 1 : -1, down, down, (age / 2) % 2 == 0, c.shirt, c.pants, c.hair, c.skin);
            return;
        }
        const int k = age - walk;
        // con la música en marcha bailan a tempo; parados, a su ritmo
        const int step = playing ? beatNo : (k + c.slot * 3) / 4;
        switch (c.action)
        {
            case Camper::dance:
            {
                const int ph = step % 4;
                const int bob = playing ? (pulse > .5f ? 1 : 0) : ph % 2;
                figure (x, yb + bob, dir, ph == 0 ? up : down, ph == 2 ? up : down, ph % 2 == 1, c.shirt, c.pants, c.hair, c.skin);
                if (ph == 1 || ph == 3) { const int nx = x + dir * 4, ny = yb - 13 - (k % 5); px ((float) nx, (float) ny, 1, 3, 0xfffff07a); px ((float) (nx + 1), (float) ny, 2, 1, 0xfffff07a); }
                break;
            }
            case Camper::smoke:
            {
                const int cyc = (k + c.slot * 5) % 24;
                figure (x, yb, dir, down, cyc < 8 ? mouth : down, true, c.shirt, c.pants, c.hair, c.skin);
                const int ex = cyc < 8 ? x + dir * 1 : x + dir * 3, ey = cyc < 8 ? yb - 8 : yb - 5;
                px ((float) ex, (float) ey, 1, 1, (k / 2) % 2 ? 0xffff5a1f : 0xffffb000);
                for (int j = 0; j < 3; ++j)
                {
                    const int a = (k + j * 8) % 24;
                    pxa ((float) (x + dir * 2) + std::sin ((k + j * 5) * .2f), (float) (yb - 12 - a * .5f), a > 12 ? 2 : 1, 1, 0xffb7a3cf, .7f * (1.f - a / 24.f));
                }
                break;
            }
            case Camper::drink:
            {
                const int cyc = (k + c.slot * 4) % 30;
                figure (x, yb, dir, down, cyc < 10 ? cup : forward, true, c.shirt, c.pants, c.hair, c.skin);
                if (cyc >= 10) px ((float) (x + dir * 3), (float) (yb - 8), 1, 2, 0xffffe2a6);   // vaso en la mano
                break;
            }
            case Camper::sit:
            {
                const bool nod = playing ? pulse > .45f : (k / 10) % 2 == 0;
                px ((float) (x - 1), (float) (yb - 2), 4, 2, c.pants);
                px ((float) (x - 1), (float) (yb - 6), 3, 4, c.shirt);
                const int hy = yb - 9 + (nod ? 1 : 0);
                px ((float) (x - 1), (float) hy, 3, 3, c.skin);
                px ((float) (x - 1), (float) hy, 3, 1, c.hair);
                px ((float) (x - dir), (float) (hy + 1), 1, 1, c.hair);
                px ((float) (x + dir * 2), (float) (yb - 5), 1, 1, c.skin);
                break;
            }
            case Camper::jump:
            {
                static const int jy[] = { 0, -2, -3, -2, 0, 0, 0, 0 };
                const int j = playing ? (pulse > .6f ? -3 : 0) : jy[(k + c.slot) % 8];
                figure (x, yb + j, dir, j < 0 ? up : wave2, j < 0 ? up : down, j < 0, c.shirt, c.pants, c.hair, c.skin);
                break;
            }
            case Camper::sleep:
            {
                // tumbado en el suelo con las zetas subiendo
                px ((float) (x - 4), (float) (yb - 2), 3, 2, c.pants);
                px ((float) (x - 1), (float) (yb - 3), 4, 3, c.shirt);
                px ((float) (x + 3), (float) (yb - 3), 3, 3, c.skin);
                px ((float) (x + 3), (float) (yb - 3), 3, 1, c.hair);
                const int z = (k / 3) % 12;
                pxa ((float) (x + 5 + z / 4), (float) (yb - 6 - z), 2, 1, 0xfff4e9c9, 1.f - z / 12.f);
                pxa ((float) (x + 6 + z / 4), (float) (yb - 5 - z), 1, 1, 0xfff4e9c9, 1.f - z / 12.f);
                pxa ((float) (x + 5 + z / 4), (float) (yb - 4 - z), 2, 1, 0xfff4e9c9, 1.f - z / 12.f);
                break;
            }
            case Camper::couple:
            {
                // pareja abrazada balanceándose, con corazones
                const int sway = playing ? (beatNo % 2) : (k / 8) % 2;
                figure (x - 2 + sway, yb, 1, down, forward, true, c.shirt, c.pants, c.hair, c.skin);
                figure (x + 2 + sway, yb, -1, down, forward, true, c.shirt2, c.pants, c.hair2, c.skin2);
                const int h = (k + c.slot * 6) % 30;
                heart ((float) (x - 1 + sway), (float) (yb - 14 - h / 3), 1.f - h / 30.f);
                break;
            }
            default: break;
        }
    }

    // letra de píxeles de 5 de alto para el cartel de la avioneta
    static const char* glyph (char c)
    {
        switch (c)
        {
            case 'A': return ".#.|#.#|###|#.#|#.#";
            case 'B': return "##.|#.#|##.|#.#|##.";
            case 'C': return ".##|#..|#..|#..|.##";
            case 'D': return "##.|#.#|#.#|#.#|##.";
            case 'E': return "###|#..|##.|#..|###";
            case 'H': return "#.#|#.#|###|#.#|#.#";
            case 'I': return "###|.#.|.#.|.#.|###";
            case 'L': return "#..|#..|#..|#..|###";
            case 'M': return "#...#|##.##|#.#.#|#...#|#...#";
            case 'O': return ".#.|#.#|#.#|#.#|.#.";
            case 'P': return "##.|#.#|##.|#..|#..";
            case 'R': return "##.|#.#|##.|#.#|#.#";
            case 'S': return ".##|#..|.#.|..#|##.";
            case 'T': return "###|.#.|.#.|.#.|.#.";
            case 'X': return "#.#|#.#|.#.|#.#|#.#";
            default:  return "..|..|..|..|..";
        }
    }
    static int glyphWidth (char c) { const char* g = glyph (c); int w = 0; while (g[w] != '|' && g[w] != 0) ++w; return w; }

    // avioneta con cartel: cruza el cielo cada minuto (la primera vez a los 10 s de abrir el plugin)
    void planeBanner (int t, float day)
    {
        static const char* text = "OLD HERMIT CRAB X HESSA X ATMOODSPHERE";
        const int period = 2700, start = 150;           // cada 3 minutos (15 fotogramas por segundo)
        if (t < start) return;
        const int k = (t - start) % period;
        int textW = 0;
        for (const char* p = text; *p; ++p) textW += glyphWidth (*p) + 1;
        const int bannerW = textW + 6, rope = 8, planeW = 14;
        const float speed = 1.6f;
        const float x = (float) W + 4.f - k * speed;      // morro de la avioneta
        if (x + planeW + rope + bannerW < -4.f) return;
        const int y = top + 14 + juce::roundToInt (std::sin (t * .05f) * 1.5f);
        const int xi = juce::roundToInt (x);

        // avioneta mirando a la izquierda
        const juce::uint32 body = 0xffe8e2cc, dark = 0xff2a2a3a, red = 0xffd46a55;
        px ((float) (xi + 1), (float) y, 10, 3, body);                 // fuselaje
        px ((float) xi, (float) (y + 1), 1, 1, body);                  // morro
        px ((float) (xi + 3), (float) (y - 1), 3, 1, 0xff7ad3ff);      // cabina
        px ((float) (xi + 4), (float) (y + 3), 4, 1, dark);            // ala (vista de lado)
        px ((float) (xi + 9), (float) (y - 3), 2, 3, body);            // cola
        px ((float) (xi + 11), (float) (y - 3), 1, 2, red);
        px ((float) (xi + 2), (float) (y + 1), 6, 1, red);             // franja
        // hélice que gira
        if ((t / 1) % 2 == 0) px ((float) (xi - 1), (float) (y - 1), 1, 5, 0xffb7b7c7);
        else px ((float) (xi - 1), (float) (y + 1), 1, 1, 0xffb7b7c7);
        // luz de posición (más visible de noche)
        if ((t / 6) % 2 == 0) pxa ((float) (xi + 11), (float) (y - 4), 1, 1, 0xffff3030, 1.f - day * .5f);

        // cuerda
        const int rx = xi + planeW - 2;
        for (int i = 0; i < rope; i += 2) px ((float) (rx + i), (float) (y + 1 + (i > 4 ? 1 : 0)), 1, 1, 0xff6a6a7a);

        // cartel que ondea: se dibuja por columnas con un desplazamiento vertical en onda
        const int bx = rx + rope;
        const int bh = 9;
        int col = 0;
        auto wave = [&] (int c) { return juce::roundToInt (std::sin (c * .18f - t * .35f) * 1.2f); };
        for (int c = 0; c < bannerW; ++c)
        {
            const int dy = wave (c);
            px ((float) (bx + c), (float) (y - 3 + dy), 1, bh, c == 0 || c == bannerW - 1 ? 0xffc46a00 : 0xfff3ebdd);
            px ((float) (bx + c), (float) (y - 3 + dy), 1, 1, 0xffc46a00);
            px ((float) (bx + c), (float) (y - 3 + dy + bh - 1), 1, 1, 0xffc46a00);
        }
        col = 3;
        for (const char* p = text; *p; ++p)
        {
            const char* gph = glyph (*p);
            const int gw = glyphWidth (*p);
            for (int gx = 0; gx < gw; ++gx)
            {
                const int dy = wave (col + gx);
                for (int gy = 0; gy < 5; ++gy)
                    if (gph[gy * (gw + 1) + gx] == '#')
                        px ((float) (bx + col + gx), (float) (y - 1 + dy + gy), 1, 1, 0xff2a1530);
            }
            col += gw + 1;
        }
    }

    //==========================================================================
    // PLATILLO VOLANTE: cada 3 minutos (un minuto después de la avioneta) baja, abduce a alguien y lo devuelve
    static constexpr int ufoStart = 1050, ufoPeriod = 2700, ufoLen = 300;
    int ufoPhase (int t) const { return t < ufoStart ? -1 : ((t - ufoStart) % ufoPeriod < ufoLen ? (t - ufoStart) % ufoPeriod : -1); }
    // a quién se lleva: un campista al azar en cada visita (o el DJ si no hay nadie)
    int ufoTargetIndex (int t) const
    {
        if (info.crowd == nullptr || info.crowd->empty()) return -1;
        juce::Random r ((juce::int64) ((t - ufoStart) / ufoPeriod) * 7717 + 5);
        for (int tries = 0; tries < 8; ++tries)
        {
            const int i = r.nextInt ((int) info.crowd->size());
            const int a = (*info.crowd)[(size_t) i].action;
            if (a != Camper::tentLove && a != Camper::couple && a != Camper::sleep) return i;   // solo gente que está sola y de pie
        }
        return -1;
    }
    // cuánto ha subido la persona (0 = en el suelo, 1 = dentro del platillo)
    static float ufoLift (int k)
    {
        if (k < 90) return 0.f;
        if (k < 150) return (k - 90) / 60.f;
        if (k < 180) return 1.f;
        if (k < 210) return 1.f - (k - 180) / 30.f;
        return 0.f;
    }
    // sin nadie en la orilla: unas veces se lleva al DJ (1) y otras al de la barca (0)
    int ufoTarget2 (int t) const { return (((t - ufoStart) / ufoPeriod) % 2) == 0 ? 1 : 0; }

    void ufo (int t, int tx, int groundY, float day, const Camper* who, bool rower = false)
    {
        const int k = ufoPhase (t);
        if (k < 0) return;
        const int hoverY = top + 14;
        float ux = (float) tx, uy = (float) hoverY;
        if (k < 60) { const float u = k / 60.f; ux = tx + (1.f - u) * 90.f; uy = hoverY - (1.f - u) * 40.f; }
        else if (k >= 240) { const float u = (k - 240) / 60.f; ux = tx - u * u * 160.f; uy = hoverY - u * u * 60.f; }
        uy += std::sin (t * .25f) * (k >= 150 && k < 180 ? 1.5f : .6f);   // se tambalea con alguien dentro

        // rayo de luz
        const bool beam = (k >= 70 && k < 150) || (k >= 175 && k < 215);
        if (beam)
        {
            const int y0 = juce::roundToInt (uy) + 3, y1 = groundY;
            for (int y = y0; y <= y1; ++y)
            {
                const float u = (float) (y - y0) / (float) juce::jmax (1, y1 - y0);
                const int half = 3 + juce::roundToInt (u * 6.f);
                const bool stripe = ((y + t / 2) % 4) == 0;
                pxa ((float) (juce::roundToInt (ux) - half), (float) y, half * 2 + 1, 1, stripe ? 0xffeaffb0 : 0xffc8ff7a, stripe ? .38f : .2f);
            }
        }
        // la persona flotando en el rayo, con los brazos arriba
        const float lift = ufoLift (k);
        if (lift > 0.f && lift < 1.f)
        {
            const int yb = groundY - juce::roundToInt (lift * (groundY - uy - 4));
            if (who != nullptr) figure (tx, yb, 1, up, up, true, who->shirt, who->pants, who->hair, who->skin);
            else if (rower) figure (tx, yb, 1, up, up, true, 0xff4fa0b4, 0xff2a2a32, 0xfff2d16b, 0xfff0be96);   // el de la guitarra
            else figure (tx, yb, -1, up, up, true, 0xff141018, 0xff2a2a32, 0xff2a1a12, 0xffc68642);   // el DJ
        }
        // el platillo
        const int xi = juce::roundToInt (ux), yi = juce::roundToInt (uy);
        px ((float) (xi - 3), (float) (yi - 3), 7, 3, 0xff7ad3ff);          // cúpula
        px ((float) (xi - 2), (float) (yi - 4), 5, 1, 0xff7ad3ff);
        pxa ((float) (xi - 1), (float) (yi - 3), 2, 1, 0xffffffff, .6f);
        px ((float) (xi - 8), (float) yi, 17, 2, 0xff9a9aaa);               // disco
        px ((float) (xi - 6), (float) (yi + 2), 13, 1, 0xff5a5a6a);
        px ((float) (xi - 9), (float) (yi + 1), 1, 1, 0xff5a5a6a); px ((float) (xi + 9), (float) (yi + 1), 1, 1, 0xff5a5a6a);
        static const juce::uint32 lights[] = { 0xffff5a1f, 0xfffff07a, 0xff4fe3c1, 0xffff7aa8 };
        for (int i = 0; i < 5; ++i) px ((float) (xi - 6 + i * 3), (float) yi, 1, 1, lights[(i + t / 3) % 4]);
        juce::ignoreUnused (day);
    }

    //==========================================================================
    // RAVER CON MOCHILA COHETE: cada 3 minutos (dos minutos después de la avioneta) cruza el cielo
    void jetpack (int t)
    {
        const int start = 1950, period = 2700, len = 260;
        if (t < start) return;
        const int k = (t - start) % period;
        if (k >= len) return;
        const float x = -10.f + k * ((float) W + 30.f) / (float) len;
        const float y = (float) top + 26.f + std::sin (k * .09f) * 5.f;
        const int xi = juce::roundToInt (x), yi = juce::roundToInt (y);
        // estela de humo
        for (int i = 1; i < 10; ++i)
        {
            const float sx = x - 4.f - i * 3.f, sy = y + 4.f + std::sin ((k - i * 3) * .09f) * 5.f - (y - ((float) top + 26.f + std::sin (k * .09f) * 5.f));
            pxa (sx, sy + i * .3f, i > 5 ? 2 : 1, i > 5 ? 2 : 1, 0xffd8d0e8, .55f * (1.f - i / 10.f));
        }
        // llama
        px ((float) (xi - 3), (float) (yi + 2), 1, 2 + (t / 2) % 2, 0xffffb000);
        px ((float) (xi - 3), (float) (yi + 4 + (t / 2) % 2), 1, 1, 0xffff5a1f);
        // mochila
        px ((float) (xi - 4), (float) (yi - 4), 2, 5, 0xff6a6a7a);
        // persona en postura de vuelo, con gafas de sol y el brazo adelante
        figure (xi, yi + 4, 1, down, up, true, 0xffff7aa8, 0xff2a3a6a, 0xfff2d16b, 0xfff1c27d);
        px ((float) xi, (float) (yi - 5), 2, 1, 0xff141018);   // gafas
    }

    // sitios fijos del lago
    static constexpr int tentX0 = 20, tentB0 = 99, tentX1 = 176, tentB1 = 99, horizon = 93;
    static constexpr int djX = 96, djB = 87;

    void paintScene()
    {
        const int t = info.t;
        // en la carpa la fiesta va de noche cerrada hasta que amanece
        const float tod = .72f * juce::jlimit (0.f, 1.f, info.tod);
        const bool playing = info.beat >= 0.0;
        const float beatFrac = playing ? (float) (info.beat - std::floor (info.beat)) : 1.f;
        const float pulse = playing ? std::pow (1.f - beatFrac, 3.f) : 0.f;
        const float lvl = juce::jlimit (0.f, 1.f, info.level);
        const int beatNo = playing ? (int) std::fmod (std::floor (info.beat), 1048576.0) : t / 8;
        const int ox = (W - 192) / 2;

        const float day = smooth (.55f, 1.f, tod);          // luz del día
        const float dawn = smooth (.4f, .7f, tod) * (1.f - smooth (.8f, 1.f, tod));   // tonos rosas del amanecer
        const float starsA = 1.f - smooth (.4f, .72f, tod);
        const float fireA = 1.f - .85f * smooth (.7f, 1.f, tod);
        juce::ignoreUnused (fireA);

        // cielo: noche -> madrugada -> amanecer -> mañana
        static const juce::uint32 skyK[4][5] = {
            { 0xff140b26, 0xff1b1030, 0xff24163f, 0xff2e1c52, 0xff3b2766 },
            { 0xff1a1038, 0xff241648, 0xff2f1d5c, 0xff40296e, 0xff5e3a7c },
            { 0xff3a2a6a, 0xff6a3f86, 0xffb0558a, 0xffe07a6a, 0xfff2a65a },
            { 0xff4a8fd0, 0xff62a4dc, 0xff7ab8e4, 0xff9acbe8, 0xffc2e0ea } };
        static const float stops[4] = { 0.f, .4f, .68f, 1.f };
        int seg = 0; while (seg < 2 && tod > stops[seg + 1]) ++seg;
        const float u = smooth (stops[seg], stops[seg + 1], tod);
        juce::Colour sky[5];
        for (int i = 0; i < 5; ++i) sky[i] = mix (skyK[seg][i], skyK[seg + 1][i], u);
        const int skyH = horizon - top, bh = (skyH + 4) / 5;
        for (int i = 0; i < 5; ++i) pxc (0, (float) (top + i * bh), W, bh, sky[i]);
        for (int i = 0; i < 4; ++i)
            for (int x = i % 2; x < W; x += 2) pxc ((float) x, (float) (top + (i + 1) * bh - 1), 1, 1, sky[i + 1]);

        // estrellas que titilan (se apagan al amanecer)
        const int starH = juce::jmax (8, 74 - top);
        const int stars = W * starH / 110;
        for (int i = 0; i < stars; ++i)
        {
            const float x = std::floor (rnd() * W), y = (float) top + std::floor (rnd() * starH);
            const bool big = rnd() < .1f;
            const float ph = rnd() * 6.28f, sp = .06f + rnd() * .16f;
            const float tw = .5f + .5f * std::sin (ph + (float) t * sp);
            if (starsA <= 0.01f) continue;
            pxa (x, y, 1, 1, 0xfff4e9c9, (.3f + .7f * tw) * starsA);
            if (big && tw > .55f) { pxa (x - 1, y, 3, 1, 0xfff4e9c9, tw * starsA); pxa (x, y - 1, 1, 3, 0xfff4e9c9, tw * starsA); }
        }
        // estrella fugaz (solo de noche)
        if (starsA > .5f)
        {
            const int cyc = t / 150, ct = t % 150;
            if (ct < 14)
            {
                juce::Random r ((juce::int64) cyc * 7919 + 3);
                const float hx = (float) (60 + r.nextInt (juce::jmax (1, W - 70))) - ct * 4.f, hy = (float) (top + 3 + r.nextInt (12)) + ct * 1.6f;
                for (int k = 0; k < 8; ++k)
                    pxa (hx + k * 2.f, hy - k * .8f, k == 0 ? 2 : 1, 1, 0xfffff4d6, (1.f - k / 8.f) * (ct < 10 ? 1.f : (14 - ct) / 4.f) * starsA);
            }
        }

        // luna que baja y se apaga; sol que sale por la izquierda
        {
            const float moonA = 1.f - smooth (.55f, .85f, tod);
            const int mx = W - 40, my = top + 15 + juce::roundToInt (tod * 46.f), mr = 7;
            if (moonA > .01f)
                for (int y = -mr - 4; y <= mr + 4; ++y)
                    for (int x = -mr - 4; x <= mr + 4; ++x)
                    {
                        const int d2 = x * x + y * y;
                        if (d2 <= mr * mr) pxa ((float) (mx + x), (float) (my + y), 1, 1, 0xfff4e9c9, moonA);
                        else if (d2 <= (mr + 4) * (mr + 4) && ((x + y) & 1) == 0) pxa ((float) (mx + x), (float) (my + y), 1, 1, 0xfff4e9c9, (.12f + .06f * std::sin (t * .05f)) * moonA);
                    }
            if (moonA > .01f) { pxa ((float) (mx - 3), (float) (my - 2), 2, 2, 0xffcfc2a0, moonA); pxa ((float) (mx + 2), (float) (my + 2), 2, 1, 0xffcfc2a0, moonA); }

            const float sunUp = smooth (.55f, 1.f, tod);
            if (sunUp > 0.f)
            {
                const int sx = 146, sy = horizon + 2 - juce::roundToInt (sunUp * 44.f), sr = 9;
                const auto core = mix (0xffff9a4a, 0xfffff2b0, sunUp);
                for (int y = -sr - 5; y <= sr + 5; ++y)
                    for (int x = -sr - 5; x <= sr + 5; ++x)
                    {
                        const int d2 = x * x + y * y;
                        if (d2 <= sr * sr) pxc ((float) (sx + x), (float) (sy + y), 1, 1, core);
                        else if (d2 <= (sr + 5) * (sr + 5) && ((x + y) & 1) == 0) pxc ((float) (sx + x), (float) (sy + y), 1, 1, core.withAlpha (.35f));
                    }
            }
        }

        // nubes que pasan despacio (violetas de noche, rosas al amanecer, blancas de día)
        const auto cloudC = mix (0xff4a3a72, 0xffe8a0b0, dawn * 1.4f).interpolatedWith (juce::Colour (0xfff4f4f4), day);
        for (int c = 0; c < 2; ++c)
        {
            const float cw = 26.f + c * 10.f;
            const float cx = std::fmod ((float) t * (.12f + c * .07f) + c * 140.f, (float) W + cw * 2) - cw;
            const float cy = (float) top + 8 + c * 13;
            pxc (cx, cy, (int) cw, 3, cloudC.withAlpha (.8f));
            pxc (cx + 5, cy - 2, (int) (cw * .55f), 2, cloudC.withAlpha (.8f));
            pxc (cx + 3, cy + 3, (int) (cw - 6), 1, cloudC.darker (.2f).withAlpha (.8f));
        }

        // pájaros: de madrugada en adelante, bandadas en uve que cruzan por detrás de los árboles
        if (tod > .45f)
        {
            const int flocks = tod > .8f ? 3 : (tod > .6f ? 2 : 1);
            const auto birdC = mix (0xff1a1028, 0xff2a2a3a, day);
            for (int f = 0; f < flocks; ++f)
            {
                const float span = (float) W + 80.f;
                const float fxp = std::fmod ((float) t * (.45f + f * .12f) + f * 97.f, span) - 40.f;
                const float fyp = (float) top + 10.f + f * 9.f + std::sin (t * .03f + f) * 3.f;
                const int n = 3 + f;
                for (int b = 0; b < n; ++b)
                {
                    // formación en uve: el primero delante, los demás detrás arriba y abajo
                    const int rank = (b + 1) / 2;
                    const float side = b == 0 ? 0.f : ((b % 2) ? -1.f : 1.f);
                    const float xx = fxp - rank * 5.f, yy = fyp + side * rank * 3.f;
                    const bool flap = ((t / 3) + b + f) % 2 == 0;
                    if (flap) { pxc (xx - 2, yy - 1, 1, 1, birdC); pxc (xx - 1, yy, 1, 1, birdC); pxc (xx, yy, 1, 1, birdC); pxc (xx + 1, yy - 1, 1, 1, birdC); }
                    else      { pxc (xx - 2, yy, 2, 1, birdC); pxc (xx, yy, 2, 1, birdC); pxc (xx - 1, yy + 1, 2, 1, birdC); }
                }
            }
        }
        else if (tod < .3f && (t / 200) % 2 == 0)
        {
            // algún murciélago de noche
            const float bx = std::fmod ((float) t * .9f, (float) W + 40.f) - 20.f, by = (float) top + 22.f + std::sin (t * .3f) * 4.f;
            const bool flap = (t / 2) % 2 == 0;
            pxc (bx - 2, by + (flap ? -1.f : 0.f), 2, 1, juce::Colour (0xff0e0818));
            pxc (bx, by, 1, 2, juce::Colour (0xff0e0818));
            pxc (bx + 1, by + (flap ? -1.f : 0.f), 2, 1, juce::Colour (0xff0e0818));
        }

        planeBanner (t, day);
        jetpack (t);

        // ---------------- LA CARPA ----------------
        g.saveState();
        g.addTransform (juce::AffineTransform::translation ((float) ox, 0.f));
        const bool kick = lvl > .55f;                        // la gente salta con cada bombo
        const float energy = playing ? .45f + .55f * lvl : .18f;

        // focos que barren el cielo desde detrás de la carpa
        {
            const juce::uint32 fc[2] = { 0xffff7aa8, 0xff7ad3ff };
            for (int k = 0; k < 2; ++k)
            {
                const float sweep = std::sin (t * (.035f + k * .012f) + k * 2.1f) * 14.f;
                const int baseX = k == 0 ? 72 : 120;
                for (int y = top; y < 73; ++y)
                {
                    const float k2 = (73 - y) / 36.f;
                    const int w = juce::roundToInt (k2 * 6.f);
                    pxa ((float) baseX + sweep * k2 - w, (float) y, 2 * w + 1, 1, fc[k], .10f * energy * (1.f - day));
                }
            }
        }
        // láseres desde la punta de la carpa
        {
            static const juce::uint32 lc[5] = { 0xff4fe3c1, 0xffff5a8a, 0xff7ad3ff, 0xffc8ff7a, 0xffff5a1f };
            static const float base[5] = { -1.2f, -.6f, .5f, 1.1f, -.2f };
            const int nOn = playing ? 5 : 2;
            for (int i = 0; i < nOn; ++i)
            {
                const float ang = base[i] + (playing ? std::sin (t * .06f + i * 1.3f + (float) beatNo * .7f) * .25f : std::sin (t * .02f + i) * .15f);
                const float a = (playing ? (kick ? .75f : .45f) : .25f) * (1.f - .7f * day);
                for (int k = 0; k < 90; k += 1)
                {
                    const float x = 96.f + std::sin (ang) * k * 1.4f, y = 64.f - std::cos (ang) * k * .9f;
                    if (y < (float) top || x < -2.f || x > 194.f) break;
                    if (((k + t) % 4) != 0 || ! playing) pxa (x, y, 1, 1, lc[i], a);
                }
            }
        }

        // bosque al fondo
        {
            juce::Random r (21);
            const auto treeC = mix (0xff101828, 0xff2a4a3a, day).interpolatedWith (juce::Colour (0xff3a2440), dawn * .6f);
            for (int x = -4; x < 198; x += 7) pine (x, 95, 14 + r.nextInt (10), treeC, treeC);
        }
        // suelo
        {
            const auto grassC = mix (0xff14281c, 0xff4a7a34, day).interpolatedWith (juce::Colour (0xff4a5a2c), dawn * .5f);
            pxc (0, 93, 192, 15, grassC);
            const auto dith = grassC.darker (.3f);
            for (int y = 95; y < 108; y += 2)
                for (int x = ((y / 2) % 2) * 2; x < 192; x += 4) pxc ((float) x, (float) y, 1, 1, dith);
        }

        // carpa de rayas
        const int cx = 96, topY = 59, baseY = 95;
        for (int y = topY; y < baseY; ++y)
        {
            const int half = (int) ((y - topY) * 1.25f) + 4;
            for (int x = cx - half; x <= cx + half; )
            {
                const bool stripe = (((x - cx) + 100) / 5) % 2 == 0;
                const int run = juce::jmin (cx + half + 1 - x, 5 - ((x - cx) + 100) % 5);
                px ((float) x, (float) y, run, 1, stripe ? 0xffc8323c : 0xfff3ebdd);
                x += run;
            }
        }
        px ((float) (cx - 1), (float) (topY - 6), 2, 6, 0xff2a1a12);
        {
            const int wv = (t / 4) % 2;
            px ((float) (cx + 1), (float) (topY - 6), 5, 3, 0xffffb000);
            px ((float) (cx + 6), (float) (topY - 6 + wv), 1, 2, 0xffffb000);
        }
        // boca de la carpa: la pista
        for (int y = 75; y < baseY; ++y)
        {
            const int half = (int) ((y - 75) * 1.1f) + 8;
            px ((float) (cx - half), (float) y, 2 * half + 1, 1, 0xff2a1030);
        }
        // luces de dentro (cambian con cada tiempo)
        {
            static const juce::uint32 lc[5] = { 0xffff5a1f, 0xfffff07a, 0xff4fe3c1, 0xffff7aa8, 0xff7ad3ff };
            for (int i = 0; i < 5; ++i)
            {
                const int ci = (i + (playing ? beatNo : t / 10)) % 5;
                pxa ((float) (cx - 18 + i * 9), 79.f, 5, 3, lc[ci], playing ? (kick ? .9f : .55f) : .35f);
            }
        }
        // gente bailando dentro (siluetas que saltan con el bombo)
        {
            static const int dxs[4] = { 74, 82, 110, 118 };
            for (int i = 0; i < 4; ++i)
            {
                const bool jump = playing ? (kick && (i + beatNo) % 2 == 0) || (kick && i == 1) : ((t / 6 + i) % 4 == 0);
                figure (dxs[i], baseY + (jump ? -2 : 0), dxs[i] < cx ? 1 : -1, jump ? up : down, jump ? up : wave2, jump,
                        0xff5a2a5a, 0xff1e1028, 0xff140a18, 0xff8a5a6a);
            }
        }

        // el DJ (el platillo a veces se lo lleva)
        const int ok = ufoPhase (t);
        {
            const int dx = djX, db = djB;
            const bool djGone = ok >= 0 && ufoTargetIndex (t) < 0 && ufoLift (ok) > 0.f;
            const bool nod = playing ? pulse > .45f : (t / 6) % 2 == 0;
            const int bar = playing ? beatNo % 8 : (t / 12) % 8;
            const Arm handUp = bar == 6 ? up : (bar == 7 ? wave2 : forward);
            if (! djGone)
            {
                figure (dx, db + (nod ? 1 : 0), 1, handUp, forward, true, 0xff141018, 0xff2a2a32, 0xff2a1a12, 0xffc68642);
                const int hy = db + (nod ? 1 : 0) - 10;
                px ((float) (dx - 2), (float) hy, 1, 2, 0xff141018);
                px ((float) (dx + 2), (float) hy, 1, 2, 0xff141018);
                px ((float) (dx - 1), (float) (hy - 2), 3, 1, 0xff141018);
                px ((float) (dx + 2), (float) hy, 1, 1, 0xffff5a1f);
            }
            px ((float) (dx - 7), (float) db, 15, 1, 0xff141018);
            px ((float) (dx - 7), (float) (db + 1), 15, 4, 0xff2a2a32);
            const juce::uint32 leds[] = { 0xffff5a1f, 0xfffff07a, 0xff4fe3c1, 0xffff7aa8 };
            for (int k = 0; k < 4; ++k)
            {
                const bool on = playing ? ((beatNo + k) % 4 == 0 || pulse > .6f) : ((t / 5 + k) % 4 == 0);
                px ((float) (dx - 5 + k * 3), (float) (db + 3), 1, 1, on ? leds[k] : 0xff3a3a44);
            }
        }
        // altavoces: la membrana late con el bombo
        for (int sx : { cx - 26, cx + 20 })
        {
            px ((float) sx, 81.f, 7, 12, 0xff141018);
            px ((float) (sx + 1), 82.f, 5, 10, 0xff2a2a32);
            if (kick) px ((float) (sx + 1), 84.f, 5, 5, 0xff8a8a9a);
            else      px ((float) (sx + 2), 85.f, 3, 3, 0xff6a6a7a);
            px ((float) (sx + 3), 90.f, 1, 1, 0xff6a6a7a);
        }
        // guirnaldas de bombillas por los bordes de la carpa
        {
            const juce::uint32 bul[] = { 0xffff5a1f, 0xfffff07a, 0xff4fe3c1, 0xffff7aa8 };
            for (int side = -1; side <= 1; side += 2)
                for (int i = 0; i < 26; ++i)
                {
                    if (i % 3 != 1) continue;
                    const float x = (float) cx + side * (10.f + i * 2.6f), y = 61.f + i * 1.1f + 2.f * std::sin (i / 3.f);
                    const bool on = playing ? ((i / 3 + beatNo) % 2 == 0 || kick) : ((i / 3 + t / 6) % 4 != 0);
                    if (on) { pxa (x - 1, y, 3, 3, bul[(i / 3) % 4], .35f); px (x, y + 1, 1, 1, bul[(i / 3) % 4]); }
                    else px (x, y + 1, 1, 1, 0xff3a3020);
                }
        }

        // tiendas de campaña a los lados (una puede estar "ocupada")
        bool love[2] = { false, false };
        if (info.crowd != nullptr)
            for (auto& c : *info.crowd)
                if (c.action == Camper::tentLove && t - c.born > 6) love[c.tent] = true;
        auto shake = [&] (int i) { return love[i] ? (((t / 2) % 3) - 1) : 0; };
        tent (tentX0 + shake (0), tentB0, 22, 12, 0xffffb000, 0xffc46a00, 0xff3a1a00);
        tent (tentX1 + shake (1), tentB1, 20, 11, 0xff4fe3c1, 0xff1f9e8a, 0xff0e3b34);
        for (int i = 0; i < 2; ++i)
            if (love[i])
                for (int h = 0; h < 2; ++h)
                {
                    const int a = (t + h * 15) % 30;
                    heart ((float) ((i == 0 ? tentX0 : tentX1) + h * 4 - 4) + std::sin ((t + h * 7) * .2f), (float) ((i == 0 ? tentB0 - 14 : tentB1 - 13) - a / 2), 1.f - a / 30.f);
                }

        // gente, de atrás hacia delante
        if (info.crowd != nullptr)
        {
            std::vector<const Camper*> order;
            for (auto& c : *info.crowd) order.push_back (&c);
            std::sort (order.begin(), order.end(), [] (const Camper* a, const Camper* b) { return slotPos (a->slot).y < slotPos (b->slot).y; });
            const int ti = ok >= 0 ? ufoTargetIndex (t) : -1;
            const Camper* taken = ti >= 0 ? &(*info.crowd)[(size_t) ti] : nullptr;
            for (auto* c : order)
                if (! (c == taken && ufoLift (ok) > 0.f)) camper (*c, t, playing, beatNo, kick ? 1.f : 0.f);
        }
        // platillo volante: se lleva a alguien; si no hay nadie, al DJ
        if (ok >= 0)
        {
            const int ti = ufoTargetIndex (t);
            if (ti >= 0)
            {
                const auto& c = (*info.crowd)[(size_t) ti];
                const auto p = slotPos (c.slot);
                ufo (t, p.x, p.y, day, &c);
            }
            else ufo (t, djX, djB, day, nullptr);
        }
        g.restoreState();

        // confeti cuando suena fuerte
        if (playing)
            for (int i = 0; i < 10; ++i)
            {
                static const juce::uint32 cc[] = { 0xffff5a1f, 0xfffff07a, 0xff4fe3c1, 0xffff7aa8, 0xff9a7cff };
                const float x = (float) ox + 40.f + std::fmod (i * 37.f + t * (.3f + (i % 3) * .1f), 112.f);
                const float y = 62.f + std::fmod (t * (.5f + (i % 4) * .15f) + i * 11.f, 34.f);
                pxa (x + std::sin (t * .2f + i) * 2.f, y, 1, 1, cc[i % 5], .8f);
            }
    }
};

//==============================================================================
// Caja con esquinas redondeadas a base de píxeles
inline void radioBox (juce::Graphics& g, juce::Rectangle<int> r, juce::Colour fill, juce::Colour border, int bw = 3, int corner = 4)
{
    g.setColour (border);
    g.fillRect (r.reduced (corner, 0));
    g.fillRect (r.reduced (0, corner));
    g.fillRect (r.reduced (corner / 2, corner / 2));
    auto in = r.reduced (bw);
    g.setColour (fill);
    g.fillRect (in.reduced (corner / 2, 0));
    g.fillRect (in.reduced (0, corner / 2));
}

// Pantalla LED encendida en 8 bits: marco hundido, fondo retroiluminado por bloques de píxel,
// rejilla de puntos apagados, reflejo arriba y líneas de escaneo
inline void ledScreenRaw (juce::Graphics& g, juce::Rectangle<int> r, juce::Colour glow)
{
    constexpr int P = 3;
    // marco: contorno oscuro y bisel hundido (sombra arriba a la izquierda, luz abajo a la derecha)
    radioBox (g, r, col::outline, col::outline, 2, 4);
    auto in = r.reduced (2);
    g.setColour (col::khakiDark.darker (.35f));
    g.fillRect (in.getX() + 2, in.getY(), in.getWidth() - 4, 2);
    g.fillRect (in.getX(), in.getY() + 2, 2, in.getHeight() - 4);
    g.setColour (col::khakiLight.withAlpha (.55f));
    g.fillRect (in.getX() + 2, in.getBottom() - 2, in.getWidth() - 4, 2);
    g.fillRect (in.getRight() - 2, in.getY() + 2, 2, in.getHeight() - 4);
    auto scr = in.reduced (2);

    // fondo: retroiluminación más fuerte en el centro, en bloques de 3x3 con tramado
    const auto base = col::display;
    const auto lit = base.interpolatedWith (glow, .32f);
    const float cx = (float) scr.getCentreX(), cy = (float) scr.getCentreY();
    const float rx = scr.getWidth() * .55f, ry = scr.getHeight() * .75f;
    for (int y = scr.getY(); y < scr.getBottom(); y += P)
        for (int x = scr.getX(); x < scr.getRight(); x += P)
        {
            const float dx = (x + P * .5f - cx) / rx, dy = (y + P * .5f - cy) / ry;
            float k = juce::jlimit (0.f, 1.f, 1.f - (dx * dx + dy * dy));
            // tramado: cuantiza el brillo en 4 niveles alternando bloques, como en las consolas
            const int bx = (x - scr.getX()) / P, by = (y - scr.getY()) / P;
            const float bayer = ((bx + by * 2) % 4) / 4.f;
            k = std::floor (k * 3.f + bayer) / 3.f;
            g.setColour (base.interpolatedWith (lit, k));
            g.fillRect (x, y, juce::jmin (P, scr.getRight() - x), juce::jmin (P, scr.getBottom() - y));
        }
    // rejilla de LEDs apagados
    g.setColour (glow.withAlpha (.07f));
    for (int y = scr.getY() + 1; y < scr.getBottom() - 1; y += P)
        for (int x = scr.getX() + 1; x < scr.getRight() - 1; x += P)
            g.fillRect (x, y, 1, 1);
    // reflejo del cristal arriba y un brillo en la esquina
    g.setColour (juce::Colours::white.withAlpha (.07f));
    g.fillRect (scr.getX() + P, scr.getY() + 1, scr.getWidth() - 2 * P, P);
    g.setColour (juce::Colours::white.withAlpha (.12f));
    g.fillRect (scr.getX() + P, scr.getY() + 1, P * 2, P);
    g.fillRect (scr.getX() + P, scr.getY() + 1 + P, P, P);
    scanlines (g, scr, .16f);
}

// Versión con memoria: cada tamaño de pantalla se dibuja una sola vez (a la resolución real) y se reutiliza
inline void ledScreen (juce::Graphics& g, juce::Rectangle<int> r, juce::Colour glow = juce::Colour (0xfff2a33a))
{
    const float sc = juce::jlimit (.5f, 4.f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const juce::String key = juce::String (r.getWidth()) + "x" + juce::String (r.getHeight()) + "@" + juce::String (juce::roundToInt (sc * 100)) + "/" + glow.toString();
    static std::map<juce::String, juce::Image> cache;
    auto it = cache.find (key);
    if (it == cache.end())
    {
        if (cache.size() > 64) cache.clear();
        juce::Image img (juce::Image::ARGB, juce::roundToInt (r.getWidth() * sc), juce::roundToInt (r.getHeight() * sc), true, juce::SoftwareImageType());
        {
            juce::Graphics gi (img);
            gi.addTransform (juce::AffineTransform::scale (sc));
            ledScreenRaw (gi, { 0, 0, r.getWidth(), r.getHeight() }, glow);
        }
        it = cache.emplace (key, juce::NativeImageType().convert (img)).first;
    }
    g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
    g.drawImage (it->second, r.toFloat());
}

// Texto que brilla en una pantalla LED: halo amplio, halo cercano y núcleo claro
inline void ledText (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area, juce::Justification just,
                     juce::Colour c = juce::Colour (0xfff2a33a), int lines = 1)
{
    g.setColour (c.withAlpha (.12f));
    for (auto o : { juce::Point<int> (-2, 0), juce::Point<int> (2, 0), juce::Point<int> (0, -2), juce::Point<int> (0, 2) })
        g.drawFittedText (text, area.translated (o.x, o.y), just, lines, 1.f);
    g.setColour (c.withAlpha (.3f));
    for (auto o : { juce::Point<int> (-1, 0), juce::Point<int> (1, 0), juce::Point<int> (0, -1), juce::Point<int> (0, 1) })
        g.drawFittedText (text, area.translated (o.x, o.y), just, lines, 1.f);
    g.setColour (c.interpolatedWith (juce::Colour (0xfffff4d6), .55f));
    g.drawFittedText (text, area, just, lines, 1.f);
}

inline void raisedButton (juce::Graphics& g, juce::Rectangle<int> r, juce::Colour face, bool down)
{
    if (! down)
    {
        radioBox (g, r, col::outline, col::outline);
        r = r.withTrimmedBottom (4);
    }
    else r = r.withTrimmedTop (3);
    radioBox (g, r, face, col::outline);
    g.setColour (face.brighter (.25f));
    g.fillRect (r.getX() + 5, r.getY() + 3, r.getWidth() - 10, 2);
    g.setColour (face.darker (.25f));
    g.fillRect (r.getX() + 5, r.getBottom() - 5, r.getWidth() - 10, 2);
}

class PixelLook : public juce::LookAndFeel_V4
{
public:
    PixelLook()
    {
        using namespace juce;
        setColour (ComboBox::textColourId, col::amber);
        setColour (PopupMenu::backgroundColourId, col::display);
        setColour (PopupMenu::textColourId, col::amber);
        setColour (PopupMenu::highlightedBackgroundColourId, col::amber);
        setColour (PopupMenu::highlightedTextColourId, col::display);
        setColour (TooltipWindow::backgroundColourId, col::cream);
        setColour (TooltipWindow::textColourId, col::labelInk);
        setColour (TooltipWindow::outlineColourId, col::outline);
    }

    static juce::Colour faceFor (juce::Button& b)
    {
        auto& p = b.getProperties();
        if (p["primary"]) return juce::Colour (0xffe8902a);
        if (p["teal"]) return col::teal;
        if (p["round"]) return col::red;
        return b.getToggleState() ? juce::Colour (0xffe6c36e) : col::khakiLight;
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds();
        auto& p = b.getProperties();
        if (p["switch"])
        {
            auto tr = r.removeFromRight (58).withSizeKeepingCentre (54, 24);
            radioBox (g, tr, juce::Colour (0xff2a1016), col::outline, 3, 6);
            auto knob = tr.reduced (4).removeFromLeft (26);
            if (b.getToggleState()) knob.setX (tr.getRight() - 4 - 26);
            radioBox (g, knob, col::khakiLight, col::outline, 2, 4);
            g.setColour (col::khakiDark);
            for (int i = 0; i < 3; ++i) g.fillRect (knob.getX() + 8 + i * 4, knob.getY() + 5, 2, knob.getHeight() - 10);
            if (b.getToggleState())
            {
                g.setColour (col::amber);
                g.fillRect (tr.getX() + 8, tr.getCentreY() - 2, 6, 4);
            }
            return;
        }
        if (p["round"])
        {
            auto c = r.reduced (2).toFloat();
            g.setColour (col::outline);
            g.fillEllipse (c);
            g.setColour (b.getToggleState() ? juce::Colour (0xff7ad36a) : col::red);
            g.fillEllipse (c.reduced (3.f).translated (0, down ? 1.f : 0));
            g.setColour (juce::Colours::white.withAlpha (.45f));
            g.fillEllipse (c.getX() + c.getWidth() * .3f, c.getY() + c.getHeight() * .22f, c.getWidth() * .22f, c.getHeight() * .18f);
            return;
        }
        auto face = faceFor (b);
        if (over && ! b.getToggleState()) face = face.brighter (.08f);
        raisedButton (g, r, face, down || b.getToggleState());
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down) override
    {
        auto& p = b.getProperties();
        if (p["round"]) return;
        auto r = b.getLocalBounds();
        if (p["icon"].toString().isNotEmpty())
        {
            // flecha curva en pixel art (deshacer / rehacer)
            static const char* arrow[] = { "..X....", ".XX....", "XXXXXX.", ".XX...X", "..X...X", "......X", "...XXX." };
            const bool redo = p["icon"].toString() == "redo";
            const int sc = 3, w = 7 * sc;
            auto c = (down || b.getToggleState() ? r.withTrimmedTop (3) : r.withTrimmedBottom (4)).getCentre();
            g.setColour (b.isEnabled() ? col::labelInk : col::labelInk.withAlpha (.35f));
            for (int y = 0; y < 7; ++y)
                for (int x = 0; x < 7; ++x)
                    if (arrow[y][x] == 'X')
                        g.fillRect (c.x - w / 2 + (redo ? 6 - x : x) * sc, c.y - w / 2 + y * sc, sc, sc);
            return;
        }
        if (p["slot"])
        {
            const bool pressed = down || b.getToggleState();
            r = pressed ? r.withTrimmedTop (3) : r.withTrimmedBottom (4);
            g.setFont (fonts().t (10.f));
            g.setColour (col::labelInk);
            g.drawText (b.getButtonText(), r, juce::Justification::centred);
            if ((bool) p["filled"]) { g.setColour (col::amber); g.fillRect (r.getCentreX() - 3, r.getBottom() - 6, 6, 3); }
            return;
        }
        if (p["switch"])
        {
            g.setFont (fonts().b (19.f));
            g.setColour ((bool) p["light"] ? col::panelInk : col::labelInk);
            g.drawText (b.getButtonText(), r.withTrimmedRight (60), juce::Justification::centredLeft);
            return;
        }
        const bool pressed = down || b.getToggleState();
        r = pressed ? r.withTrimmedTop (3) : r.withTrimmedBottom (4);
        const bool primary = p["primary"];
        const auto ink = p["teal"] ? col::display : col::labelInk;
        const auto ic = p["ic"].toString();
        auto font = primary ? fonts().b (22.f) : fonts().b (16.f);
        const auto text = b.getButtonText().toUpperCase();
        if (ic.isNotEmpty())
        {
            const int sc = 2;
            const float tw = juce::GlyphArrangement::getStringWidth (font, text);
            const int total = 7 * sc + 7 + (int) tw;
            const int x0 = r.getCentreX() - total / 2;
            pixelIcon (g, ic, x0, r.getCentreY() - 7, sc, ink);
            g.setFont (font);
            g.setColour (ink);
            g.drawText (text, x0 + 7 * sc + 7, r.getY(), (int) tw + 4, r.getHeight(), juce::Justification::centredLeft);
            return;
        }
        g.setFont (font);
        g.setColour (ink);
        g.drawFittedText (text, r.reduced (5, 2), juce::Justification::centred, 2, .8f);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override { return fonts().b (17.f); }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        auto r = juce::Rectangle<int> (0, 0, w, h);
        auto arrows = r.removeFromRight (16);
        ledScreen (g, r.withTrimmedRight (2));
        radioBox (g, arrows, col::khakiLight, col::outline, 2, 3);
        g.setColour (col::outline);
        g.fillRect (arrows.getX() + 2, arrows.getCentreY() - 1, arrows.getWidth() - 4, 2);
        const int ax = arrows.getCentreX();
        for (int i = 0; i < 3; ++i)
        {
            g.fillRect (ax - i, arrows.getY() + 6 + i, i * 2 + 1, 1);
            g.fillRect (ax - i, arrows.getBottom() - 7 - i, i * 2 + 1, 1);
        }
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return fonts().b (16.f); }
    juce::Font getPopupMenuFont() override { return fonts().t (9.f); }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (5, 3, box.getWidth() - 24, box.getHeight() - 6);
        label.setFont (fonts().t (8.f));
        label.setJustificationType (juce::Justification::centred);
        label.setMinimumHorizontalScale (.85f);
    }

    // texto de los displays: pantalla LCD con letras de bloques en mayúsculas, brillo y líneas de escaneo
    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        if (dynamic_cast<juce::ComboBox*> (label.getParentComponent()) == nullptr) { LookAndFeel_V4::drawLabel (g, label); return; }
        const auto text = lcdText (label.getText());
        auto area = label.getLocalBounds().reduced (2, 1);
        g.setFont (fonts().t (8.f));
        ledText (g, text, area, juce::Justification::centred, col::amber, 2);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle, juce::Slider&) override
    {
        const int cy = y + h / 2;
        radioBox (g, { x, cy - 7, w, 14 }, juce::Colour (0xff2a1016), col::outline, 3, 6);
        auto t = juce::Rectangle<int> (juce::roundToInt (pos) - 9, cy - 13, 18, 26);
        radioBox (g, t, col::khakiLight, col::outline, 2, 4);
        g.setColour (col::khakiDark);
        g.fillRect (t.getX() + 5, t.getY() + 6, 2, t.getHeight() - 12);
        g.fillRect (t.getX() + 9, t.getY() + 6, 2, t.getHeight() - 12);
    }

    int getSliderThumbRadius (juce::Slider&) override { return 10; }

    // dial giratorio en pixel art
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override
    {
        const int px = 3;
        const int n = juce::jmin (w, h) / px;           // tamaño en píxeles gordos
        const float cxp = n * .5f, R = n * .5f - 2.f;
        const int ox = x + (w - n * px) / 2, oy = y + (h - n * px) / 2;
        auto P = [&] (int i, int j, juce::Colour c) { g.setColour (c); g.fillRect (ox + i * px, oy + j * px, px, px); };
        // marcas alrededor
        for (int t = 0; t <= 10; ++t)
        {
            const float a = a0 + (a1 - a0) * t / 10.f;
            const int i = (int) std::floor (cxp + std::sin (a) * (R + 1.5f)), j = (int) std::floor (cxp - std::cos (a) * (R + 1.5f));
            P (i, j, col::khakiDark);
        }
        // toda la rueda gira con el valor: dientes del borde y estrías siguen al indicador
        const float ang = a0 + pos * (a1 - a0);
        const float twoPi = juce::MathConstants<float>::twoPi;
        for (int j = 0; j < n; ++j)
            for (int i = 0; i < n; ++i)
            {
                const float dx = i + .5f - cxp, dy = j + .5f - cxp, d = std::sqrt (dx * dx + dy * dy);
                if (d > R) continue;
                // ángulo del píxel medido como el del indicador (0 arriba, sentido horario), relativo al giro actual
                float rel = std::atan2 (dx, -dy) - ang;
                rel -= twoPi * std::floor (rel / twoPi);
                const float tooth = rel * 14.f / twoPi;
                if (d > R - 1.5f)
                    P (i, j, (tooth - std::floor (tooth)) < .5f ? col::outline : col::khakiDark);   // dientes del borde
                else
                {
                    auto c = (dx + dy) < -2.f ? col::khakiLight.brighter (.15f) : ((dx + dy) > 3.f ? col::khakiDark : col::khakiLight);
                    // estrías interiores que giran
                    if (d > R - 3.6f && (tooth - std::floor (tooth)) < .34f) c = c.darker (.35f);
                    P (i, j, c);
                }
            }
        // indicador ámbar
        for (float r = 1.f; r < R - 2.f; r += .7f)
            P ((int) std::floor (cxp + std::sin (ang) * r), (int) std::floor (cxp - std::cos (ang) * r), r > R - 4.f ? col::amberHi : col::amber);
    }
};
} // namespace ddc
