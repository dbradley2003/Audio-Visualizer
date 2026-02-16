#ifndef PARTICLE_H
#define PARTICLE_H

#include "raylib.h"

class Particle {
public:
  Particle(const Vector2 pos, const Vector2 velocity, const float size,
           const float alpha, const float pulse, const Color base,
           const float id)
      : pos_(pos), velocity_(velocity), size_(size), alpha_(alpha),
        pulse_(pulse), base_(base), id_(id) {}

  [[nodiscard]] Vector2 position() const { return pos_; }
  [[nodiscard]] Vector2 velocity() const { return velocity_; }
  [[nodiscard]] float size() const { return size_; }
  [[nodiscard]] float alpha() const { return alpha_; }
  [[nodiscard]] float pulse() const { return pulse_; }
  [[nodiscard]] Color base() const { return base_; }
  [[nodiscard]] float id() const { return id_; }

  void setPosition(const Vector2 pos) { pos_ = pos; }
  void setVelocity(const Vector2 velocity) { velocity_ = velocity; }
  void setSize(const float size) { size_ = size; }
  void setAlpha(const float alpha) { alpha_ = alpha; }
  void setPulse(const float pulse) { pulse_ = pulse; }
  void setBase(const Color base) { base_ = base; }
  void setId(const float id) { id_ = id; }

private:
  Vector2 pos_;
  Vector2 velocity_;
  float size_;
  float alpha_;
  float pulse_;
  Color base_;
  float id_;
};

#endif
