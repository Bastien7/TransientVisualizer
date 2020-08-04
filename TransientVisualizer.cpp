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

//ITextControl* text1 = new ITextControl(IRECT(200, 0, 300, 200), "test1", IText(30));
//ITextControl* text2 = new ITextControl(IRECT(200, 30, 300, 230), "test2", IText(30));
bool connected = true;
int inputGeneratorCounter = 0;
double inputGeneratorValue = .01;


TransientVisualizer::TransientVisualizer(const InstanceInfo& info) : Plugin(info, MakeConfig(kNumParams, kNumPresets)),
    memoryInputSmoothing(new FifoMemory(MAX_MEMORY_SIZE, -1)),
    memoryInputPeak(new FifoMemory(MAX_MEMORY_SIZE, -1)),
    memorySidechainPeak(new FifoMemory(MAX_MEMORY_SIZE, -1)) {

  GetParam(kZoom)->InitDouble("Zoom", 100, 25, 1000, 1, "%", 0, "", IParam::ShapePowCurve(3));
  GetParam(kSmooth)->InitDouble("Smooth", 40, 0, 100.0, 1, "%", 0, "", IParam::ShapePowCurve(.5), IParam::kUnitPercentage);
  GetParam(kModeScrolling)->InitEnum("Scrolling mode", 1, 2, "", IParam::kFlagsNone, "", "Stop on silence", "Never stop");
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

    //pGraphics->AttachControl(new ITextControl(upperPart.GetMidVPadded(50), "Hello Mouth!", IText(50)));
    pGraphics->AttachControl(new ITextControl(upperPart.GetFromBottom(30), "v1.18", IText(20)));
    //pGraphics->AttachControl(text1);
    //pGraphics->AttachControl(text2);
    pGraphics->AttachControl(new IVKnobControl(upperPart.GetFromRight(120), kZoom));
    pGraphics->AttachControl(new IVKnobControl(upperPart.GetReducedFromRight(120).GetFromRight(120), kSmooth));
    pGraphics->AttachControl(new IVSwitchControl(upperPart.GetFromLeft(120), kModeScrolling, "Scrolling mode", DEFAULT_STYLE), 3);

    pGraphics->AttachControl(new GraphVisualizer(lowerPart, setting, this->memoryInputPeak, this->memoryInputSmoothing, this->memorySidechainPeak));
  };
  //#endif
}

