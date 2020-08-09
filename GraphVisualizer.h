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


//grey levels
/*const IColor lineColor = IColor(255, 160, 150, 145);
const IColor fillColor = IColor(230, lineColor.R * .5, lineColor.G * .5, lineColor.B * .5);
const IColor fillDifferenceUpperColor = IColor(160, fillColor.R, fillColor.G, fillColor.B);
const IColor fillDifferenceLowerColor = IColor(200, lineColor.R*.75, lineColor.G*.75, lineColor.B*.75);
*/

const IColor peakLineColor = IColor(255, 230, 115, 115);
const IColor rmsLineColor = IColor(255, 115, 115, 230);
const IColor peakFillColor = IColor(255, peakLineColor.R * .45, peakLineColor.G * .45, peakLineColor.B * .45);
const IColor rmsFillColor = IColor(255, rmsLineColor.R * .45, rmsLineColor.G * .45, rmsLineColor.B * .45);
const IColor fillDifferenceUpperColor = IColor(100, 243, 172, 172);
const IColor fillDifferenceLowerColor = IColor(255, 40, 80, 40);

const IColor textMarkerColor = IColor(160, 255, 255, 255);
const IColor markerColor = IColor(80, 255, 255, 255);
const IColor markerAlternativeColor = IColor(50, 255, 255, 255);
const IColor backgroundColor = IColor(255, 18, 30, 43);
const char* DB_LEVELS[] = { "-3", "-6", "-9", "-12", "-15", "-18", "-21", "-24", "-27", "-30", "-33", "-36", "-39", "-42", "-45", "-48", "-51", "-54" };
const IText TEXT_COMPONENT = IText(14.0F, textMarkerColor);


class GraphVisualizer : public IControl {
  private:
    UiSetting *setting;

    FifoMemory *memoryInputPeak, *memoryInputSmoothing;
    FifoMemory *memorySidechainPeak, *memorySidechainSmoothing;

    FifoMemory *memoryInputRms, *memorySidechainRms;

    int previousMemoryIterator = -1;
    float previousDisplayFactor = -1;
    float previousSmooth = 0;
    float previousDetectionMode = 0;

    long counterResetDone = 0;
    long counterResetAvoided = 0;
    float* polygonX = (float*)calloc(4000 * 4 + 2, sizeof(float)); //4K width screens should be managed with this
    float* polygonPeakY = (float*)calloc(4000 * 4 + 2, sizeof(float));
    float* polygonRmsY = (float*)calloc(4000 * 4 + 2, sizeof(float));

    vector<vector<float>> peakDifferenceUpperPolygonsX;
    vector<vector<float>> peakDifferenceUpperPolygonsY;
    vector<vector<float>> peakDifferenceLowerPolygonsX;
    vector<vector<float>> peakDifferenceLowerPolygonsY;


    int mouseThreshold = -1;


  public:
    GraphVisualizer(const IRECT& bounds, UiSetting* setting, FifoMemory* memoryInputPeak, FifoMemory* memoryInputSmoothing, FifoMemory* memorySidechainPeak, FifoMemory* memorySidechainSmoothing, FifoMemory* memoryInputRms, FifoMemory* memorySidechainRms) :
      IControl(bounds),
      setting(setting),
      memoryInputPeak(memoryInputPeak), memoryInputSmoothing(memoryInputSmoothing),
      memorySidechainPeak(memorySidechainPeak), memorySidechainSmoothing(memorySidechainSmoothing),
      memoryInputRms(memoryInputRms), memorySidechainRms(memorySidechainRms)
    { }

    void Draw(IGraphics& g) override {
      //auto start_time = std::chrono::high_resolution_clock::now();
      g.EnableMouseOver(true);
      g.FillRect(backgroundColor, this->mRECT);

      float displayFactor = setting->zoom;
      int width = floor(this->mRECT.R / displayFactor);

      if (this->memoryInputSmoothing->currentIterator == previousMemoryIterator && displayFactor == previousDisplayFactor && setting->smooth == previousSmooth && setting->detectionMode == previousDetectionMode) {
        SetDirty(false);

        drawPeakChart(g, width + 2);
        drawMarkers(g);

        counterResetAvoided++;
        return;
      } else {
        SetDirty(true);
        resetPolygonPositions(g, width, displayFactor, memoryInputPeak);
        drawPeakChart(g, width + 2);
        drawMarkers(g);

        previousMemoryIterator = this->memoryInputSmoothing->currentIterator;
        previousDisplayFactor = displayFactor;
        previousSmooth = setting->smooth;
        previousDetectionMode = setting->detectionMode;
        counterResetDone++;
      }
    }


