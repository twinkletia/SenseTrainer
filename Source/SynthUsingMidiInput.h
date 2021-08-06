/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2020 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "SenseComponent.h"
struct SineWaveSound : public juce::SynthesiserSound
{
    SineWaveSound() {}

    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
struct SineWaveVoice : public juce::SynthesiserVoice
{
    SineWaveVoice() {}

    bool canPlaySound(juce::SynthesiserSound *sound) override
    {
        return dynamic_cast<SineWaveSound *>(sound) != nullptr;
    }

    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound *, int) override
    {
        currentAngle = 0.0;
        level = velocity * 0.15;
        tailOff = 0.0;

        auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        auto cyclesPerSample = cyclesPerSecond / getSampleRate();

        angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
    }

    void stopNote(float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            if (tailOff == 0.0)
                tailOff = 1.0;
        }
        else
        {
            clearCurrentNote();
            angleDelta = 0.0;
        }
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioSampleBuffer &outputBuffer, int startSample, int numSamples) override
    {
        if (angleDelta != 0.0)
        {
            if (tailOff > 0.0)
            {
                while (--numSamples >= 0)
                {
                    auto currentSample = (float)(std::sin(currentAngle) * level * tailOff);

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample(i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;

                    tailOff *= 0.99;

                    if (tailOff <= 0.005)
                    {
                        clearCurrentNote();

                        angleDelta = 0.0;
                        break;
                    }
                }
            }
            else
            {
                while (--numSamples >= 0)
                {
                    auto currentSample = (float)(std::sin(currentAngle) * level);

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample(i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;
                }
            }
        }
    }

private:
    double currentAngle = 0.0, angleDelta = 0.0, level = 0.0, tailOff = 0.0;
};
class SynthAudioSource : public juce::AudioSource,
                         public juce::Timer
{
public:
    SynthAudioSource(juce::MidiKeyboardState &keyState, UserInterface &ui)
        : keyboardState(keyState), UI(ui)
    {
        for (auto i = 0; i < 4; ++i)
            synth.addVoice(new SineWaveVoice());

        synth.addSound(new SineWaveSound());
    }

    void setUsingSineWaveSound()
    {
        synth.clearSounds();
    }

    void nextQuizNote()
    {
        if (UI.quiz[currentNote])
        {
            quizMessage = juce::MidiMessage::MidiMessage(144, UI.quiz[currentNote], 100);
            quizMessage.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
            quizMidi = juce::MidiBuffer::MidiBuffer(quizMessage);
        }
        else
        {
            const juce::MessageManagerLock messageManagerLock;
            UI.replayCompleted();
            currentNote = -1;
        }
    }

    void prepareToPlay(int, double sampleRate) override
    {
        synth.setCurrentPlaybackSampleRate(sampleRate);
        midiCollector.reset(sampleRate);
    }

    void releaseResources() override {}

    void getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override
    {
        bufferToFill.clearActiveBufferRegion();

        if (UI.buttonflag)
        {
            if (juce::Time::getMillisecondCounterHiRes() * 0.001 - quizMessage.getTimeStamp() > 0.5)
            {
                if (currentNote == -1)
                {
                    synth.allNotesOff(0, true);
                    keyboardState.allNotesOff(0);
                }
                else
                {
                    quizMidi = juce::MidiBuffer::MidiBuffer(juce::MidiMessage::MidiMessage(128, UI.quiz[currentNote], 0, 0.0));
                }
                synth.renderNextBlock(*bufferToFill.buffer, quizMidi,
                                      bufferToFill.startSample, bufferToFill.numSamples);

                quizMidi.clear();
                currentNote++;
                nextQuizNote();
            }
            else
            {
                synth.renderNextBlock(*bufferToFill.buffer, quizMidi,
                                      bufferToFill.startSample, bufferToFill.numSamples);

                quizMidi.clear();
            }
        }
        else
        {
            juce::MidiBuffer incomingMidi;
            midiCollector.removeNextBlockOfMessages(incomingMidi, bufferToFill.numSamples);

            keyboardState.processNextMidiBuffer(incomingMidi, bufferToFill.startSample,
                                                bufferToFill.numSamples, true);

            synth.renderNextBlock(*bufferToFill.buffer, incomingMidi,
                                  bufferToFill.startSample, bufferToFill.numSamples);
        }
    }

    void timerCallback() override
    {
        quizMidi = juce::MidiBuffer::MidiBuffer(juce::MidiMessage::MidiMessage(128, UI.quiz[currentNote], 0, 0.0));
        currentNote++;
        stopTimer();
    }

    juce::MidiMessageCollector *getMidiCollector()
    {
        return &midiCollector;
    }

private:
    juce::MidiKeyboardState &keyboardState;
    juce::Synthesiser synth;
    juce::MidiMessageCollector midiCollector;
    bool flag = false;
    int currentNote = -1;
    juce::MidiMessage quizMessage;
    juce::MidiBuffer quizMidi = juce::MidiBuffer::MidiBuffer();
    bool noteOffflag = true;
    UserInterface &UI;
};