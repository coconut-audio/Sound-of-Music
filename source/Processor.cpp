#include "Processor.h"
#include "Editor.h"

using namespace std;

Processor::Processor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", AudioChannelSet::stereo(), true)
        .withOutput("Output", AudioChannelSet::stereo(), true)),
      treeState(*this, nullptr, "PARAMETER", { createParameterLayout() })
{
    treeState.state = ValueTree("savedParams");
}

Processor::~Processor() {}

const String Processor::getName() const { return JucePlugin_Name; }
bool Processor::acceptsMidi() const { return false; }
bool Processor::producesMidi() const { return false; }
bool Processor::isMidiEffect() const { return false; }
double Processor::getTailLengthSeconds() const { return 0.0; }
int Processor::getNumPrograms() { return 1; }
int Processor::getCurrentProgram() { return 0; }
void Processor::setCurrentProgram(int index) { ignoreUnused(index); }
const String Processor::getProgramName(int index) { ignoreUnused(index); return {}; }
void Processor::changeProgramName(int index, const String& newName) { ignoreUnused(index, newName); }

void Processor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate;

    const SpinLock::ScopedLockType lock(configurationLock);
    dividerFrequencies = pendingDividerFrequencies;
    bandSplitter.prepare(sampleRate, dividerFrequencies);
}

void Processor::releaseResources() {}

bool Processor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void Processor::processBlock(AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int numberOfChannels = getTotalNumInputChannels();
    const int numberOfSamples = buffer.getNumSamples();

    for (int channel = numberOfChannels; channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, numberOfSamples);

    {
        const SpinLock::ScopedTryLockType tryLock(configurationLock);
        if (tryLock.isLocked() && pendingDividerFrequencies != dividerFrequencies)
        {
            dividerFrequencies = pendingDividerFrequencies;
            if (static_cast<int>(dividerFrequencies.size()) > maxBands - 1)
                dividerFrequencies.resize(maxBands - 1);
            bandSplitter.prepare(currentSampleRate, dividerFrequencies);
        }
    }

    int numberOfBands = jmax(jmin(bandSplitter.getNumberOfBands(), maxBands), 1);

    float* leftChannelData = buffer.getWritePointer(0);
    float* rightChannelData = buffer.getWritePointer(1);

    for (int sample = 0; sample < numberOfSamples; sample++)
        pushNextDrySample((leftChannelData[sample] + rightChannelData[sample]) * 0.5f);

    bool allBandsAtDefault = true;

    for (int band = 0; band < numberOfBands && allBandsAtDefault; band++)
    {
        float crushValue = *treeState.getRawParameterValue(ParamIDs::crush(band));
        float downsampleValue = *treeState.getRawParameterValue(ParamIDs::downsample(band));
        float jitterValue = *treeState.getRawParameterValue(ParamIDs::jitter(band));
        float clipValue = *treeState.getRawParameterValue(ParamIDs::clip(band));
        float widthValue = *treeState.getRawParameterValue(ParamIDs::width(band));

        if (crushValue < 31.0f || downsampleValue < 44000.0f || jitterValue > 0.1f || clipValue < -0.1f || std::abs(widthValue - 100.0f) > 0.1f)
            allBandsAtDefault = false;
    }

    if (allBandsAtDefault)
    {
        for (int sample = 0; sample < numberOfSamples; sample++)
            pushNextWetSample((leftChannelData[sample] + rightChannelData[sample]) * 0.5f);
        return;
    }

    AudioSampleBuffer dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    for (int band = 0; band < numberOfBands; band++)
    {
        crushers[band].prepare(*treeState.getRawParameterValue(ParamIDs::crush(band)));
        downsamplers[band].prepare(currentSampleRate, *treeState.getRawParameterValue(ParamIDs::downsample(band)));
        jitters[band].prepare(*treeState.getRawParameterValue(ParamIDs::jitter(band)));
        clippers[band].prepare(*treeState.getRawParameterValue(ParamIDs::clip(band)));
    }

    vector<AudioSampleBuffer> bands(static_cast<size_t>(numberOfBands));
    for (int band = 0; band < numberOfBands; band++)
        bands[static_cast<size_t>(band)].makeCopyOf(dryBuffer);

    bandSplitter.process(bands[0], bands, numberOfSamples);

    for (int band = 0; band < numberOfBands; band++)
    {
        float widthValue = *treeState.getRawParameterValue(ParamIDs::width(band));
        float postFilterValue = *treeState.getRawParameterValue(ParamIDs::postFilter(band));
        float postFilterAmount = postFilterValue / 100.0f;
        bool bandHasEffects = (crushers[band].getCrushLevel() < 31.0f)
                           || (downsamplers[band].getDecimationStep() > 1)
                           || (jitters[band].isActive())
                           || (clippers[band].isActive())
                           || (std::abs(widthValue - 100.0f) > 0.1f);

        for (int channel = 0; channel < numberOfChannels; ++channel)
        {
            float* bandData = bands[static_cast<size_t>(band)].getWritePointer(channel);

            for (int sample = 0; sample < numberOfSamples; sample++)
            {
                float drySample = bandData[sample];
                float wetSample = crushers[band].process(drySample);
                wetSample = downsamplers[band].process(wetSample);
                wetSample = jitters[band].process(wetSample, drySample);
                wetSample = clippers[band].process(wetSample);
                bandData[sample] = wetSample;
            }
        }

        float widthFactor = widthValue / 100.0f;
        if (numberOfChannels == 2 && std::abs(widthFactor - 1.0f) > 0.01f)
        {
            float* leftBand = bands[static_cast<size_t>(band)].getWritePointer(0);
            float* rightBand = bands[static_cast<size_t>(band)].getWritePointer(1);

            for (int sample = 0; sample < numberOfSamples; sample++)
            {
                float mid = (leftBand[sample] + rightBand[sample]) * 0.5f;
                float side = (leftBand[sample] - rightBand[sample]) * 0.5f;
                leftBand[sample] = mid + side * widthFactor;
                rightBand[sample] = mid - side * widthFactor;
            }
        }

        if (bandHasEffects)
        {
            if (postFilterAmount < 0.999f)
            {
                AudioSampleBuffer unfilteredBand;
                unfilteredBand.makeCopyOf(bands[static_cast<size_t>(band)]);

                bandSplitter.filterBand(bands[static_cast<size_t>(band)], band, numberOfSamples);

                for (int channel = 0; channel < numberOfChannels; ++channel)
                {
                    float* filteredData = bands[static_cast<size_t>(band)].getWritePointer(channel);
                    const float* unfilteredData = unfilteredBand.getReadPointer(channel);

                    for (int sample = 0; sample < numberOfSamples; sample++)
                        filteredData[sample] = unfilteredData[sample] + postFilterAmount * (filteredData[sample] - unfilteredData[sample]);
                }
            }
            else
            {
                bandSplitter.filterBand(bands[static_cast<size_t>(band)], band, numberOfSamples);
            }
        }
    }

    for (int channel = 0; channel < numberOfChannels; ++channel)
    {
        buffer.clear(channel, 0, numberOfSamples);
        for (int band = 0; band < numberOfBands; band++)
            buffer.addFrom(channel, 0, bands[static_cast<size_t>(band)], channel, 0, numberOfSamples);
    }

    for (int sample = 0; sample < numberOfSamples; sample++)
        pushNextWetSample((buffer.getSample(0, sample) + buffer.getSample(1, sample)) * 0.5f);
}

