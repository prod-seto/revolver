#include "DevServer.h"
#include <JuceHeader.h>
#include <iostream>
#include <atomic>
#include <csignal>

static std::atomic<bool> running { true };
static void onSignal(int) { running = false; }

int main() {
    signal(SIGINT,  onSignal);
    signal(SIGTERM, onSignal);

    DevServer server;

    server.onMessage = [&server](const juce::String& json) {
        std::cout << "[webplug] recv: " << json << std::endl;

        juce::var msg;
        if (juce::JSON::parse(json, msg).failed()) return;

        const auto type = msg["type"].toString();

        // ping → pong: the scaffold's proof-of-life round trip
        if (type == "ping") {
            juce::DynamicObject* payload = new juce::DynamicObject();
            payload->setProperty("message", "Hello from C++!");
            juce::DynamicObject* pong = new juce::DynamicObject();
            pong->setProperty("type",    "pong");
            pong->setProperty("payload", juce::var(payload));
            server.sendMessage(juce::JSON::toString(juce::var(pong)));
        }

        // uiReady: browser signals it finished loading
        if (type == "uiReady") {
            std::cout << "[webplug] UI ready" << std::endl;
        }
    };

    server.start(9001);
    std::cout << "[webplug] DevServer on ws://localhost:9001\n"
              << "[webplug] Open http://localhost:5173 in your browser\n"
              << "[webplug] Ctrl+C to stop\n";

    while (running)
        juce::Thread::sleep(100);

    server.stop();
    return 0;
}
