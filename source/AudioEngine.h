#pragma once
#include <JuceHeader.h>

// Opens the system default audio output and plays a loaded WAV sample
// on noteOn. Designed for dev-mode testing — instrument plugins trigger
// their own audio generation; effect plugins route the sample through
// their processBlock (hook in via the processCallback).
class AudioEngine : public juce::AudioIODeviceCallback {
public:
    // Called on the audio thread each block. Hook in your plugin's DSP here.
    // Signature: (inputBuffer, outputBuffer, numSamples, sampleRate)
    using ProcessCallback = std::function<void(const juce::AudioBuffer<float>&,
                                               juce::AudioBuffer<float>&,
                                               int, double)>;

    AudioEngine();
    ~AudioEngine() override;

    bool initialise();
    bool loadSample(const juce::File& file);
    bool isSampleLoaded() const noexcept { return loaded.load(); }

    void noteOn(int midiNote, float velocity);
    void noteOff(int midiNote);

    ProcessCallback processCallback;

private:
    void audioDeviceIOCallbackWithContext(
        const float* const* inputData, int numInputs,
        float* const*       outputData, int numOutputs,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager formatManager;

    std::unique_ptr<juce::AudioBuffer<float>> sampleBuffer;
    double            sampleRate { 44100.0 };
    std::atomic<bool> loaded     { false };
    std::atomic<bool> playing    { false };
    std::atomic<int>  readPos    { 0 };
    float             gain       { 1.0f };
};
