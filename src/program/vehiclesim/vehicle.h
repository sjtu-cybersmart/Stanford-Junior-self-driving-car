#ifndef VLR_VEHICLE_H
#define VLR_VEHICLE_H

#include <roadrunner.h>
#include <applanix_interface.h>
#include <can_interface.h>

namespace vlr {

  typedef struct {
  int torque_mode;
  double max_steering, max_throttle, max_brake, max_torque;
  double max_wheel_angle, max_wheel_rate;

  double steering_ratio, wheel_base, imu_to_cg_dist;
  double a, b, tire_stiffness;
  double mass, iz, tau;
  double max_steering_rate;
  double steer_inertia;

  double brake_decel_coef, throttle_accel_coef;
  int bicycle_model;
} vehicle_params;

class vehicle_state {
public:
  void set_passat_params(void);
  void reset(void);
  void update(double dt);
  void set_direction(int forward);
  void set_position(double lat, double lon, double yaw);
  void set_position(double x, double y, char *utmzone, double yaw);
  void set_velocity(double forward_vel, double yaw_rate);
  void set_controls(double steering_angle, double throttle_fraction,
		    double brake_pressure);
  void set_torque_controls(double torque, double throttle_fraction,
			   double brake_pressure);
  void fill_applanix_message(dgc::ApplanixPose *pose);
  void fill_can_message(dgc::CanStatus *can);

  vehicle_params param;

  /* offset to real world coordinates */
  double origin_x, origin_y;
  char utmzone[5];

  int paused;

  double x, y, yaw;
  double v_x, v_y, yaw_rate;
  double wheel_angle, wheel_angle_rate;
  double commanded_wheel_angle, commanded_forward_accel;
  double actual_forward_accel;
  double lateral_accel;

  double added_error_x, added_error_y;

  double torque, throttle;

  int shifting;
  double shift_timer;
  int commanded_direction, direction;
};

} // namespace vlr
#endif
