#pragma once
#include <math.h>
using namespace std;


namespace converters {

  inline static double convertRatioToDbGain(double ratio) {
    return 20 * log10(ratio);
  };
  inline static double convertRmsToDb(double ratio) {
    return convertRatioToDbGain(ratio);// +5; //I don't yet know why it's needed... Maybe because of a specific signal level referential?
  };

  inline static double convertDbGainToRatio(double gain) {
    return pow(10, gain / 20);
  };
  inline static double convertDbLevelToRatio(double gain) {
    return convertDbGainToRatio(gain /*- 5*/);
  };

  inline static double round(double value, int precision) {
    auto power = pow(10, precision);
    return ceil(value * power) / power;
  };

}