/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 6.0.8

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2020 - Raw Material Software Limited.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include <JuceHeader.h>
#include "Nowplaying.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class UserInterface  : public juce::Component,
                       public juce::ComboBox::Listener,
                       public juce::Button::Listener
{
public:
    //==============================================================================
    UserInterface (juce::TextEditor& tex);
    ~UserInterface() override;

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    int quiz[6];
    bool buttonflag;
    bool answerflag;

    void replayCompleted();
    void generateQuiz(int difficulty);
    void nextQuiz();
    int generateRand(int range);
    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;

    // Binary resources:
    static const char* speaker_off_png;
    static const int speaker_off_pngSize;


private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    int previousRand = -1;
    int center = 60;
    juce::TextEditor& messagesBox;
    Speaker_on speaker_on;
    //[/UserVariables]

    //==============================================================================
    std::unique_ptr<juce::ComboBox> juce__comboBox;
    std::unique_ptr<juce::TextButton> juce__textButton;
    std::unique_ptr<juce::TextButton> juce__textButton2;
    std::unique_ptr<juce::TextButton> juce__textButton3;
    std::unique_ptr<juce::ComboBox> juce__comboBox2;
    std::unique_ptr<juce::Label> juce__label;
    std::unique_ptr<juce::Label> juce__label2;
    juce::Image cachedImage_speaker_off_png_1;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UserInterface)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

