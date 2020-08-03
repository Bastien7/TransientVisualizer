#pragma once


#include <iostream>
#include <string>
#include <stdlib.h>     /* calloc, exit, free */
#include <math.h>       /* floor */
#include <vector> 

//#include <chrono>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

#include "IControl.h"
#include "UiSetting.h"
#include "FifoMemory.h"
#include "Converters.h"

using namespace converters;
using namespace iplug;
using namespace igraphics;


const IColor lineColor = IColor(255, 230, 115, 115);
const IColor fillColor = IColor(230, 102, 50, 50);
const IColor fillDifferenceColor = IColor(40, 255, 255, 255);
const IColor textMarkerColor = IColor(160, 255, 255, 255);
const IColor markerColor = IColor(80, 255, 255, 255);
const IColor markerAlternativeColor = IColor(50, 255, 255, 255);
const IColor backgroundColor = IColor(255, 18, 30, 43);
const char* DB_LEVELS[] = { "-3", "-6", "-9", "-12", "-15", "-18", "-21", "-24", "-27", "-30", "-33", "-36", "-39", "-42", "-45", "-48", "-51", "-54" };
const IText TEXT_COMPONENT = IText(14.0F, textMarkerColor);


class GraphVisualizer : public IControl {
  private:
    UiSetting* setting;
    FifoMemory* memoryInputPeak;
    FifoMemory* memoryInputSmoothing;
    FifoMemory* memorySidechainPeak;

    int previousMemoryIterator = -1;
    float previousDisplayFactor = -1;
    float previousSmooth = 0;

    long counterResetDone = 0;
    long counterResetAvoided = 0;
    float* polygonX = (float*)calloc(4000 * 4 + 2, sizeof(float)); //4K width screens should be managed with this
    float* polygonY = (float*)calloc(4000 * 4 + 2, sizeof(float));
    vector<vector<float>> differencePolygonsX;
    vector<vector<float>> differencePolygonsY;

  public:
    GraphVisualizer(const IRECT& bounds, UiSetting* setting, FifoMemory* memoryInputPeak, FifoMemory* memoryInputSmoothing, FifoMemory* memorySidechainPeak) :
      IControl(bounds),
      setting(setting),
      memoryInputPeak(memoryInputPeak),
      memoryInputSmoothing(memoryInputSmoothing),
      memorySidechainPeak(memorySidechainPeak) { }

    void Draw(IGraphics& g) override {
      //auto start_time = std::chrono::high_resolution_clock::now();

      g.FillRect(backgroundColor, this->mRECT);

      float displayFactor = setting->zoom;
      int width = floor(this->mRECT.R / displayFactor);

      if (this->memoryInputSmoothing->currentIterator == previousMemoryIterator && displayFactor == previousDisplayFactor && setting->smooth == previousSmooth) {
        SetDirty(false);

        drawPolygons(g, width + 2);
        drawMarkers(g);

        counterResetAvoided++;
        return;
      } else {
        SetDirty(true);
        resetPolygonPositions(width, displayFactor);
        drawPolygons(g, width + 2);
        drawMarkers(g);

        previousMemoryIterator = this->memoryInputSmoothing->currentIterator;
        previousDisplayFactor = displayFactor;
        previousSmooth = setting->smooth;
        counterResetDone++;
      }
    }


  private:
    void drawPolygons(IGraphics& g, int polygonLength) {
      g.FillConvexPolygon(fillColor, polygonX, polygonY, polygonLength);

      for (int i = 0; i < this->differencePolygonsX.size(); i++) {
        float* arrayX = &differencePolygonsX[i][0]; //magic
        float* arrayY = &differencePolygonsY[i][0];
        g.FillConvexPolygon(fillDifferenceColor, arrayX, arrayY, differencePolygonsX[i].size(), 0);
      }

      g.DrawConvexPolygon(lineColor, polygonX, polygonY, polygonLength, 0, 1.5);
    }

    void drawMarkers(IGraphics& g) {
      int top = this->mRECT.T;
      int width = this->mRECT.R;
      int pixelStep = 55 / 3 * setting->visualizerHeight;

      for (int levelDb = -3; levelDb > -55; levelDb -= 3) {
        int markerY = top - (levelDb / 55.0 * setting->visualizerHeight);
        //string textString = to_string((int)levelDb);
        auto text = DB_LEVELS[-levelDb / 3 - 1]; //textString.c_str();
        IColor lineColor = levelDb % 2 != 0 ? markerAlternativeColor : markerColor;
        const int textOffset = 20;

        g.DrawText(TEXT_COMPONENT, text, IRECT(width - textOffset, markerY - pixelStep-2, width, markerY + pixelStep));
        //g.DrawDottedLine(lineColor, 0, markerY, width - textOffset, markerY, 0, 1.0f, 3.0f);
        g.DrawLine(lineColor, 0, markerY, width - textOffset, markerY, 0, 1.0f);
      }
    }

