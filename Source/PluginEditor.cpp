// Copyright (c) 2026 GuitarFros
// SPDX-License-Identifier: AGPL-3.0-or-later  (links against JUCE, AGPLv3)
#include "PluginEditor.h"
#include <BinaryData.h>
#include <algorithm>

namespace
{
// All layout numbers below are in "panel pixels" of the artwork (3376 x 608).
constexpr float kPanelW = 3376.0f, kPanelH = 608.0f;

juce::Image img (const void* data, int size) { return juce::ImageCache::getFromMemory (data, size); }

juce::File presetDir()
{
    auto d = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                 .getChildFile ("GuitarFros").getChildFile ("StudioFros-NothingFilter").getChildFile ("Presets");
    d.createDirectory();
    return d;
}

juce::String signedDb (double v) { return (v >= 0.0 ? "+" : "") + juce::String (v, 1) + " dB"; }
} // namespace

NothingFilterEditor::NothingFilterEditor (NothingFilterProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p),
      panel  (img (BinaryData::panel_jpg,   BinaryData::panel_jpgSize)),
      ledOff (img (BinaryData::led_off_png, BinaryData::led_off_pngSize)),
      kPos   (img (BinaryData::knob_1_albedo_png, BinaryData::knob_1_albedo_pngSize), img (BinaryData::knob_1_relief_png, BinaryData::knob_1_relief_pngSize)),
      kQ     (img (BinaryData::knob_2_albedo_png, BinaryData::knob_2_albedo_pngSize), img (BinaryData::knob_2_relief_png, BinaryData::knob_2_relief_pngSize)),
      kRange (img (BinaryData::knob_3_albedo_png, BinaryData::knob_3_albedo_pngSize), img (BinaryData::knob_3_relief_png, BinaryData::knob_3_relief_pngSize)),
      kLevel (img (BinaryData::knob_4_albedo_png, BinaryData::knob_4_albedo_pngSize), img (BinaryData::knob_4_relief_png, BinaryData::knob_4_relief_pngSize)),
      swBypass  (img (BinaryData::sw_up_png, BinaryData::sw_up_pngSize), img (BinaryData::sw_down_png, BinaryData::sw_down_pngSize)),
      swMode    (img (BinaryData::sw_up_png, BinaryData::sw_up_pngSize), img (BinaryData::sw_down_png, BinaryData::sw_down_pngSize)),
      swVoicing (img (BinaryData::sw_up_png, BinaryData::sw_up_pngSize), img (BinaryData::sw_down_png, BinaryData::sw_down_pngSize))
{
    // ---- knobs ----
    for (auto* k : { &kPos, &kQ, &kRange, &kLevel })
        addAndMakeVisible (*k);

    aPos   = std::make_unique<APVTS::SliderAttachment> (proc.apvts, "position", kPos);
    aQ     = std::make_unique<APVTS::SliderAttachment> (proc.apvts, "q",        kQ);
    aRange = std::make_unique<APVTS::SliderAttachment> (proc.apvts, "range",    kRange);
    aLevel = std::make_unique<APVTS::SliderAttachment> (proc.apvts, "level",    kLevel);

    kPos.onValueChange   = [this] { lcd.showMessage ("Wah Position " + juce::String (juce::roundToInt (kPos.getValue() * 100.0)) + "%"); };
    kQ.onValueChange     = [this] { lcd.showMessage ("Q/Peak " + juce::String (kQ.getValue(), 1)); };
    kRange.onValueChange = [this] { lcd.showMessage ("Range " + juce::String (juce::roundToInt (kRange.getValue())) + " Hz"); };
    kLevel.onValueChange = [this] { lcd.showMessage ("Level " + signedDb (kLevel.getValue())); };

    // ---- switches (lever down = parameter value 1) ----
    auto setupSwitch = [this] (nf::PanelSwitch& sw, std::unique_ptr<juce::ParameterAttachment>& att,
                               const char* id, std::function<juce::String (bool)> msg)
    {
        addAndMakeVisible (sw);
        auto* param = proc.apvts.getParameter (id);
        jassert (param != nullptr);
        att = std::make_unique<juce::ParameterAttachment> (*param, [this, &sw, msg] (float v)
        {
            const bool d = v > 0.5f;
            sw.setDown (d);
            if (uiReady) lcd.showMessage (msg (d));
            repaint();
        });
        sw.onToggle = [&att] (bool nowDown) { att->setValueAsCompleteGesture (nowDown ? 1.0f : 0.0f); };
        att->sendInitialUpdate();
    };
    setupSwitch (swBypass,  pBypass,  "bypass",  [] (bool d) { return juce::String (d ? "Bypassed" : "Active"); });
    setupSwitch (swMode,    pMode,    "mode",    [] (bool d) { return juce::String (d ? "Mode: PEDAL MIDI CC" : "Mode: FIXED"); });
    setupSwitch (swVoicing, pVoicing, "voicing", [] (bool d) { return juce::String (d ? "Voicing: MODERN" : "Voicing: VINTAGE"); });

    // ---- LCD + buttons ----
    addAndMakeVisible (lcd);
    lcd.idleText = [this]
    {
        const auto n = presetName();
        return n.isNotEmpty() ? n : "Wah Position " + juce::String (juce::roundToInt (proc.getLivePosition() * 100.0f)) + "%";
    };
    lcd.onClick  = [this] { showPresetMenu(); };

    for (auto* b : { &btSave, &btSaveAs, &btDelete, &btInit, &btPrev, &btNext })
        addAndMakeVisible (*b);

    btSave.onClick   = [this] { auto n = presetName(); if (n.isEmpty() || presetIsFactory()) askNameAndSave(); else savePreset (n); };
    btSaveAs.onClick = [this] { askNameAndSave(); };
    btDelete.onClick = [this] { deleteCurrent(); };
    btInit.onClick   = [this] { initAll(); };
    btPrev.onClick   = [this] { stepPreset (-1); };
    btNext.onClick   = [this] { stepPreset (+1); };

    // ---- window ----
    setResizable (true, true);
    setResizeLimits (800, (int) (800.0f * kPanelH / kPanelW), 2500, (int) (2500.0f * kPanelH / kPanelW));
    getConstrainer()->setFixedAspectRatio ((double) (kPanelW / kPanelH));
    setSize (1250, (int) std::round (1250.0f * kPanelH / kPanelW));

    uiReady = true;
    startTimerHz (10);
}

