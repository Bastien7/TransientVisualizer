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
    IColor markerColor = IColor(51, 255, 255, 255);
    IColor backgroundColor = IColor(255, 18, 30, 43);

  public:
    UiSetting* setting;
    FifoMemory* memory;
    int stepper = 0;
    int side = 1;
    int previousMemoryIterator = -1;
    float previousDisplayFactor = -1;

    long counterResetDone = 0;
    long counterResetAvoided = 0;
    float* polygonX = nullptr;
    float* polygonY = nullptr;

    GraphVisualizer(const IRECT& bounds, UiSetting* setting, FifoMemory* memory) : IControl(bounds), setting(setting), memory(memory) { }

    void Draw(IGraphics& g) override {
      //auto start_time = std::chrono::high_resolution_clock::now();

      g.FillRect(backgroundColor, this->mRECT);
      SetDirty();

      float gain = setting->gain;
      float displayFactor = (1 - gain*gain) + (10 * gain*gain);
      int width = floor(this->mRECT.R / displayFactor);

      if (this->memory->currentIterator == previousMemoryIterator && displayFactor == previousDisplayFactor) {
        drawPolygons(g, width + 2);
        drawMarkers(g);

        counterResetAvoided++;
        return;
      } else {
        resetPolygonPositions(width, displayFactor);
        drawPolygons(g, width + 2);
        drawMarkers(g);

        previousMemoryIterator = this->memory->currentIterator;
        previousDisplayFactor = displayFactor;
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

      for (float levelDb = -3; levelDb > -55; levelDb -= 3) {
        int markerY = top - ((levelDb) / 55 * setting->visualizerHeight);
        string textString = (to_string((int)levelDb) + "dB");
        auto text = textString.c_str();
        g.DrawText(IText(14.0F, markerColor), text, IRECT(0, markerY - pixelStep-2, 30, markerY + pixelStep));
        g.DrawDottedLine(markerColor, 30, markerY, width, markerY, 0, 1.0f, 2.0f);
      }
    }

    void resetPolygonPositions(int width, float displayFactor) {
      int top = this->mRECT.T;
      int bottom = this->mRECT.B;
      int height = this->mRECT.H();
      setting->visualizerHeight = height;

      if (polygonX != nullptr) {
        free(polygonX);
        free(polygonY);
      }

      int polygonLength = width + 2;
      polygonX = (float*)calloc(polygonLength, sizeof(float));
      polygonY = (float*)calloc(polygonLength, sizeof(float));

      for (int column = width; column >= 0; column--) {
        int dataIndex = this->memory->currentIterator - (width - column); // (width - 1 - column);

        if (dataIndex < 1) {
          dataIndex += this->memory->size - 1;
        }

        //int y = (bottom + 1) - memory->get(dataIndex) * 20;
        auto value = memory->get(dataIndex);
        int y;

        if (value == -1) {
          y = bottom + 1;
        } else {
          y = top + value;
        }

        polygonX[column + 1] = column * displayFactor;
        polygonY[column + 1] = max(y, top + 1);
      }

      polygonX[0] = 0;
      polygonY[0] = bottom + 1;
      polygonX[polygonLength - 1] = polygonLength * displayFactor - 1;
      polygonY[polygonLength - 1] = bottom + 1;
    }

    void drawDemo() {
      /*
      int value = 0;
      int incrementer = 1;
      auto a = setting->gain->Value() / 100;
      auto MAX_HEIGHT = a * 40; //setting->gain->Value()/100 * 40;

      for (int i = stepper; i < 500; i++) {
        g.DrawLine(COLOR_RED, i - 1, 40 + value - incrementer, i, 40 + value, 0, setting->gain->Value() / 100 * 5);

        if (value >= MAX_HEIGHT) {
          incrementer = -1;
        }
        else if (value <= -MAX_HEIGHT) {
          incrementer = 1;
        }

        value += incrementer;
      }

      if (stepper >= 500) {
        side = -3;
      }
      if (stepper <= 0) {
        side = 3;
      }
      stepper += side;*/
    }

    /*
    void OnRescale() override {
    }

    void OnResize() override {
    }
    */
};