#include "PluginEditor.h"

using namespace ddc;

namespace
{
constexpr int NK = 13;   // número de diales de la fila de abajo
const char* knobIds[NK]    = { "density", "variation", "lvl", "pan", "col", "comp", "tune", "dec", "fillAmt", "verb", "vol", "swing", "humanize" };
bool isLaneKnob (int i) { return i == 2 || i == 3 || i == 4 || i == 6 || i == 7; }
const char* knobLabels[NK] = { "Densidad", "Variaci\xc3\xb3n", "Nivel", "Paneo", "Color", "Compresi\xc3\xb3n", "Tono", "Decay", "Fills", "Reverb", "Volumen", "Swing", "Humano" };
const char* lockIds[3]     = { "lockSound", "lockPattern", "lockFx" };

juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

// Pegatinas pixel art (árbol y tienda) con borde blanco
void sticker (juce::Graphics& g, int x0, int y0, int s, bool tree)
{
    auto P = [&] (int x, int y, int w, int h, juce::Colour c) { g.setColour (c); g.fillRect (x0 + x * s, y0 + y * s, w * s, h * s); };
    const auto white = juce::Colour (0xfff7f4ea);
    if (tree)
    {
        for (int pass = 0; pass < 2; ++pass)
        {
            const int o = pass == 0 ? 1 : 0;
            for (int t = 0; t < 3; ++t)
                for (int y = 0; y < 5; ++y)
                {
                    const int w = y + t + 1, yy = t * 4 + y;
                    P (7 - w - o, yy - o + 1, w * 2 + 1 + o * 2, 1 + o * 2, pass == 0 ? white : (y % 2 ? juce::Colour (0xff3f6b3a) : juce::Colour (0xff548a4a)));
                }
            P (6 - o, 13 - o, 3 + o * 2, 3 + o * 2, pass == 0 ? white : juce::Colour (0xff6a3b1a));
        }
    }
    else
    {
        for (int pass = 0; pass < 2; ++pass)
        {
            const int o = pass == 0 ? 1 : 0;
            for (int y = 0; y < 9; ++y)
            {
                const int half = y + 1;
                if (pass == 0) P (8 - half - o, y + 1 - o, half * 2 + 1 + o * 2, 1 + o * 2, white);
                else { P (8 - half, y + 1, half, 1, juce::Colour (0xffe07a2a)); P (8, y + 1, half + 1, 1, juce::Colour (0xffb05a1a)); }
            }
            if (pass == 1) for (int y = 0; y < 5; ++y) P (8 - y / 2, 5 + y, y + 1, 1, juce::Colour (0xff4a2410));
        }
    }
}

// Pantallas del piano roll y del carril: se dibujan en baja resolución (1 píxel = 3x3) y se amplían
// sin suavizar, con el cielo nocturno de la escena de arriba.
constexpr int PX = 3;
// pantallas violetas del Chord Maker
const juce::uint32 nightSky[] = { 0xff0b1222, 0xff0e172b, 0xff111c34, 0xff15223d, 0xff1a2846 };   // pantallas azul marino
const juce::Colour lilac { 0xff9fb4dc };
const juce::Colour toneC { 0xffe9d6a6 };   // notas del acorde que no son la raíz

void nightScreenPx (juce::Graphics& q, juce::Rectangle<int> area, juce::Rectangle<int> gr)
{
    q.setColour (juce::Colour (nightSky[0]));
    q.fillRect (area);
    const int bh = juce::jmax (1, (gr.getHeight() + 4) / 5);
    for (int i = 0; i < 5; ++i)
    {
        const int y0 = gr.getY() + i * bh, h = juce::jmin (bh, gr.getBottom() - y0);
        if (h <= 0) break;
        q.setColour (juce::Colour (nightSky[i]));
        q.fillRect (gr.getX(), y0, gr.getWidth(), h);
        if (i < 4 && y0 + bh < gr.getBottom())   // tramado entre bandas, como en la escena
        {
            q.setColour (juce::Colour (nightSky[i + 1]));
            for (int x = gr.getX() + (i % 2); x < gr.getRight(); x += 2) q.fillRect (x, y0 + bh - 1, 1, 1);
        }
    }
    juce::uint32 seed = 1234;
    auto rnd = [&seed] { seed = seed * 1664525u + 1013904223u; return (float) ((seed >> 8) & 0xffffff) / 16777216.f; };
    const int n = gr.getWidth() * gr.getHeight() / 260;
    for (int i = 0; i < n; ++i)
    {
        const int x = gr.getX() + (int) (rnd() * gr.getWidth()), y = gr.getY() + (int) (rnd() * gr.getHeight());
        q.setColour (juce::Colour (0xfff4e9c9).withAlpha (.10f + rnd() * .25f));
        q.fillRect (x, y, 1, 1);
    }
}

// Nota en pixel art: borde oscuro con las esquinas recortadas, relleno y un brillo arriba
void pixelNote (juce::Graphics& q, juce::Rectangle<int> r, juce::Colour fill, juce::Colour border)
{
    if (r.getWidth() < 2 || r.getHeight() < 2) { q.setColour (fill); q.fillRect (r); return; }
    if (r.getWidth() < 6)
    {
        // nota estrecha (patrones de 4 compases): casi todo relleno, con brillo arriba y sombra abajo
        q.setColour (fill);
        q.fillRect (r);
        q.setColour (fill.brighter (.45f));
        q.fillRect (r.getX(), r.getY(), r.getWidth(), 1);
        q.setColour (border);
        q.fillRect (r.getX(), r.getBottom() - 1, r.getWidth(), 1);
        return;
    }
    q.setColour (border);
    q.fillRect (r.reduced (1, 0));
    q.fillRect (r.reduced (0, 1));
    q.setColour (fill);
    q.fillRect (r.reduced (1));
    if (r.getHeight() >= 4 && r.getWidth() >= 4)
    {
        q.setColour (fill.brighter (.45f));
        q.fillRect (r.getX() + 1, r.getY() + 1, r.getWidth() - 3, 1);
    }
}

// Copia la imagen de baja resolución a la pantalla, píxel a píxel
void blitPx (juce::Graphics& g, const juce::Image& img)
{
    g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
    g.drawImage (img, 0, 0, img.getWidth() * PX, img.getHeight() * PX, 0, 0, img.getWidth(), img.getHeight());
}

void screw (juce::Graphics& g, juce::Point<int> c)
{
    g.setColour (col::outline);
    g.fillEllipse ((float) c.x - 11.f, (float) c.y - 11.f, 22.f, 22.f);
    g.setColour (col::khakiDark);
    g.fillEllipse ((float) c.x - 8.f, (float) c.y - 8.f, 16.f, 16.f);
    g.setColour (col::outline);
    g.drawLine ((float) c.x - 6.f, (float) c.y + 4.f, (float) c.x + 6.f, (float) c.y - 4.f, 3.f);
}
}

//==============================================================================
// REJILLA DE PASOS
namespace
{
const juce::Colour navy0 { 0xff0b1222 }, navy1 { 0xff111c34 }, navyLine { 0xff28385a };
}

bool PianoRoll::cellAt (juce::Point<int> pos, int& lane, int& step) const
{
    const auto gr = grid();
    if (! gr.contains (pos)) return false;
    const int L = proc.getLength();
    step = juce::jlimit (0, L - 1, (int) ((pos.x - gr.getX()) * L / (float) gr.getWidth()));
    lane = juce::jlimit (0, ddc::numDrums - 1, (int) ((pos.y - gr.getY()) * ddc::numDrums / (float) gr.getHeight()));
    return true;
}

int PianoRoll::laneAtLabel (juce::Point<int> pos) const
{
    const auto gr = grid();
    if (pos.x >= gr.getX() || pos.y < gr.getY() || pos.y >= gr.getBottom()) return -1;
    return juce::jlimit (0, ddc::numDrums - 1, (int) ((pos.y - gr.getY()) * ddc::numDrums / (float) gr.getHeight()));
}

juce::Rectangle<int> PianoRoll::diceRect (int segStart) const
{
    const auto gr = grid();
    const int x = gr.getX() + juce::roundToInt (segStart * gr.getWidth() / (float) proc.getLength()) + 3;
    return { x, gr.getBottom() + 3, 14, 14 };
}

int PianoRoll::segmentAt (juce::Point<int> pos) const
{
    const int L = proc.getLength(), seg = segmentLength();
    for (int s0 = 0; s0 < L; s0 += seg)
        if (diceRect (s0).expanded (3).contains (pos)) return s0;
    return -1;
}