void NothingFilterEditor::paint (juce::Graphics& g)
{
    const float s = (float) getWidth() / kPanelW;
    auto R = [s] (float x, float y, float w, float h) { return juce::Rectangle<float> (x * s, y * s, w * s, h * s); };

    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (panel, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);

    // Dim the "active" LED when bypassed.
    if (swBypass.isDown() && ledOff.isValid())
        g.drawImage (ledOff, R (3109.0f - 17.0f, 144.0f - 17.0f, 34.0f, 34.0f), juce::RectanglePlacement::stretchToFit);

    // Live values under the knob labels.
    const juce::String vals[4] = {
        juce::String (juce::roundToInt (proc.getLivePosition() * 100.0f)) + "%",
        juce::String (proc.apvts.getRawParameterValue ("q")->load(), 1),
        juce::String (juce::roundToInt (proc.apvts.getRawParameterValue ("range")->load())) + " Hz",
        signedDb (proc.apvts.getRawParameterValue ("level")->load())
    };
    const float cx[4] = { 285.0f, 605.0f, 925.0f, 1245.0f };
    g.setFont (g.getCurrentFont().withHeight (juce::jmax (9.0f, 31.0f * s)).boldened());
    g.setColour (juce::Colour (0xffd8d8d0));
    for (int i = 0; i < 4; ++i)
        g.drawText (vals[i], R (cx[i] - 150.0f, 546.0f, 300.0f, 32.0f), juce::Justification::centred);
}

