#pragma once


#include <iostream>
#include <string>
#include <stdlib.h>     /* calloc, exit, free */
#include <math.h>       /* floor */

//#include <chrono>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

#include "IControl.h"
#include "UiSetting.h"
#include "FifoMemory.h"

using namespace iplug;
using namespace igraphics;



class GraphVisualizer : public IControl {
  private:
    IColor lineColor = IColor(255, 230, 115, 115);
    IColor fillColor = IColor(230, 102, 50, 50);
    IColor textMarkerColor = IColor(160, 255, 255, 255);
    IColor markerColor = IColor(80, 255, 255, 255);
    IColor markerAlternativeColor = IColor(50, 255, 255, 255);
    IColor backgroundColor = IColor(255, 18, 30, 43);

  public:
    UiSetting* setting;
    FifoMemory* memoryRms;
    FifoMemory* memoryPeak;
    int stepper = 0;
    int side = 1;
    int previousMemoryIterator = -1;
    float previousDisplayFactor = -1;
    float previousSmooth = 0;

    long counterResetDone = 0;
    long counterResetAvoided = 0;
    float* polygonX = (float*)calloc(4000 * 4 + 2, sizeof(float));;
    float* polygonY = (float*)calloc(4000 * 4 + 2, sizeof(float));;

    GraphVisualizer(const IRECT& bounds, UiSetting* setting, FifoMemory* memoryRms, FifoMemory* memoryPeak) : IControl(bounds), setting(setting), memoryRms(memoryRms), memoryPeak(memoryPeak) { }

    void Draw(IGraphics& g) override {
      //auto start_time = std::chrono::high_resolution_clock::now();

      g.FillRect(backgroundColor, this->mRECT);
      SetDirty();

      float displayFactor = setting->gain;
      int width = floor(this->mRECT.R / displayFactor);

      if (this->memoryRms->currentIterator == previousMemoryIterator && displayFactor == previousDisplayFactor && setting->smooth == previousSmooth) {
        drawPolygons(g, width + 2);
        drawMarkers(g);

        counterResetAvoided++;
        return;
      } else {
        resetPolygonPositions(width, displayFactor);
        drawPolygons(g, width + 2);
        drawMarkers(g);

        previousMemoryIterator = this->memoryRms->currentIterator;
        previousDisplayFactor = displayFactor;
        previousSmooth = setting->smooth;
        counterResetDone++;
      }
    }


  private:
    void drawPolygons(IGraphics& g, int polygonLength) {
      g.FillConvexPolygon(fillColor, polygonX, polygonY, polygonLength);
      g.DrawConvexPolygon(lineColor, polygonX, polygonY, polygonLength, 0, 1.5);
    }

    void drawMarkers(IGraphics& g) {
      int top = this->mRECT.T;
      int width = this->mRECT.R;
      int pixelStep = 55 / 3 * setting->visualizerHeight;

      for (int levelDb = -3; levelDb > -55; levelDb -= 3) {
        int markerY = top - (levelDb / 55.0 * setting->visualizerHeight);
        string textString = to_string((int)levelDb);
        auto text = textString.c_str();
        IColor lineColor = levelDb % 2 != 0 ? markerAlternativeColor : markerColor;
        const int textOffset = 20;

        g.DrawText(IText(14.0F, textMarkerColor), text, IRECT(width - textOffset, markerY - pixelStep-2, width, markerY + pixelStep));
        g.DrawDottedLine(lineColor, 0, markerY, width - textOffset, markerY, 0, 1.0f, 3.0f);
      }
    }

    void resetPolygonPositions(int width, float displayFactor) {
      float top = this->mRECT.T;
      int bottom = this->mRECT.B;
      int height = this->mRECT.H();
      setting->visualizerHeight = height;

      int polygonLength = width + 2;

      //initialize all columns to an empty data
      for (int i = 0; i < polygonLength; i++) {
        polygonY[i] = bottom + 1;
      }

      //instantiate all polygon data points
      for (int column = width; column >= 0; column--) {
        int dataIndex = this->memoryRms->currentIterator - (width - column);

        if (dataIndex < 0) {
          dataIndex += this->memoryRms->size - 1;
        }

        double yRelativeValue;
        //auto a = memoryRms->get(dataIndex);
        //auto b = memoryPeak->get(dataIndex);

        const double rmsValue = memoryRms->get(dataIndex);
        const double peakValue = memoryPeak->get(dataIndex);
        yRelativeValue = setting->smooth * min(rmsValue, peakValue) + (1.0f - setting->smooth) * peakValue;

        float y;

        if (peakValue == -1 || dataIndex < 0) {
          y = bottom + 1;
        } else {
          y = top + yRelativeValue;
        }

        polygonX[column + 1] = column * displayFactor;
        polygonY[column + 1] = max(y, top + 1);
      }

      //set tup first and last polygon point, so that they invisibly join under the screen
      polygonX[0] = 0;
      polygonY[0] = bottom + 1;
      polygonX[polygonLength - 1] = polygonLength * displayFactor - 1;
      polygonY[polygonLength - 1] = bottom + 1;
    }
};