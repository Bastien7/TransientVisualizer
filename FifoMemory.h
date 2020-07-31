#pragma once


class FifoMemory {
  private:
    double* values;

  public:
    const int size;
    int currentIterator = 0;

    FifoMemory(const int size, const int defaultValue = 0) : values(new double[size]), size(size) {
      for (int i = 0; i < size; i++) {
        values[i] = defaultValue;
      }
    }

    void addValue(double newValue) {
      values[currentIterator] = newValue;

      currentIterator += 1;

      if (currentIterator == size) {
        currentIterator = 0;
      }
    }

    inline double get(int index) {
      return values[index];
    }
};