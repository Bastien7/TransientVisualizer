#include <iostream>
#include <string>
using namespace std;


#include "TransientVisualizer.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "GraphVisualizer.h"
#include "UiSetting.h"
#include "Measure.h"
#include "Converters.h"
#include "RandomGenerator.h"
using namespace converters;

RandomGenerator random{ 0,.01 };

const int GRAPH_HEIGHT = 435; //px
const int MAX_MEMORY_SIZE = 6000; //should it be set higher? (in relation to GraphVisualizer constant defined max memory size)

//ITextControl* text1 = new ITextControl(IRECT(200, 0, 300, 200), "test1", IText(20));
//ITextControl* text2 = new ITextControl(IRECT(200, 30, 300, 230), "test2", IText(30));
bool connected = true;
int inputGeneratorCounter = 0;
double inputGeneratorValue = .01;


TransientVisualizer::TransientVisualizer(const InstanceInfo& info) : Plugin(info, MakeConfig(kNumParams, kNumPresets)),
    loudnessRmsInput(Loudness(GetSampleRate())),
    loudnessRmsSidechain(Loudness(GetSampleRate())),
    memoryInputSmoothing(new FifoMemory(MAX_MEMORY_SIZE, -1)),
    memoryInputPeak(new FifoMemory(MAX_MEMORY_SIZE, -1)),
    //measureInputPeakSmoothingRatio(MeasureAverageContinuous(GetSampleRate(), 10, 1)),
    memorySidechainPeak(new FifoMemory(MAX_MEMORY_SIZE, -1)),
    memorySidechainSmoothing(new FifoMemory(MAX_MEMORY_SIZE, -1)),
    //measureSidechainPeakSmoothingRatio(MeasureAverageContinuous(GetSampleRate(), 10, 1)),
    memoryInputRms(new FifoMemory(MAX_MEMORY_SIZE, -1)),
    memorySidechainRms(new FifoMemory(MAX_MEMORY_SIZE, -1)),
    measureInputRms(MeasureGroupAverage({ 20, 10, 5, 3 }, GetSampleRate())), // 50/100/200/333 ms
    measureSidechainRms(MeasureGroupAverage({ 20, 10, 5, 3 }, GetSampleRate())) // 50/100/200/333 ms
{

  GetParam(zoomParameter)->InitDouble("Zoom", 100, 25, 1000, 1, "%", 0, "", IParam::ShapePowCurve(3));
  GetParam(smoothParameter)->InitDouble("Smooth", 40, 0, 100, 1, "%", 0, "", IParam::ShapePowCurve(.5), IParam::kUnitPercentage);
  GetParam(scrollingModeParameter)->InitEnum("Scrolling", 0, 2, "", IParam::kFlagsNone, "", "Stop on silence", "Never stop");
  GetParam(detectionModeParameter)->InitEnum("Detection", 0, 3, "", IParam::kFlagsNone, "", "Peak", "RMS", "Peak & RMS");
  this->setting = new UiSetting();

  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, 950, GRAPH_HEIGHT * 4/3, PLUG_FPS, GetScaleForScreen(PLUG_HEIGHT));
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->AttachPanelBackground(COLOR_GRAY);

    const IRECT b = pGraphics->GetBounds();
    auto upperPart = IRECT(b.L, b.T, b.R, b.H() / 4);
    auto lowerPart = IRECT(b.L, b.T + b.H() / 4, b.R, b.B);

    //pGraphics->AttachControl(new ITextControl(upperPart.GetMidVPadded(50), "Hello Mouth!", IText(50)));
    pGraphics->AttachControl(new ITextControl(upperPart.GetFromBottom(30), "v1.22-release", IText(20)));
    //pGraphics->AttachControl(text1);
    //pGraphics->AttachControl(text2);
    pGraphics->AttachControl(new IVKnobControl(upperPart.GetFromRight(120), zoomParameter));
    pGraphics->AttachControl(new IVKnobControl(upperPart.GetReducedFromRight(120).GetFromRight(120), smoothParameter));
    pGraphics->AttachControl(new IVSwitchControl(upperPart.GetFromLeft(120).GetVPadded(-40).GetVShifted(-35), scrollingModeParameter, "Scrolling", DEFAULT_STYLE));
    pGraphics->AttachControl(new IVSwitchControl(upperPart.GetFromLeft(120).GetVPadded(-40).GetVShifted(35), detectionModeParameter, "Detection", DEFAULT_STYLE));

    pGraphics->AttachControl(new GraphVisualizer(lowerPart, setting, memoryInputPeak, memoryInputSmoothing, memorySidechainPeak, memorySidechainSmoothing, memoryInputRms, memorySidechainRms));
  };
}

