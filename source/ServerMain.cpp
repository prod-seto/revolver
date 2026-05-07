#include "DevServer.h"
#include "AudioEngine.h"
#include <JuceHeader.h>
#include <iostream>
#include <atomic>
#include <csignal>

static std::atomic<bool> running { true };
static void onSignal(int) { running = false; }

int main(int argc, char* argv[]) {
    signal(SIGINT,  onSignal);
    signal(SIGTERM, onSignal);

    AudioEngine audio;
    if (!audio.initialise()) {
        std::cerr << "[webplug] Failed to open audio device\n";
        return 1;
    }

    // Load sample: prefer CLI argument, then fall back to first WAV in samples/
    if (argc > 1) {
        audio.loadSample(juce::File(juce::String(argv[1])));
    } else {
        juce::File samplesDir = juce::File::getCurrentWorkingDirectory()
                                    .getChildFile("samples");
        if (samplesDir.isDirectory()) {
            auto wavs = samplesDir.findChildFiles(
                juce::File::findFiles, false, "*.wav;*.aif;*.aiff");
            if (!wavs.isEmpty())
                audio.loadSample(wavs[0]);
        }
    }

    DevServer server;

    server.onMessage = [&server, &audio](const juce::String& json) {
        std::cout << "[webplug] recv: " << json << "\n";

        juce::var msg;
        if (juce::JSON::parse(json, msg).failed()) return;
        const auto type = msg["type"].toString();

        if (type == "ping") {
            juce::DynamicObject* payload = new juce::DynamicObject();
            payload->setProperty("message", "Hello from C++!");
            juce::DynamicObject* pong = new juce::DynamicObject();
            pong->setProperty("type",    "pong");
            pong->setProperty("payload", juce::var(payload));
            server.sendMessage(juce::JSON::toString(juce::var(pong)));
        }

        if (type == "noteOn") {
            const int   note = (int)  msg["payload"]["note"];
            const float vel  = (float)(double)msg["payload"]["velocity"];
            audio.noteOn(note, vel);
        }

        if (type == "noteOff") {
            const int note = (int)msg["payload"]["note"];
            audio.noteOff(note);
        }

        if (type == "uiReady") {
            juce::DynamicObject* payload = new juce::DynamicObject();
            payload->setProperty("sampleLoaded", audio.isSampleLoaded());
            juce::DynamicObject* resp = new juce::DynamicObject();
            resp->setProperty("type",    "audioStatus");
            resp->setProperty("payload", juce::var(payload));
            server.sendMessage(juce::JSON::toString(juce::var(resp)));
        }
    };

    server.start(9001);

    std::cout << "[webplug] DevServer on ws://localhost:9001\n"
              << "[webplug] Open http://localhost:5173\n"
              << "[webplug] Ctrl+C to stop\n";

    if (!audio.isSampleLoaded())
        std::cout << "[webplug] Tip: drop a WAV into samples/ or pass a path as an argument\n";

    while (running)
        juce::Thread::sleep(100);

    server.stop();
    return 0;
}
