#pragma once

#include <vector>
#include <math.h>
using namespace std;

const double MINIMUM_VOLUME = 0.001; // -55dB threshold
const double MINIMUM_VOLUME_DB = -55; // dB

class Measure {
public:
  virtual void resetSample() = 0;
  virtual void resetSampleIfNeeded(long countTarget, double newStartValue) = 0;
  void resetSampleIfNeeded(long countTarget) {
    resetSampleIfNeeded(countTarget, getResult());
  }

  virtual void learnNewLevel(double sampleLevel, bool ignoreLowNoise = true) = 0;
  virtual inline double getResult() = 0;

protected:
  inline bool hasSignalToLearn(double sampleLevel) {
    return sampleLevel > MINIMUM_VOLUME;
  };
};

class SimpleMeasure : public Measure {
protected:
  long count = 0;

public:
  using Measure::resetSampleIfNeeded;

  long getCount() {
    return count;
  }

  bool needReset(long countTarget) {
    return count >= countTarget;
  }

  void resetSampleIfNeeded(long countTarget, double newStartValue) {
    if (needReset(countTarget)) {
      resetSample();
      learnNewLevel(newStartValue);
    }
  }
};

class MeasureAverage : public SimpleMeasure {
  double average = 0;

public:
  using SimpleMeasure::resetSampleIfNeeded;
  using SimpleMeasure::needReset;

  inline double getResult() {
    return average;
  }

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
};

class MeasureMax : public SimpleMeasure {
private:
  double max = 0;

public:
  using SimpleMeasure::resetSampleIfNeeded;
  using SimpleMeasure::needReset;

  inline double getResult() {
    return max;
  }

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
};


class MeasureGroupMax {
private:
  vector<MeasureMax> measures;
  vector<int> countTargetSampleDivier;

public:
  MeasureGroupMax(vector<int> targets) : countTargetSampleDivier(targets) {
    for (int i = 0; i < targets.size(); i++) {
      measures.push_back(MeasureMax());
    }
  }

  void resetSample() {
    for (int i = 0; i < measures.size(); i++) {
      measures[i].resetSample();
    }
  }

  void resetSampleIfNeeded(int sampleRate, double newStartValue) {
    for (int i = 0; i < measures.size(); i++) {
      measures[i].resetSampleIfNeeded(sampleRate / this->countTargetSampleDivier[i], newStartValue);
    }
  }

  virtual void learnNewLevel(double sampleLevel, bool ignoreLowNoise = true) {
    for (int i = 0; i < measures.size(); i++) {
      measures[i].learnNewLevel(sampleLevel, ignoreLowNoise);
    }
  }

  double getResult() {
    double sum = 0;

    for (int i = 0; i < measures.size(); i++) {
      sum += measures[i].getResult();
    }

    return sum / measures.size();
  }
};



class MeasureGroupAverage {
private:
  vector<MeasureAverage> measures;
  vector<int> countTargetSampleDivier;

public:
  MeasureGroupAverage(vector<int> targets) : countTargetSampleDivier(targets) {
    for (int i = 0; i < targets.size(); i++) {
      measures.push_back(MeasureAverage());
    }
  }

  void resetSample() {
    for (int i = 0; i < measures.size(); i++) {
      measures[i].resetSample();
    }
  }

  void resetSampleIfNeeded(int sampleRate) {
    const double currentAverage = this->getResult();

    for (int i = 0; i < measures.size(); i++) {
      if (measures[i].needReset(sampleRate / this->countTargetSampleDivier[i])) {
        measures[i].resetSampleIfNeeded(sampleRate / this->countTargetSampleDivier[i], currentAverage);
      }
    }
  }

  virtual void learnNewLevel(double sampleLevel, bool ignoreLowNoise = true) {
    for (int i = 0; i < measures.size(); i++) {
      measures[i].learnNewLevel(sampleLevel, ignoreLowNoise);
    }
  }

  double getResult() {
    double sum = 0;

    for (int i = 0; i < measures.size(); i++) {
      sum += measures[i].getResult();
    }

    return sum / measures.size();
  }
};