#pragma once

#include <functional>
#include <random>

class RandomGenerator {
  public:
    RandomGenerator(double low, double high) : r(std::bind(std::uniform_real_distribution<>(low, high), std::default_random_engine())) {}

    double operator()() { return r(); }

  private:
    std::function<double()> r;
};