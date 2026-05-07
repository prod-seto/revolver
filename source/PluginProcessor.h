#pragma once
#include <JuceHeader.h>
#include <functional>

class WebPlugProcessor : public juce::AudioProcessor {
public:
    WebPlugProcessor();
    ~WebPlugProcessor() override = default;

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "WebPlug"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    // Called by the editor when a message arrives from the UI.
    void handleMessageFromUI(const juce::String& json);

    // Set by the editor to route C++ → UI messages. Cleared on editor close.
    std::function<void(const juce::String&)> onMessageToUI;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebPlugProcessor)
};
