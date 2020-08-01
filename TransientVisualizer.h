#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IControls.h"
#include "GraphVisualizer.h"
#include "FifoMemory.h"
#include "Measure.h"
#include "UiSetting.h"

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kSmooth = 1,
  kModePeakRms,
  kNumParams
};

using namespace iplug;
using namespace igraphics;



class TransientVisualizer final : public Plugin {
  private:
    UiSetting* setting;
    FifoMemory* memoryRms;
    FifoMemory* memoryPeak;
    Measure measure = Measure();
    MeasureMax peakMeasure = MeasureMax();

  public:
    TransientVisualizer(const InstanceInfo& info);

    #if IPLUG_DSP // http://bit.ly/2S64BDd
    void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
    #endif
};
