#pragma once

#include <SFML/Graphics.hpp>

constexpr float DEFAULT_RADIUS = 10.0f;
constexpr float COLOUR_COEFFICIENT = 0.0015f;
constexpr float DAMPING_FACTOR = 0.9999f;

struct VerletObject {
  sf::Vector2f curr_position = {0.0f, 0.0f};
  sf::Vector2f last_position = {0.0f, 0.0f};
  ;
  sf::Vector2f acceleration = {0.0f, 0.0f};
  sf::Color colour = sf::Color::Red;
  float radius = DEFAULT_RADIUS;
  bool hidden = false;
  bool fixed = false;

  VerletObject() = default;
  VerletObject(sf::Vector2f pos, float radius, bool fixed)
      : curr_position{pos}, last_position{pos}, acceleration{0.0f, 0.0f},
        radius{radius}, fixed{fixed} {}

  void updatePosition(float dt) {
    const sf::Vector2f displacement =
        (curr_position - last_position) * DAMPING_FACTOR;
    last_position = curr_position;
    curr_position = curr_position + displacement + acceleration * dt * dt;
    acceleration = {};
  }

  void updateColour(float dt) {
    const sf::Vector2f velocity = getVelocity(dt);
    const float colour_theta =
        COLOUR_COEFFICIENT *
        sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    const float r = sin(colour_theta);
    const float g = sin(colour_theta + 0.33f * 2.0f * M_PI);
    const float b = sin(colour_theta + 0.66f * 2.0f * M_PI);
    this->colour = {static_cast<uint8_t>(255.0f * r * r),
                    static_cast<uint8_t>(255.0f * g * g),
                    static_cast<uint8_t>(255.0f * b * b)};
  }

  void accelerate(sf::Vector2f a) { acceleration += a; }

  void addVelocity(sf::Vector2f v, float dt) { last_position -= v * dt; }

  void setVelocity(sf::Vector2f v, float dt) {
    last_position = curr_position - v * dt;
  }

  sf::Vector2f getVelocity(float dt) {
    return (curr_position - last_position) / dt;
  }
};

struct VerletConstraint {
  VerletObject &object_1;
  VerletObject &object_2;
  float target_distance;
  bool in_body;

  VerletConstraint(VerletObject &object_1, VerletObject &object_2,
                   float target_distance)
      : object_1{object_1}, object_2{object_2},
        target_distance{target_distance} {}

  void apply() {
    if (object_1.fixed && object_2.fixed)
      return;
    const sf::Vector2f displacement =
        object_1.curr_position - object_2.curr_position;
    const float distance =
        sqrt(displacement.x * displacement.x + displacement.y * displacement.y);
    const sf::Vector2f normal = displacement / distance;
    const float delta = target_distance - distance;
    if (object_1.fixed && !object_2.fixed) {
      object_2.curr_position -= delta * normal;
    } else if (!object_1.fixed && object_2.fixed) {
      object_1.curr_position += delta * normal;
    } else {
      object_1.curr_position += 0.5f * delta * normal;
      object_2.curr_position -= 0.5f * delta * normal;
    }
  }
};

struct VerletSoftBody {
  std::vector<VerletObject *> vertices;
  std::vector<VerletConstraint *> segments;
  int32_t points;
  float desired_area;

  VerletSoftBody(std::vector<VerletObject *> vertices,
                 std::vector<VerletConstraint *> segments, float radius)
      : vertices{vertices}, segments{segments} {
    points = vertices.size();
    desired_area = M_PI * radius * radius;
  }

  void apply() {
    float current_area;
    float area = 0.0f;
    int32_t points = vertices.size();
    for (int32_t i = 0; i < points; i++) {
      const sf::Vector2f &vertex1 = vertices[i]->curr_position;
      const sf::Vector2f &vertex2 = vertices[(i + 1) % points]->curr_position;
      area += vertex1.x * vertex2.y - vertex2.x * vertex1.y;
    }
    current_area = std::abs(area) / 2.0f;

    float area_error = desired_area - current_area;
    float delta = area_error / (points * 2.0f);
    for (int32_t i = 0; i < points; i++) {
      int32_t prev_idx = (i == 0) ? points - 1 : i - 1;
      int32_t next_idx = (i == points - 1) ? 0 : i + 1;
      sf::Vector2f prev_point = vertices[prev_idx]->curr_position;
      sf::Vector2f next_point = vertices[next_idx]->curr_position;
      sf::Vector2f normal = next_point - prev_point;
      normal = sf::Vector2f(-normal.y, normal.x);
      normal /= sqrt(normal.x * normal.x + normal.y * normal.y);
      vertices[i]->curr_position += 0.01f * normal * delta;
    }
  }
};

struct VerletRigidBody {
  std::vector<VerletObject *> vertices;
  std::vector<VerletConstraint *> segments;
  float side_length;
  int32_t points;

  VerletRigidBody(std::vector<VerletObject *> vertices,
                  std::vector<VerletConstraint *> segments, float side_length)
      : vertices{vertices}, segments{segments}, side_length{side_length} {
    points = vertices.size();
  }
};
