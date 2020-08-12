#pragma once
#include <math.h>
using namespace std;


class Highpass {
  public:
   double cosw01 = 0., alpha1 = 0., xl11 = 0., xl21 = 0., yl11 = 0., yl21 = 0.;

    Highpass(double sampleRate) {
      const double freq1 = 60.;
      const double a1 = 1.;
      const double s1 = 1.;
      //check on q to set the Q? it seems to work
      const double q1 = 1. / (sqrt((a1 + 1./a1)*(1./s1 - 1.) + 2.));
      const double w01 = 2. * M_PI * freq1/ sampleRate;
      this->cosw01 = cos(w01);
      const double sinw01 = sin(w01);
      this->alpha1 = sinw01 / (2. * q1);
    }


    double applyHighpass(double inputSignal) {
      double b01 = (1 + this->cosw01) / 2;
      double b11 = -(1 + this->cosw01);
      double b21 = (1 + this->cosw01) / 2;
      const double a01 = 1 + this->alpha1;
      double a11 = -2 * this->cosw01;
      double a21 = 1 - this->alpha1;
      b01 /= a01;
      b11 /= a01;
      b21 /= a01;
      a11 /= a01;
      a21 /= a01;

      const double ospl0 = inputSignal;
      inputSignal = b01 * inputSignal + b11 * this->xl11 + b21 * this->xl21 - a11 * this->yl11 - a21 * this->yl21;
      this->xl21 = this->xl11;
      this->xl11 = ospl0;
      this->yl21 = this->yl11;
      this->yl11 = inputSignal;

      return inputSignal;
    };
};

class HighShelf {
  public:
    double yl_hs = 0., x1l_hs = 0., x2l_hs = 0., y1l_hs = 0., y2l_hs = 0.;
    double a0_hs = 0., a1_hs = 0., a2_hs = 0., b1_hs = 0., b2_hs = 0.;

    HighShelf(double sampleRate) {
      const double SPN=0.;
      const double sx = 16+59.14*1.20103; //59.14 corresponds to 1200hz
      double cf = floor(exp(sx*log(1.059))*8.17742);
      //freq2 = cf;
      cf /= sampleRate;
      const double boost = -3.75; //+3.75db boost on the high shelf

      const double sa = tan(M_PI *(cf-0.25));
      const double asq = sa*sa;
      const double A = pow(10, (boost/20.0));
      double F;

      if((boost < 6.0) && (boost > -6.0)) {
        F = sqrt(A);
      } else {
        if(A > 1.0) {
            F = A/sqrt(2.0);
          } else {
            F = A*sqrt(2.0);
          }
      }

      const double F2 = F*F;
      const double tmp = A*A - F2;
      double gammad;
      if(abs(tmp) <= SPN) {
        gammad = 1.0;
      } else {
        gammad = pow((F2-1.0)/tmp, 0.25);
      }
      const double gamman = sqrt(A)*gammad;
      double gamma2 = gamman*gamman;
      double gam2p1 = 1.0 + gamma2;
      double siggam2 = 2.0*sqrt(2.0)/2.0*gamman;
      const double ta0 = gam2p1 + siggam2;
      const double ta1 = -2.0*(1.0 - gamma2);
      const double ta2 = gam2p1 - siggam2;
      gamma2 = gammad*gammad;
      gam2p1 = 1.0 + gamma2;
      siggam2 = 2.0*sqrt(2.0)/2.0*gammad;
      const double tb0 = gam2p1 + siggam2;
      const double tb1 = -2.0*(1.0 - gamma2);
      const double tb2 = gam2p1 - siggam2;

      const double aa1 = sa*ta1;
      double a0 = ta0 + aa1 + asq*ta2;
      double a1 = 2.0*sa*(ta0+ta2)+(1.0+asq)*ta1;
      double a2 = asq*ta0 + aa1 + ta2;

      const double ab1 = sa*tb1;
      const double b0 = tb0 + ab1 + asq*tb2;
      double b1 = 2.0*sa*(tb0+tb2)+(1.0+asq)*tb1;
      double b2 = asq*tb0 + ab1 + tb2;

      const double recipb0 = 1.0/b0;
      a0 *= recipb0;
      a1 *= recipb0;
      a2 *= recipb0;
      b1 *= recipb0;
      b2 *= recipb0;

      const double gain = pow(10, boost/20.0);
      this->a0_hs = a0/gain;
      this->a1_hs = a1/gain;
      this->a2_hs = a2/gain;
      this->b1_hs = -b1;
      this->b2_hs = -b2;
    }


    double applyHighShelf(double signalLevel) {
      double yl_hs = this->a0_hs * signalLevel + this->a1_hs * this->x1l_hs + this->a2_hs * this->x2l_hs + this->b1_hs * this->y1l_hs + this->b2_hs * this->y2l_hs;
      this->x2l_hs = this->x1l_hs;
      this->x1l_hs = signalLevel;
      this->y2l_hs = this->y1l_hs;
      this->y1l_hs = yl_hs;

      return yl_hs;
    };
};

class Loudness {
private:
  Highpass highpass;
  HighShelf highShelf;

public:
  Loudness(double sampleRate) : highpass(Highpass(sampleRate)), highShelf(HighShelf(sampleRate)) { }

  double applyLoudnessWeighting(double signalLevel) {
    return highShelf.applyHighShelf(highpass.applyHighpass(signalLevel));
  }
};
