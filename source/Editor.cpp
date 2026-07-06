#include "Editor.h"
#include "common/Colours.h"
#include "common/Parameters.h"

using namespace std;

Editor::Editor(Processor& sourceProcessor)
    : AudioProcessorEditor(&sourceProcessor)
    , processorRef(sourceProcessor)
    , forwardFFT(processorRef.fftOrder)
    , windowFunction(1 << processorRef.fftOrder, dsp::WindowingFunction<float>::hann)
{
    setSize(880, 480);
    startTimerHz(30);

    Font labelFont { FontOptions(boldFont) };
    Font indicatorFont { FontOptions(boldFont) };

    auto setupKnob = [&](Slider& slider, Label& nameLabel, Label& indicator,
                         double min, double max, double interval)
    {
        slider.setSliderStyle(Slider::RotaryVerticalDrag);
        slider.setRange(min, max, interval);
        slider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel(&lookAndFeel);
        slider.addListener(this);
        addAndMakeVisible(slider);

        nameLabel.setJustificationType(Justification::centred);
        nameLabel.setColour(Label::textColourId, Theme::text);
        nameLabel.setFont(labelFont.withHeight(18));
        addAndMakeVisible(nameLabel);

        indicator.setJustificationType(Justification::centred);
        indicator.setColour(Label::textColourId, Theme::accent);
        indicator.setFont(indicatorFont.withHeight(26));
        indicator.setVisible(false);
        addAndMakeVisible(indicator);
    };

    setupKnob(crushSlider, crushName, crushIndicator, 2, 32, 1);
    crushName.setText("Quantization", dontSendNotification);
    setupKnob(downsamplingSlider, downsamplingName, downsamplingIndicator, 440, 44100, 1);
    downsamplingName.setText("Downsampling", dontSendNotification);
    setupKnob(jitterSlider, jitterName, jitterIndicator, 0, 100, 1);
    jitterName.setText("Error", dontSendNotification);
    setupKnob(clipSlider, clipName, clipIndicator, -15, 0, 0.1);
    clipName.setText("Clipping", dontSendNotification);
    setupKnob(widthSlider, widthName, widthIndicator, 0, 200, 1);
    widthName.setText("Width", dontSendNotification);
    setupKnob(postFilterSlider, postFilterName, postFilterIndicator, 0, 100, 1);
    postFilterName.setText("Post Filter", dontSendNotification);

    spectrumAnalyser.addMouseListener(this, false);
    addAndMakeVisible(spectrumAnalyser);

    dividerFrequencies = processorRef.getDividerFrequencies();
    updateBandAttachments();
}

Editor::~Editor()
{
    stopTimer();
}

int Editor::findNearestDivider(float mouseX) const
{
    int nearestDividerIndex = -1;
    float nearestDistance = numeric_limits<float>::max();

    for (size_t i = 0; i < dividerFrequencies.size(); i++)
    {
        float distance = std::abs(mouseX - spectrumAnalyser.frequencyToX(dividerFrequencies[i]));
        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            nearestDividerIndex = static_cast<int>(i);
        }
    }

    return (nearestDividerIndex >= 0 && nearestDistance < 8.0f) ? nearestDividerIndex : -1;
}

int Editor::findBandAtPosition(float mouseX) const
{
    int numberOfBands = static_cast<int>(dividerFrequencies.size()) + 1;

    for (int band = 0; band < numberOfBands; band++)
    {
        float leftEdge = (band == 0) ? 0.0f
                                     : spectrumAnalyser.frequencyToX(dividerFrequencies[static_cast<size_t>(band - 1)]);
        float rightEdge = (band == numberOfBands - 1) ? static_cast<float>(spectrumAnalyser.getWidth())
                                                       : spectrumAnalyser.frequencyToX(dividerFrequencies[static_cast<size_t>(band)]);
        if (mouseX >= leftEdge && mouseX <= rightEdge)
            return band;
    }
    return 0;
}

