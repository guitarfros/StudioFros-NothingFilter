// Copyright (c) 2026 GuitarFros
// SPDX-License-Identifier: AGPL-3.0-or-later  (links against JUCE, AGPLv3)
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "wah_core.h"

class NothingFilterProcessor : public juce::AudioProcessor
{
public:
    NothingFilterProcessor();
    ~NothingFilterProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "StudioFros - Nothing Filter"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Current wah position (0..1), including MIDI-pedal mode. Used by the LCD.
    float getLivePosition() const noexcept { return livePosition.load(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    wah::Channel channels[2];

    juce::SmoothedValue<float> posSmooth, wetSmooth, levelSmooth;
    std::atomic<float> ccPosition { 0.5f };
    std::atomic<float> livePosition { 0.5f };

    std::atomic<float>* pPosition = nullptr;
    std::atomic<float>* pQ        = nullptr;
    std::atomic<float>* pRange    = nullptr;
    std::atomic<float>* pLevel    = nullptr;
    std::atomic<float>* pBypass   = nullptr;
    std::atomic<float>* pMode     = nullptr;
    std::atomic<float>* pVoicing  = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NothingFilterProcessor)
};
