// Copyright (c) 2026 GuitarFros
// SPDX-License-Identifier: AGPL-3.0-or-later  (links against JUCE, AGPLv3)
#pragma once
#include "PluginProcessor.h"
#include "FactoryPresets.h"
#include <functional>
#include <vector>
#include <cmath>

namespace nf
{
// Real-time lit knob cap. The light source is FIXED (top-left); the knob's texture, relief
// (scratches, dents, wear) and pointer rotate underneath it, so shading reacts correctly.
class KnobRenderer
{
public:
    static constexpr int N = 256;   // size of the albedo / relief maps

    bool isValid() const noexcept { return valid; }

    // albedo: RGB texture without baked lighting.
    // relief: R,G = surface tilt (x,y) encoded around 128, B = wear mask (shinier where worn).
    void load (const juce::Image& albedo, const juce::Image& relief)
    {
        valid = false;
        if (! albedo.isValid() || ! relief.isValid()
            || albedo.getWidth() != N || albedo.getHeight() != N
            || relief.getWidth() != N || relief.getHeight() != N)
            return;

        for (auto* v : { &ar, &ag, &ab, &tx, &ty, &wear })
            v->assign ((size_t) (N * N), 0.0f);

        const juce::Image::BitmapData A (albedo, 0, 0, N, N);
        const juce::Image::BitmapData R (relief, 0, 0, N, N);
        double sum = 0.0;
        for (int y = 0; y < N; ++y)
            for (int x = 0; x < N; ++x)
            {
                const auto ca = A.getPixelColour (x, y);
                const auto cr = R.getPixelColour (x, y);
                const size_t i = (size_t) (y * N + x);
                ar[i] = ca.getFloatRed();
                ag[i] = ca.getFloatGreen();
                ab[i] = ca.getFloatBlue();
                tx[i] = ((float) cr.getRed()   - 128.0f) / 127.0f * 2.0f;
                ty[i] = ((float) cr.getGreen() - 128.0f) / 127.0f * 2.0f;
                wear[i] = cr.getFloatBlue();
                sum += (double) (ar[i] + ag[i] + ab[i]) / 3.0;
            }
        meanLum = (float) (sum / (double) (N * N));
        valid = meanLum > 1.0e-4f;
    }

