#include <SFML/Graphics.hpp>
#include <algorithm>
#include <optional>
#include <string>
#include <vector>
#include <cmath>

// ---------- Фізична модель ----------
struct PhysicsModel {
    float mass        = 1.0f;    // маса вантажу, кг
    float stiffness   = 50.0f;   // жорсткість пружини k, Н/м
    float equilibriumY = 200.f;  // положення рівноваги (верх кріплення + довжина)

    float y       = 120.f;       // поточна координата вантажу (верх прямокутника)
    float velocityY = 0.f;       // вертикальна швидкість

    bool running = true;

    void reset() {
        y = equilibriumY - 120.f; // початкове відхилення вгору
        velocityY = 0.f;
        running = true;
    }

    void step(float dt) {
        if (!running) return;
        // Обмеження кроку для чисельної стійкості
        dt = std::min(dt, 0.01f);

        float dy = y - equilibriumY;            // відхилення від рівноваги
        float acceleration = -(stiffness / mass) * dy; // a = -k/m * dy
        velocityY += acceleration * dt;
        y += velocityY * dt;
    }

    float period() const {
        return 2.f * 3.14159265f * std::sqrt(mass / stiffness);
    }
};

// ---------- Константи сцени ----------
const float ANCHOR_X   = 400.f;
const float ANCHOR_Y   = 40.f;
const float WEIGHT_W   = 60.f;
const float WEIGHT_H   = 60.f;

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "SFML Physics Demo - Spring Oscillator");

    // Шрифт
    sf::Font font;
    if (!font.openFromFile("../resources/fonts/arial.ttf")) {
        return 1;
    }

    PhysicsModel model;
    sf::Clock clock;

    // Траєкторія руху
    std::vector<sf::Vector2f> trajectory;
    sf::VertexArray trajectoryLine(sf::PrimitiveType::LineStrip);

    // Опорна стіна (стеля)
    sf::RectangleShape ceiling({800.f, 20.f});
    ceiling.setFillColor(sf::Color(100, 100, 100));
    ceiling.setPosition({0.f, 0.f});

    // Вантаж
    sf::RectangleShape weight({WEIGHT_W, WEIGHT_H});
    weight.setFillColor(sf::Color(200, 80, 80));

    // Пружина (лінія, що з'єднує стелю та вантаж)
    sf::VertexArray spring(sf::PrimitiveType::LineStrip);

    // Текстова інформація
    sf::Text infoText(font, "", 18);
    infoText.setFillColor(sf::Color::White);
    infoText.setPosition({20.f, 20.f});

    // ---------- Функції ----------
    auto handleInput = [&](const sf::Event& event) {
        if (event.is<sf::Event::KeyPressed>()) {
            const auto* key = event.getIf<sf::Event::KeyPressed>();
            switch (key->code) {
                case sf::Keyboard::Key::Up:
                    model.stiffness = std::clamp(model.stiffness + 5.f, 10.f, 200.f);
                    break;
                case sf::Keyboard::Key::Down:
                    model.stiffness = std::clamp(model.stiffness - 5.f, 10.f, 200.f);
                    break;
                case sf::Keyboard::Key::Right:
                    model.mass = std::clamp(model.mass + 0.1f, 0.1f, 5.f);
                    break;
                case sf::Keyboard::Key::Left:
                    model.mass = std::clamp(model.mass - 0.1f, 0.1f, 5.f);
                    break;
                case sf::Keyboard::Key::Space:
                    model.reset();
                    trajectory.clear();
                    break;
                case sf::Keyboard::Key::P:
                    model.running = !model.running;
                    break;
                case sf::Keyboard::Key::R:
                    model.reset();
                    trajectory.clear();
                    break;
                default:
                    break;
            }
        }
    };

    auto updatePhysics = [&](float dt) {
        model.step(dt);
        trajectory.push_back({ANCHOR_X, model.y});
        if (trajectory.size() > 500) {
            trajectory.erase(trajectory.begin());
        }
    };

    auto drawScene = [&]() {
        window.clear(sf::Color::Black);
        window.draw(ceiling);

        // Пружина
        spring.clear();
        float topX = ANCHOR_X;
        float topY = ANCHOR_Y;
        float botX = ANCHOR_X;
        float botY = model.y;
        const int segments = 20;
        for (int i = 0; i <= segments; ++i) {
            float t = static_cast<float>(i) / segments;
            float x = topX + (botX - topX) * t;
            float y = topY + (botY - topY) * t;
            // додаємо зигзаг
            float offset = (i % 2 == 0) ? 12.f : -12.f;
            if (i != 0 && i != segments) x += offset;
            spring.append(sf::Vertex{{x, y}, sf::Color(180, 180, 180)});
        }
        window.draw(spring);

        // Траєкторія
        trajectoryLine.clear();
        for (const auto& p : trajectory) {
            trajectoryLine.append(sf::Vertex{p, sf::Color(120, 120, 120, 150)});
        }
        window.draw(trajectoryLine);

        // Вантаж
        weight.setPosition({ANCHOR_X - WEIGHT_W / 2.f, model.y});
        window.draw(weight);

        // Текст
        std::string status = model.running ? "RUNNING" : "PAUSED";
        infoText.setString(
            "Status: " + status +
            "\nMass: " + std::to_string(model.mass) + " kg" +
            "\nStiffness: " + std::to_string(model.stiffness) + " N/m" +
            "\nVelocity: " + std::to_string(model.velocityY) +
            "\nTheoretical period T = " + std::to_string(model.period()) + " s" +
            "\n\nUp/Down - stiffness | Left/Right - mass" +
            "\nSpace - reset | P - pause/run"
        );
        window.draw(infoText);

        window.display();
    };

    // ---------- Головний цикл ----------
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            handleInput(*event);
        }

        float dt = clock.restart().asSeconds();
        updatePhysics(dt);
        drawScene();
    }
    return 0;
}