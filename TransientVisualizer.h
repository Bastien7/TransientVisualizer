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
  zoomParameter = 0,
  smoothParameter = 1,
  scrollingModeParameter = 2,
  detectionModeParameter = 3,
  kNumParams
};

using namespace iplug;
using namespace igraphics;


/** TODO
  Implement RMS difference graph (refactor the main method?)
  Set a Y level threshold when click
    mouse management
  Timing metering: get X mouse position, trace vertical line, and trace vertical lines at its right with multiple timing helpers (like a mm/cm ruler)
    mouse management
  Auto Y "Zoom" button with two states: (1) automatic Y scaling, (2) full Y scale (switch from one to other)
  Optimization: remove point too much near (or on same X position?) --> for statistics, calculate % of points on same column
  Lower the resolution for easier readability
  Select point P1 to P2 (clickDown+drag), if P1 < P2 -> calculate attack time, else calculate release time
  Improve the attack/release calculator by choosing optimized point (90% same abs(P1-P2) value but shorter duration)

  Fix: try to round/ceil value in case of displayFactor. The goal is to fix graphical issue that occurs with 0 < displayFactor < 1
  Idea: see if override isDirty in IControl can help on performance instead of doing setDirty()
*/
class TransientVisualizer final : public Plugin {
  private:
    UiSetting* setting;

    FifoMemory* memoryInputPeak;
    FifoMemory* memoryInputSmoothing;
    MeasureMax measureInputPeak = MeasureMax();
    MeasureGroupMax measureInputSmooth = MeasureGroupMax({ 10, 20, 40, 80 }); // 12.5/25/50/100 ms
    MeasureAverage measureInputPeakSmoothingRatio = MeasureAverage();

    FifoMemory* memorySidechainPeak;
    FifoMemory* memorySidechainSmoothing;
    MeasureMax measureSidechainPeak = MeasureMax();
    MeasureGroupMax measureSidechainSmooth = MeasureGroupMax({ 10, 20, 40, 80 }); // 12.5/25/50/100 ms
    MeasureAverage measureSidechainPeakSmoothingRatio = MeasureAverage();

    FifoMemory* memoryInputRms;
    MeasureGroupAverage measureInputRms = MeasureGroupAverage({20, 10, 5, 3}); // 50/100/200/333 ms
    FifoMemory* memorySidechainRms;
    MeasureGroupAverage measureSidechainRms = MeasureGroupAverage({ 20, 10, 5, 3 }); // 50/100/200/333 ms


  public:
    TransientVisualizer(const InstanceInfo& info);

    #if IPLUG_DSP // http://bit.ly/2S64BDd
    void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
    #endif
};
