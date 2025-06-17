#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <ranges>
#include <algorithm>
#include <thread>  
#include <chrono>
struct Vec2 {  
    float x, y;  
    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }  
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }  
    Vec2 operator*(float s) const { return {x * s, y * s}; }  
    Vec2 operator/(float s) const { return {x / s, y / s}; }  
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }  
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; } // <-- THIS LINE IS CRUCIAL  
    float length() const { return std::sqrt(x * x + y * y); }  
    Vec2 normalized() const { float l = length(); return l > 0 ? (*this) / l : Vec2{0, 0}; }  
};  
struct Boid {
    Vec2 position;
    Vec2 velocity;
};
constexpr int NUM_BOIDS = 30;
constexpr float WIDTH = 80.0f, HEIGHT = 24.0f;
constexpr float MAX_SPEED = 1.0f;
constexpr float NEIGHBOR_RADIUS = 5.0f;
constexpr float SEPARATION_RADIUS = 2.0f;
void limit_speed(Vec2& v, float max_speed) {
    if (v.length() > max_speed) v = v.normalized() * max_speed;
}
void update_boids(std::vector<Boid>& boids) {
    std::vector<Boid> new_boids = boids;
    for (size_t i = 0; i < boids.size(); ++i) {
        Vec2 separation{0, 0}, alignment{0, 0}, cohesion{0, 0};
        int count = 0, sep_count = 0;
        for (size_t j = 0; j < boids.size(); ++j) {
            if (i == j) continue;
            float dist = (boids[j].position - boids[i].position).length();
            if (dist < NEIGHBOR_RADIUS) {
                alignment += boids[j].velocity;
                cohesion += boids[j].position;
                ++count;
                if (dist < SEPARATION_RADIUS) {
                    separation -= (boids[j].position - boids[i].position);
                    ++sep_count;
                }
            }
        }
        Vec2 v = boids[i].velocity;
        if (count > 0) {
            alignment = (alignment / static_cast<float>(count)).normalized();
            cohesion = ((cohesion / static_cast<float>(count)) - boids[i].position).normalized();
        }
        if (sep_count > 0) {
            separation = (separation / static_cast<float>(sep_count)).normalized();
        }
        // Weights for each rule
        v += separation * 1.5f + alignment * 1.0f + cohesion * 1.0f;
        limit_speed(v, MAX_SPEED);
        // Move and wrap around screen
        Vec2 pos = boids[i].position + v;
        if (pos.x < 0) pos.x += WIDTH;
        if (pos.x >= WIDTH) pos.x -= WIDTH;
        if (pos.y < 0) pos.y += HEIGHT;
        if (pos.y >= HEIGHT) pos.y -= HEIGHT;
        new_boids[i].position = pos;
        new_boids[i].velocity = v;
    }
    boids = new_boids;
}
void print_boids(const std::vector<Boid>& boids) {
    std::vector<std::string> grid(static_cast<size_t>(HEIGHT), std::string(static_cast<size_t>(WIDTH), ' '));
    for (const auto& b : boids) {
        int x = static_cast<int>(b.position.x);
        int y = static_cast<int>(b.position.y);
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
            grid[y][x] = '*';
    }
    for (const auto& row : grid) std::cout << row << '\n';
}
int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xdist(0, WIDTH), ydist(0, HEIGHT), vdist(-1, 1);
    std::vector<Boid> boids;
    for (int i = 0; i < NUM_BOIDS; ++i) {
        boids.push_back({{xdist(gen), ydist(gen)}, {vdist(gen), vdist(gen)}});
    }
    for (int step = 0; step < 200; ++step) {
        std::system("clear"); // or "cls" on Windows
        print_boids(boids);
        update_boids(boids);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}