void NothingFilterEditor::resized()
{
    const float s = (float) getWidth() / kPanelW;
    auto R = [s] (float x, float y, float w, float h)
    {
        return juce::Rectangle<float> (x * s, y * s, w * s, h * s).toNearestInt();
    };

    // knob caps: 200 x 200 px, centred 6 px left / 9 px above each skirt centre (skirt y = 338)
    const float kr = 100.0f, ky = 338.0f - 9.0f;
    kPos.setBounds   (R (285.0f  - 6.0f - kr, ky - kr, 2 * kr, 2 * kr));
    kQ.setBounds     (R (605.0f  - 6.0f - kr, ky - kr, 2 * kr, 2 * kr));
    kRange.setBounds (R (925.0f  - 6.0f - kr, ky - kr, 2 * kr, 2 * kr));
    kLevel.setBounds (R (1245.0f - 6.0f - kr, ky - kr, 2 * kr, 2 * kr));

    // LCD and the four printed buttons
    lcd.setBounds (R (1688.0f, 178.0f, 740.0f, 152.0f));
    btPrev.setBounds (R (1505.0f, 218.0f, 160.0f, 68.0f));
    btNext.setBounds (R (2450.0f, 218.0f, 160.0f, 68.0f));
    btSave.setBounds   (R (1562.0f, 449.0f, 150.0f, 64.0f));
    btSaveAs.setBounds (R (1838.0f, 448.0f, 151.0f, 64.0f));
    btDelete.setBounds (R (2120.0f, 448.0f, 159.0f, 64.0f));
    btInit.setBounds   (R (2405.0f, 449.0f, 156.0f, 64.0f));

    // toggle switches: plate is 174 x 96 around the nut centre, sprite 110 x 84
    auto place = [&] (nf::PanelSwitch& sw, float nutX, float nutY)
    {
        sw.setBounds (R (2895.0f, nutY - 48.0f, 174.0f, 96.0f));
        sw.setSpriteFraction ({ (nutX - 56.0f - 2895.0f) / 174.0f, 6.0f / 96.0f, 110.0f / 174.0f, 84.0f / 96.0f });
    };
    place (swBypass,  2980.0f, 119.0f);
    place (swMode,    2982.0f, 279.0f);
    place (swVoicing, 2982.0f, 450.0f);

    stripArea = R (100.0f, 546.0f, 1300.0f, 32.0f);
}

// ------------------------------------------------------------------ presets
std::vector<NothingFilterEditor::PresetEntry> NothingFilterEditor::listPresets() const
{
    std::vector<PresetEntry> list;
    for (int i = 0; i < nf::kNumFactoryPresets; ++i)
    {
        PresetEntry e;
        e.name = nf::kFactoryPresets[i].name;
        e.factory = true;
        e.factoryIndex = i;
        list.push_back (e);
    }

    auto files = presetDir().findChildFiles (juce::File::findFiles, false, "*.xml");
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
    {
        return a.getFileName().compareIgnoreCase (b.getFileName()) < 0;
    });
    for (const auto& f : files)
    {
        PresetEntry e;
        e.name = f.getFileNameWithoutExtension();
        e.file = f;
        list.push_back (e);
    }
    return list;
}

bool NothingFilterEditor::savePreset (const juce::String& rawName)
{
    const auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty())
        return false;

    proc.apvts.state.setProperty ("presetName", name, nullptr);
    proc.apvts.state.setProperty ("presetFactory", false, nullptr);
    auto xml = proc.apvts.copyState().createXml();
    const bool ok = xml != nullptr && xml->writeTo (presetDir().getChildFile (name + ".xml"));
    lcd.showMessage (ok ? "Saved: " + name : juce::String ("Save failed"), 2200);
    return ok;
}

void NothingFilterEditor::applyFactory (const nf::FactoryPreset& p)
{
    auto set = [this] (const char* id, float value)
    {
        if (auto* prm = proc.apvts.getParameter (id))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (value));
            prm->endChangeGesture();
        }
    };
    set ("position", p.position);
    set ("q",        p.q);
    set ("range",    p.range);
    set ("level",    p.level);
    set ("bypass",   0.0f);
    set ("mode",     (float) p.mode);
    set ("voicing",  (float) p.voicing);

    proc.apvts.state.setProperty ("presetName", juce::String (p.name), nullptr);
    proc.apvts.state.setProperty ("presetFactory", true, nullptr);
}