void PianoRoll::paint (juce::Graphics& g)
{
    using namespace ddc;
    const int L = proc.getLength();
    const auto gr = grid();
    const float cw = gr.getWidth() / (float) L, rh = gr.getHeight() / (float) numDrums;
    auto colX = [&] (int i) { return gr.getX() + juce::roundToInt (i * cw); };
    auto rowY = [&] (int r) { return gr.getY() + juce::roundToInt (r * rh); };
    const auto p = proc.getDrumPattern();
    const int sel = proc.selectedLane.load();
    const float density = proc.getDensity();
    const long long ab = proc.isRunning.load() ? proc.absStep.load() : -1;
    bool anySolo = false;
    for (auto& s : proc.solo) anySolo |= s.load();

    g.fillAll (navy0);
    for (int r = 0; r < numDrums; ++r)
    {
        const int y0 = rowY (r), y1 = rowY (r + 1);
        const bool silent = proc.mute[(size_t) r].load() || (anySolo && ! proc.solo[(size_t) r].load());
        if (r % 2 == 0) { g.setColour (navy1); g.fillRect (gr.getX(), y0, gr.getWidth(), y1 - y0); }
        if (r == sel) { g.setColour (col::amber.withAlpha (.08f)); g.fillRect (0, y0, getWidth(), y1 - y0); }
        // nombre de la fila
        g.setFont (fonts().t (7.f));
        g.setColour (r == sel ? col::amberHi : (silent ? lilac.withAlpha (.35f) : lilac));
        g.drawText (drumNames()[r], 6, y0, 72, y1 - y0, juce::Justification::centredLeft);
        if (r == sel) { g.setColour (col::amber); g.drawRect (2, y0 + 1, 74, y1 - y0 - 2, 1); }

        const int ll = laneLength (p, r, L);
        // pasos fuera de la longitud de la fila (polimetría): tramados
        if (ll < L)
        {
            g.setColour (juce::Colours::black.withAlpha (.35f));
            g.fillRect (colX (ll), y0, gr.getRight() - colX (ll), y1 - y0);
            g.setColour (col::amber.withAlpha (.7f));
            g.fillRect (colX (ll) - 1, y0 + 2, 2, y1 - y0 - 4);
        }
        for (int i = 0; i < L; ++i)
        {
            const int x0 = colX (i) + 1, x1 = colX (i + 1) - 2;
            const int cy0 = y0 + 3, cy1 = y1 - 3;
            const auto& st = p.s[(size_t) r][(size_t) (i % ll)];
            const bool ghostRepeat = i >= ll;   // se ve la repetición de la fila más corta, apagada
            if (! st.on())
            {
                g.setColour (navyLine);
                g.fillRect ((x0 + x1) / 2, (cy0 + cy1) / 2, 2, 2);
                continue;
            }
            const bool audible = drumAudible (st, density);
            auto c = st.vel > .95f ? col::amberHi : col::amber;
            float a = audible ? 1.f : .28f;
            if (st.prob < .99f) a *= .45f + .55f * st.prob;
            if (silent || ghostRepeat) a *= .4f;
            const float h = (cy1 - cy0) * (.45f + .55f * st.vel);
            const int top = cy1 - juce::roundToInt (h);
            g.setColour (juce::Colour (0xff7a3a10).withAlpha (a));
            g.fillRect (x0, top, x1 - x0, cy1 - top);
            g.setColour (c.withAlpha (a));
            g.fillRect (x0 + 1, top + 1, x1 - x0 - 2, cy1 - top - 2);
            g.setColour (juce::Colour (0xfffff0c8).withAlpha (a));
            g.fillRect (x0 + 1, top + 1, x1 - x0 - 2, 1);
            // redoble: rayitas; empuje: flechita; probabilidad: un punto oscuro
            if (st.ratchet > 1)
            {
                g.setColour (juce::Colour (0xff3a1408).withAlpha (a));
                for (int k = 0; k < st.ratchet; ++k) g.fillRect (x0 + 2 + k * 4, cy1 - 5, 2, 2);
            }
            if (st.nudge != 0)
            {
                g.setColour (juce::Colour (0xff3a1408).withAlpha (a));
                const int ax = st.nudge < 0 ? x0 + 2 : x1 - 4;
                g.fillRect (ax, top + 4, 2, 2);
                g.fillRect (ax + (st.nudge < 0 ? 1 : -1), top + 3, 1, 4);
            }
            if (st.prob < .99f) { g.setColour (juce::Colour (0xff3a1408)); g.fillRect (x1 - 5, top + 3, 3, 3); }
        }
        // paso que suena en esta fila (con polimetría cada fila va a su ritmo)
        if (ab >= 0)
        {
            const int idx = (int) (((ab % ll) + ll) % ll);
            g.setColour (col::amberHi.withAlpha (.75f));
            g.drawRect (colX (idx), y0 + 1, colX (idx + 1) - colX (idx), y1 - y0 - 2, 1);
        }
    }
    // líneas de compás y de tiempo
    for (int i = 0; i <= L; i += 4)
    {
        g.setColour (i % 16 == 0 ? col::amber.withAlpha (.8f) : navyLine);
        g.fillRect (colX (i), gr.getY(), 1, gr.getHeight());
    }
    g.setColour (navyLine);
    g.drawRect (gr.expanded (1), 1);

    // números de compás y dados para rehacer cada trozo
    g.setFont (fonts().t (8.f));
    const int seg = segmentLength();
    for (int i = 0; i < L; i += 4)
    {
        g.setColour (i % 16 == 0 ? col::amberHi : lilac.withAlpha (.5f));
        const auto txt = juce::String (i / 4 + 1);
        if (i % seg == 0) g.drawText (txt, diceRect (i).getRight() + 4, gr.getBottom() + 3, 30, 15, juce::Justification::centredLeft);
        else g.drawText (txt, colX (i) - 8, gr.getBottom() + 3, 24, 15, juce::Justification::centred);
    }
    for (int s0 = 0; s0 < L; s0 += seg)
    {
        const auto d = diceRect (s0);
        const bool hot = s0 == hotSegment;
        if (hot) { g.setColour (col::amber.withAlpha (.25f)); g.fillRect (d.expanded (2)); }
        pixelIcon (g, "dice", d.getX(), d.getY(), 2, hot ? col::amberHi : col::amber.withAlpha (.75f));
    }
    scanlines (g, getLocalBounds(), .12f);
}

void PianoRoll::laneMenu (int lane)
{
    juce::PopupMenu m, lens;
    const auto p = proc.getDrumPattern();
    const int cur = p.len[(size_t) lane];
    lens.addItem (100, u8 ("La del patr\xc3\xb3n"), true, cur == 0);
    for (int l : { 16, 15, 14, 12, 10, 9, 8, 7, 6, 5, 3 }) lens.addItem (100 + l, juce::String (l) + " pasos", true, cur == l);
    m.addSectionHeader (ddc::drumNames()[lane]);
    m.addSubMenu (u8 ("Longitud de la fila (polimetr\xc3\xad" "a)"), lens);
    m.addItem (1, "Rehacer esta fila");
    m.addItem (2, "Borrar esta fila");
    m.addSeparator();
    m.addItem (3, "Silenciar (M)", true, proc.mute[(size_t) lane].load());
    m.addItem (4, "Solo (S)", true, proc.solo[(size_t) lane].load());
    juce::Component::SafePointer<PianoRoll> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe, lane] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        auto& pr = safe->proc;
        if (r >= 100) pr.setLaneLength (lane, r - 100);
        else if (r == 1) pr.regenerateLane (lane);
        else if (r == 2) pr.clearLane (lane);
        else if (r == 3) pr.mute[(size_t) lane].store (! pr.mute[(size_t) lane].load());
        else if (r == 4) pr.solo[(size_t) lane].store (! pr.solo[(size_t) lane].load());
        safe->repaint();
        if (auto* parent = safe->getParentComponent()) parent->repaint();
    });
}

void PianoRoll::mouseMove (const juce::MouseEvent& e)
{
    const int h = segmentAt (e.getPosition());
    if (h != hotSegment) { hotSegment = h; setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor); repaint(); }
}

void PianoRoll::mouseExit (const juce::MouseEvent&)
{
    if (hotSegment >= 0) { hotSegment = -1; setMouseCursor (juce::MouseCursor::NormalCursor); repaint(); }
}

void PianoRoll::mouseDown (const juce::MouseEvent& e)
{
    painting = false;
    const int segHit = segmentAt (e.getPosition());
    if (segHit >= 0) { proc.regenerateSegment (segHit, segmentLength()); repaint(); if (auto* p = getParentComponent()) p->repaint(); return; }
    const int lbl = laneAtLabel (e.getPosition());
    if (lbl >= 0)
    {
        proc.selectedLane.store (lbl);
        if (onLaneSelected) onLaneSelected();
        if (e.mods.isPopupMenu()) laneMenu (lbl);
        else proc.auditionLane (lbl);
        repaint();
        return;
    }
    int lane, step;
    if (! cellAt (e.getPosition(), lane, step)) return;
    if (lane != proc.selectedLane.load()) { proc.selectedLane.store (lane); if (onLaneSelected) onLaneSelected(); }
    if (tool == 0 && ! e.mods.isPopupMenu())
    {
        // poner/quitar: arrastrando se pintan (o se borran) más pasos
        const auto p = proc.getDrumPattern();
        paintOn = ! p.s[(size_t) lane][(size_t) step].on();
        proc.beginPaint();
        proc.paintStep (lane, step, paintOn);
        painting = true; lastPaintLane = lane; lastPaintStep = step;
        if (paintOn) proc.auditionLane (lane);
    }
    else if (tool == 0) proc.editStep (lane, step, 0);
    else proc.editStep (lane, step, tool, e.mods.isPopupMenu() ? -1 : 1);
    repaint();
}

