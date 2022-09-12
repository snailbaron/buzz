#pragma once

#include "sdl.hpp"

#include "tempo.hpp"
#include "thing.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

class Texture final {
public:
    Texture() {}
    Texture(SDL_Renderer* renderer, const std::filesystem::path& pathToImageFile);

    SDL_Texture* raw() const;
    const int width() const;
    const int height() const;

private:
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> _sdlTexture {nullptr, SDL_DestroyTexture};
    int _width = 0;
    int _height = 0;

    friend class View;
};

enum class AnimatedDirection {
    Down,
    Up,
    Left,
    Right,
};

enum class AnimatedMotion {
    Stand,
    Walk,
};

class StaticSprite {
public:
    StaticSprite() {}
    StaticSprite(const Texture& texture, int w, int h, int scale);

    void position(float x, float y); 
    void update(double delta);

    const SDL_Rect* frame() const;
    const SDL_FRect* targetRect() const;
    SDL_Texture* texture() const;

private:
    const Texture* _texture = nullptr;
    int _width = 0;
    int _height = 0;
    int _scale = 0;
    SDL_Rect _rect {};
    SDL_FRect _targetRect {};
};

class DirectionalSprite {
public:
    DirectionalSprite() {}
    DirectionalSprite(const Texture& texture, int w, int h, int scale);

    const SDL_Rect* frame() const;
    const SDL_FRect* targetRect() const;
    SDL_Texture* texture() const;

    void position(float x, float y);
    void velocity(float vx, float vy);
    void update(double delta);

private:
    static constexpr float _minWalkSpeed = 50.f;

    const Texture* _texture = nullptr;
    std::map<
        std::tuple<AnimatedDirection, AnimatedMotion>,
        std::vector<SDL_Rect>> _frames;
    size_t _frameCount = 0;
    int _width = 0;
    int _height = 0;
    int _scale = 0;
    size_t _currentFrameIndex = 0;
    AnimatedDirection _animatedDirection = AnimatedDirection::Down;
    AnimatedMotion _animatedMotion = AnimatedMotion::Stand;
    SDL_FRect _targetRect {};
    tempo::Metronome _metronome;
};

struct Camera {
    float x = 0;
    float y = 0;
};

class View final {
public:
    View();
    ~View();

    // hack before the introduction of events
    void addHero(thing::Entity e);
    void addTree(thing::Entity e);

    void update(const thing::EntityManager& ecs, double delta);
    void present();

private:
    SDL_Window* _window = nullptr;
    SDL_Renderer* _renderer = nullptr;

    Texture _heroTexture;
    Texture _treeTexture;
    Texture _grassTexture;

    std::map<thing::Entity, StaticSprite> _staticSprites;
    std::map<thing::Entity, DirectionalSprite> _movingSprites;

    Camera _camera;
};
