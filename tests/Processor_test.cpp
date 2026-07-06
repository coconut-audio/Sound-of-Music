#include <gtest/gtest.h>
#include "Processor.h"

class ProcessorTest : public ::testing::Test
{
protected:
    Processor processor;

    void prepareToPlay(double sampleRate = 44100.0, int blockSize = 512)
    {
        processor.prepareToPlay(sampleRate, blockSize);
    }

    AudioBuffer<float> processAudio(int numSamples = 512, float frequency = 1000.0f, float amplitude = 0.5f)
    {
        AudioBuffer<float> buffer(2, numSamples);
        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < numSamples; ++sample)
                buffer.setSample(channel, sample, amplitude * std::sin(2.0f * MathConstants<float>::pi * frequency * static_cast<float>(sample) / 44100.0f));

        MidiBuffer midi;
        processor.processBlock(buffer, midi);
        return buffer;
    }

    float bufferRms(const AudioBuffer<float>& buffer)
    {
        return (buffer.getRMSLevel(0, 0, buffer.getNumSamples())
              + buffer.getRMSLevel(1, 0, buffer.getNumSamples())) / 2.0f;
    }

    void setParamNormalized(const String& paramId, float normalizedValue)
    {
        processor.getApvts().getParameter(paramId)->setValueNotifyingHost(normalizedValue);
    }
};

TEST_F(ProcessorTest, ProducesNonSilentOutput)
{
    prepareToPlay();
    AudioBuffer<float> result = processAudio();
    EXPECT_GT(bufferRms(result), 0.0f);
}

TEST_F(ProcessorTest, HandlesDifferentSampleRates)
{
    prepareToPlay(44100.0, 512);
    AudioBuffer<float> at44k = processAudio();

    prepareToPlay(48000.0, 512);
    AudioBuffer<float> at48k = processAudio();

    EXPECT_GT(bufferRms(at44k), 0.0f);
    EXPECT_GT(bufferRms(at48k), 0.0f);
}

TEST_F(ProcessorTest, HandlesDifferentBlockSizes)
{
    prepareToPlay(44100.0, 128);
    AudioBuffer<float> smallBlock = processAudio(128);

    prepareToPlay(44100.0, 1024);
    AudioBuffer<float> largeBlock = processAudio(1024);

    EXPECT_GT(bufferRms(smallBlock), 0.0f);
    EXPECT_GT(bufferRms(largeBlock), 0.0f);
}

TEST_F(ProcessorTest, CrushAffectsOutput)
{
    prepareToPlay();
    setParamNormalized(ParamIDs::crush(0), 0.0f);
    AudioBuffer<float> lowCrush = processAudio();

    setParamNormalized(ParamIDs::crush(0), 1.0f);
    AudioBuffer<float> highCrush = processAudio();

    EXPECT_NE(bufferRms(lowCrush), bufferRms(highCrush));
}

TEST_F(ProcessorTest, DownsampleAffectsOutput)
{
    prepareToPlay();
    setParamNormalized(ParamIDs::downsample(0), 0.0f);
    AudioBuffer<float> lowDownsample = processAudio();

    setParamNormalized(ParamIDs::downsample(0), 1.0f);
    AudioBuffer<float> highDownsample = processAudio();

    EXPECT_NE(bufferRms(lowDownsample), bufferRms(highDownsample));
}

TEST_F(ProcessorTest, JitterAffectsOutput)
{
    prepareToPlay();
    setParamNormalized(ParamIDs::jitter(0), 0.0f);
    AudioBuffer<float> noJitter = processAudio();

    setParamNormalized(ParamIDs::jitter(0), 1.0f);
    AudioBuffer<float> fullJitter = processAudio();

    EXPECT_NE(bufferRms(noJitter), bufferRms(fullJitter));
}

TEST_F(ProcessorTest, ClipAffectsOutput)
{
    prepareToPlay();
    setParamNormalized(ParamIDs::clip(0), 0.0f);
    AudioBuffer<float> noClip = processAudio();

    setParamNormalized(ParamIDs::clip(0), 1.0f);
    AudioBuffer<float> fullClip = processAudio();

    EXPECT_NE(bufferRms(noClip), bufferRms(fullClip));
}

