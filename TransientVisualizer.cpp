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
using namespace converters;

TransientVisualizer::TransientVisualizer(const InstanceInfo& info) : Plugin(info, MakeConfig(kNumParams, kNumPresets)), memory(new FifoMemory(3000, -1)) {
  GetParam(kGain)->InitDouble("Gain", 0, 0., 100.0, 0.01, "%");
  GetParam(kModePeakRms)->InitEnum("Mode", 0, 3, "", IParam::kFlagsNone, "", "RMS", "PEAK", "RMS + PEAK");
  this->setting = new UiSetting();

  //#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->AttachPanelBackground(COLOR_GRAY);

    const IRECT b = pGraphics->GetBounds();
    auto upperPart = IRECT(b.L, b.T, b.R, b.H() / 2);
    auto lowerPart = IRECT(b.L, b.T + (b.H() / 2), b.R, b.B);

    pGraphics->AttachControl(new ITextControl(upperPart.GetMidVPadded(50), "Hello Mouth!", IText(50)));
    pGraphics->AttachControl(new ITextControl(upperPart.GetFromTRHC(30, 30), "v1.2", IText(20)));
    pGraphics->AttachControl(new IVKnobControl(upperPart.GetCentredInside(100).GetVShifted(-100), kGain));
    pGraphics->AttachControl(new IVSwitchControl(upperPart.GetFromBLHC(120, 60), kModePeakRms, "Detection mode", DEFAULT_STYLE), 2);
    //pGraphics->AttachControl(new IVSwitchControl(nextCell().SubRectVertical(3, 0), kParamMode, "IVSwitchControl", style.WithValueText(IText(24.f, EAlign::Center))), kNoTag, "vcontrols");


    auto rectangle = IRECT(0, 0, 500, 400);

    pGraphics->AttachControl(new GraphVisualizer(lowerPart, setting, this->memory));
  };
  //#endif
}

//#if IPLUG_DSP
void TransientVisualizer::ProcessBlock(sample** inputs, sample** outputs, int nFrames) {
  const float gain = GetParam(kGain)->Value() / 100.;
  this->setting->gain = gain;
  const int nChans = NOutChansConnected();
  
  for (int sampleIndex = 0; sampleIndex < nFrames; sampleIndex++) {
    auto left = inputs[0][sampleIndex];
    auto right = inputs[1][sampleIndex];
    auto level = (left + right) / 2;
    /*
    for (int channelIndex = 0; channelIndex < nChans; channelIndex++) {
      outputs[channelIndex][sampleIndex] = inputs[channelIndex][sampleIndex];
    }*/
    measure.learnNewLevel(level);
    peakMeasure = max(peakMeasure, level);

    if (this->measure.count >= this->GetSampleRate()/100) {
      double inputVolumeDb;
      auto mode = GetParam(kModePeakRms)->Value();

      if (mode == 0) {
        inputVolumeDb = convertRmsToDb(this->measure.average);
      } else if(mode == 1) {
        inputVolumeDb = convertRmsToDb(peakMeasure);
      } else {
        inputVolumeDb = convertRmsToDb(max(this->measure.average, peakMeasure));
      }

      memory->addValue(-inputVolumeDb / 55 * setting->visualizerHeight);
      //memory->addValue(measure.average);
      measure.resetSample();
      peakMeasure = 0;
    }

    //string newText = "Sample rate " + to_string(this->GetSampleRate()) + ", measure count: " + to_string(this->measure.count);
    //textDisplayer->SetStr(newText);
  }
}
//#endif