  public:
    //UI input controls
    void OnMouseOver(float x, float y, const IMouseMod& mod) override {
      mouseThreshold = y;
    }
    void OnMouseDown(float x, float y, const IMouseMod& mod) override {
      mouseThreshold = y;
    }

  private:
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

        g.DrawText(TEXT_COMPONENT, text, IRECT(width - textOffset, markerY - pixelStep - 2, width, markerY + pixelStep));
        //g.DrawDottedLine(lineColor, 0, markerY, width - textOffset, markerY, 0, 1.0f, 3.0f);
        g.DrawLine(lineColor, 0, markerY, width - textOffset, markerY, 0, 1.0f);
      }

      if (mouseThreshold != -1) {
        g.DrawLine(COLOR_YELLOW, 0, mouseThreshold, width, mouseThreshold, 0, 1.5f);
      }
    }

    void drawPeakChart(IGraphics& g, int polygonLength) {
      if (setting->detectionMode == 0) {
        g.FillConvexPolygon(peakFillColor, polygonX, polygonPeakY, polygonLength);

        for (int i = 0; i < this->peakDifferenceUpperPolygonsX.size(); i++) {
          float* arrayX = &peakDifferenceUpperPolygonsX[i][0]; //magic
          float* arrayY = &peakDifferenceUpperPolygonsY[i][0];
          g.FillConvexPolygon(fillDifferenceUpperColor, arrayX, arrayY, peakDifferenceUpperPolygonsX[i].size());
        }
        for (int i = 0; i < this->peakDifferenceLowerPolygonsX.size(); i++) {
          float* arrayX = &peakDifferenceLowerPolygonsX[i][0]; //magic
          float* arrayY = &peakDifferenceLowerPolygonsY[i][0];
          g.FillConvexPolygon(fillDifferenceLowerColor, arrayX, arrayY, peakDifferenceLowerPolygonsX[i].size());
        }

        g.DrawConvexPolygon(peakLineColor, polygonX, polygonPeakY, polygonLength, 0, 1.5);
      } else if (setting->detectionMode == 1) {
        g.FillConvexPolygon(rmsFillColor, polygonX, polygonRmsY, polygonLength);

        /*for (int i = 0; i < this->peakDifferenceUpperPolygonsX.size(); i++) {
          float* arrayX = &peakDifferenceUpperPolygonsX[i][0]; //magic
          float* arrayY = &peakDifferenceUpperPolygonsY[i][0];
          g.FillConvexPolygon(fillDifferenceUpperColor, arrayX, arrayY, peakDifferenceUpperPolygonsX[i].size());
        }
        for (int i = 0; i < this->peakDifferenceLowerPolygonsX.size(); i++) {
          float* arrayX = &peakDifferenceLowerPolygonsX[i][0]; //magic
          float* arrayY = &peakDifferenceLowerPolygonsY[i][0];
          g.FillConvexPolygon(fillDifferenceLowerColor, arrayX, arrayY, peakDifferenceLowerPolygonsX[i].size());
        }*/

        g.DrawConvexPolygon(rmsLineColor, polygonX, polygonRmsY, polygonLength, 0, 1.5);
      } else {
        g.DrawConvexPolygon(rmsLineColor, polygonX, polygonRmsY, polygonLength, 0, 1.5);
        g.DrawConvexPolygon(peakLineColor, polygonX, polygonPeakY, polygonLength, 0, 1.5);
      }
    }

    //startColumn has to be the column just before the difference start (where the two graph start to separate)
    void addPeakDifferencePolygon(IGraphics& g, int mode, vector<float> differencePoints, vector<float> originalPoints, int startColumn, float displayFactor) {
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
        peakDifferenceUpperPolygonsX.push_back(differencePolygonX);
        peakDifferenceUpperPolygonsY.push_back(differencePolygonY);
      } else {
        peakDifferenceLowerPolygonsX.push_back(differencePolygonX);
        peakDifferenceLowerPolygonsY.push_back(differencePolygonY);
      }
    }

    void resetPolygonPositions(IGraphics& g, int width, float displayFactor, FifoMemory* mainMemory) {
      float top = this->mRECT.T;
      int bottom = this->mRECT.B;
      int height = this->mRECT.H();
      setting->visualizerHeight = height;

      int polygonLength = width + 2;

      //initialize all columns to an empty data
      for (int i = 0; i < polygonLength; i++) {
        polygonPeakY[i] = bottom + 1;
        polygonRmsY[i] = bottom + 1;
      }


      peakDifferenceUpperPolygonsX.clear();
      peakDifferenceUpperPolygonsY.clear();
      peakDifferenceLowerPolygonsX.clear();
      peakDifferenceLowerPolygonsY.clear();

      vector<float> differencePoints;
      vector<float> originalPoints;
      int differenceMode = 0;

      //instantiate all polygon data points
      for (int column = width; column >= 0; column--) {
        int dataIndex = mainMemory->currentIterator - (width - column);

        if (dataIndex < 0) {
          dataIndex += mainMemory->size - 1;
        }

        float inputY, sidechainY;

        if (setting->detectionMode == 0) {
          inputY = getYPeakValue(memoryInputPeak, memoryInputSmoothing, dataIndex, top, bottom);
          sidechainY = getYPeakValue(memorySidechainPeak, memorySidechainSmoothing, dataIndex, top, bottom);;
        } else {
          inputY = getYRmsValue(memoryInputRms, dataIndex, top, bottom);
          sidechainY = getYRmsValue(memorySidechainRms, dataIndex, top, bottom);
        }

        polygonX[column + 1] = column * displayFactor;
        polygonPeakY[column + 1] = getYPeakValue(memoryInputPeak, memoryInputSmoothing, dataIndex, top, bottom);
        polygonRmsY[column + 1] = getYRmsValue(memoryInputRms, dataIndex, top, bottom);

        //difference polygon graph construction
        if (sidechainY < inputY) {
          if (differenceMode == 1) { //continue negative polygon
            differencePoints.push_back(sidechainY);
            originalPoints.push_back(inputY);
          } else {
            if (differenceMode == -1) { //end positive polygon
              originalPoints.push_back(inputY);
              addPeakDifferencePolygon(g, differenceMode, differencePoints, originalPoints, column+2, displayFactor);
            }
            //create negative polygon
            differencePoints = vector<float>();
            differencePoints.push_back(sidechainY);
            originalPoints = vector<float>();
            originalPoints.push_back(polygonPeakY[column+2]);
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
              addPeakDifferencePolygon(g, differenceMode, differencePoints, originalPoints, column+2, displayFactor);
            }
            //create positive polygon
            differencePoints = vector<float>();
            differencePoints.push_back(sidechainY);
            originalPoints = vector<float>();
            originalPoints.push_back(polygonPeakY[column+2]);
            originalPoints.push_back(inputY);
          }
          differenceMode = -1;

        } else if (differenceMode != 0) { //end current difference polygon
          originalPoints.push_back(inputY);
          addPeakDifferencePolygon(g, differenceMode, differencePoints, originalPoints, column+2, displayFactor);
          differenceMode = 0;
        }
      }

      //set tup first and last polygon point, so that they invisibly join under the screen
      polygonX[0] = 0;
      polygonPeakY[0] = bottom + 1;
      polygonRmsY[0] = bottom + 1;
      polygonX[polygonLength - 1] = polygonX[polygonLength - 2];
      polygonPeakY[polygonLength - 1] = bottom + 1;
      polygonRmsY[polygonLength - 1] = bottom + 1;

      if (differenceMode != 0) { //end current difference polygon
        originalPoints.push_back(polygonPeakY[0]);
        addPeakDifferencePolygon(g, differenceMode, differencePoints, originalPoints, 1, displayFactor);
        differenceMode = 0;
      }
    }


    float getYPeakValue(FifoMemory* inputMemory, FifoMemory* smoothingMemory, int dataIndex, float top, float bottom) {
      const float instantValue = inputMemory->get(dataIndex);
      const float smoothValue = smoothingMemory->get(dataIndex);
      float inputY;

      if (instantValue == -1 || dataIndex < 0) {
        inputY = bottom + 1;
      }
      else {
        float yValue = (setting->smooth) * min(smoothValue, instantValue) + (1.0 - setting->smooth) * instantValue;

        inputY = top + round(yValue, 1); //TODO maybe remove this round() here? but take care to graphic flickering?
      }

      return max(inputY, top); //TODO maybe get back a round() here, but take care to graphic flickering?
    }

    float getYRmsValue(FifoMemory* inputMemory, int dataIndex, float top, float bottom) {
      const float inputValue = inputMemory->get(dataIndex);
      float inputY;

      if (inputValue == -1 || dataIndex < 0) {
        inputY = bottom + 1;
      }
      else {
        inputY = top + round(inputValue, 1); //TODO maybe get back a round() here, but take care to graphic flickering?
      }

      return max(inputY, top);
    }
};