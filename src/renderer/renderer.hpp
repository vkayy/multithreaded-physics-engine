#pragma once

#include <iostream>

#include "../physics/solver.hpp"

constexpr float OUTLINE_THICKNESS = 0.0f;

class Renderer {
public:
    explicit
    Renderer(sf::RenderTarget &target)
        : target{target}
    {}
    void render(const Solver &solver) const {
        sf::CircleShape circle{1.0f};
        circle.setPointCount(32);
        circle.setOrigin(1.0f, 1.0f);
        int32_t object_id = 0;
        const auto &objects = solver.objects;
        for (const auto &object : objects) {
            if (object.hidden) continue;
            circle.setPosition(object.curr_position);
            circle.setScale(object.radius, object.radius);
            circle.setFillColor(object.colour);
            circle.setOutlineColor(sf::Color::Black);
            circle.setOutlineThickness(-OUTLINE_THICKNESS / object.radius);
            target.draw(circle);
            object_id++;
        }

        sf::Vertex constraint_line[2];
        const auto &constraints = solver.constraints;
        for (const auto &constraint : constraints) {
            if (constraint.in_body) continue;
            constraint_line[0].position = constraint.object_1.curr_position;
            constraint_line[1].position = constraint.object_2.curr_position;
            constraint_line[0].color = sf::Color::Black;
            constraint_line[1].color = sf::Color::Black;
            target.draw(constraint_line, 2, sf::Lines);
        }

        const auto &soft_bodies = solver.soft_bodies;
        for (const auto &soft_body : soft_bodies) {
            sf::Vertex polygon[soft_body.points];
            for (int32_t i=0; i<soft_body.points; i++) {
                polygon[i].position = soft_body.vertices[i]->curr_position;
                polygon[i].color = soft_body.vertices[i]->colour;
            }
            target.draw(polygon, soft_body.points, sf::TriangleFan);
        }

        const auto &rigid_bodies = solver.rigid_bodies;
        for (const auto &rigid_body : rigid_bodies) {
            sf::Vertex polygon[rigid_body.points];
            for (int32_t i=0; i<rigid_body.points; i++) {
                polygon[i].position = rigid_body.vertices[i]->curr_position;
                polygon[i].color = rigid_body.vertices[i]->colour;
            }
            target.draw(polygon, rigid_body.points, sf::TriangleFan);
        }
    }
private:
    sf::RenderTarget &target;
};