    // angle: radians, clockwise, 0 = pointer at 12 o'clock.
    juce::Image render (int S, float angle) const
    {
        juce::Image out (juce::Image::ARGB, S, S, true);
        if (! valid)
            return out;

        juce::Image::BitmapData bd (out, 0, 0, S, S, juce::Image::BitmapData::writeOnly);

        const float c = std::cos (angle), s = std::sin (angle);

        // fixed light (top-left, in front) and half vector
        float lx = -0.50f, ly = -0.62f, lz = 0.60f;
        const float ll = std::sqrt (lx * lx + ly * ly + lz * lz);
        lx /= ll; ly /= ll; lz /= ll;
        float hx = lx, hy = ly, hz = lz + 1.0f;
        const float hl = std::sqrt (hx * hx + hy * hy + hz * hz);
        hx /= hl; hy /= hl; hz /= hl;

        constexpr float bump = 0.35f, amb = 0.45f, dif = 0.92f, ks = 0.5f;
        constexpr float pY0 = -0.96f, pY1 = -0.27f, pHalfW = 0.034f;   // pointer, in texture space

        for (int y = 0; y < S; ++y)
        {
            for (int x = 0; x < S; ++x)
            {
                const float u = ((float) x + 0.5f) / (float) S * 2.0f - 1.0f;
                const float v = ((float) y + 0.5f) / (float) S * 2.0f - 1.0f;
                const float r = std::sqrt (u * u + v * v);
                const float alpha = juce::jlimit (0.0f, 1.0f, (1.0f - r) * (float) S * 0.5f);
                if (alpha <= 0.0f)
                    continue;
                const float rr = std::max (r, 1.0e-4f);

                // screen -> texture space (rotate by -angle)
                const float tu =  c * u + s * v;
                const float tv = -s * u + c * v;
                const float px = (tu + 1.0f) * 0.5f * (float) N - 0.5f;
                const float py = (tv + 1.0f) * 0.5f * (float) N - 0.5f;

                float cr = bil (ar, px, py), cg = bil (ag, px, py), cb = bil (ab, px, py);
                const float gxt = bil (tx, px, py), gyt = bil (ty, px, py);
                const float W   = bil (wear, px, py);

                // relief tilt: texture space -> screen space (rotate by +angle)
                const float gxs = c * gxt - s * gyt;
                const float gys = s * gxt + c * gyt;

                // pointer: a short bar along the texture's -y axis through the centre
                const float dyp = tv < pY0 ? tv - pY0 : (tv > pY1 ? tv - pY1 : 0.0f);
                const float dseg = std::sqrt (tu * tu + dyp * dyp);
                const float cov = juce::jlimit (0.0f, 1.0f, (pHalfW - dseg) * (float) S * 0.5f + 0.5f);
                if (cov > 0.0f)
                {
                    const float grain = juce::jlimit (0.5f, 1.6f, ((cr + cg + cb) / 3.0f) / meanLum);
                    const float k = juce::jlimit (0.6f, 1.1f, 0.75f + 0.30f * grain);
                    const float worn = cov * (1.0f - 0.25f * W);
                    cr = cr * (1.0f - cov) + 0.62f * k * worn;
                    cg = cg * (1.0f - cov) + 0.60f * k * worn;
                    cb = cb * (1.0f - cov) + 0.55f * k * worn;
                }

                // surface normal = shallow dome + rim bevel + rotating relief
                const float t = juce::jlimit (0.0f, 1.0f, (r - 0.86f) / 0.14f);
                const float sm = t * t * (3.0f - 2.0f * t);
                const float slope = 0.55f * r * r + 2.4f * sm * sm;
                float nx = slope * u / rr + bump * gxs;
                float ny = slope * v / rr + bump * gys;
                float nz = 1.0f;
                const float nl = std::sqrt (nx * nx + ny * ny + nz * nz);
                nx /= nl; ny /= nl; nz /= nl;

                const float diff = std::max (nx * lx + ny * ly + nz * lz, 0.0f);
                const float spec = std::pow (std::max (nx * hx + ny * hy + nz * hz, 0.0f), 22.0f)
                                   * (0.15f + 0.85f * std::max (W, cov * 0.6f));
                const float shade = amb + dif * diff;

                const float sp = ks * spec * 0.55f;
                const float R = juce::jlimit (0.0f, 1.0f, cr * shade + sp);
                const float G = juce::jlimit (0.0f, 1.0f, cg * shade + sp);
                const float B = juce::jlimit (0.0f, 1.0f, cb * shade + sp);
                bd.setPixelColour (x, y, juce::Colour::fromFloatRGBA (R, G, B, alpha));
            }
        }
        return out;
    }

private:
    static float bil (const std::vector<float>& a, float px, float py)
    {
        px = juce::jlimit (0.0f, (float) N - 1.001f, px);
        py = juce::jlimit (0.0f, (float) N - 1.001f, py);
        const int x0 = (int) px, y0 = (int) py;
        const float fx = px - (float) x0, fy = py - (float) y0;
        const size_t i = (size_t) (y0 * N + x0);
        return a[i] * (1.0f - fx) * (1.0f - fy) + a[i + 1] * fx * (1.0f - fy)
             + a[i + (size_t) N] * (1.0f - fx) * fy + a[i + (size_t) N + 1] * fx * fy;
    }

    std::vector<float> ar, ag, ab, tx, ty, wear;
    float meanLum = 0.0f;
    bool valid = false;
};

// Rotary knob (-135..+135 deg) drawn by KnobRenderer.
class PanelKnob : public juce::Slider
{
public:
    PanelKnob (const juce::Image& albedo, const juce::Image& relief)
    {
        renderer.load (albedo, relief);
        setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                             juce::MathConstants<float>::pi * 2.75f, true);
        setMouseDragSensitivity (220);
    }

    void paint (juce::Graphics& g) override
    {
        if (! renderer.isValid())
            return;
        const auto b = getLocalBounds().toFloat();
        const float prop  = (float) valueToProportionOfLength (getValue());
        const float angle = juce::jmap (prop, 0.0f, 1.0f, -0.75f, 0.75f) * juce::MathConstants<float>::pi;
        const int S = juce::jlimit (96, 256, juce::roundToInt (b.getWidth() * 2.0f));

        if (! cache.isValid() || S != cacheSize || std::abs (angle - cacheAngle) > 0.0015f)
        {
            cache = renderer.render (S, angle);
            cacheSize = S;
            cacheAngle = angle;
        }
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (cache, b, juce::RectanglePlacement::stretchToFit);
    }

private:
    KnobRenderer renderer;
    juce::Image cache;
    int cacheSize = 0;
    float cacheAngle = 0.0f;
};

// Two-state toggle switch drawn from bitmaps (lever up / lever down).
class PanelSwitch : public juce::Component
{
public:
    PanelSwitch (const juce::Image& upImg, const juce::Image& downImg)
        : up (upImg), down (downImg)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    std::function<void (bool)> onToggle;   // receives the NEW "down" state

    // Where the sprite sits inside this component, as fractions of its size.
    void setSpriteFraction (juce::Rectangle<float> f) { frac = f; repaint(); }
    void setDown (bool d) { if (d != isLeverDown) { isLeverDown = d; repaint(); } }
    bool isDown() const noexcept { return isLeverDown; }

