#pragma once
#include <JuceHeader.h>
#include <functional>

// WebSocket server that runs during browser-based development.
// Listens on ws://localhost:{port}. Supports one client at a time (last wins).
// Uses the same JSON message protocol as the JUCE plugin bridge.
class DevServer : public juce::Thread {
public:
    using MessageHandler = std::function<void(const juce::String& json)>;

    DevServer();
    ~DevServer() override;

    void start(int port = 9001);
    void stop();

    void sendMessage(const juce::String& json);

    MessageHandler onMessage;

private:
    void run() override;
    void handleClient(juce::StreamingSocket& client);
    bool performHandshake(juce::StreamingSocket& client);
    bool sendFrame(juce::StreamingSocket& client, const juce::String& text);
    bool readFrame(juce::StreamingSocket& client, juce::String& outText, bool& outClosed);

    int port { 9001 };
    juce::StreamingSocket serverSocket;
    std::unique_ptr<juce::StreamingSocket> activeClient;
    juce::CriticalSection clientLock;
};
