#include "PluginProcessor.h"
#include "PluginEditor.h"

WebPlugProcessor::WebPlugProcessor()
    : AudioProcessor(BusesProperties())
{}

juce::AudioProcessorEditor* WebPlugProcessor::createEditor()
{
    return new WebPlugEditor(*this);
}

void WebPlugProcessor::handleMessageFromUI(const juce::String& json)
{
    juce::var msg;
    if (juce::JSON::parse(json, msg).failed()) return;

    const auto type = msg["type"].toString();

    // ping → pong: proof-of-life round trip (same as DevServer)
    if (type == "ping") {
        if (!onMessageToUI) return;
        juce::DynamicObject* payload = new juce::DynamicObject();
        payload->setProperty("message", "Hello from the C++ plugin!");
        juce::DynamicObject* pong = new juce::DynamicObject();
        pong->setProperty("type",    "pong");
        pong->setProperty("payload", juce::var(payload));
        onMessageToUI(juce::JSON::toString(juce::var(pong)));
        return;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WebPlugProcessor();
}