void Editor::updateBandAttachments()
{
    crushAttachment.reset();
    downsamplingAttachment.reset();
    jitterAttachment.reset();
    clipAttachment.reset();
    widthAttachment.reset();
    postFilterAttachment.reset();

    int numberOfBands = static_cast<int>(dividerFrequencies.size()) + 1;
    selectedBandIndex = jlimit(0, numberOfBands - 1, selectedBandIndex);

    crushAttachment = make_unique<AudioProcessorValueTreeState::SliderAttachment>(processorRef.getApvts(), ParamIDs::crush(selectedBandIndex), crushSlider);
    downsamplingAttachment = make_unique<AudioProcessorValueTreeState::SliderAttachment>(processorRef.getApvts(), ParamIDs::downsample(selectedBandIndex), downsamplingSlider);
    jitterAttachment = make_unique<AudioProcessorValueTreeState::SliderAttachment>(processorRef.getApvts(), ParamIDs::jitter(selectedBandIndex), jitterSlider);
    clipAttachment = make_unique<AudioProcessorValueTreeState::SliderAttachment>(processorRef.getApvts(), ParamIDs::clip(selectedBandIndex), clipSlider);
    widthAttachment = make_unique<AudioProcessorValueTreeState::SliderAttachment>(processorRef.getApvts(), ParamIDs::width(selectedBandIndex), widthSlider);
    postFilterAttachment = make_unique<AudioProcessorValueTreeState::SliderAttachment>(processorRef.getApvts(), ParamIDs::postFilter(selectedBandIndex), postFilterSlider);
}

void Editor::paint(Graphics& g)
{
    g.fillAll(Theme::background);
    g.setColour(Theme::surface);
    g.fillRoundedRectangle(20, 280, 840, 180, 6.0f);
}

void Editor::resized()
{
    spectrumAnalyser.setBounds(20, 20, 840, 240);

    int bandBoxY = 280;
    int bandSpacing = 840 / 6;
    auto bandKnob = [&](Slider& slider, Label& nameLabel, Label& indicator, int bandIndex)
    {
        int centerX = 20 + bandSpacing * bandIndex + bandSpacing / 2;
        slider.setBounds(centerX - 35, bandBoxY + 35, 70, 70);
        nameLabel.setBounds(centerX - 50, bandBoxY + 120, 100, 20);
        indicator.setBounds(centerX - 50, bandBoxY + 120, 100, 20);
    };
    bandKnob(crushSlider, crushName, crushIndicator, 0);
    bandKnob(downsamplingSlider, downsamplingName, downsamplingIndicator, 1);
    bandKnob(jitterSlider, jitterName, jitterIndicator, 2);
    bandKnob(clipSlider, clipName, clipIndicator, 3);
    bandKnob(widthSlider, widthName, widthIndicator, 4);
    bandKnob(postFilterSlider, postFilterName, postFilterIndicator, 5);
}

