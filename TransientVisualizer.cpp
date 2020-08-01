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


TransientVisualizer::TransientVisualizer(const InstanceInfo& info) : Plugin(info, MakeConfig(kNumParams, kNumPresets)), memoryRms(new FifoMemory(6000, -1)), memoryPeak(new FifoMemory(6000, -1)) {
  GetParam(kGain)->InitDouble("Zoom", 50, 0, 100.0, 1, "%");
  GetParam(kSmooth)->InitDouble("Smooth", 0, 0, 100.0, 1, "%");
  GetParam(kModePeakRms)->InitEnum("Mode", 1, 3, "", IParam::kFlagsNone, "", "RMS", "PEAK", "RMS + PEAK");
  this->setting = new UiSetting();

  //#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, 1000, 500, PLUG_FPS, GetScaleForScreen(PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->AttachPanelBackground(COLOR_GRAY);

    const IRECT b = pGraphics->GetBounds();
    auto upperPart = IRECT(b.L, b.T, b.R, b.H() / 4);
    auto lowerPart = IRECT(b.L, b.T + b.H() / 4, b.R, b.B);

    pGraphics->AttachControl(new ITextControl(upperPart.GetMidVPadded(50), "Hello Mouth!", IText(50)));
    pGraphics->AttachControl(new ITextControl(upperPart.GetFromBottom(30), "v1.6", IText(20)));
    pGraphics->AttachControl(new IVKnobControl(upperPart.GetFromRight(120), kGain));
    pGraphics->AttachControl(new IVKnobControl(upperPart.GetReducedFromRight(120).GetFromRight(120), kSmooth));
    pGraphics->AttachControl(new IVSwitchControl(upperPart.GetFromLeft(120), kModePeakRms, "Detection mode", DEFAULT_STYLE), 2);

    pGraphics->AttachControl(new GraphVisualizer(lowerPart, setting, this->memoryRms, this->memoryPeak));
  };
  //#endif
}

//#if IPLUG_DSP
void TransientVisualizer::ProcessBlock(sample** inputs, sample** outputs, int nFrames) {
  this->setting->gain = GetParam(kGain)->Value() / 100.;
  this->setting->smooth = GetParam(kSmooth)->Value() / 100;
  this->setting->modeRmsPeak = GetParam(kModePeakRms)->Value();
  const int nChans = NOutChansConnected();

  for (int sampleIndex = 0; sampleIndex < nFrames; sampleIndex++) {
    auto left = inputs[0][sampleIndex];
    auto right = inputs[1][sampleIndex];
    auto level = (left + right) / 2; //random();
    /*
    for (int channelIndex = 0; channelIndex < nChans; channelIndex++) {
      outputs[channelIndex][sampleIndex] = inputs[channelIndex][sampleIndex];
    }*/
    measure.learnNewLevel(level);
    peakMeasure.learnNewLevel(level);

    if (peakMeasure.count >= GetSampleRate() / 1000) {
      double inputVolumeRmsDb = convertRmsToDb(measure.average);
      memoryRms->addValue(-inputVolumeRmsDb / 55 * setting->visualizerHeight);

      double inputVolumePeakDb = convertRmsToDb(peakMeasure.max);
      memoryPeak->addValue(-inputVolumePeakDb / 55 * setting->visualizerHeight);

      peakMeasure.resetSample();

      if (measure.count >= 1 + GetSampleRate() / 20) {
        const double oldAverage = measure.average;
        measure.resetSample();
        measure.learnNewLevel(oldAverage); //allow to not start on a immediate volume change
      }
    }
  }
}
//#endif