void PianoRoll::mouseDrag (const juce::MouseEvent& e)
{
    if (! painting) return;
    int lane, step;
    if (! cellAt (e.getPosition(), lane, step)) return;
    if (lane == lastPaintLane && step == lastPaintStep) return;
    lastPaintLane = lane; lastPaintStep = step;
    proc.paintStep (lane, step, paintOn);
    repaint();
}

//==============================================================================
// CARRIL DE VELOCITY / MOOD
void LaneEditor::paint (juce::Graphics& g)
{
    const int L = proc.getLength();
    const auto gr = grid();
    const float cw = gr.getWidth() / (float) L;
    auto colX = [&] (int i) { return gr.getX() + juce::roundToInt (i * cw); };
    g.fillAll (navy0);
    for (int i = 0; i <= L; i += 4)
    {
        g.setColour (i % 16 == 0 ? col::amber.withAlpha (.7f) : navyLine);
        g.fillRect (colX (i), gr.getY(), 1, gr.getHeight());
    }
    auto bar = [&] (int i, float v) { const int h = juce::jlimit (1, gr.getHeight(), juce::roundToInt (v * gr.getHeight()));
                                      return juce::Rectangle<int> (colX (i) + 2, gr.getBottom() - h, juce::jmax (1, colX (i + 1) - colX (i) - 4), h); };
    const int sel = proc.selectedLane.load();
    if (mode == 0)
    {
        const auto p = proc.getDrumPattern();
        const int ll = ddc::laneLength (p, sel, L);
        for (int i = 0; i < L; ++i)
        {
            const auto& st = p.s[(size_t) sel][(size_t) (i % ll)];
            if (! st.on()) continue;
            const auto r = bar (i, st.vel);
            const bool aud = ddc::drumAudible (st, proc.getDensity()) && i < ll;
            if (! aud) { g.setColour (col::amber.withAlpha (.35f)); g.drawRect (r, 1); continue; }
            g.setColour (st.vel > .95f ? col::amberHi : col::amber);
            g.fillRect (r);
            g.setColour (juce::Colour (0xfffff0c8));
            g.fillRect (r.getX(), r.getY(), r.getWidth(), 1);
        }
    }
    else
    {
        const auto lane = proc.getMood();
        for (int i = 0; i < L; ++i)
        {
            const float v = lane[(size_t) i];
            const auto r = bar (i, v);
            if (v * gr.getHeight() < .5f) { g.setColour (col::teal.withAlpha (.4f)); g.fillRect (r.getX(), gr.getBottom() - 1, r.getWidth(), 1); continue; }
            g.setColour (col::teal); g.fillRect (r);
            g.setColour (col::teal.brighter (.6f)); g.fillRect (r.getX(), r.getY(), r.getWidth(), 1);
        }
    }
    g.setColour (navyLine);
    g.drawRect (gr.expanded (1), 1);
    g.setFont (fonts().t (7.f));
    g.setColour (col::amberHi);
    g.drawFittedText (mode == 0 ? ddc::drumNames()[sel] : juce::String ("MOOD"), 4, gr.getY(), 72, gr.getHeight(), juce::Justification::centredLeft, 2);
    scanlines (g, getLocalBounds(), .12f);
}

void LaneEditor::apply (juce::Point<int> pos, bool erase)
{
    const int L = proc.getLength();
    const auto gr = grid();
    const int colI = juce::jlimit (0, L - 1, (int) ((pos.x - gr.getX()) * L / (float) gr.getWidth()));
    float val = juce::jlimit (0.f, 1.f, 1.f - (pos.y - gr.getY()) / (float) gr.getHeight());
    if (mode == 0)
    {
        const int sel = proc.selectedLane.load();
        const auto p = proc.getDrumPattern();
        const int ll = ddc::laneLength (p, sel, L);
        if (colI < ll && p.s[(size_t) sel][(size_t) colI].on())
        {
            proc.setStepVelocity (sel, colI, erase ? .8f : val, ! undoDone);
            undoDone = true;
        }
    }
    else
    {
        if (erase) val = 0.f;
        if (lastCol >= 0 && lastCol != colI)
        {
            const int d = colI > lastCol ? 1 : -1;
            for (int k = lastCol + d; k != colI; k += d)
                proc.setMoodValue (k, lastVal + (val - lastVal) * (float) (k - lastCol) / (float) (colI - lastCol));
        }
        proc.setMoodValue (colI, val);
        lastCol = colI; lastVal = val;
    }
    repaint();
}

void LaneEditor::mouseDown (const juce::MouseEvent& e) { lastCol = -1; undoDone = false; apply (e.getPosition(), e.mods.isPopupMenu()); }
void LaneEditor::mouseDrag (const juce::MouseEvent& e) { apply (e.getPosition(), e.mods.isPopupMenu()); }

//==============================================================================
// PADS DE BATERÍA
juce::Rectangle<int> ChordPads::padRect (int i) const
{
    const int cols = 2, rows = numPads / cols, gap = 8, m = 9;
    const int pw = (getWidth() - 2 * m - gap * (cols - 1)) / cols;
    const int ph = (getHeight() - 2 * m - 18 - gap * (rows - 1)) / rows;
    return { m + (i % cols) * (pw + gap), m + (i / cols) * (ph + gap), pw, ph };
}

void ChordPads::update()
{
    bool dirty = false;
    for (int i = 0; i < numPads; ++i)
    {
        const int h = proc.hitCount[(size_t) i].load();
        if (h != lastHits[(size_t) i]) { lastHits[(size_t) i] = h; glow[(size_t) i] = 6; dirty = true; }
        else if (glow[(size_t) i] > 0) { --glow[(size_t) i]; dirty = true; }
    }
    int ms = 0;
    for (int i = 0; i < numPads; ++i) ms |= (proc.mute[(size_t) i].load() ? 1 : 0) << (i * 2) | (proc.solo[(size_t) i].load() ? 1 : 0) << (i * 2 + 1);
    if (ms != lastMs || proc.selectedLane.load() != lastSel) { lastMs = ms; lastSel = proc.selectedLane.load(); dirty = true; }
    if (dirty) repaint();
}

void ChordPads::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < numPads; ++i)
    {
        if (muteRect (i).expanded (2).contains (e.getPosition())) { proc.mute[(size_t) i].store (! proc.mute[(size_t) i].load()); update(); if (auto* p = getParentComponent()) p->repaint(); return; }
        if (soloRect (i).expanded (2).contains (e.getPosition())) { proc.solo[(size_t) i].store (! proc.solo[(size_t) i].load()); update(); if (auto* p = getParentComponent()) p->repaint(); return; }
        if (padRect (i).contains (e.getPosition()))
        {
            proc.auditionLane (i);
            proc.selectedLane.store (i);
            if (onLaneSelected) onLaneSelected();
            repaint();
            return;
        }
    }
}

void ChordPads::paint (juce::Graphics& g)
{
    g.fillAll (col::khakiDark);
    auto plate = getLocalBounds().reduced (2);
    g.setColour (col::charcoal);
    g.fillRect (plate);
    g.setColour (col::charcoalLo);
    g.fillRect (plate.getX(), plate.getY(), plate.getWidth(), 3);
    bool anySolo = false;
    for (auto& s : proc.solo) anySolo |= s.load();
    const int sel = proc.selectedLane.load();
    for (int i = 0; i < numPads; ++i)
    {
        const auto r = padRect (i);
        const float gl = glow[(size_t) i] / 6.f;
        const bool silent = proc.mute[(size_t) i].load() || (anySolo && ! proc.solo[(size_t) i].load());
        const auto off = juce::Colour (silent ? 0xff3e3a42 : 0xff56505a), on = juce::Colour (0xffffaa46);
        const auto base = off.interpolatedWith (on, gl * (silent ? .3f : 1.f));
        if (gl > 0.f && ! silent) { g.setColour (on.withAlpha (.2f * gl)); g.fillRect (r.expanded (4)); }
        g.setColour (juce::Colour (0xff141016)); g.fillRect (r.translated (0, 3));
        g.setColour (base.withMultipliedBrightness (.62f)); g.fillRect (r);
        auto face = r.reduced (3).withTrimmedBottom (3);
        g.setColour (base); g.fillRect (face);
        g.setColour (base.brighter (.25f));
        g.fillRect (face.getX(), face.getY(), face.getWidth(), 3);
        g.fillRect (face.getX(), face.getY(), 3, face.getHeight());
        if (i == sel) { g.setColour (col::amber); g.drawRect (r.expanded (2), 2); }
        const auto ink = gl > .5f && ! silent ? juce::Colour (0xff2a1408) : juce::Colour (0xffe6dccb).withAlpha (silent ? .5f : 1.f);
        g.setColour (ink);
        g.setFont (fonts().t (7.f));
        g.drawText (juce::String (i + 1), face.getX() + 4, face.getY() + 4, 20, 10, juce::Justification::topLeft);
        g.drawFittedText (ddc::drumNames()[i], face.getX() + 2, face.getBottom() - 18, face.getWidth() - 4, 14, juce::Justification::centred, 1, .7f);
        // M y S
        for (int k = 0; k < 2; ++k)
        {
            const auto b = k == 0 ? muteRect (i) : soloRect (i);
            const bool st = k == 0 ? proc.mute[(size_t) i].load() : proc.solo[(size_t) i].load();
            g.setColour (st ? (k == 0 ? col::red : juce::Colour (0xfff2d16b)) : juce::Colour (0xff2c2830));
            g.fillRect (b);
            g.setColour (ink);
            g.drawRect (b, 1);
            g.setFont (fonts().t (6.f));
            g.setColour (st ? juce::Colour (0xff1c070c) : ink);
            g.drawText (k == 0 ? "M" : "S", b.translated (1, 1), juce::Justification::centred);
        }
    }
    g.setFont (fonts().t (7.f));
    g.setColour (juce::Colour (0xffc8a078));
    g.drawText ("PADS BATERIA", 0, getHeight() - 18, getWidth(), 12, juce::Justification::centred);
}