void Editor::timerCallback()
{
    if (processorRef.isDryFFTBlockReady())
    {
        windowFunction.multiplyWithWindowingTable(const_cast<float*>(processorRef.getDryFFTData()), processorRef.fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform(const_cast<float*>(processorRef.getDryFFTData()));
    }

    if (processorRef.isWetFFTBlockReady())
    {
        windowFunction.multiplyWithWindowingTable(const_cast<float*>(processorRef.getWetFFTData()), processorRef.fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform(const_cast<float*>(processorRef.getWetFFTData()));
    }

    dryInterpolator.process(static_cast<float>(processorRef.fftSize) / static_cast<float>(interpolatedSize),
                            processorRef.getDryFFTData(), dryInterpolatedData, interpolatedSize);
    wetInterpolator.process(static_cast<float>(processorRef.fftSize) / static_cast<float>(interpolatedSize),
                            processorRef.getWetFFTData(), wetInterpolatedData, interpolatedSize);

    spectrumAnalyser.updateSpectra(dryInterpolatedData, wetInterpolatedData, static_cast<float>(interpolatedSize),
                                   dividerFrequencies, selectedBandIndex);
    processorRef.clearDryFFTReady();
    processorRef.clearWetFFTReady();
}

void Editor::mouseDown(const MouseEvent& event)
{
    if (event.eventComponent != &spectrumAnalyser)
        return;

    float mouseX = static_cast<float>(event.getMouseDownX());

    int nearestDividerIndex = findNearestDivider(mouseX);
    if (nearestDividerIndex >= 0)
    {
        draggedDividerIndex = nearestDividerIndex;
        return;
    }

    float mouseY = static_cast<float>(event.getMouseDownY());
    if (mouseY < 0 || mouseY > static_cast<float>(spectrumAnalyser.getHeight()))
        return;

    int clickedBandIndex = findBandAtPosition(mouseX);
    if (clickedBandIndex >= 0)
    {
        selectedBandIndex = clickedBandIndex;
        updateBandAttachments();
        repaint();
    }
}

void Editor::mouseDrag(const MouseEvent& event)
{
    if (draggedDividerIndex < 0 || draggedDividerIndex >= static_cast<int>(dividerFrequencies.size()))
        return;

    float mouseX = static_cast<float>(event.getPosition().getX());
    float frequency = jlimit(20.0f, 20000.0f, spectrumAnalyser.xToFrequency(mouseX));

    dividerFrequencies[static_cast<size_t>(draggedDividerIndex)] = frequency;
    sort(dividerFrequencies.begin(), dividerFrequencies.end());

    for (size_t i = 0; i < dividerFrequencies.size(); i++)
    {
        if (std::abs(dividerFrequencies[i] - frequency) < 0.01f)
        {
            draggedDividerIndex = static_cast<int>(i);
            break;
        }
    }

    selectedBandIndex = findBandAtPosition(mouseX);
    processorRef.setBandConfiguration(dividerFrequencies);
    updateBandAttachments();
    repaint();
}

void Editor::mouseUp(const MouseEvent&)
{
    draggedDividerIndex = -1;
}

void Editor::mouseDoubleClick(const MouseEvent& event)
{
    if (event.eventComponent != &spectrumAnalyser)
        return;

    float mouseX = static_cast<float>(event.getMouseDownX());
    float mouseY = static_cast<float>(event.getMouseDownY());

    if (mouseY < 0 || mouseY > static_cast<float>(spectrumAnalyser.getHeight()))
        return;

    int nearestDividerIndex = findNearestDivider(mouseX);

    if (nearestDividerIndex >= 0)
    {
        dividerFrequencies.erase(dividerFrequencies.begin() + nearestDividerIndex);
        if (selectedBandIndex >= static_cast<int>(dividerFrequencies.size()) + 1)
            selectedBandIndex = static_cast<int>(dividerFrequencies.size());
    }
    else
    {
        if (static_cast<int>(dividerFrequencies.size()) >= maxBands - 1)
            return;

        float frequency = jlimit(20.0f, 20000.0f, spectrumAnalyser.xToFrequency(mouseX));
        dividerFrequencies.push_back(frequency);
        sort(dividerFrequencies.begin(), dividerFrequencies.end());
        selectedBandIndex = findBandAtPosition(mouseX);
    }

    processorRef.setBandConfiguration(dividerFrequencies);
    updateBandAttachments();
    repaint();
}

void Editor::sliderValueChanged(Slider* slider)
{
    if (slider != currentDraggingSlider)
        return;

    if (slider == &crushSlider)
        crushName.setText(String(static_cast<int>(crushSlider.getValue())) + " bits", dontSendNotification);
    else if (slider == &downsamplingSlider)
    {
        float hz = static_cast<float>(downsamplingSlider.getValue());
        downsamplingName.setText(hz >= 1000.0f ? String(hz / 1000.0f, 1) + " kHz"
                                              : String(jmax(440, static_cast<int>(hz + 0.5f))) + " Hz", dontSendNotification);
    }
    else if (slider == &jitterSlider)
        jitterName.setText(String(static_cast<int>(jitterSlider.getValue())) + " %", dontSendNotification);
    else if (slider == &clipSlider)
        clipName.setText(String(clipSlider.getValue(), 1) + " dB", dontSendNotification);
    else if (slider == &widthSlider)
        widthName.setText(String(static_cast<int>(widthSlider.getValue())) + " %", dontSendNotification);
    else if (slider == &postFilterSlider)
        postFilterName.setText(String(static_cast<int>(postFilterSlider.getValue())) + " %", dontSendNotification);
}

void Editor::sliderDragStarted(Slider* slider)
{
    currentDraggingSlider = slider;
}

void Editor::sliderDragEnded(Slider* slider)
{
    currentDraggingSlider = nullptr;

    if (slider == &crushSlider)          crushName.setText("Quantization", dontSendNotification);
    else if (slider == &downsamplingSlider) downsamplingName.setText("Downsampling", dontSendNotification);
    else if (slider == &jitterSlider)       jitterName.setText("Error", dontSendNotification);
    else if (slider == &clipSlider)         clipName.setText("Clipping", dontSendNotification);
    else if (slider == &widthSlider)        widthName.setText("Width", dontSendNotification);
    else if (slider == &postFilterSlider)   postFilterName.setText("Post Filter", dontSendNotification);
}