//#if IPLUG_DSP
void TransientVisualizer::ProcessBlock(sample** inputs, sample** outputs, int nFrames) {
  this->setting->zoom = GetParam(kZoom)->Value() / 100.;
  this->setting->smooth = round(GetParam(kSmooth)->Value()) / 100;
  const int nChans = NOutChansConnected();
  const int sampleRate = GetSampleRate();

  for (int sampleIndex = 0; sampleIndex < nFrames; sampleIndex++) {
    double level = -1;
    double sidechainLevel = -1.0;

    //if (IsChannelConnected(ERoute::kInput, 0) && IsChannelConnected(ERoute::kInput, 1)) { //TODO see how to optimize this to not call it each time
      double left = inputs[0][sampleIndex];
      double right = inputs[1][sampleIndex];
      //level = (abs(left) + abs(right)) / 2;
      level = abs(left);
      sidechainLevel = abs(right);
    //}
    
    /*if (inputGeneratorCounter >= sampleRate/2) {
      inputGeneratorCounter = 0;
      if (inputGeneratorValue == .2) {
        inputGeneratorValue = .01;
      } else if (inputGeneratorValue == .01) {
        inputGeneratorValue = 0;
      } else inputGeneratorValue = .2;
    }

    level = inputGeneratorValue;
    inputGeneratorCounter++;
    */
    //level = inputGeneratorCounter / 60000.0;
    //if(inputGeneratorCounter > 0) inputGeneratorCounter--;
    
    /*
    if (IsChannelConnected(ERoute::kInput, 2) && IsChannelConnected(ERoute::kInput, 3)) { //TODO see how to optimize this to not call it each time
      if(!connected)
        text1->SetStr("Sidechain connected");
      connected = true;
      double sidechainLeft = inputs[2][sampleIndex];
      double sidechainRight = inputs[3][sampleIndex];
      sidechainLevel = (abs(sidechainLeft) + abs(sidechainRight)) / 2;
    } else {
      if(connected)
        text1->SetStr("Sidechain not connected");
      connected = false;
    }*/

    
    for (int channelIndex = 0; channelIndex < nChans; channelIndex++) {
      outputs[channelIndex][sampleIndex] = inputs[channelIndex][sampleIndex];
    }

    bool autoScroll = GetParam(kModeScrolling)->Value() == 1;
    
    measurePeak1.learnNewLevel(level, !autoScroll);
    measurePeak2.learnNewLevel(level, !autoScroll);
    measurePeak3.learnNewLevel(level, !autoScroll);
    measurePeak4.learnNewLevel(level, !autoScroll);
    measurePeak5.learnNewLevel(level, !autoScroll);

    measureSidechainPeak1.learnNewLevel(sidechainLevel, !autoScroll);

    if (measurePeak1.count >= sampleRate / 200) {
      double inputVolumePeakDb = convertRmsToDb(measurePeak1.max);

      if (inputVolumePeakDb < -6000) {
        if (measurePeak1.count > 0) { //only reset if needed
          measurePeak1.resetSample();
          measurePeak2.resetSample();
          measurePeak3.resetSample();
          measurePeak4.resetSample();
          measurePeak5.resetSample();
          measureSidechainPeak1.resetSample();
        }

        memoryInputPeak->addValue(setting->visualizerHeight);
        memoryInputSmoothing->addValue(setting->visualizerHeight);
        memorySidechainPeak->addValue(setting->visualizerHeight);

        return;
      }

      memoryInputPeak->addValue(inputVolumePeakDb / MINIMUM_VOLUME_DB * setting->visualizerHeight);

      double smoothingValue = (measurePeak2.max + measurePeak3.max + measurePeak4.max + measurePeak5.max) / 4;
      if (smoothingValue == 0) { smoothingValue = 1; }
      measureRatioPeakSmoothing.learnNewLevel(max(smoothingValue, measurePeak1.max) / smoothingValue);

      double inputVolumeSmoothingDb = convertRmsToDb(measureRatioPeakSmoothing.average * smoothingValue);
      memoryInputSmoothing->addValue(inputVolumeSmoothingDb / MINIMUM_VOLUME_DB * setting->visualizerHeight);


      double sidechainVolumePeakDb = /*(inputVolumePeakDb + 2.0 * (-40.0)) / 3.0; */convertRmsToDb(measureSidechainPeak1.max);
      memorySidechainPeak->addValue(sidechainVolumePeakDb / MINIMUM_VOLUME_DB * setting->visualizerHeight);


      if (measurePeak2.count >= sampleRate / 10) {
        measurePeak2.resetSample();
        measurePeak2.learnNewLevel(measurePeak1.max);
      }
      if (measurePeak3.count >= sampleRate / 20) {
        measurePeak3.resetSample();
        measurePeak3.learnNewLevel(measurePeak1.max);
      }
      if (measurePeak4.count >= sampleRate / 40) {
        measurePeak4.resetSample();
        measurePeak4.learnNewLevel(measurePeak1.max);
      }
      if (measurePeak5.count >= sampleRate / 80) {
        measurePeak5.resetSample();
        measurePeak5.learnNewLevel(measurePeak1.max);
      }

      measurePeak1.resetSample();
      measureSidechainPeak1.resetSample();
    }

    if (measureRatioPeakSmoothing.count >= sampleRate / 10) {
      const double oldRatioAverage = measureRatioPeakSmoothing.average;
      measureRatioPeakSmoothing.resetSample();
      measureRatioPeakSmoothing.learnNewLevel(oldRatioAverage);
    }
  }
}
//#endif
