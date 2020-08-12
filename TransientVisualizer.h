#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IControls.h"
#include "GraphVisualizer.h"
#include "FifoMemory.h"
#include "Measure.h"
#include "UiSetting.h"
#include "Loudness.h"

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
  improvement : divide control zone height by two, by putting all element on a single line
    maybe try to insert controls on the chart
  5 feature: Auto Y "Zoom" button with two states: (1) automatic Y scaling, (2) full Y scale (switch from one to other)
  1 improvement: move onMouseDown threshold set to onMouseUp
  1 feature: remove threshold on mouseUp RIGHT click
  3 feature: Select point P1 to P2 (clickDown+drag), if P1 < P2 -> calculate attack time, else calculate release time


  2 improvement: see if override isDirty in IControl can help on performance instead of doing setDirty()
  3 optimization: remove point too much near (or on same X position?) --> for statistics, calculate % of points on same column
  8 feature: Lower the resolution for easier readability
  5 feature: Improve the attack/release calculator by finding existing graph points and choosing optimized point (90% same abs(P1-P2) value but shorter duration)
  3 improvement: re-implement the dual measure with 50% over-coverage to smooth out Peak smooth measures
  ? fix: try to round/ceil value in case of displayFactor. The goal is to fix graphical issue that occurs with 0 < displayFactor < 1
  ? fix: check if peakSmoothingRatio could be useful or if they have never been useful (they are currently disabled)
*/
class TransientVisualizer final : public Plugin {
  private:
    UiSetting* setting;

    Loudness loudnessRmsInput, loudnessRmsSidechain;

    FifoMemory* memoryInputPeak;
    FifoMemory* memoryInputSmoothing;
    MeasureMax measureInputPeak = MeasureMax();
    MeasureGroupMax measureInputSmooth = MeasureGroupMax({ 10, 20, 40, 80 }); // 12.5/25/50/100 ms
    //MeasureAverageContinuous measureInputPeakSmoothingRatio;// = MeasureAverage();

    FifoMemory* memorySidechainPeak;
    FifoMemory* memorySidechainSmoothing;
    MeasureMax measureSidechainPeak = MeasureMax();
    MeasureGroupMax measureSidechainSmooth = MeasureGroupMax({ 10, 20, 40, 80 }); // 12.5/25/50/100 ms
    //MeasureAverage measureSidechainPeakSmoothingRatio = MeasureAverage();

    FifoMemory* memoryInputRms;
    MeasureGroupAverage measureInputRms;
    FifoMemory* memorySidechainRms;
    MeasureGroupAverage measureSidechainRms;


  public:
    TransientVisualizer(const InstanceInfo& info);

    #if IPLUG_DSP // http://bit.ly/2S64BDd
    void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
    #endif
};
