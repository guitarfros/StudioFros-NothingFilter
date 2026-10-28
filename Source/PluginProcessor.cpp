// Copyright (c) 2026 GuitarFros
// SPDX-License-Identifier: AGPL-3.0-or-later  (links against JUCE, AGPLv3)
#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout NothingFilterProcessor::createLayout()
{
    using juce::ParameterID;
    using juce::NormalisableRange;

    APVTS::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterID { "position", 1 }, "Position",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterID { "q", 1 }, "Q / Peak",
        NormalisableRange<float> (2.0f, 12.0f, 0.01f), 5.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterID { "range", 1 }, "Range (Hz max)",
        NormalisableRange<float> (1200.0f, 3500.0f, 1.0f), 2000.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterID { "level", 1 }, "Level (dB)",
        NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        ParameterID { "bypass", 1 }, "Bypass", false));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        ParameterID { "mode", 1 }, "Mode",
        juce::StringArray { "Fixed", "Pedal (MIDI CC)" }, 0));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        ParameterID { "voicing", 1 }, "Voicing",
        juce::StringArray { "Vintage", "Modern" }, 0));

    return layout;
}

NothingFilterProcessor::NothingFilterProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pPosition = apvts.getRawParameterValue ("position");
    pQ        = apvts.getRawParameterValue ("q");
    pRange    = apvts.getRawParameterValue ("range");
    pLevel    = apvts.getRawParameterValue ("level");
    pBypass   = apvts.getRawParameterValue ("bypass");
    pMode     = apvts.getRawParameterValue ("mode");
    pVoicing  = apvts.getRawParameterValue ("voicing");
}

bool NothingFilterProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void NothingFilterProcessor::prepareToPlay (double sampleRate, int)
{
    for (auto& c : channels) { c.setSampleRate (sampleRate); c.reset(); }

    posSmooth.reset   (sampleRate, 0.015);
    wetSmooth.reset   (sampleRate, 0.010);
    levelSmooth.reset (sampleRate, 0.020);

    posSmooth.setCurrentAndTargetValue   (pPosition->load());
    wetSmooth.setCurrentAndTargetValue   (pBypass->load() > 0.5f ? 0.0f : 1.0f);
    levelSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pLevel->load()));
}

void NothingFilterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // Pedal mode: expression (CC11) or foot controller (CC4) -> position.
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isController() && (m.getControllerNumber() == 11 || m.getControllerNumber() == 4))
            ccPosition.store (m.getControllerValue() / 127.0f);
    }

    const int numCh      = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    const bool pedal  = pMode->load() > 0.5f;
    const bool modern = pVoicing->load() > 0.5f;
    const float q     = pQ->load();

    wah::Params p;
    p.fMin    = 250.0;
    p.fMax    = (double) pRange->load();
    p.vintage = ! modern;
    p.qLow    = modern ? q : q * 0.7;
    p.qHigh   = modern ? q : q * 1.3;
    p.peakDb  = 0.0;   // Level is applied below, smoothed

    posSmooth.setTargetValue   (pedal ? ccPosition.load() : pPosition->load());
    livePosition.store (posSmooth.getTargetValue());
    wetSmooth.setTargetValue   (pBypass->load() > 0.5f ? 0.0f : 1.0f);
    levelSmooth.setTargetValue (juce::Decibels::decibelsToGain (pLevel->load()));

    constexpr int chunk = 32;   // coefficients refreshed every 32 samples
    int done = 0;
    while (done < numSamples)
    {
        const int n = juce::jmin (chunk, numSamples - done);
        p.position = (double) posSmooth.skip (n);
        for (int c = 0; c < numCh; ++c)
            channels[c].update (p);

        for (int i = 0; i < n; ++i)
        {
            const float w = wetSmooth.getNextValue();
            const float g = levelSmooth.getNextValue();
            for (int c = 0; c < numCh; ++c)
            {
                float* d = buffer.getWritePointer (c) + done + i;
                const float dry = *d;
                const float wet = (float) channels[c].process ((double) dry) * g;
                *d = dry * (1.0f - w) + wet * w;
            }
        }
        done += n;
    }

    for (int c = numCh; c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, numSamples);
}

juce::AudioProcessorEditor* NothingFilterProcessor::createEditor()
{
    return new NothingFilterEditor (*this);
}

void NothingFilterProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void NothingFilterProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NothingFilterProcessor();
}
