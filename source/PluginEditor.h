#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class WebPlugEditor : public juce::AudioProcessorEditor {
public:
    explicit WebPlugEditor(WebPlugProcessor&);
    ~WebPlugEditor() override;
    void resized() override;

    WebPlugProcessor& getWebPlugProcessor() const noexcept {
        return *static_cast<WebPlugProcessor*>(&processor);
    }

private:
    std::unique_ptr<juce::WebBrowserComponent> browser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebPlugEditor)
};
