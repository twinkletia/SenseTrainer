#pragma once

#include <JuceHeader.h>
#include "SenseComponent.h"
#include "SynthUsingMidiInput.h"

class MainContentComponent : public juce::AudioAppComponent,
                             private juce::MidiInputCallback,
                             private juce::MidiKeyboardStateListener,
                             private juce::MultiTimer
{
public:
    MainContentComponent()
        : synthAudioSource(keyboardState, UI),
          UI(midiMessagesBox),
          keyboardComponent(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
          startTime(juce::Time::getMillisecondCounterHiRes() * 0.001)
    {
#if JUCE_WINDOWS
        juce::String typeFaceName = "Arial Unicode MS";
        juce::Desktop::getInstance().getDefaultLookAndFeel().setDefaultSansSerifTypefaceName(typeFaceName);
#elif JUCE_MAC
        juce::String typeFaceName = "Arial Unicode MS";
        juce::Desktop::getInstance().getDefaultLookAndFeel().setDefaultSansSerifTypefaceName(typeFaceName);
#elif JUCE_LINUX
        juce::String typeFaceName = "IPAGothic";
        juce::Desktop::getInstance().getDefaultLookAndFeel().setDefaultSansSerifTypefaceName(typeFaceName);
#endif
        setAudioChannels(0, 2);

        setSize(800, 500);
        startTimer(0, 400);
        addAndMakeVisible(midiInputListLabel);
        midiInputListLabel.setText("MIDI Input:", juce::dontSendNotification);
        midiInputListLabel.attachToComponent(&midiInputList, true);

        auto midiInputs = juce::MidiInput::getAvailableDevices();
        addAndMakeVisible(midiInputList);
        midiInputList.setTextWhenNoChoicesAvailable("No MIDI Inputs Enabled");

        juce::StringArray midiInputNames;
        for (auto input : midiInputs)
            midiInputNames.add(input.name);

        midiInputList.addItemList(midiInputNames, 1);
        midiInputList.onChange = [this]
        { setMidiInput(midiInputList.getSelectedItemIndex()); };

        for (auto input : midiInputs)
        {
            if (deviceManager.isMidiInputDeviceEnabled(input.identifier))
            {
                setMidiInput(midiInputs.indexOf(input));
                break;
            }
        }

        if (midiInputList.getSelectedId() == 0)
            setMidiInput(0);

        addAndMakeVisible(keyboardComponent);
        keyboardState.addListener(this);

        addAndMakeVisible(midiMessagesBox);
        midiMessagesBox.setMultiLine(true);
        midiMessagesBox.setReturnKeyStartsNewLine(true);
        midiMessagesBox.setReadOnly(true);
        midiMessagesBox.setScrollbarsShown(true);
        midiMessagesBox.setCaretVisible(false);
        midiMessagesBox.setPopupMenuEnabled(true);
        midiMessagesBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0x32ffffff));
        midiMessagesBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(0x1c000000));
        midiMessagesBox.setColour(juce::TextEditor::shadowColourId, juce::Colour(0x16000000));

        addAndMakeVisible(UI);
    }

    ~MainContentComponent() override
    {
        shutdownAudio();
    }

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::cadetblue);
    }

    void resized() override
    {
        auto area = getLocalBounds();

        midiInputList.setBounds(area.removeFromTop(36).removeFromRight(getWidth() - 80).reduced(8));
        keyboardComponent.setBounds(area.removeFromBottom(110).reduced(8));
        midiMessagesBox.setBounds(area.removeFromRight(getWidth() - 400).reduced(8));
        UI.setBounds(area.removeFromLeft(400).reduced(8));
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
    {
        synthAudioSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override
    {
        synthAudioSource.getNextAudioBlock(bufferToFill);
    }

    void releaseResources() override
    {
        synthAudioSource.releaseResources();
    }

private:
    void timerCallback(int timerID) override
    {
        switch (timerID)
        {
        case 0:
            keyboardComponent.grabKeyboardFocus();
            stopTimer(0);
            break;
        case 1:
            UI.setEnabled(true);
            UI.nextQuiz();
            stopTimer(1);
            mistakes = 0;
            count = 0;
            break;
        }
    }

    void setMidiInput(int index)
    {
        auto list = juce::MidiInput::getAvailableDevices();

        deviceManager.removeMidiInputDeviceCallback(list[lastInputIndex].identifier,
                                                    synthAudioSource.getMidiCollector());

        auto newInput = list[index];

        if (!deviceManager.isMidiInputDeviceEnabled(newInput.identifier))
            deviceManager.setMidiInputDeviceEnabled(newInput.identifier, true);

        deviceManager.addMidiInputDeviceCallback(newInput.identifier, synthAudioSource.getMidiCollector());
        midiInputList.setSelectedId(index + 1, juce::dontSendNotification);

        lastInputIndex = index;
    }
    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override
    {
    }

    void handleNoteOn(juce::MidiKeyboardState *, int midiChannel, int midiNoteNumber, float velocity) override
    {
        if (!isAddingFromMidiInput && UI.buttonflag == false && UI.answerflag == true && UI.quiz[count])
        {
            auto m = juce::MidiMessage::noteOn(midiChannel, midiNoteNumber, velocity);
            m.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
            const juce::MessageManagerLock messageManagerLock;
            if (previousNoteNumber == midiNoteNumber)
            {
                return;
            }
            if ((midiNoteNumber % 12 != UI.quiz[count] % 12) || (count ? (midiNoteNumber - previousNoteNumber != UI.quiz[count] - UI.quiz[count - 1]) : 0))
            {
                midiMessagesBox.setColour(juce::TextEditor::textColourId, juce::Colours::orangered);
            }
            previousNoteNumber = midiNoteNumber;
            count++;
            postMessageToList(m, juce::MidiMessage::getMidiNoteName(m.getNoteNumber(), true, true, 3));
        }
    }

    void handleNoteOff(juce::MidiKeyboardState *, int midiChannel, int midiNoteNumber, float) override
    {
    }
    class IncomingMessageCallback : public juce::CallbackMessage
    {
    public:
        IncomingMessageCallback(MainContentComponent *o, const juce::MidiMessage &m, const juce::String &s)
            : owner(o), message(m), source(s)
        {
        }

        void messageCallback() override
        {
            if (owner != nullptr)
                owner->addMessageToList(message, source);
        }

        Component::SafePointer<MainContentComponent> owner;
        juce::MidiMessage message;
        juce::String source;
    };

    void postMessageToList(const juce::MidiMessage &message, const juce::String &source)
    {
        (new IncomingMessageCallback(this, message, source))->post();
    }

    void addMessageToList(const juce::MidiMessage &message, const juce::String &source)
    {
        juce::String displayText;
        UI.quiz[count] ? displayText = source + juce::String::fromUTF8(u8"->") : displayText = source;
        midiMessagesBox.moveCaretToEnd();
        midiMessagesBox.insertTextAtCaret(displayText);
        if (!UI.quiz[count] && midiMessagesBox.findColour(midiMessagesBox.textColourId) == juce::Colours::white)
        {
            midiMessagesBox.setColour(juce::TextEditor::textColourId, juce::Colours::yellow);
            displayText = juce::String::fromUTF8(u8" Correct! (mistakes: ") + juce::String::String(mistakes) + juce::String::fromUTF8(u8")");
            midiMessagesBox.moveCaretToEnd();
            midiMessagesBox.insertTextAtCaret(displayText + juce::newLine);
            midiMessagesBox.setColour(juce::TextEditor::textColourId, juce::Colours::white);
            const juce::MessageManagerLock messageManagerLock;
            UI.setEnabled(false);
            startTimer(1, 1000);
        }
        else if (midiMessagesBox.findColour(midiMessagesBox.textColourId) == juce::Colours::orangered)
        {
            midiMessagesBox.insertTextAtCaret(juce::newLine);
            midiMessagesBox.setColour(juce::TextEditor::textColourId, juce::Colours::white);
            mistakes++;
            count = 0;
        }
    }

    juce::MidiKeyboardState keyboardState;
    SynthAudioSource synthAudioSource;
    juce::MidiKeyboardComponent keyboardComponent;
    juce::ComboBox midiInputList;
    juce::Label midiInputListLabel;
    int lastInputIndex = 0;
    int count = 0;
    int mistakes = 0;
    int previousNoteNumber = 0;
    juce::AudioDeviceManager deviceManager;
    bool isAddingFromMidiInput = false;
    juce::TextEditor midiMessagesBox;
    double startTime;
    UserInterface UI;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContentComponent)
};