void TempoLed::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.f);
    const bool on = isOn();
    if (on)
    {
        g.setColour (juce::Colour (0xffff5a1f).withAlpha (.35f));
        g.fillEllipse (r.expanded (1.5f));
    }
    g.setColour (col::outline);
    g.fillEllipse (r);
    g.setColour (on ? juce::Colour (0xffff7a3a) : juce::Colour (0xff5a2418));
    g.fillEllipse (r.reduced (3.f));
    g.setColour (juce::Colours::white.withAlpha (on ? .6f : .2f));
    g.fillRect (juce::roundToInt (r.getX() + r.getWidth() * .32f), juce::roundToInt (r.getY() + r.getHeight() * .28f), 3, 3);
}

//==============================================================================
// EXPORTAR MIDI / WAV
void ExportDragArea::paint (juce::Graphics& g)
{
    raisedButton (g, getLocalBounds(), col::teal, down);
    auto r = down ? getLocalBounds().withTrimmedTop (3) : getLocalBounds().withTrimmedBottom (4);
    const auto font = fonts().b (16.f);
    const juce::String text = wav ? "WAV" : "MIDI";
    const int tw = (int) juce::GlyphArrangement::getStringWidth (font, text);
    const int x0 = r.getCentreX() - (14 + 6 + tw) / 2;
    pixelIcon (g, "down", x0, r.getCentreY() - 7, 2, col::display);
    g.setFont (font);
    g.setColour (col::display);
    g.drawText (text, x0 + 20, r.getY(), tw + 4, r.getHeight(), juce::Justification::centredLeft);
}

void ExportDragArea::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6) return;
    dragging = didDrag = true;
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (fileName());
    if (writeTo (f))
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this,
                                                                   [this] { dragging = false; });
}

void ExportDragArea::mouseUp (const juce::MouseEvent&)
{
    down = false;
    repaint();
    if (! didDrag)
    {
        // un clic sin arrastrar abre el diálogo para guardar el archivo
        chooser = std::make_unique<juce::FileChooser> (wav ? "Guardar WAV" : "Guardar MIDI",
            juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile (fileName()), wav ? "*.wav" : "*.mid");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (f != juce::File()) writeTo (f.withFileExtension (wav ? "wav" : "mid"));
                              });
    }
    didDrag = false;
    dragging = false;
}

//==============================================================================
// PANEL DE LA RADIO
//==============================================================================
// VENTANITA PARA GUARDAR PRESETS
SaveDialog::SaveDialog()
{
    setInterceptsMouseClicks (true, true);
    name.setFont (fonts().t (12.f));
    name.setJustification (juce::Justification::centredLeft);
    name.setColour (juce::TextEditor::backgroundColourId, col::display);
    name.setColour (juce::TextEditor::textColourId, col::amberHi);
    name.setColour (juce::TextEditor::highlightColourId, col::amber.withAlpha (.4f));
    name.setColour (juce::TextEditor::outlineColourId, col::outline);
    name.setColour (juce::TextEditor::focusedOutlineColourId, col::amber);
    name.setColour (juce::CaretComponent::caretColourId, col::amberHi);
    name.setInputRestrictions (32);
    name.setIndents (10, 8);
    name.onReturnKey = [this] { if (onSave) onSave (name.getText()); };
    name.onEscapeKey = [this] { close(); };
    ok.onClick = [this] { if (onSave) onSave (name.getText()); };
    cancel.onClick = [this] { close(); };
    for (auto* c : std::initializer_list<juce::Component*> { &name, &ok, &cancel }) addAndMakeVisible (c);
}

void SaveDialog::open (const juce::String& suggestion)
{
    name.setText (suggestion, false);
    name.selectAll();
    setVisible (true);
    toFront (true);
    name.grabKeyboardFocus();
}

void SaveDialog::resized()
{
    auto b = box();
    name.setBounds (b.getX() + 24, b.getY() + 62, b.getWidth() - 48, 38);
    cancel.setBounds (b.getX() + 24, b.getBottom() - 60, 150, 40);
    ok.setBounds (b.getRight() - 24 - 150, b.getBottom() - 60, 150, 40);
}

void SaveDialog::paint (juce::Graphics& g)
{
    g.fillAll (col::outline.withAlpha (.55f));
    auto b = box();
    radioBox (g, b.translated (0, 6), col::outline, col::outline, 4, 8);
    radioBox (g, b, col::khaki, col::outline, 4, 8);
    g.setColour (col::khakiLight);
    g.fillRect (b.getX() + 8, b.getY() + 6, b.getWidth() - 16, 3);
    g.setFont (fonts().t (12.f));
    g.setColour (col::labelInk);
    g.drawText ("GUARDAR PRESET", b.getX(), b.getY() + 18, b.getWidth(), 20, juce::Justification::centred);
    g.setFont (fonts().b (15.f));
    g.setColour (col::khakiDark.darker (.4f));
    g.drawText (u8 ("Sonido, patrones A-H y Mood. Escribe un nombre:"), b.getX(), b.getY() + 38, b.getWidth(), 20, juce::Justification::centred);
}

void RadioPanel::refreshUserPresets()
{
    userFiles = proc.userPresets();
    userBox.clear (juce::dontSendNotification);
    for (int i = 0; i < userFiles.size(); ++i)
        userBox.addItem (lcdText (userFiles[i].getFileNameWithoutExtension()), i + 1);
    if (userFiles.isEmpty()) userBox.addItem ("(TODAVIA NO HAY)", 999), userBox.setItemEnabled (999, false);
    userBox.addSeparator();
    userBox.addItem ("ABRIR CARPETA", 1000);
    // se muestra el nombre del último preset sin dejarlo "elegido", así volver a elegirlo lo recarga
    if (proc.currentUserPreset.isNotEmpty()) userBox.setText (lcdText (proc.currentUserPreset), juce::dontSendNotification);
}