    //startIndex has to be the column just before the difference start (where the two graph start to separate)
    void addDifferencePolygon(vector<float> differencePoints, vector<float> originalPoints, int startIndex, float displayFactor) {
      const int length = differencePoints.size();
      vector<float> differencePolygonX;
      vector<float> differencePolygonY;
      /*
      int indexBefore = startIndex - 1;
      indexBefore = startIndex >= 0 ? startIndex : startIndex + memoryInputPeak->size;

      int indexAfter = startIndex + length;
      indexAfter = indexAfter < memoryInputPeak->size ? indexAfter : indexAfter - memoryInputPeak->size;*/

      for (int index = 0; index < length; index++) {
        differencePolygonX.push_back(startIndex + index * displayFactor);
        differencePolygonY.push_back(differencePoints[index]);
      }
      for (int index = 0; index < length + 2; index++) {
        differencePolygonX.push_back(startIndex + length * displayFactor - index * displayFactor);
        differencePolygonY.push_back(originalPoints[index]);
      }

      differencePolygonsX.push_back(differencePolygonX);
      differencePolygonsY.push_back(differencePolygonY);
    }

    double getInputSmoothY(int index) {
      const double peakValue = memoryInputPeak->get(index);
      const double smoothValue = memoryInputSmoothing->get(index);

      return getInputSmoothY(peakValue, smoothValue);
    }

    inline double getInputSmoothY(double peakValue, double smoothValue) {
      return ((double)setting->smooth) * min(smoothValue, peakValue) + (1.0 - (double)setting->smooth) * peakValue;
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


      differencePolygonsX.clear();
      differencePolygonsY.clear();
      vector<float> differencePoints;
      vector<float> originalPoints;
      int differenceMode = 0;

      //instantiate all polygon data points
      for (int column = width; column >= 0; column--) {
        int dataIndex = this->memoryInputSmoothing->currentIterator - (width - column);

        if (dataIndex < 0) {
          dataIndex += this->memoryInputSmoothing->size - 1;
        }

        const double inputPeakValue = memoryInputPeak->get(dataIndex);
        const double inputSmoothValue = memoryInputSmoothing->get(dataIndex);
        double yRelativeValue = getInputSmoothY(inputPeakValue, inputSmoothValue);
        
        /*if (setting->smooth == 0) {
          yRelativeValue = memorySidechainPeak->get(dataIndex);
        }*/


        float inputY;

        if (inputPeakValue == -1 || dataIndex < 0) {
          inputY = bottom + 1;
        } else {
          inputY = top + round(yRelativeValue, 1);
        }

        polygonX[column + 1] = column * displayFactor;
        polygonY[column + 1] = max(inputY, top + 1);


        const double sidechainPeakValue = memorySidechainPeak->get(dataIndex);
        float sidechainY;

        if (sidechainPeakValue == -1 || dataIndex < 0) {
          sidechainY = bottom + 1;
        } else {
          sidechainY = top + round(sidechainPeakValue, 1);
        }

        /*int wantedDifferenceMode;
        if (sidechainY < inputY) {
          wantedDifferenceMode = -1;
        } else if (sidechainY > inputY) {
          wantedDifferenceMode = 1;
        } else {
          wantedDifferenceMode = 0;
        }*/

        if (sidechainY < inputY) {
          if (differenceMode == -1) { //continue negative polygon
            differencePoints.push_back(sidechainY);
            originalPoints.push_back(inputY);
          } else {
            if (differenceMode == 1) { //end positive polygon
              originalPoints.push_back(inputY);
              addDifferencePolygon(differencePoints, originalPoints, (column + 1) * displayFactor, displayFactor);
            }
            //create negative polygon
            differencePoints = vector<float>();
            differencePoints.push_back(sidechainY);
            originalPoints = vector<float>();
            originalPoints.push_back(polygonY[column+2]); //TODO make sure it exists? (case index -1)
            originalPoints.push_back(inputY);
          }

          differenceMode = -1;

        } else if (sidechainY > inputY) {
          if (differenceMode == 1) { //continue positive polygon
            differencePoints.push_back(sidechainY);
            originalPoints.push_back(inputY);
          } else {
            if (differenceMode == -1) { //end negative polygon
              originalPoints.push_back(inputY);
              addDifferencePolygon(differencePoints, originalPoints, (column+1) * displayFactor, displayFactor);
            }
            //create positive polygon
            differencePoints = vector<float>();
            differencePoints.push_back(sidechainY);
            originalPoints = vector<float>();
            originalPoints.push_back(polygonY[column+2]); //TODO make sure it exists? (case index -1)
            originalPoints.push_back(inputY);
          }
          differenceMode = 1;

        } else if (differenceMode != 0) { //end current difference polygon
          originalPoints.push_back(inputY);
          addDifferencePolygon(differencePoints, originalPoints, (column + 1) * displayFactor, displayFactor);
          differenceMode = 0;
        }
      }

      //set tup first and last polygon point, so that they invisibly join under the screen
      polygonX[0] = 0;
      polygonY[0] = bottom + 1;
      polygonX[polygonLength - 1] = polygonX[polygonLength - 2]; //polygonLength * displayFactor - 1;
      polygonY[polygonLength - 1] = bottom + 1;

      if (differenceMode != 0) { //end current difference polygon
        originalPoints.push_back(polygonY[0]);
        addDifferencePolygon(differencePoints, originalPoints, 0, displayFactor);
        differenceMode = 0;
      }
    }
};