    void paint (juce::Graphics& g) override
    {
        const auto& s = isLeverDown ? down : up;
        if (! s.isValid())
            return;
        const float w = (float) getWidth(), h = (float) getHeight();
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (s, juce::Rectangle<float> (frac.getX() * w, frac.getY() * h,
                                                 frac.getWidth() * w, frac.getHeight() * h),
                     juce::RectanglePlacement::stretchToFit);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mouseWasClicked() && contains (e.getPosition()) && onToggle)
            onToggle (! isLeverDown);
    }

private:
    juce::Image up, down;
    juce::Rectangle<float> frac { 0.0f, 0.0f, 1.0f, 1.0f };
    bool isLeverDown = false;
};

// Invisible button sitting on top of the printed button in the panel image.
class PanelButton : public juce::Button
{
public:
    explicit PanelButton (const juce::String& n) : juce::Button (n)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void paintButton (juce::Graphics& g, bool over, bool isDown) override
    {
        const auto b = getLocalBounds().toFloat().reduced (1.0f);
        if (isDown)      { g.setColour (juce::Colours::black.withAlpha (0.45f));  g.fillRoundedRectangle (b, 4.0f); }
        else if (over)   { g.setColour (juce::Colours::white.withAlpha (0.08f));  g.fillRoundedRectangle (b, 4.0f); }
    }
};

// Blue LCD: shows a short message after any change, else the idle text.
class Lcd : public juce::Component, private juce::Timer
{
public:
    Lcd() { startTimerHz (20); }

    std::function<juce::String()> idleText;
    std::function<void()> onClick;

    void showMessage (const juce::String& m, int ms = 1800)
    {
        message = m;
        messageUntil = juce::Time::getMillisecondCounter() + (juce::uint32) ms;
        refresh();
    }

    void paint (juce::Graphics& g) override
    {
        g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(),
                               (float) getHeight() * 0.50f, juce::Font::bold));
        const auto glow = juce::Colour (0xff3aa8ff).withAlpha (0.18f);
        for (int dx = -2; dx <= 2; dx += 2)
            for (int dy = -2; dy <= 2; dy += 2)
            {
                g.setColour (glow);
                g.drawFittedText (shown, getLocalBounds().translated (dx, dy),
                                  juce::Justification::centred, 1, 0.5f);
            }
        g.setColour (juce::Colour (0xff9fe4ff));
        g.drawFittedText (shown, getLocalBounds(), juce::Justification::centred, 1, 0.5f);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mouseWasClicked() && onClick)
            onClick();
    }

private:
    void timerCallback() override { refresh(); }

    void refresh()
    {
        const bool showMsg = juce::Time::getMillisecondCounter() < messageUntil;
        const auto t = showMsg ? message : (idleText ? idleText() : juce::String());
        if (t != shown) { shown = t; repaint(); }
    }

    juce::String message, shown;
    juce::uint32 messageUntil = 0;
};
} // namespace nf

class NothingFilterEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NothingFilterEditor (NothingFilterProcessor&);
    ~NothingFilterEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;

    void timerCallback() override { repaint (stripArea); }

    struct PresetEntry
    {
        juce::String name;
        bool factory = false;
        int factoryIndex = -1;
        juce::File file;
    };

    juce::String presetName() const { return proc.apvts.state.getProperty ("presetName").toString(); }
    bool presetIsFactory() const { return (bool) proc.apvts.state.getProperty ("presetFactory", false); }
    std::vector<PresetEntry> listPresets() const;
    bool savePreset (const juce::String& rawName);
    void applyFactory (const nf::FactoryPreset& p);
    void loadPreset (const juce::File& f);
    void loadEntry (const PresetEntry& e, int index, int total);
    void stepPreset (int direction);
    void askNameAndSave();
    void deleteCurrent();
    void initAll();
    void showPresetMenu();

    NothingFilterProcessor& proc;

    juce::Image panel, ledOff;

    nf::PanelKnob kPos, kQ, kRange, kLevel;
    std::unique_ptr<APVTS::SliderAttachment> aPos, aQ, aRange, aLevel;

    nf::PanelSwitch swBypass, swMode, swVoicing;
    std::unique_ptr<juce::ParameterAttachment> pBypass, pMode, pVoicing;

    nf::Lcd lcd;
    nf::PanelButton btSave { "SAVE" }, btSaveAs { "SAVE AS" }, btDelete { "DELETE" }, btInit { "INIT" };
    nf::PanelButton btPrev { "PREVIOUS PRESET" }, btNext { "NEXT PRESET" };

    juce::Rectangle<int> stripArea;
    bool uiReady = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NothingFilterEditor)
};
