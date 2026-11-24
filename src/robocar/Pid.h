#pragma once
// PID controller with output clamping, integral anti-windup and
// derivative-on-measurement (no derivative kick on setpoint steps).

class Pid {
 public:
  struct Gains {
    float kp;
    float ki;
    float kd;
  };

  Pid(const Gains &gains, float outMin, float outMax);

  // Returns the correction for this step. dt in seconds.
  float update(float setpoint, float measured, float dt);

  void reset();
  void setGains(const Gains &gains) { gains_ = gains; }

 private:
  Gains gains_;
  float outMin_;
  float outMax_;
  float integral_ = 0.0f;
  float lastMeasured_ = 0.0f;
  bool first_ = true;
};