RadioPanel::RadioPanel (DrumMakerProcessor& p)
    : proc (p), roll (p, tool), lane (p), pads (p), led (p), dragMidi (p, false), dragWav (p, true)
{
    setLookAndFeel (&look);

    for (auto* b : { &previewBtn, &genBtn, &varyBtn, &undoBtn, &redoBtn, &clearBtn })
        addAndMakeVisible (b);
    genBtn.getProperties().set ("primary", true);
    genBtn.getProperties().set ("ic", "dice");
    varyBtn.getProperties().set ("ic", "wave");
    clearBtn.getProperties().set ("ic", "trash");
    previewBtn.getProperties().set ("round", true);
    previewBtn.setClickingTogglesState (true);
    previewBtn.setTooltip (u8 ("Escucha el patr\xc3\xb3n aunque Ableton est\xc3\xa9 parado"));
    previewBtn.onClick = [this] { proc.previewOn.store (previewBtn.getToggleState()); };
    logoImg = juce::ImageFileFormat::loadFrom (ddc::logo::png, (size_t) ddc::logo::pngSize);

    // presets del usuario
    userBox.setTextWhenNothingSelected ("MIS PRESETS");
    userBox.setTooltip (u8 ("Tus presets guardados (sonido, patrones A-H y Mood)"));
    userBox.onBeforePopup = [this] { refreshUserPresets(); };
    userBox.onChange = [this]
    {
        const int id = userBox.getSelectedId();
        if (id == 1000)
        {
            auto folder = DrumMakerProcessor::userPresetFolder();
            folder.createDirectory();
            folder.revealToUser();
        }
        else if (id >= 1 && id <= userFiles.size() && proc.loadUserPreset (userFiles[id - 1]))
        {
            lastPresetSeen = -1;
            afterPatternChange();
            repaint();
        }
        juce::Component::SafePointer<RadioPanel> safe (this);
        juce::MessageManager::callAsync ([safe] { if (safe != nullptr) safe->refreshUserPresets(); });
    };
    addAndMakeVisible (userBox);
    refreshUserPresets();
    saveBtn.setTooltip (u8 ("Guarda el sonido, los patrones A-H y el Mood como preset"));
    saveBtn.onClick = [this]
    {
        auto suggestion = proc.currentUserPreset;
        if (suggestion.isEmpty()) suggestion = juce::String (proc.lastStyleUsed).isNotEmpty() ? juce::String (proc.lastStyleUsed) + " 1" : "Mi groove 1";
        saveDlg.open (suggestion);
    };
    addAndMakeVisible (saveBtn);
    saveDlg.onSave = [this] (const juce::String& n)
    {
        if (n.trim().isEmpty()) return;
        if (proc.saveUserPreset (n)) { saveDlg.close(); refreshUserPresets(); }
    };
    addChildComponent (saveDlg);
    genBtn.onClick = [this] { proc.generate(); advanceCamp(); redrawBadge(); afterPatternChange(); repaint(); };
    varyBtn.onClick = [this] { proc.vary(); roll.repaint(); lane.repaint(); };
    undoBtn.onClick = [this] { proc.undo(); afterPatternChange(); };
    redoBtn.onClick = [this] { proc.redo(); afterPatternChange(); };
    clearBtn.onClick = [this] { proc.clearPattern(); roll.repaint(); lane.repaint(); };
    undoBtn.setTooltip ("Deshacer");
    redoBtn.setTooltip ("Rehacer");
    undoBtn.getProperties().set ("icon", "undo");
    redoBtn.getProperties().set ("icon", "redo");

    // banco de patrones A-H (mayúsculas + clic copia el patrón actual en esa memoria)
    for (int i = 0; i < 8; ++i)
    {
        auto& b = slotBtns[i];
        b.setButtonText (juce::String::charToString ((juce::juce_wchar) ('A' + i)));
        b.getProperties().set ("slot", true);
        b.setTooltip (u8 ("Patr\xc3\xb3n ") + b.getButtonText() + u8 (". May\xc3\xbas + clic: copia aqu\xc3\xad el patr\xc3\xb3n actual"));
        b.onClick = [this, i]
        {
            if (juce::ModifierKeys::currentModifiers.isShiftDown()) proc.copyCurrentToSlot (i);
            else proc.selectSlot (i);
            afterPatternChange();
        };
        addAndMakeVisible (b);
    }
    dragMidi.setTooltip (u8 ("Arrastra el patr\xc3\xb3n en MIDI a una pista de Ableton, o haz clic para guardar el archivo"));
    dragWav.setTooltip (u8 ("Arrastra 4 compases en WAV a una pista de Ableton, o haz clic para guardar el archivo"));

    const char* toolNames[5] = { "Poner/\nquitar", "Acento", "Redoble", "Probab.", "Empuje" };
    for (int i = 0; i < 5; ++i)
    {
        auto& b = toolBtns[i];
        b.setButtonText (u8 (toolNames[i]));
        b.setClickingTogglesState (true);
        b.setRadioGroupId (77);
        b.onClick = [this, i] { if (toolBtns[i].getToggleState()) tool = i; };
        addAndMakeVisible (b);
    }
    toolBtns[0].setToggleState (true, juce::dontSendNotification);
    toolBtns[0].setTooltip (u8 ("Clic: pone o quita un golpe. Arrastra para pintar o borrar varios."));
    toolBtns[1].setTooltip (u8 ("Clic en un golpe: lo acent\xc3\xba" "a (m\xc3\xa1s fuerte). Clic derecho: golpe fantasma (muy flojo)."));
    toolBtns[2].setTooltip (u8 ("Clic en un golpe: redoble (2, 3 o 4 golpes dentro del mismo paso). Clic derecho: uno menos."));
    toolBtns[3].setTooltip (u8 ("Clic en un golpe: baja su probabilidad (100, 75, 50, 25 %). As\xc3\xad el patr\xc3\xb3n cambia solo cada vuelta."));
    toolBtns[4].setTooltip (u8 ("Clic en un golpe: lo retrasa un poco (empuje). Clic derecho: lo adelanta. Da el swing de cada golpe por separado."));

    const char* lockNames[3] = { "Sonido", "Patr\xc3\xb3n", "FX" };
    for (int i = 0; i < 3; ++i)
    {
        lockBtns[i].setButtonText (u8 (lockNames[i]));
        lockBtns[i].setClickingTogglesState (true);
        lockBtns[i].getProperties().set ("switch", true);
        lockBtns[i].getProperties().set ("light", true);
        addAndMakeVisible (lockBtns[i]);
        buttonAtt.push_back (std::make_unique<BA> (proc.apvts, lockIds[i], lockBtns[i]));
    }

    addCombo (styleBox, "style", "Estilo");
    addCombo (keyBox, "key", "Tono");
    addCombo (fillsBox, "fills", "Fills");
    fillsBox.setTooltip (u8 ("Cada cu\xc3\xa1ntos compases entra un redoble al final (la cantidad va con la rueda Fills)"));
    addCombo (modeBox, "groove", "Groove");
    modeBox.setTooltip (u8 ("Recto: a tiempo. Shuffle: semicorcheas en ternario. Suelto: algunos golpes caen un poco tarde, como tocado a mano."));
    keyBox.setTooltip (u8 ("Afina el bombo y el tom a esta nota, para que peguen con el bajo"));
    addCombo (lenBox, "length", "Longitud");
    addCombo (presetBox, "kit", "Kit");
    presetBox.setTooltip (u8 ("El kit de sonidos. GENERAR elige uno que va con el estilo (salvo si Sonido est\xc3\xa1 bloqueado)."));

    // carril: pestañas VELOCITY / MOOD y controles del Mood
    const char* tabNames[2] = { "VELOCITY", "MOOD" };
    for (int i = 0; i < 2; ++i)
    {
        laneTabs[i].setButtonText (tabNames[i]);
        laneTabs[i].getProperties().set ("slot", true);
        laneTabs[i].setClickingTogglesState (true);
        laneTabs[i].setRadioGroupId (88);
        laneTabs[i].onClick = [this, i] { if (laneTabs[i].getToggleState()) setLaneMode (i); };
        addAndMakeVisible (laneTabs[i]);
    }
    laneTabs[0].setTooltip (u8 ("Arrastra las barras para cambiar la fuerza de cada nota. Clic derecho: valor normal."));
    laneTabs[1].setTooltip (u8 ("Dibuja un valor por paso que modula el par\xc3\xa1metro elegido en Destino, junto con el LFO. Clic derecho: borra."));
    addCombo (moodDestBox, "moodDest", "", true);
    addCombo (lfoShapeBox, "lfoShape", "", true);
    addCombo (lfoRateBox, "lfoRate", "", true);
    for (auto* k : { &moodKnob, &lfoKnob })
    {
        k->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        k->setMouseDragSensitivity (160);
        k->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k->onValueChange = [this] { repaint (modR); };
        addAndMakeVisible (k);
    }
    moodKnob.setTooltip (u8 ("Cu\xc3\xa1nto afecta el Mood al destino. A la izquierda del centro, en sentido contrario."));
    lfoKnob.setTooltip ("Cantidad del LFO que se suma al carril Mood");
    sliderAtt.push_back (std::make_unique<SA> (proc.apvts, "moodAmt", moodKnob));
    sliderAtt.push_back (std::make_unique<SA> (proc.apvts, "lfoAmt", lfoKnob));
    moodKnob.setDoubleClickReturnValue (true, .5);
    lfoKnob.setDoubleClickReturnValue (true, 0.);

    for (int i = 0; i < NK; ++i)
    {
        auto& k = knobs[i];
        k.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        k.setMouseDragSensitivity (180);
        k.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k.onValueChange = [this] { repaint (knobR); };
        addAndMakeVisible (k);
        if (isLaneKnob (i)) continue;   // estas cambian de instrumento con la fila elegida
        if (auto* prm = proc.apvts.getParameter (knobIds[i])) k.setDoubleClickReturnValue (true, prm->getDefaultValue());
        sliderAtt.push_back (std::make_unique<SA> (proc.apvts, knobIds[i], k));
    }
    attachLaneKnobs();
    roll.onLaneSelected = [this] { attachLaneKnobs(); lane.repaint(); pads.repaint(); };
    pads.onLaneSelected = [this] { attachLaneKnobs(); lane.repaint(); roll.repaint(); };
    knobs[2].setTooltip (u8 ("Nivel del instrumento elegido (la fila marcada en la rejilla)"));
    knobs[3].setTooltip (u8 ("Paneo del instrumento elegido"));
    knobs[4].setTooltip (u8 ("Color del instrumento elegido: a la izquierda m\xc3\xa1s oscuro, a la derecha m\xc3\xa1s brillante y sucio"));
    knobs[5].setTooltip (u8 ("Compresi\xc3\xb3n en paralelo de toda la bater\xc3\xad" "a: m\xc3\xa1s pegada sin perder golpe"));
    knobs[6].setTooltip (u8 ("Tono del instrumento elegido (\xc2\xb1 1 octava). El bombo y el tom ya siguen el Tono de arriba."));
    knobs[7].setTooltip (u8 ("Decay: lo que dura el instrumento elegido"));
    knobs[8].setTooltip (u8 ("Fills: cu\xc3\xa1nto se llenan los redobles de final de frase"));
    knobs[9].setTooltip (u8 ("Reverb: espacio (el bombo queda seco)"));
    knobs[12].setTooltip (u8 ("Humano: cada golpe sale unos milisegundos tarde al azar y con la fuerza algo distinta. A cero, exacto."));

    // cadena de patrones y sub en mono
    addCombo (chainBox, "chain", "", true);
    chainBox.setTooltip (u8 ("Cadena: con Ableton en marcha, los patrones del banco suenan uno detr\xc3\xa1s de otro (A-B, A-D o A-H)"));
    subMonoBtn.setButtonText ("Sub mono");
    subMonoBtn.setClickingTogglesState (true);
    subMonoBtn.getProperties().set ("switch", true);
    subMonoBtn.getProperties().set ("light", true);
    subMonoBtn.setTooltip (u8 ("Deja en mono todo lo que est\xc3\xa1 por debajo de 120 Hz: los graves quedan centrados y el Ancho y el chorus siguen abriendo arriba"));
    addAndMakeVisible (subMonoBtn);
    buttonAtt.push_back (std::make_unique<BA> (proc.apvts, "subMono", subMonoBtn));

    knobs[0].setTooltip (u8 ("Densidad en directo: quita y pone golpes mientras suena (los importantes se quedan)"));

    addAndMakeVisible (roll);
    addAndMakeVisible (lane);
    addAndMakeVisible (dragMidi);
    addAndMakeVisible (dragWav);
    addAndMakeVisible (pads);
    pads.setTooltip (u8 ("Pads: se iluminan con cada golpe. Clic: escuchar y elegir el instrumento. M: silenciar. S: solo."));
    addAndMakeVisible (led);

    laneTabs[0].setToggleState (true, juce::dontSendNotification);
    setLaneMode (0);
    setSize (baseW, baseH);
    redrawBadge();
    rebuildCrowd (-100000);
    todShown = (float) campStepShown / 11.f;
    startTimerHz (30);
}

