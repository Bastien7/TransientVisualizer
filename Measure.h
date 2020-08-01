#pragma once

#include <math.h>

const double MINIMUM_VOLUME = 0.001; // -55dB threshold


class Measure {
  public:
    double average = 0;
    long count = 0;

    void resetSample() {
      this->count = 0;
      this->average = 1;
    };

    void learnNewLevel(double sampleLevel, bool ignoreLowNoise = true) {
      if (!ignoreLowNoise || hasSignalToLearn(sampleLevel)) {
        this->average = (this->average * this->count + sampleLevel) / ((double)this->count + 1);
        this->count += 1;
      }
    };

  private:
    inline bool hasSignalToLearn(double sampleLevel) {
      return sampleLevel > MINIMUM_VOLUME;
    };
};

class MeasureMax {
public:
  double max = 0;
  long count = 0;

  void resetSample() {
    this->count = 0;
    this->max = 0;
  };


  void learnNewLevel(double sampleLevel, bool ignoreLowNoise = true) {
    if (!ignoreLowNoise || hasSignalToLearn(sampleLevel)) {
      this->max = std::max(this->max, sampleLevel);
      this->count += 1;
    }
  };

  private:
    inline bool hasSignalToLearn(double sampleLevel) {
      return sampleLevel > MINIMUM_VOLUME;
    };
};