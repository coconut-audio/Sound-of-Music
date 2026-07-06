#pragma once

#include <JuceHeader.h>
#include "Processor.h"
#include "ui/SpectrumAnalyser.h"
#include "ui/LookAndFeel.h"
#include "common/Colours.h"

class Editor final : public AudioProcessorEditor, public Timer, public Slider::Listener
{
public:
    explicit Editor(Processor&);
    ~Editor() override;

    void paint(Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;
    void mouseUp(const MouseEvent& event) override;
    void mouseDoubleClick(const MouseEvent& event) override;
    void sliderValueChanged(Slider* slider) override;
    void sliderDragStarted(Slider*) override;
    void sliderDragEnded(Slider* slider) override;

private:
    Processor& processorRef;

    Slider* currentDraggingSlider = nullptr;

    Typeface::Ptr boldFont = Typeface::createSystemTypefaceFor(
        BinaryData::SpaceMonoBold_ttf, BinaryData::SpaceMonoBold_ttfSize);

    static constexpr int interpolatedSize = 16000;
    dsp::FFT forwardFFT;
    dsp::WindowingFunction<float> windowFunction;
    LagrangeInterpolator dryInterpolator;
    LagrangeInterpolator wetInterpolator;
    float dryInterpolatedData[interpolatedSize] = {};
    float wetInterpolatedData[interpolatedSize] = {};

    Slider crushSlider, downsamplingSlider, jitterSlider, clipSlider, widthSlider, postFilterSlider;
    SliderLookAndFeel lookAndFeel;

    Label crushName, downsamplingName, jitterName, clipName, widthName, postFilterName;
    Label crushIndicator, downsamplingIndicator, jitterIndicator, clipIndicator, widthIndicator, postFilterIndicator;

    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> crushAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> downsamplingAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> jitterAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> clipAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> widthAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> postFilterAttachment;

    SpectrumAnalyser spectrumAnalyser;

    std::vector<float> dividerFrequencies;
    int selectedBandIndex = 0;
    int draggedDividerIndex = -1;

    int findNearestDivider(float mouseX) const;
    int findBandAtPosition(float mouseX) const;
    void updateBandAttachments();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};