void NothingFilterEditor::loadPreset (const juce::File& f)
{
    if (auto xml = juce::parseXML (f))
    {
        if (xml->hasTagName (proc.apvts.state.getType()))
        {
            proc.apvts.replaceState (juce::ValueTree::fromXml (*xml));
            proc.apvts.state.setProperty ("presetName", f.getFileNameWithoutExtension(), nullptr);
            proc.apvts.state.setProperty ("presetFactory", false, nullptr);
        }
    }
}

void NothingFilterEditor::loadEntry (const PresetEntry& e, int index, int total)
{
    if (e.factory)
        applyFactory (nf::kFactoryPresets[e.factoryIndex]);
    else
        loadPreset (e.file);

    lcd.showMessage (juce::String (index + 1) + "/" + juce::String (total) + " " + e.name, 2500);
}

void NothingFilterEditor::stepPreset (int direction)
{
    const auto list = listPresets();
    const int n = (int) list.size();
    if (n == 0)
        return;

    const auto cur = presetName();
    const bool curFactory = presetIsFactory();
    int idx = -1;
    for (int i = 0; i < n; ++i)
        if (list[(size_t) i].name == cur && list[(size_t) i].factory == curFactory)
        {
            idx = i;
            break;
        }

    const int next = idx < 0 ? (direction > 0 ? 0 : n - 1) : (idx + direction + n) % n;
    loadEntry (list[(size_t) next], next, n);
}

void NothingFilterEditor::askNameAndSave()
{
    auto* w = new juce::AlertWindow ("Save preset", "Preset name:", juce::MessageBoxIconType::NoIcon, this);
    const auto cur = presetIsFactory() ? juce::String() : presetName();
    w->addTextEditor ("name", cur.isEmpty() ? juce::String ("My preset") : cur);
    w->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<NothingFilterEditor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int r)
    {
        if (r == 1 && safe != nullptr)
            safe->savePreset (w->getTextEditorContents ("name"));
    }), true);
}

void NothingFilterEditor::deleteCurrent()
{
    if (presetIsFactory())
    {
        lcd.showMessage ("Factory preset: locked");
        return;
    }

    const auto name = presetName();
    const auto f = presetDir().getChildFile (name + ".xml");
    if (name.isEmpty() || ! f.existsAsFile())
    {
        lcd.showMessage ("No preset to delete");
        return;
    }

    auto* w = new juce::AlertWindow ("Delete preset", "Delete \"" + name + "\"?",
                                     juce::MessageBoxIconType::QuestionIcon, this);
    w->addButton ("Delete", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<NothingFilterEditor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, f, name] (int r)
    {
        if (r == 1 && f.deleteFile() && safe != nullptr)
        {
            safe->proc.apvts.state.setProperty ("presetName", "", nullptr);
            safe->proc.apvts.state.setProperty ("presetFactory", false, nullptr);
            safe->lcd.showMessage ("Deleted: " + name);
        }
    }), true);
}

void NothingFilterEditor::initAll()
{
    for (auto* prm : proc.getParameters())
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->getDefaultValue());
        prm->endChangeGesture();
    }
    proc.apvts.state.setProperty ("presetName", "", nullptr);
    proc.apvts.state.setProperty ("presetFactory", false, nullptr);
    lcd.showMessage ("INIT");
}

void NothingFilterEditor::showPresetMenu()
{
    const auto list = listPresets();
    const int n = (int) list.size();
    const auto cur = presetName();
    const bool curFactory = presetIsFactory();

    juce::PopupMenu m;
    bool factoryHeader = false, userHeader = false;
    for (int i = 0; i < n; ++i)
    {
        const auto& e = list[(size_t) i];
        if (e.factory && ! factoryHeader) { m.addSectionHeader ("Factory"); factoryHeader = true; }
        if (! e.factory && ! userHeader)  { m.addSectionHeader ("User");    userHeader = true; }
        m.addItem (i + 1, e.name, true, e.name == cur && e.factory == curFactory);
    }

    juce::Component::SafePointer<NothingFilterEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&lcd), [safe, list, n] (int id)
    {
        if (safe != nullptr && id >= 1 && id <= n)
            safe->loadEntry (list[(size_t) (id - 1)], id - 1, n);
    });
}
