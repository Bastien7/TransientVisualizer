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


//const IColor lineColor = IColor(255, 230, 115, 115);

//grey levels
/*const IColor lineColor = IColor(255, 160, 150, 145);
const IColor fillColor = IColor(230, lineColor.R * .5, lineColor.G * .5, lineColor.B * .5);
const IColor fillDifferenceUpperColor = IColor(160, fillColor.R, fillColor.G, fillColor.B);
const IColor fillDifferenceLowerColor = IColor(200, lineColor.R*.75, lineColor.G*.75, lineColor.B*.75);
*/

const IColor lineColor = IColor(255, 230, 115, 115);
const IColor fillColor = IColor(230, lineColor.R * .5, lineColor.G * .5, lineColor.B * .5);
const IColor fillDifferenceUpperColor = IColor(160, fillColor.R, fillColor.G, fillColor.B);
const IColor fillDifferenceLowerColor = IColor(60, 255, 40, 80); //IColor(50, 0, 0, 0);

const IColor textMarkerColor = IColor(160, 255, 255, 255);
const IColor markerColor = IColor(80, 255, 255, 255);
const IColor markerAlternativeColor = IColor(50, 255, 255, 255);
const IColor backgroundColor = IColor(255, 18, 30, 43);
//const IColor backgroundColor = IColor(255, 35, 35, 35);
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
    vector<vector<float>> differenceUpperPolygonsX;
    vector<vector<float>> differenceUpperPolygonsY;
    vector<vector<float>> differenceLowerPolygonsX;
    vector<vector<float>> differenceLowerPolygonsY;

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
        resetPolygonPositions(g, width, displayFactor);
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

      for (int i = 0; i < this->differenceUpperPolygonsX.size(); i++) {
        float* arrayX = &differenceUpperPolygonsX[i][0]; //magic
        float* arrayY = &differenceUpperPolygonsY[i][0];
        g.FillConvexPolygon(fillDifferenceUpperColor, arrayX, arrayY, differenceUpperPolygonsX[i].size(), 0);
      }
      for (int i = 0; i < this->differenceLowerPolygonsX.size(); i++) {
        float* arrayX = &differenceLowerPolygonsX[i][0]; //magic
        float* arrayY = &differenceLowerPolygonsY[i][0];
        g.FillConvexPolygon(fillDifferenceLowerColor, arrayX, arrayY, differenceLowerPolygonsX[i].size(), 0);
      }

      g.DrawConvexPolygon(lineColor, polygonX, polygonY, polygonLength, 0, 1.5);
    }

    void drawMarkers(IGraphics& g) {
      int top = this->mRECT.T;
      int width = this->mRECT.R;
      int pixelStep = 55 / 3 * setting->visualizerHeight;
      bool alternate = false;
      const int textOffset = 20;

      for (int levelDb = -3; levelDb > -55; levelDb -= 3) {
        int markerY = top - (levelDb / 55.0 * setting->visualizerHeight);
        auto text = DB_LEVELS[-levelDb / 3 - 1];
        IColor lineColor = alternate ? markerAlternativeColor : markerColor;
        alternate = !alternate;
        
        g.DrawText(TEXT_COMPONENT, text, IRECT(width - textOffset, markerY - pixelStep-2, width, markerY + pixelStep));
        //g.DrawDottedLine(lineColor, 0, markerY, width - textOffset, markerY, 0, 1.0f, 3.0f);
        g.DrawLine(lineColor, 0, markerY, width - textOffset, markerY, 0, 1.0f);
      }
    }

    //startColumn has to be the column just before the difference start (where the two graph start to separate)
    void addDifferencePolygon(IGraphics& g, int mode, vector<float> differencePoints, vector<float> originalPoints, int startColumn, float displayFactor) {
      const int endColumn = startColumn + originalPoints.size() - 2;
      vector<float> differencePolygonX;
      vector<float> differencePolygonY;
      //g.DrawLine(COLOR_YELLOW, startColumn * displayFactor, 0, startColumn * displayFactor, 580);
      //g.DrawLine(COLOR_BLUE, endColumn * displayFactor, 0, endColumn * displayFactor, 580);

      for (int index = 0; index < differencePoints.size(); index++) {
        differencePolygonX.push_back((endColumn - 2 - index) * displayFactor);
        differencePolygonY.push_back(differencePoints[index]);
      }
      for (int index = originalPoints.size() - 1; index >= 0; index--) {
        differencePolygonX.push_back((endColumn - 1 - index) * displayFactor);
        differencePolygonY.push_back(originalPoints[index]);
      }

      if (mode == 1) {
        differenceUpperPolygonsX.push_back(differencePolygonX);
        differenceUpperPolygonsY.push_back(differencePolygonY);
      } else {
        differenceLowerPolygonsX.push_back(differencePolygonX);
        differenceLowerPolygonsY.push_back(differencePolygonY);
      }
    }

    double getInputSmoothY(int index) {
      const double peakValue = memoryInputPeak->get(index);
      const double smoothValue = memoryInputSmoothing->get(index);

      return getInputSmoothY(peakValue, smoothValue);
    }

    inline double getInputSmoothY(double peakValue, double smoothValue) {
      return ((double)setting->smooth) * min(smoothValue, peakValue) + (1.0 - (double)setting->smooth) * peakValue;
    }

    void resetPolygonPositions(IGraphics& g, int width, float displayFactor) {
      float top = this->mRECT.T;
      int bottom = this->mRECT.B;
      int height = this->mRECT.H();
      setting->visualizerHeight = height;

      int polygonLength = width + 2;

      //initialize all columns to an empty data
      for (int i = 0; i < polygonLength; i++) {
        polygonY[i] = bottom + 1;
      }


      differenceUpperPolygonsX.clear();
      differenceUpperPolygonsY.clear();
      differenceLowerPolygonsX.clear();
      differenceLowerPolygonsY.clear();

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

        //difference polygon graph construction
        if (sidechainY < inputY) {
          if (differenceMode == 1) { //continue negative polygon
            differencePoints.push_back(sidechainY);
            originalPoints.push_back(inputY);
          } else {
            if (differenceMode == -1) { //end positive polygon
              originalPoints.push_back(inputY);
              addDifferencePolygon(g, differenceMode, differencePoints, originalPoints, column+2, displayFactor);
            }
            //create negative polygon
            differencePoints = vector<float>();
            differencePoints.push_back(sidechainY);
            originalPoints = vector<float>();
            originalPoints.push_back(polygonY[column+2]);
            originalPoints.push_back(inputY);
          }

          differenceMode = 1;

        } else if (sidechainY > inputY) {
          if (differenceMode == -1) { //continue positive polygon
            differencePoints.push_back(sidechainY);
            originalPoints.push_back(inputY);
          } else {
            if (differenceMode == 1) { //end negative polygon
              originalPoints.push_back(inputY);
              addDifferencePolygon(g, differenceMode, differencePoints, originalPoints, column+2, displayFactor);
            }
            //create positive polygon
            differencePoints = vector<float>();
            differencePoints.push_back(sidechainY);
            originalPoints = vector<float>();
            originalPoints.push_back(polygonY[column+2]);
            originalPoints.push_back(inputY);
          }
          differenceMode = -1;

        } else if (differenceMode != 0) { //end current difference polygon
          originalPoints.push_back(inputY);
          addDifferencePolygon(g, differenceMode, differencePoints, originalPoints, column+2, displayFactor);
          differenceMode = 0;
        }
      }

      //set tup first and last polygon point, so that they invisibly join under the screen
      polygonX[0] = 0;
      polygonY[0] = bottom + 1;
      polygonX[polygonLength - 1] = polygonX[polygonLength - 2];
      polygonY[polygonLength - 1] = bottom + 1;

      if (differenceMode != 0) { //end current difference polygon
        originalPoints.push_back(polygonY[0]);
        addDifferencePolygon(g, differenceMode, differencePoints, originalPoints, 1, displayFactor);
        differenceMode = 0;
      }
    }
};