void RadioPanel::attachLaneKnobs()
{
    const int ln = juce::jlimit (0, ddc::numDrums - 1, proc.selectedLane.load());
    laneAtt.clear();
    for (int i = 0; i < NK; ++i)
    {
        if (! isLaneKnob (i)) continue;
        const auto id = proc.laneParamId (knobIds[i], ln);
        if (auto* prm = proc.apvts.getParameter (id)) knobs[i].setDoubleClickReturnValue (true, prm->getDefaultValue());
        laneAtt.push_back (std::make_unique<SA> (proc.apvts, id, knobs[i]));
    }
    attachedLane = ln;
    repaint (knobR);
}

RadioPanel::~RadioPanel()
{
    stopTimer();
    laneAtt.clear();   // antes que los diales
    setLookAndFeel (nullptr);
}

void RadioPanel::addCombo (juce::ComboBox& box, const char* paramId, const juce::String& label, bool laneCombo)
{
    if (paramId != nullptr)
    {
        if (auto* pc = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (paramId)))
        {
            juce::StringArray up; for (auto& c : pc->choices) up.add (lcdText (c));
            box.addItemList (up, 1);
        }
        comboAtt.push_back (std::make_unique<CA> (proc.apvts, paramId, box));
    }
    if (laneCombo) { addAndMakeVisible (box); return; }
    combos.push_back (&box);
    comboLabels.add (label);
    addAndMakeVisible (box);
}

// El campamento: cada GENERAR sale alguien que se queda, y la noche avanza hasta la mañana.
// Al pasar de la mañana empieza una noche nueva (todos a dormir y vuelve a salir el primero).
void RadioPanel::rebuildCrowd (int newestBorn)
{
    const juce::ScopedLock sl (proc.campLock);
    std::vector<Camper> next;
    bool free0 = true, free1 = true;
    for (int i = 0; i < proc.campSeeds.size(); ++i)
    {
        auto c = Camper::fromSeed (proc.campSeeds[i], i, free0, free1);
        if (c.action == Camper::tentLove) (c.tent == 0 ? free0 : free1) = false;
        c.born = i < (int) crowd.size() ? crowd[(size_t) i].born : -100000;
        if (i == proc.campSeeds.size() - 1 && newestBorn > -100000) c.born = newestBorn;
        next.push_back (c);
    }
    crowd = std::move (next);
    campStepShown = proc.campStep;
}

void RadioPanel::advanceCamp()
{
    {
        const juce::ScopedLock sl (proc.campLock);
        if (++proc.campStep > 11) { proc.campStep = 0; proc.campSeeds.clear(); crowd.clear(); }
        if (proc.campSeeds.size() < Scene::numSlots) proc.campSeeds.add (rng.nextInt (1000000));
    }
    rebuildCrowd (sceneT);
}

void RadioPanel::setLaneMode (int m)
{
    if (lane.mode != m) repaint();
    lane.mode = m;
    lane.repaint();
    repaint (laneHeadR);
}

void RadioPanel::redrawBadge()
{
    // ventana panorámica: escena completa (192 x 56 píxeles) mostrada al doble
    // pantalla panorámica: escena de baja resolución mostrada al triple
    const int iw = (badgeR.getWidth() - 12) / 3, ih = (badgeR.getHeight() - 12) / 3;   // píxeles al triple
    if (! badge.isValid() || badge.getWidth() != iw || badge.getHeight() != ih) badge = juce::Image (juce::Image::RGB, iw, ih, true, juce::SoftwareImageType());
    SceneInfo info;
    info.t = sceneT;
    info.level = sceneLevel;
    info.beat = proc.beatPos.load();
    info.tod = todShown;
    info.crowd = &crowd;
    Scene::draw (badge, 108 - ih, info);
}

void RadioPanel::timerCallback()
{
    {
        // el piano roll y el carril solo se redibujan cuando cambia el paso que suena (o de vez en cuando)
        const int ps = proc.isRunning.load() ? proc.playStep.load() : -1;
        if (ps != lastPlayStep || ticks % 6 == 0) { lastPlayStep = ps; roll.repaint(); lane.repaint(); }
    }
    pads.update(); led.update();
    undoBtn.setEnabled (proc.canUndo());
    redoBtn.setEnabled (proc.canRedo());
    {
        const int cs = proc.curSlot();
        for (int i = 0; i < 8; ++i)
        {
            slotBtns[i].setToggleState (i == cs, juce::dontSendNotification);
            slotBtns[i].getProperties().set ("filled", proc.slotHasNotes (i));
        }
        if (cs != lastSlot) { lastSlot = cs; roll.followNotes(); }   // cambio de patrón (también automatizado)
        const int ps = proc.playSlot.load();
        if (ps != lastPlaySlot) { lastPlaySlot = ps; repaint (slotR.withHeight (slotR.getHeight() + 5)); }
        static int lastT = 99;
        if (proc.transpose.load() != lastT) { lastT = proc.transpose.load(); repaint (statusR); }
    }
    if (proc.selectedLane.load() != attachedLane) { attachLaneKnobs(); lane.repaint(); }
    {
        // la hora avanza suave hacia la que toca
        const float target = (float) campStepShown / 11.f, d = target - todShown;
        if (std::abs (d) > .0005f) todShown += d > 0 ? juce::jmin (d, juce::jmax (.004f, d * .06f)) : juce::jmax (d, juce::jmin (-.004f, d * .06f));
    }
    {
        // la gente salta con cada bombo
        const int kh = proc.hitCount[0].load();
        if (kh != lastKickHits) { lastKickHits = kh; sceneLevel = 1.f; }
        else sceneLevel *= .82f;
    }
    if (++ticks % 2 == 0)
    {
        ++sceneT;
        redrawBadge();
        repaint (badgeR);
    }
    if (ticks % 8 == 0) { ++frame; repaint (statusR); }
}

