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


TransientVisualizer::TransientVisualizer(const InstanceInfo& info) : Plugin(info, MakeConfig(kNumParams, kNumPresets)), memorySmoothing(new FifoMemory(6000, -1)), memoryPeak(new FifoMemory(6000, -1)) {
  GetParam(kZoom)->InitDouble("Zoom", 25, 0, 100.0, 1, "%");
  GetParam(kSmooth)->InitDouble("Smooth", 0, 0, 100.0, 1, "%");
  GetParam(kModeScrolling)->InitEnum("Scrolling mode", 0, 2, "", IParam::kFlagsNone, "", "Stop on silence", "Never stop");
  this->setting = new UiSetting();

  //#if IPLUG_EDITOR // http://bit.ly/2S64BDd
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

    pGraphics->AttachControl(new ITextControl(upperPart.GetMidVPadded(50), "Hello Mouth!", IText(50)));
    pGraphics->AttachControl(new ITextControl(upperPart.GetFromBottom(30), "v1.13", IText(20)));
    pGraphics->AttachControl(new IVKnobControl(upperPart.GetFromRight(120), kZoom));
    pGraphics->AttachControl(new IVKnobControl(upperPart.GetReducedFromRight(120).GetFromRight(120), kSmooth));
    pGraphics->AttachControl(new IVSwitchControl(upperPart.GetFromLeft(120), kModeScrolling, "Scrolling mode", DEFAULT_STYLE), 3);

    pGraphics->AttachControl(new GraphVisualizer(lowerPart, setting, this->memorySmoothing, this->memoryPeak));
  };
  //#endif
}

//#if IPLUG_DSP
void TransientVisualizer::ProcessBlock(sample** inputs, sample** outputs, int nFrames) {
  this->setting->gain = GetParam(kZoom)->Value() / 100.;
  this->setting->smooth = GetParam(kSmooth)->Value() / 100;
  const int nChans = NOutChansConnected();

  for (int sampleIndex = 0; sampleIndex < nFrames; sampleIndex++) {
    auto left = inputs[0][sampleIndex];
    auto right = inputs[1][sampleIndex];
    auto level = (abs(left) + abs(right)) / 2;

    /*double sidechainLeft;
    double sidechainRight;

    if (inputs[2] && inputs[3]) {
      sidechainLeft = inputs[2][sampleIndex];
      sidechainRight = inputs[3][sampleIndex];
    }*/
    /*
    for (int channelIndex = 0; channelIndex < nChans; channelIndex++) {
      outputs[channelIndex][sampleIndex] = inputs[channelIndex][sampleIndex];
    }*/

    bool autoScroll = GetParam(kModeScrolling)->Value() == 1;
    if (autoScroll && level < MINIMUM_VOLUME) {
      level = 0;
    }

    measurePeak1.learnNewLevel(level, !autoScroll);
    measurePeak2.learnNewLevel(level, !autoScroll);
    measurePeak3.learnNewLevel(level, !autoScroll);
    measurePeak4.learnNewLevel(level, !autoScroll);
    measurePeak5.learnNewLevel(level, !autoScroll);

    if (measurePeak1.count >= GetSampleRate() / 200) {
      const double smoothingValue = (measurePeak2.max + measurePeak3.max + measurePeak4.max + measurePeak5.max) / 4;
      measureRatioPeakSmoothing.learnNewLevel(max(smoothingValue, measurePeak1.max) / smoothingValue);

      double inputVolumeRmsDb = convertRmsToDb(measureRatioPeakSmoothing.average * smoothingValue);
      memorySmoothing->addValue(inputVolumeRmsDb / MINIMUM_VOLUME_DB * setting->visualizerHeight);

      double inputVolumePeakDb = convertRmsToDb(measurePeak1.max);
      memoryPeak->addValue(inputVolumePeakDb / MINIMUM_VOLUME_DB * setting->visualizerHeight);

      if (measurePeak2.count >= GetSampleRate() / 10) {
        measurePeak2.resetSample();
        measurePeak2.learnNewLevel(measurePeak1.max);
      }
      if (measurePeak3.count >= GetSampleRate() / 20) {
        measurePeak3.resetSample();
        measurePeak3.learnNewLevel(measurePeak1.max);
      }
      if (measurePeak4.count >= GetSampleRate() / 40) {
        measurePeak4.resetSample();
        measurePeak4.learnNewLevel(measurePeak1.max);
      }
      if (measurePeak5.count >= GetSampleRate() / 80) {
        measurePeak5.resetSample();
        measurePeak5.learnNewLevel(measurePeak1.max);
      }

      measurePeak1.resetSample();
    }
    
    if (measureRatioPeakSmoothing.count >= GetSampleRate() / 10) {
      const double oldRatioAverage = measureRatioPeakSmoothing.average;
      measureRatioPeakSmoothing.resetSample();
      measureRatioPeakSmoothing.learnNewLevel(oldRatioAverage);
    }
  }
}
//#endif