void TransientVisualizer::ProcessBlock(sample** inputs, sample** outputs, int nFrames) {
  this->setting->zoom = GetParam(zoomParameter)->Value() / 100.;
  this->setting->smooth = round(GetParam(smoothParameter)->Value()) / 100;
  this->setting->detectionMode = GetParam(detectionModeParameter)->Value();
  const int nChans = NOutChansConnected();
  const int sampleRate = GetSampleRate();
  const float proportionDbScreenHeight = 1 / MINIMUM_VOLUME_DB * setting->visualizerHeight;

  const bool autoScroll = GetParam(scrollingModeParameter)->Value() == 1;


  for (int sampleIndex = 0; sampleIndex < nFrames; sampleIndex++) {
    double level = -1;
    double sidechainLevel = -1.0;

    //if (IsChannelConnected(ERoute::kInput, 0) && IsChannelConnected(ERoute::kInput, 1)) { //TODO see how to optimize this to not call it each time
      double left = inputs[0][sampleIndex];
      double right = inputs[1][sampleIndex];
      //level = (abs(left) + abs(right)) / 2;
      level = abs(left/2); //workaround, waiting for true sidechain channels. Divide by 2 because left and right are on the same input channel.
      sidechainLevel = abs(right/2);
    //}

      double inputLoudnessLevel = abs(loudnessRmsInput.applyLoudnessWeighting(left/2));
      double sidechainLoudnessLevel = abs(loudnessRmsSidechain.applyLoudnessWeighting(right/2));

    for (int channelIndex = 0; channelIndex < nChans; channelIndex++) {
      outputs[channelIndex][sampleIndex] = inputs[channelIndex][sampleIndex];
    }


    measureInputPeak.learnNewLevel(level, !autoScroll);
    measureInputSmooth.learnNewLevel(level, !autoScroll);

    measureSidechainPeak.learnNewLevel(sidechainLevel, !autoScroll);
    measureSidechainSmooth.learnNewLevel(sidechainLevel, !autoScroll);

    measureInputRms.learnNewLevel(inputLoudnessLevel, !autoScroll);
    measureSidechainRms.learnNewLevel(sidechainLoudnessLevel, !autoScroll);


    if (measureInputPeak.needReset(sampleRate / 200)) {
      //PEAK
      double inputVolumePeakDb = convertRmsToDb(measureInputPeak.getResult());

      if (inputVolumePeakDb < -6000) { //protection
        if (measureInputPeak.getCount() > 0) { //only reset if needed
          measureInputPeak.resetSample();
          measureInputSmooth.resetSample();
          measureSidechainPeak.resetSample();
          measureSidechainSmooth.resetSample();
        }

        memoryInputPeak->addValue(setting->visualizerHeight);
        memoryInputSmoothing->addValue(setting->visualizerHeight);
        memorySidechainPeak->addValue(setting->visualizerHeight);
        memorySidechainSmoothing->addValue(setting->visualizerHeight);
        memoryInputRms->addValue(setting->visualizerHeight);
        memorySidechainRms->addValue(setting->visualizerHeight);

        return;
      }

      //input memory
      memoryInputPeak->addValue(inputVolumePeakDb * proportionDbScreenHeight);

      double inputSmoothingValue = measureInputSmooth.getResult();
      if (inputSmoothingValue == 0) { inputSmoothingValue = 1; }
      //measureInputPeakSmoothingRatio.learnNewLevel(max(inputSmoothingValue, measureInputPeak.getResult()) / inputSmoothingValue);

      double inputVolumeSmoothingDb = convertRmsToDb(/*measureInputPeakSmoothingRatio.getResult() **/ inputSmoothingValue);
      memoryInputSmoothing->addValue(inputVolumeSmoothingDb * proportionDbScreenHeight);

      measureInputSmooth.resetSampleIfNeeded(sampleRate, measureInputPeak.getResult());
      measureInputPeak.resetSample();


      //sidechain memory
      double sidechainVolumePeakDb = convertRmsToDb(measureSidechainPeak.getResult());
      memorySidechainPeak->addValue(sidechainVolumePeakDb * proportionDbScreenHeight);

      double sidechainSmoothingValue = measureSidechainSmooth.getResult();
      if (sidechainSmoothingValue == 0) { sidechainSmoothingValue = 1; }
      //measureSidechainPeakSmoothingRatio.learnNewLevel(max(sidechainSmoothingValue, measureSidechainPeak.getResult()) / sidechainSmoothingValue);

      double sidechainVolumeSmoothingDb = convertRmsToDb(/*measureSidechainPeakSmoothingRatio.getResult() **/ sidechainSmoothingValue);
      memorySidechainSmoothing->addValue(sidechainVolumeSmoothingDb * proportionDbScreenHeight);

      measureSidechainSmooth.resetSampleIfNeeded(sampleRate, measureSidechainPeak.getResult());
      measureSidechainPeak.resetSample();


      //RMS
      double inputVolumeRmsDb = convertRmsToDb(measureInputRms.getResult());
      double sidechainVolumeRmsDb = convertRmsToDb(measureSidechainRms.getResult());

      memoryInputRms->addValue(inputVolumeRmsDb * proportionDbScreenHeight);
      memorySidechainRms->addValue(sidechainVolumeRmsDb * proportionDbScreenHeight);
    }
  }
}