//==============================================================================
void RadioPanel::resized()
{
    const int W = getWidth();
    bodyR     = getLocalBounds();
    badgeR    = { 40, 22, 588, 250 };                  // pantalla grande: marco + escena de 192 x 79 píxeles al triple
    lockR     = { W - 216, 98, 168, 118 };
    selR      = { badgeR.getRight() + 24, 92, lockR.getX() - 24 - badgeR.getRight() - 24, 126 };
    statusR   = { selR.getX(), 232, 150, 34 };                // pantallita con el tempo y la transposición
    logoR     = { W - 66 - 260, 27, 260, 33 };
    toolR     = { 186, 296, 0, 50 };   // ancho = el del piano roll (se fija abajo)
    modR      = { W - 226, 368, 204, 320 };            // panel de modulación (Mood + LFO), donde estaba el altavoz derecho
    rollR     = { 186, 358, modR.getX() - 16 - 186, 214 };
    toolR.setWidth (rollR.getWidth());
    laneHeadR = { rollR.getX() + 6, 580, rollR.getWidth() - 12, 30 };
    laneR     = { rollR.getX(), 614, rollR.getWidth(), 74 };
    speakL    = { 22, 296, 150, 392 };                 // pads de acordes: desde la fila de botones hasta abajo
    plaqueR   = { 22, 584, 150, 104 };                 // placa de los autores, bajo el altavoz
    knobR     = { 56, 694, W - 112, 104 };

    pads.setBounds (speakL);
    slotR = { selR.getX() + 66, 26, 8 * 34, 34 };
    for (int i = 0; i < 8; ++i) slotBtns[i].setBounds (slotR.getX() + i * 34, slotR.getY(), 32, 34);
    chainBox.setBounds (slotR.getX(), slotR.getBottom() + 6, 118, 24);
    subMonoBtn.setBounds (chainBox.getRight() + 12, slotR.getBottom() + 4, 142, 28);
    saveBtn.setBounds (selR.getRight() - 96, statusR.getY(), 96, 34);
    userBox.setBounds (statusR.getRight() + 12, statusR.getY(), saveBtn.getX() - 10 - statusR.getRight() - 12, 34);
    saveDlg.setBounds (getLocalBounds());
    previewBtn.setBounds (lockR.getX(), statusR.getY(), 34, 34);
    led.setBounds (lockR.getX() + 112, statusR.getY() + 6, 22, 22);

    const int cw = (selR.getWidth() - 2 * 14) / 3;
    for (size_t i = 0; i < combos.size(); ++i)
    {
        const int cx = selR.getX() + (int) (i % 3) * (cw + 14);
        const int cy = selR.getY() + 26 + (int) (i / 3) * 64;
        combos[i]->setBounds (cx, cy, cw, 36);
    }

    for (int i = 0; i < 3; ++i) lockBtns[i].setBounds (lockR.getX() + 12, lockR.getY() + 24 + i * 30, lockR.getWidth() - 20, 30);

    // los botones se reparten el ancho exacto del piano roll
    {
        juce::Component* row[] = { &genBtn, &varyBtn, &undoBtn, &redoBtn, &clearBtn, &toolBtns[0], &toolBtns[1], &toolBtns[2], &toolBtns[3], &toolBtns[4] };
        const int base[] = { 124, 88, 46, 46, 96, 78, 72, 62, 72, 72 };
        const int gap = 8, n = 10;
        int sum = 0; for (int w : base) sum += w;
        const float k = (float) (toolR.getWidth() - gap * (n - 1)) / (float) sum;
        float x = (float) toolR.getX();
        for (int i = 0; i < n; ++i)
        {
            const int x0 = juce::roundToInt (x), x1 = i == n - 1 ? toolR.getRight() : juce::roundToInt (x + base[i] * k);
            row[i]->setBounds (x0, toolR.getY(), x1 - x0, toolR.getHeight());
            x += base[i] * k + gap;
        }
    }
    // MIDI y WAV encima del panel de modulación, con su mismo ancho
    const int ew = (modR.getWidth() - 8) / 2;
    dragMidi.setBounds (modR.getX(), toolR.getY(), ew, toolR.getHeight());
    dragWav.setBounds (modR.getRight() - ew, toolR.getY(), ew, toolR.getHeight());

    roll.setBounds (rollR.reduced (12));
    lane.setBounds (laneR.reduced (12, 7));

    // cabecera del carril
    auto h = laneHeadR;
    laneTabs[0].setBounds (h.removeFromLeft (104)); h.removeFromLeft (6);
    laneTabs[1].setBounds (h.removeFromLeft (76)); h.removeFromLeft (22);

    // panel de modulación
    const int mx = modR.getX() + 14, mw = modR.getWidth() - 28;
    moodDestBox.setBounds (mx, modR.getY() + 48, mw, 34);
    lfoShapeBox.setBounds (mx, modR.getY() + 118, mw, 34);
    lfoRateBox.setBounds  (mx, modR.getY() + 160, mw, 34);
    moodKnob.setBounds (mx + mw / 4 - 29, modR.getY() + 230, 58, 58);
    lfoKnob.setBounds  (mx + 3 * mw / 4 - 29, modR.getY() + 230, 58, 58);

    const int kw = knobR.getWidth() / NK;
    for (int i = 0; i < NK; ++i) knobs[i].setBounds (knobR.getX() + i * kw + (kw - 58) / 2, knobR.getY() + 27, 58, 58);
}

void RadioPanel::paintStatic (juce::Graphics& g)
{
    // carcasa: ocupa toda la ventana
    g.fillAll (col::outline);
    auto body = bodyR.reduced (4);
    g.setColour (col::khaki);
    g.fillRect (body);
    // tramado muy suave para dar textura al plástico
    g.setColour (col::khakiDark.withAlpha (.18f));
    for (int y = body.getY(); y < body.getBottom(); y += 4)
        for (int x = body.getX() + ((y / 4) % 2) * 2; x < body.getRight(); x += 4)
            g.fillRect (x, y, 1, 1);
    // bisel
    g.setColour (col::khakiLight);
    g.fillRect (body.getX(), body.getY(), body.getWidth(), 3);
    g.fillRect (body.getX(), body.getY(), 3, body.getHeight());
    g.setColour (col::khakiDark);
    g.fillRect (body.getX(), body.getBottom() - 4, body.getWidth(), 4);
    g.fillRect (body.getRight() - 4, body.getY(), 4, body.getHeight());
    // tapa de arriba oscura, como en los samplers (con su textura y bisel)
    {
        const juce::Rectangle<int> lid (body.getX(), body.getY(), body.getWidth(), 282 - body.getY());
        g.setColour (col::charcoal);
        g.fillRect (lid);
        g.setColour (col::charcoalHi.withAlpha (.35f));
        for (int y = lid.getY(); y < lid.getBottom(); y += 4)
            for (int x = lid.getX() + ((y / 4) % 2) * 2; x < lid.getRight(); x += 4)
                g.fillRect (x, y, 1, 1);
        g.setColour (col::charcoalHi);
        g.fillRect (lid.getX(), lid.getY(), lid.getWidth(), 3);
        g.fillRect (lid.getX(), lid.getY(), 3, lid.getHeight());
        g.setColour (col::charcoalLo);
        g.fillRect (lid.getRight() - 4, lid.getY(), 4, lid.getHeight());
    }
    // separador entre la parte de arriba y la de abajo
    g.setColour (col::khakiDark);
    g.fillRect (body.getX() + 8, 282, body.getWidth() - 16, 3);
    g.setColour (col::khakiLight);
    g.fillRect (body.getX() + 8, 285, body.getWidth() - 16, 2);

    for (auto c : { juce::Point<int> (24, 24), juce::Point<int> (getWidth() - 24, 24),
                    juce::Point<int> (24, getHeight() - 24), juce::Point<int> (getWidth() - 24, getHeight() - 24) })
        screw (g, c);

    // rótulos de PREVIEW y del piloto
    g.setFont (fonts().b (16.f));
    g.setColour (col::panelInk);
    g.drawText ("PREVIEW", previewBtn.getRight() + 6, previewBtn.getY(), 60, previewBtn.getHeight(), juce::Justification::centredLeft);
    g.drawText ("TEMPO", led.getRight() + 6, previewBtn.getY(), 50, previewBtn.getHeight(), juce::Justification::centredLeft);

    // logo
    if (logoImg.isValid())
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.setColour (col::khakiDark);
        g.drawImage (logoImg, logoR.translated (2, 2).toFloat(), juce::RectanglePlacement::centred, true);
        g.setColour (col::panelInk);
        g.drawImage (logoImg, logoR.toFloat(), juce::RectanglePlacement::centred, true);
    }
    g.setFont (fonts().t (10.f));
    g.setColour (col::khakiLight);
    g.drawText ("DRUM MAKER", logoR.getX(), logoR.getBottom() + 8, logoR.getWidth(), 14, juce::Justification::centredRight);

    // pantallita de estado: tempo y transposición
    {
        ledScreen (g, statusR);
    }
    g.setFont (fonts().b (16.f));
    g.setColour (col::panelInk);
    g.drawText (u8 ("PATR\xc3\x93N"), slotR.getX() - 80, slotR.getY(), 74, slotR.getHeight(), juce::Justification::centredRight);
    g.drawText ("CADENA", chainBox.getX() - 80, chainBox.getY(), 74, chainBox.getHeight(), juce::Justification::centredRight);

    // ventana del campamento
    radioBox (g, badgeR, col::charcoalLo, col::charcoalLo, 4, 8);
    radioBox (g, badgeR.reduced (2), juce::Colour (0xff5a5460), col::charcoalLo, 2, 6);
    g.setColour (juce::Colour (0xff7a7482));
    g.fillRect (badgeR.getX() + 8, badgeR.getY() + 3, badgeR.getWidth() - 16, 2);

    // etiquetas de los selectores
    g.setFont (fonts().b (21.f));
    g.setColour (col::panelInk);
    for (size_t i = 0; i < combos.size(); ++i)
        g.drawText (comboLabels[(int) i], combos[i]->getX(), combos[i]->getY() - 26, combos[i]->getWidth() - 20, 24, juce::Justification::centred);

    // grupo MANTENER
    g.setColour (col::khakiDark);
    g.drawRect (lockR, 3);
    g.setColour (col::charcoal);
    g.fillRect (lockR.getX() + 24, lockR.getY() - 4, 112, 8);
    g.setFont (fonts().b (19.f));
    g.setColour (col::panelInk);
    g.drawText ("MANTENER", lockR.getX() + 24, lockR.getY() - 12, 112, 22, juce::Justification::centred);

    // altavoces y pegatinas

    // pantallas del piano roll y del carril
    for (auto box : { rollR, laneR })
    {
        // mismo marco que la pantalla del campamento
        radioBox (g, box, col::outline, col::outline, 4, 8);
        radioBox (g, box.reduced (2), col::khakiDark, col::outline, 2, 6);
        g.setColour (juce::Colour (nightSky[0]));
        g.fillRect (box.reduced (6));
    }

    // cabecera del carril: etiquetas de los controles del Mood, o una pista en modo velocity
    g.setFont (fonts().b (16.f));
    g.setColour (col::labelInk);
    g.drawText (lane.mode == 1 ? u8 ("Dibuja un valor por paso para modular el destino  \xc2\xb7  clic derecho: borrar")
                               : u8 ("Arrastra las barras para cambiar la fuerza de cada nota  \xc2\xb7  clic derecho: valor normal"),
                laneTabs[1].getRight() + 22, laneHeadR.getY(), laneHeadR.getRight() - laneTabs[1].getRight() - 22, laneHeadR.getHeight(),
                juce::Justification::centredLeft);

    // panel de modulación: marco como el de MANTENER, con sus rótulos y el valor de los dos diales
    {
        g.setColour (col::display);
        g.drawRect (modR, 3);
        g.setColour (col::khaki);
        g.fillRect (modR.getX() + 22, modR.getY() - 4, modR.getWidth() - 44, 8);
        g.setFont (fonts().b (19.f));
        g.setColour (col::labelInk);
        g.drawText (u8 ("MODULACI\xc3\x93N"), modR.getX() + 22, modR.getY() - 12, modR.getWidth() - 44, 22, juce::Justification::centred);
        g.setFont (fonts().b (16.f));
        auto cap = [&] (const juce::Component& c, const char* t)
        { g.drawText (u8 (t), c.getX(), c.getY() - 22, c.getWidth(), 20, juce::Justification::centred); };
        cap (moodDestBox, "Destino");
        cap (lfoShapeBox, "LFO");
        g.setColour (col::khakiDark);
        g.fillRect (modR.getX() + 14, modR.getY() + 104 - 12, modR.getWidth() - 28, 2);
        for (auto* k : { &moodKnob, &lfoKnob })
        {
            auto kb = k->getBounds();
            ledScreen (g, juce::Rectangle<int> (kb.getCentreX() - 24, kb.getY() - 30, 48, 26));
            g.setFont (fonts().b (18.f));
            g.setColour (col::cream);
            g.drawText (k == &moodKnob ? "Mood" : "Nivel LFO", kb.getCentreX() - 50, kb.getBottom() - 4, 100, 22, juce::Justification::centred);
        }
    }

    // diales
    for (int i = 0; i < NK; ++i)
    {
        auto kb = knobs[i].getBounds();
        ledScreen (g, juce::Rectangle<int> (kb.getCentreX() - 22, knobR.getY(), 44, 26));
        g.setFont (fonts().b (18.f));
        g.setColour (i < 2 ? col::amberHi : col::cream);
        g.drawText (juce::String::fromUTF8 (knobLabels[i]), kb.getCentreX() - 45, kb.getBottom() - 4, 90, 22, juce::Justification::centred);
    }
}


