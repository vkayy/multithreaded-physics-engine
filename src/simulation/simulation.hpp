#pragma once

#include <queue>

#include <SFML/Graphics.hpp>

#include "../utils/maths.hpp"
#include "../physics/solver.hpp"
#include "../renderer/renderer.hpp"
#include "../thread_pool/thread_pool.hpp"

struct SpawnTask {
    bool linked;
    std::pair<float, float> position;
    float speed;
    float delay;
    float angle;
    float min_radius;
    float max_radius;
    float target_distance;

    SpawnTask(
        bool linked,
        std::pair<float, float> position,
        float speed,
        float delay,
        float angle,
        float min_radius,
        float max_radius,
        float target_distance
    )
    : linked{linked}
    , position{position}
    , speed{speed}
    , delay{delay}
    , angle{angle}
    , min_radius{min_radius}
    , max_radius{max_radius}
    , target_distance{target_distance}
    {}
};

struct Simulation {
    Simulation(
        bool render_display,
        int32_t window_width,
        int32_t window_height,
        float min_radius,
        float max_radius,
        float max_angle,
        bool speed_colouring,
        int32_t framerate_limit,
        int32_t thread_count,
        int32_t substeps,
        int8_t collision_resolver,
        bool gravity_on,
        std::string name
    )
    : render_display{render_display}
    , window_width{window_width}
    , min_radius{min_radius}
    , max_radius{max_radius}
    , max_angle{max_angle}
    , speed_colouring{speed_colouring}
    , framerate_limit{framerate_limit}
    , thread_count{thread_count}
    , substeps{substeps}
    , collision_resolver{collision_resolver}
    , gravity_on{gravity_on}
    , thread_pool{tp::ThreadPool(thread_count)}
    , window{
        sf::VideoMode(window_width, window_height),
        name,
        sf::Style::Default,
    }
    , cell_size{
        collision_resolver == 2 && window_width / 2 / max_radius / thread_count < 2
        ? window_width / thread_count / 2
        : max_radius * 2
    }
    , solver{
        sf::Vector2f(window_width, window_height),
        substeps,
        cell_size,
        framerate_limit,
        speed_colouring,
        thread_pool,
        gravity_on
    }
    , renderer{window}
    {}

public:

    void enqueueSpawn(
        bool linked,
        int32_t count,
        std::pair<float, float> spawn_position,
        float spawn_speed,
        float spawn_delay,
        float spawn_angle,
        float min_radius,
        float max_radius,
        float target_distance
    ) {
        for (int i=0; i<count; i++) {
            spawn_queue.emplace(
                linked,
                spawn_position,
                spawn_speed,
                spawn_delay,
                spawn_angle,
                min_radius,
                max_radius,
                target_distance
            );
        }
    }

    void run() {
        while (window.isOpen()) {
            handleWindowEvents();
            if (!spawn_queue.empty()) {
                dequeueSpawn();
            }
            update();
            handleRender();
        }
    }

private:

    bool render_display;
    int32_t window_width;
    int32_t window_height;
    float min_radius;
    float max_radius;
    float max_angle;
    bool speed_colouring;
    float cell_size;
    int32_t framerate_limit;
    int32_t thread_count;
    int32_t substeps;
    int8_t collision_resolver;
    bool gravity_on;
    sf::RenderWindow window;
    tp::ThreadPool thread_pool;
    Solver solver;
    Renderer renderer;
    sf::Clock clock;
    RNG<float> rng;
    VerletObject *last_linked;
    std::queue<SpawnTask> spawn_queue;

    static sf::Color getRainbowColour(float time) {
        const float r = sin(time);
        const float g = sin(time + 0.33f * 2.0f * M_PI);
        const float b = sin(time + 0.66f * 2.0f * M_PI);
        return {
            static_cast<uint8_t>(255.0f * r * r),
            static_cast<uint8_t>(255.0f * g * g),
            static_cast<uint8_t>(255.0f * b * b)
        };
    }

    void handleWindowEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
                window.close();
            } else {
                solver.setAttractor(sf::Keyboard::isKeyPressed(sf::Keyboard::A));
                solver.setRepeller(sf::Keyboard::isKeyPressed(sf::Keyboard::R));
                solver.setSpeedUp(sf::Keyboard::isKeyPressed(sf::Keyboard::S));
                solver.setSlowDown(sf::Keyboard::isKeyPressed(sf::Keyboard::W));
                solver.setSlomo(sf::Keyboard::isKeyPressed(sf::Keyboard::F));
            }
        }
    }

    void dequeueSpawn() {
        SpawnTask current_spawn = spawn_queue.front();
        if (clock.getElapsedTime().asSeconds() >= current_spawn.delay) {
            clock.restart();
            VerletObject &object = solver.addObject(
                {
                    current_spawn.position.first * window_width,
                    current_spawn.position.second * window_height
                },
                rng.getRange(current_spawn.min_radius, current_spawn.max_radius)
            );
            const float time = solver.time;
            const float angle = current_spawn.angle;
            object.colour = getRainbowColour(time);
            solver.setObjectVelocity(
                object,
                current_spawn.speed * sf::Vector2f{cos(angle), sin(angle)}
            );
            // if (current_spawn.linked && last_linked) {
            //     VerletLink &link = solver.addLink(
            //         *last_linked,
            //         object,
            //         current_spawn.target_distance
            //     );
            // }
            // if (current_spawn.linked) {
            //     last_linked = &object;
            // }
            spawn_queue.pop();
        }
    }

    void update() {
        switch (collision_resolver) {
            case 0: solver.updateThreaded(); break;
            case 1: solver.updateCellular(); break;
            case 2: solver.updateNaive(); break;
            default: solver.updateThreaded();
        }
    }

    void handleRender() {
        window.clear(sf::Color::White);
        renderer.render(solver);
        window.display();
    }
};
