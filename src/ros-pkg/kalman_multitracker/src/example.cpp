#include <kalman_multitracker/kalman_multitracker.h>

using namespace std;
using namespace Eigen;
using boost::shared_ptr;

double sampleFromGaussian(double variance) {
  double sample = 0;
  for(int i = 0; i < 12; ++i)
    sample += (((double)rand() / (double) RAND_MAX) * 2.0 - 1.0) * variance;
  sample /= 2.0;
  return sample;
}

int main(int argc, char** argv) {
  // -- Set up the multitracker.
  double correspondence_thresh = 0.01;
  double pruning_thresh = 0.5;
  double measurement_variance = 0.1;
  double position_variance = 0;
  double velocity_variance = 0.7;
  double initial_position_variance = 0.1;
  double initial_velocity_variance = 1;
  KalmanMultiTracker kmt(correspondence_thresh, pruning_thresh, measurement_variance, position_variance,
			 velocity_variance, initial_position_variance, initial_velocity_variance);

  // -- Set up matplotlib_interface for display.
  mpliBegin();
  mpli("from pylab import *");
  mpli("import time");
  mpli("delta = 0.02");
  mpli("x = arange(0, 10, delta)");
  mpli("y = arange(-1.5, 1.5, delta)");
  mpli("X, Y = meshgrid(x, y)");
  
  // -- Run the multitracker and display output.
  double delta_t = 0.1;
  double t = 0;
  while(true) {
    // Get new measurements.
    double var = 0.003;
    vector<MultiTrackerMeasurement> measurements;
    measurements.push_back(MultiTrackerMeasurement());
    measurements.back().centroid_(0) = M_PI / 2.0 + sampleFromGaussian(var);
    measurements.back().centroid_(1) = 0 + sampleFromGaussian(var);
    measurements.back().timestamp_ = t;
    measurements.push_back(MultiTrackerMeasurement());
    measurements.back().centroid_(0) = t + sampleFromGaussian(var);
    measurements.back().centroid_(1) = cos(t) + sampleFromGaussian(var);
    measurements.back().timestamp_ = t;
    measurements.push_back(MultiTrackerMeasurement());
    measurements.back().centroid_(0) = t + sampleFromGaussian(var);
    measurements.back().centroid_(1) = sin(t) + sampleFromGaussian(var);
    measurements.back().timestamp_ = t;

    if(t > M_PI / 2.0)
      measurements.pop_back();
    t += delta_t;
    
    // Run the prediction step and display.
    kmt.step(measurements, t);
    list<LinearKalmanFilter>::iterator it = kmt.filters_.begin();
    for(int i = 0; it != kmt.filters_.end(); ++it, ++i) {
      LinearKalmanFilter& kf = *it;
      mpliNamedExport("mu", kf.mu_);
      mpliNamedExport("sigma", kf.sigma_);
      mpli("Z = bivariate_normal(X, Y, sigma[0, 0], sigma[1, 1], mu[0], mu[1], sigma[0, 1])");
      mpli("CS = contour(X, Y, Z, levels=[0.001, 0.01, 0.1, 1])");
      mpli("clabel(CS, inline=1, fontsize=10)");
      mpliNamedExport("id", kf.id_);
      mpli("annotate(str(id), (mu[0], mu[1]), (mu[0]+0.2, mu[1]+0.2), arrowprops=dict(arrowstyle='->'))");
    }
    mpli("axis([0, 3, -1.5, 1.5])");
    mpli("draw()");
    mpli("waitforbuttonpress()");
    mpli("clf()");
  }

  return 0;
}
    