// Todo lo que no cambia se dibuja una vez en una imagen (a la resolución real de la pantalla) y se reutiliza;
// en cada fotograma solo se pinta encima lo que se mueve. Así la interfaz gasta muy poco procesador.
void RadioPanel::paint (juce::Graphics& g)
{
    const float sc = juce::jlimit (.5f, 4.f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (! bg.isValid() || std::abs (sc - bgScale) > .01f || bgLaneMode != lane.mode)
    {
        bgScale = sc;
        bgLaneMode = lane.mode;
        juce::Image soft (juce::Image::RGB, juce::roundToInt (getWidth() * sc), juce::roundToInt (getHeight() * sc), false, juce::SoftwareImageType());
        {
            juce::Graphics gb (soft);
            gb.addTransform (juce::AffineTransform::scale (sc));
            paintStatic (gb);
        }
        bg = juce::NativeImageType().convert (soft);   // una sola vez al formato de la tarjeta gráfica
    }
    g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
    g.drawImage (bg, getLocalBounds().toFloat());

    // patrón que está sonando (con la cadena): rayita ámbar bajo su letra
    {
        const int ps = proc.playSlot.load();
        if (ps >= 0 && ps < 8)
        {
            const auto b = slotBtns[ps].getBounds();
            g.setColour (col::amberHi);
            g.fillRect (b.getX() + 6, b.getBottom() + 1, b.getWidth() - 12, 3);
        }
    }

    // escena del campamento
    if (g.clipRegionIntersects (badgeR))
        g.drawImage (badge, badgeR.reduced (6).toFloat());

    // pantallita de estado
    if (g.clipRegionIntersects (statusR))
    {
        juce::String status = juce::String (juce::roundToInt (proc.lastBpm.load())) + " BPM";
        const int tr = proc.transpose.load();
        if (tr != 0) status += juce::String ("  ") + (tr > 0 ? "+" : "") + juce::String (tr);
        g.setFont (fonts().t (11.f));
        ledText (g, status, statusR.reduced (16, 0).translated (0, 1), juce::Justification::centredLeft);
    }

    // valores de los diales de modulación
    g.setFont (fonts().t (11.f));
    for (auto* k : { &moodKnob, &lfoKnob })
    {
        auto kb = k->getBounds();
        auto badgeBox = juce::Rectangle<int> (kb.getCentreX() - 24, kb.getY() - 30, 48, 26);
        if (! g.clipRegionIntersects (badgeBox)) continue;
        const double v = k->getValue();
        const juce::String txt = k == &moodKnob ? juce::String (juce::roundToInt (v * 100))
                                                : juce::String (juce::roundToInt (v * 100)).paddedLeft ('0', 2);
        ledText (g, txt, badgeBox.translated (1, 1), juce::Justification::centred);
    }
    // valores de los diales grandes
    if (g.clipRegionIntersects (knobR))
        for (int i = 0; i < NK; ++i)
        {
            auto kb = knobs[i].getBounds();
            auto badgeBox = juce::Rectangle<int> (kb.getCentreX() - 22, knobR.getY(), 44, 26);
            if (! g.clipRegionIntersects (badgeBox)) continue;
            const float v = isLaneKnob (i) ? (float) knobs[i].getValue() : proc.apvts.getRawParameterValue (knobIds[i])->load();
            const juce::String txt = i == 11 ? juce::String (juce::roundToInt (50 + v * 16.6f)) : juce::String (juce::roundToInt (v * 100)).paddedLeft ('0', 2);
            ledText (g, txt, badgeBox.translated (1, 1), juce::Justification::centred);
        }
}

//==============================================================================
// EDITOR: escala la radio al tamaño de la ventana manteniendo la proporción
DrumMakerEditor::DrumMakerEditor (DrumMakerProcessor& p)
    : AudioProcessorEditor (&p), proc (p), panel (p)
{
    // el tamaño guardado se lee antes de fijar los límites (al fijarlos se llama a resized con un tamaño provisional)
    const int saved = proc.editorW.load();
    addAndMakeVisible (panel);
    setResizable (true, true);
    setResizeLimits (RadioPanel::baseW * 55 / 100, RadioPanel::baseH * 55 / 100, RadioPanel::baseW * 2, RadioPanel::baseH * 2);
    getConstrainer()->setFixedAspectRatio ((double) RadioPanel::baseW / (double) RadioPanel::baseH);
    const int w = juce::jlimit (RadioPanel::baseW * 55 / 100, RadioPanel::baseW * 2, saved > 0 ? saved : RadioPanel::baseW * 9 / 10);
    setSize (w, juce::roundToInt (w * (double) RadioPanel::baseH / RadioPanel::baseW));
}

void DrumMakerEditor::resized()
{
    const float s = getWidth() / (float) RadioPanel::baseW;
    panel.setBounds (0, 0, RadioPanel::baseW, RadioPanel::baseH);
    panel.setTransform (juce::AffineTransform::scale (s));
    proc.editorW.store (getWidth());
}
