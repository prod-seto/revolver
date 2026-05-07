#include "AudioEngine.h"
#include <cstring>
#include <iostream>

AudioEngine::AudioEngine() {
    formatManager.registerBasicFormats();
}

AudioEngine::~AudioEngine() {
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
}

bool AudioEngine::initialise() {
    juce::String err = deviceManager.initialiseWithDefaultDevices(0, 2);
    if (err.isNotEmpty()) {
        std::cerr << "[audio] Failed to open audio device: " << err << "\n";
        return false;
    }
    deviceManager.addAudioCallback(this);
    std::cout << "[audio] Audio device opened\n";
    return true;
}

bool AudioEngine::loadSample(const juce::File& file) {
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));
    if (!reader) {
        std::cerr << "[audio] Could not read: " << file.getFullPathName() << "\n";
        return false;
    }

    auto buf = std::make_unique<juce::AudioBuffer<float>>(
        (int)reader->numChannels, (int)reader->lengthInSamples);
    reader->read(buf.get(), 0, (int)reader->lengthInSamples, 0, true, true);
    sampleBuffer = std::move(buf);
    loaded.store(true);

    std::cout << "[audio] Sample loaded: " << file.getFileName()
              << "  (" << reader->numChannels << "ch, "
              << reader->sampleRate << "Hz, "
              << reader->lengthInSamples << " frames)\n";
    return true;
}

void AudioEngine::noteOn(int /*note*/, float velocity) {
    gain = juce::jlimit(0.0f, 1.0f, velocity);
    readPos.store(0);
    playing.store(true);
}

void AudioEngine::noteOff(int /*note*/) {
    playing.store(false);
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device) {
    sampleRate = device->getCurrentSampleRate();
}

void AudioEngine::audioDeviceStopped() {}

void AudioEngine::audioDeviceIOCallbackWithContext(
    const float* const* /*inputData*/, int /*numInputs*/,
    float* const* outputData, int numOutputs,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& /*ctx*/)
{
    // Zero output
    for (int ch = 0; ch < numOutputs; ++ch)
        if (outputData[ch])
            std::memset(outputData[ch], 0, (size_t)numSamples * sizeof(float));

    if (!playing.load() || !loaded.load() || !sampleBuffer) return;

    const int totalSamples = sampleBuffer->getNumSamples();
    const int numChannels  = sampleBuffer->getNumChannels();
    const int pos          = readPos.load();
    const int toCopy       = std::min(numSamples, totalSamples - pos);

    // Build input and output buffers for the process callback
    juce::AudioBuffer<float> inBuf(numChannels, toCopy);
    for (int ch = 0; ch < numChannels; ++ch)
        inBuf.copyFrom(ch, 0, *sampleBuffer, ch, pos, toCopy);

    juce::AudioBuffer<float> outBuf(numOutputs, toCopy);
    outBuf.clear();

    if (processCallback) {
        // Plugin's DSP processes the sample (effect or instrument mode)
        processCallback(inBuf, outBuf, toCopy, sampleRate);
    } else {
        // Passthrough: copy sample directly to output
        for (int ch = 0; ch < numOutputs; ++ch)
            outBuf.copyFrom(ch, 0, inBuf, std::min(ch, numChannels - 1), 0, toCopy);
    }

    // Write to device output
    for (int ch = 0; ch < numOutputs; ++ch) {
        if (!outputData[ch]) continue;
        const float* src = outBuf.getReadPointer(ch);
        for (int i = 0; i < toCopy; ++i)
            outputData[ch][i] += src[i] * gain;
    }

    const int newPos = pos + toCopy;
    if (newPos >= totalSamples)
        playing.store(false);
    readPos.store(newPos >= totalSamples ? 0 : newPos);
}
