#pragma once


class Measure {
  public:
    double average = 0;
    int count = 0;

    void resetSample() {
      this->count = 0;
      this->average = 1;
    };

    bool hasSignalToLearn(double sampleLevel) {
      return sampleLevel > 0.001; // -55dB threshold
    };

    void learnNewLevel(double sampleLevel, bool ignoreLowNoise = true) {
      if (!ignoreLowNoise || hasSignalToLearn(sampleLevel)) {
        this->average = (this->average * this->count + sampleLevel) / (this->count + 1);
        this->count += 1;
      }
    };

    inline double getAverage() {
      return this->average;
    }
};