TEST_F(ProcessorTest, WidthAffectsOutput)
{
    prepareToPlay();
    setParamNormalized(ParamIDs::crush(0), 0.0f);

    AudioBuffer<float> input(2, 512);
    for (int sample = 0; sample < 512; ++sample)
    {
        float t = static_cast<float>(sample) / 44100.0f;
        input.setSample(0, sample, 0.5f * std::sin(2.0f * MathConstants<float>::pi * 1000.0f * t));
        input.setSample(1, sample, 0.5f * std::sin(2.0f * MathConstants<float>::pi * 1500.0f * t));
    }

    AudioBuffer<float> narrow = input;
    setParamNormalized(ParamIDs::width(0), 0.0f);
    MidiBuffer midi;
    processor.processBlock(narrow, midi);

    AudioBuffer<float> wide = input;
    setParamNormalized(ParamIDs::width(0), 1.0f);
    processor.processBlock(wide, midi);

    EXPECT_NE(bufferRms(narrow), bufferRms(wide));
}

TEST_F(ProcessorTest, FftDataReadyAfterEnoughBlocks)
{
    prepareToPlay();
    for (int block = 0; block < 3; ++block)
        processAudio();

    EXPECT_TRUE(processor.isDryFFTBlockReady());
    EXPECT_TRUE(processor.isWetFFTBlockReady());
}

TEST_F(ProcessorTest, StateSaveAndLoad)
{
    prepareToPlay();
    setParamNormalized(ParamIDs::crush(0), 0.0f);

    MemoryBlock destData;
    processor.getStateInformation(destData);
    EXPECT_GT(destData.getSize(), 0u);

    Processor newProcessor;
    newProcessor.prepareToPlay(44100.0, 512);
    newProcessor.setStateInformation(destData.getData(), static_cast<int>(destData.getSize()));

    auto savedXml = processor.getXmlFromBinary(destData.getData(), static_cast<int>(destData.getSize()));
    ASSERT_NE(savedXml, nullptr);
    EXPECT_TRUE(savedXml->hasTagName("savedParams"));
}

TEST_F(ProcessorTest, ConsecutiveBlocksAreStable)
{
    prepareToPlay();
    for (int block = 0; block < 20; ++block)
    {
        AudioBuffer<float> result = processAudio();
        float rms = bufferRms(result);
        EXPECT_FALSE(std::isnan(rms));
        EXPECT_FALSE(std::isinf(rms));
    }
}

TEST_F(ProcessorTest, DefaultPassthrough)
{
    prepareToPlay();
    AudioBuffer<float> input(2, 512);
    for (int channel = 0; channel < 2; ++channel)
        for (int sample = 0; sample < 512; ++sample)
            input.setSample(channel, sample, 0.5f * std::sin(2.0f * MathConstants<float>::pi * 1000.0f * static_cast<float>(sample) / 44100.0f));

    AudioBuffer<float> output = input;

    MidiBuffer midi;
    processor.processBlock(output, midi);

    for (int sample = 0; sample < 512; ++sample)
        EXPECT_NEAR(input.getSample(0, sample), output.getSample(0, sample), 1e-6f);
}

TEST_F(ProcessorTest, BandConfigurationUpdates)
{
    prepareToPlay();
    std::vector<float> newFreqs = { 200.0f, 2000.0f };
    processor.setBandConfiguration(newFreqs);

    prepareToPlay();
    std::vector<float> returned = processor.getDividerFrequencies();
    EXPECT_EQ(returned.size(), newFreqs.size());
    for (size_t i = 0; i < newFreqs.size(); ++i)
        EXPECT_FLOAT_EQ(returned[i], newFreqs[i]);
}

TEST_F(ProcessorTest, BandConfigurationCapsAtMax)
{
    prepareToPlay();
    std::vector<float> tooManyFreqs = { 100.0f, 200.0f, 300.0f, 400.0f, 500.0f, 600.0f, 700.0f, 800.0f, 900.0f };
    processor.setBandConfiguration(tooManyFreqs);
    prepareToPlay();
    std::vector<float> returned = processor.getDividerFrequencies();
    EXPECT_LE(static_cast<int>(returned.size()), maxBands - 1);
}

TEST_F(ProcessorTest, MultipleBandsProcessCorrectly)
{
    prepareToPlay();
    processor.setBandConfiguration({ 500.0f, 2000.0f, 8000.0f });
    prepareToPlay();

    setParamNormalized(ParamIDs::crush(0), 0.0f);
    setParamNormalized(ParamIDs::crush(1), 0.0f);
    setParamNormalized(ParamIDs::crush(2), 0.0f);
    setParamNormalized(ParamIDs::crush(3), 0.0f);

    AudioBuffer<float> result = processAudio();
    EXPECT_GT(bufferRms(result), 0.0f);
}