bool Processor::hasEditor() const { return true; }

AudioProcessorEditor* Processor::createEditor()
{
    return new Editor(*this);
}

void Processor::getStateInformation(MemoryBlock& destData)
{
    ValueTree state = treeState.state.getOrCreateChildWithName("dividers", nullptr);
    state.removeAllChildren(nullptr);

    for (size_t i = 0; i < dividerFrequencies.size(); i++)
        state.appendChild(ValueTree("freq").setProperty("value", dividerFrequencies[i], nullptr), nullptr);

    unique_ptr<XmlElement> xml(treeState.state.createXml());
    copyXmlToBinary(*xml, destData);
}

void Processor::setStateInformation(const void* data, int sizeInBytes)
{
    unique_ptr<XmlElement> params(getXmlFromBinary(data, sizeInBytes));

    if (params != nullptr && params->hasTagName(treeState.state.getType()))
    {
        treeState.state = ValueTree::fromXml(*params);

        ValueTree state = treeState.state.getChildWithName("dividers");
        dividerFrequencies.clear();

        if (state.isValid())
        {
            for (int i = 0; i < state.getNumChildren(); i++)
            {
                float value = static_cast<float>(state.getChild(i)["value"]);
                if (find(dividerFrequencies.begin(), dividerFrequencies.end(), value) == dividerFrequencies.end())
                    dividerFrequencies.push_back(value);
            }
        }

        std::sort(dividerFrequencies.begin(), dividerFrequencies.end());
        if (static_cast<int>(dividerFrequencies.size()) > maxBands - 1)
            dividerFrequencies.resize(maxBands - 1);

        {
            const SpinLock::ScopedLockType lock(configurationLock);
            pendingDividerFrequencies = dividerFrequencies;
        }
    }
}

void Processor::setBandConfiguration(const std::vector<float>& newDividerFrequencies)
{
    const SpinLock::ScopedLockType lock(configurationLock);
    pendingDividerFrequencies = newDividerFrequencies;
    if (static_cast<int>(pendingDividerFrequencies.size()) > maxBands - 1)
        pendingDividerFrequencies.resize(maxBands - 1);
}

std::vector<float> Processor::getDividerFrequencies() const
{
    const SpinLock::ScopedLockType lock(configurationLock);
    return dividerFrequencies;
}

void Processor::pushNextDrySample(float sample)
{
    if (inputFIFOIndex == fftSize)
    {
        if (!nextInputFFTBlockReady)
        {
            zeromem(inputFFTData, sizeof(inputFFTData));
            memcpy(inputFFTData, inputFIFO, sizeof(inputFIFO));
            nextInputFFTBlockReady = true;
        }
        inputFIFOIndex = 0;
    }

    inputFIFO[inputFIFOIndex++] = sample;
}

void Processor::pushNextWetSample(float sample)
{
    if (outputFIFOIndex == fftSize)
    {
        if (!nextOutputFFTBlockReady)
        {
            zeromem(outputFFTData, sizeof(outputFFTData));
            memcpy(outputFFTData, outputFIFO, sizeof(outputFIFO));
            nextOutputFFTBlockReady = true;
        }
        outputFIFOIndex = 0;
    }

    outputFIFO[outputFIFOIndex++] = sample;
}

void Processor::parameterChanged(const String&, float) {}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Processor();
}
