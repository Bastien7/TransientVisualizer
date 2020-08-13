#pragma once

#include <vector>
#include <math.h>
#include "FifoMemory.h"
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

public:
  inline bool hasSignalToLearn(double sampleLevel) {
    return sampleLevel > MINIMUM_VOLUME;
  };
};

class ResettableMeasure : public Measure {
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

class MeasureAverage : public ResettableMeasure {
  double average = 0;

public:
  using ResettableMeasure::resetSampleIfNeeded;
  using ResettableMeasure::needReset;

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

  void forceCountValue(int newCount) {
    this->count = newCount;
  }
};

//if sample rate changes, this class become broken
class MeasureAverageContinuous {
  double average = 0;
  FifoMemory memory;

public:
  MeasureAverageContinuous(double sampleRate, int durationTarget /*in ms*/, double defaultValue = 0.) : memory(FifoMemory(sampleRate / durationTarget, defaultValue)) { }

  inline double getResult() {
    return average;
  }

  void learnNewLevel(double sampleLevel, bool ignoreLowNoise = true) {
    if (!ignoreLowNoise || hasSignalToLearn(sampleLevel)) {
      const auto removedValue = memory.get(memory.currentIterator);
      average = (average * memory.size - removedValue + sampleLevel) / memory.size;
      memory.addValue(sampleLevel);
    }
  };

  inline bool hasSignalToLearn(double sampleLevel) {
    return sampleLevel > MINIMUM_VOLUME;
  };
};

class MeasureMax : public ResettableMeasure {
private:
  double max = 0;

public:
  using ResettableMeasure::resetSampleIfNeeded;
  using ResettableMeasure::needReset;

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
  vector<int> countTargetSampleDivider;
  const double size; //optimization, since the size will never change

public:
  MeasureGroupMax(vector<int> targets) : countTargetSampleDivider(targets), size(targets.size()) {
    for (int i = 0; i < targets.size(); i++) {
      measures.push_back(MeasureMax());
    }
  }

  void resetSample() {
    for (int i = 0; i < size; i++) {
      measures[i].resetSample();
    }
  }

  void resetSampleIfNeeded(int sampleRate, double newStartValue) {
    for (int i = 0; i < size; i++) {
      measures[i].resetSampleIfNeeded(sampleRate / this->countTargetSampleDivider[i], newStartValue);
    }
  }

  virtual void learnNewLevel(double sampleLevel, bool ignoreLowNoise = true) {
    const bool hasSignalToLearn = measures[0].hasSignalToLearn(sampleLevel);

    if (hasSignalToLearn || !ignoreLowNoise) {
      for (int i = 0; i < size; i++) {
        measures[i].learnNewLevel(sampleLevel, false);
      }
    }
  }

  double getResult() {
    double sum = 0;

    for (int i = 0; i < size; i++) {
      sum += measures[i].getResult();
    }

    return sum / size;
  }
};



class MeasureGroupAverage {
private:
  vector<MeasureAverageContinuous> measures;
  const double size; //optimization, since the size will never change

public:
  MeasureGroupAverage(vector<int> targets, double sampleRate) : size(targets.size()) {
    vector<int> extendedTargets;

    for (int i = 0; i < targets.size(); i++) {
      measures.push_back(MeasureAverageContinuous(sampleRate, targets[i]));
    }
  }

  virtual void learnNewLevel(double sampleLevel, bool ignoreLowNoise = true) {
    const bool hasSignalToLearn = measures[0].hasSignalToLearn(sampleLevel);

    if (hasSignalToLearn || !ignoreLowNoise) {
      for (int i = 0; i < size; i++) {
        measures[i].learnNewLevel(sampleLevel, false);
      }
    }
  }

  double getResult() {
    double sum = 0;

    for (int i = 0; i < size; i++) {
      sum += measures[i].getResult();
    }

    return sum / size;
  }
};