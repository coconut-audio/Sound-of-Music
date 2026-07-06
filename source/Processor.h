#pragma once

#include <JuceHeader.h>
#include "common/Parameters.h"
#include "dsp/BitCrusher.h"
#include "dsp/Downsampler.h"
#include "dsp/Jitter.h"
#include "dsp/Clipper.h"
#include "dsp/BandSplitter.h"

class Processor final : public AudioProcessor, public AudioProcessorValueTreeState::Listener
{
public:
    static constexpr int fftOrder = 10;
    static constexpr int fftSize = 1 << fftOrder;

    Processor();
    ~Processor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(AudioBuffer<float>&, MidiBuffer&) override;
    using AudioProcessor::processBlock;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const String getProgramName(int index) override;
    void changeProgramName(int index, const String& newName) override;

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    AudioProcessorValueTreeState& getApvts() { return treeState; }

    bool isDryFFTBlockReady() const { return nextInputFFTBlockReady; }
    bool isWetFFTBlockReady() const { return nextOutputFFTBlockReady; }
    const float* getDryFFTData() const { return inputFFTData; }
    const float* getWetFFTData() const { return outputFFTData; }
    void clearDryFFTReady() { nextInputFFTBlockReady = false; }
    void clearWetFFTReady() { nextOutputFFTBlockReady = false; }

    void parameterChanged(const String& parameterID, float newValue) override;

    void setBandConfiguration(const std::vector<float>& newDividerFrequencies);
    std::vector<float> getDividerFrequencies() const;

private:
    void pushNextDrySample(float sample);
    void pushNextWetSample(float sample);

    AudioProcessorValueTreeState treeState;

    BandSplitter bandSplitter;

    BitCrusher crushers[maxBands];
    Downsampler downsamplers[maxBands];
    Jitter jitters[maxBands];
    Clipper clippers[maxBands];

    double currentSampleRate = 44100.0;

    mutable SpinLock configurationLock;
    std::vector<float> pendingDividerFrequencies;
    std::vector<float> dividerFrequencies;

    float inputFIFO[fftSize] = {};
    float inputFFTData[2 * fftSize] = {};
    int inputFIFOIndex = 0;
    bool nextInputFFTBlockReady = false;

    float outputFIFO[fftSize] = {};
    float outputFFTData[2 * fftSize] = {};
    int outputFIFOIndex = 0;
    bool nextOutputFFTBlockReady = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
};
