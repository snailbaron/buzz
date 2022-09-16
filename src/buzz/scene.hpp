#pragma once

#include "sdl.hpp"
#include "types.hpp"

#include "evening.hpp"
#include "tempo.hpp"
#include "thing.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
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

class Sprite {
public:
    virtual ~Sprite() {}

    virtual SDL_Texture* texture() const = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual const SDL_Rect* frame() const = 0;

    virtual void velocity(const XYVector& velocity) {}
    virtual void update(double delta) {}
};

class AnimatedSprite : public Sprite {
public:
    AnimatedSprite(SDL_Texture* texture, int w, int h, int frames = 1);

    SDL_Texture* texture() const override { return _texture; }
    int width() const override { return _width; }
    int height() const override { return _height; }
    const SDL_Rect* frame() const override { return &_frames.at(_currentFrameIndex); }

    void update(double delta) override;

private:
    SDL_Texture* _texture = nullptr;
    int _width = 0;
    int _height = 0;
    std::vector<SDL_Rect> _frames;
    size_t _currentFrameIndex = 0;
    tempo::Metronome _metronome {3};
};

class DirectionalSprite : public Sprite {
public:
    DirectionalSprite(SDL_Texture* texture, int w, int h, int frames = 1);

    SDL_Texture* texture() const override { return _texture; }
    int width() const override { return _width; }
    int height() const override { return _height; }
    const SDL_Rect* frame() const override;

    void velocity(const XYVector& velocity) override;
    void update(double delta) override;

private:
    enum class AnimatedDirection {Down, Up, Left, Right};
    enum class AnimatedMotion {Stand, Walk};

    static constexpr float _minWalkSpeed = 3.f;

    SDL_Texture* _texture = nullptr;
    int _width = 0;
    int _height = 0;

    std::map<
        std::tuple<AnimatedDirection, AnimatedMotion>,
        std::vector<SDL_Rect>> _frames;
    size_t _frameCount = 0;
    size_t _currentFrameIndex = 0;
    AnimatedDirection _animatedDirection = AnimatedDirection::Down;
    AnimatedMotion _animatedMotion = AnimatedMotion::Stand;
    tempo::Metronome _metronome {3};
};

struct Camera {
    SDL_FRect rect(const XYVector& position, int spriteWidth, int spriteHeight)
    {
        return {
            .x = (position.x - center.x) * pixelsPerUnit * scale + screenSize.x / 2.f - spriteWidth * scale / 2.f,
            .y = (center.y - position.y) * pixelsPerUnit * scale + screenSize.y / 2.f - spriteHeight * scale / 2.f,
            .w = 1.f * spriteWidth * scale,
            .h = 1.f * spriteHeight * scale,
        };
    }

    int pixelsPerUnit = 0;
    int scale = 0;
    ScreenVector screenSize;
    XYVector center;
};

struct KeyboardControllerState {
    bool leftPressed = false;
    bool rightPressed = false;
    bool upPressed = false;
    bool downPressed = false;

    XYVector control() const
    {
        return XYVector{
            1.f * rightPressed - leftPressed,
            1.f * upPressed - downPressed};
    }
};

class View final : public evening::Subscriber {
public:
    View(evening::Channel& worldEvents, evening::Channel& controlEvents);
    ~View();

    bool processEvents();
    void update(double delta);
    void present();

private:
    struct SpriteAndPosition {
        std::unique_ptr<Sprite> sprite;
        XYVector position;
    };

    void layTexture(const Texture& texture);

    SDL_Window* _window = nullptr;
    SDL_Renderer* _renderer = nullptr;

    Texture _heroTexture;
    Texture _treeTexture;
    Texture _grassTexture;

    std::map<thing::Entity, SpriteAndPosition> _sprites;

    evening::Channel& _controlEvents;
    Camera _camera;
    KeyboardControllerState _controller;
    std::optional<thing::Entity> _focusEntity;
    XYVector _focusPosition;
};
