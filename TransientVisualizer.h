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
  kZoom = 0,
  kSmooth = 1,
  kModeScrolling,
  kNumParams
};

using namespace iplug;
using namespace igraphics;



class TransientVisualizer final : public Plugin {
  private:
    UiSetting* setting;

    FifoMemory* memoryInputPeak;
    FifoMemory* memoryInputSmoothing;
    MeasureMax measurePeak1 = MeasureMax();
    MeasureMax measurePeak2 = MeasureMax();
    MeasureMax measurePeak3 = MeasureMax();
    MeasureMax measurePeak4 = MeasureMax();
    MeasureMax measurePeak5 = MeasureMax();
    Measure measureRatioPeakSmoothing = Measure();

    FifoMemory* memorySidechainPeak;
    MeasureMax measureSidechainPeak1 = MeasureMax();

  public:
    TransientVisualizer(const InstanceInfo& info);

    #if IPLUG_DSP // http://bit.ly/2S64BDd
    void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
    #endif
};
