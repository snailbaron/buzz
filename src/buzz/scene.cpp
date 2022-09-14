#include "scene.hpp"

#include "config.hpp"
#include "events.hpp"
#include "world.hpp"

#include "build-info.hpp"

#include <iostream>

namespace {

constexpr int PixelScale = 10;
constexpr int PixelsInUnit = 8;

} // namespace

Texture::Texture(
        SDL_Renderer* renderer, const std::filesystem::path& pathToImageFile)
{
    _sdlTexture.reset(
        sdlCheck(IMG_LoadTexture(renderer, pathToImageFile.c_str())));
    sdlCheck(SDL_QueryTexture(_sdlTexture.get(), nullptr, nullptr, &_width, &_height));
}

SDL_Texture* Texture::raw() const
{
    return _sdlTexture.get();
}

const int Texture::width() const
{
    return _width;
}

const int Texture::height() const
{
    return _height;
}

AnimatedSprite::AnimatedSprite(SDL_Texture* texture, int w, int h, int frames)
    : _texture(texture)
    , _width(w)
    , _height(h)
{
    for (int i = 0; i < frames; i++) {
        _frames.push_back({.x = i * w, .y = 0, .w = w, .h = h});
    }
}

void AnimatedSprite::update(double delta)
{
    _currentFrameIndex =
        (_currentFrameIndex + _metronome.ticks(delta)) % _frames.size();
}

DirectionalSprite::DirectionalSprite(
        SDL_Texture* texture, int w, int h, int frames)
    : _texture(texture)
    , _width(w)
    , _height(h)
    , _frameCount(frames)
{
    for (auto animatedMotion : {AnimatedMotion::Walk, AnimatedMotion::Stand}) {
        for (auto animatedDirection : {
                AnimatedDirection::Right,
                AnimatedDirection::Left,
                AnimatedDirection::Down,
                AnimatedDirection::Up}) {
            int i = _frames.size();
            std::vector<SDL_Rect> rs;
            for (int j = 0; j < _frameCount; j++) {
                rs.push_back(SDL_Rect{.x = w * j, .y = h * i, .w = w, .h = h});
            }
            _frames.emplace(
                std::tuple{animatedDirection, animatedMotion}, std::move(rs));
        }
    }
}

const SDL_Rect* DirectionalSprite::frame() const
{
    return &_frames.at({_animatedDirection, _animatedMotion})
        .at(_currentFrameIndex);
}

void DirectionalSprite::velocity(const XYVector& velocity)
{
    auto speed = length(velocity);

    if (speed > 0) {
        if (1.5 * std::abs(velocity.x) > std::abs(velocity.y)) {
            if (velocity.x > 0) {
                _animatedDirection = AnimatedDirection::Right;
            } else {
                _animatedDirection = AnimatedDirection::Left;
            }
        } else {
            if (velocity.y > 0) {
                _animatedDirection = AnimatedDirection::Up;
            } else {
                _animatedDirection = AnimatedDirection::Down;
            }
        }
    }

    if (speed < _minWalkSpeed) {
        _animatedMotion = AnimatedMotion::Stand;
    } else {
        _animatedMotion = AnimatedMotion::Walk;
    }
}

void DirectionalSprite::update(double delta)
{
    _currentFrameIndex =
        (_currentFrameIndex + _metronome.ticks(delta)) % _frameCount;
}

View::View(evening::Channel& worldEvents)
    : _camera({.pixelsPerUnit = PixelsInUnit, .scale = PixelScale})
{
    subscribe<Spawn>(worldEvents, [this] (const auto& spawn) {
        switch (spawn.objectType) {
            case ObjectType::Hero:
                _sprites.emplace(
                    spawn.entity,
                    SpriteAndPosition{
                        .sprite = std::make_unique<DirectionalSprite>(
                            _heroTexture.raw(), 16, 16, 2),
                        .position = spawn.location,
                    });
                _focusEntity = spawn.entity;
                _focusPosition = spawn.location;
                break;
            case ObjectType::Tree:
                _sprites.emplace(
                    spawn.entity,
                    SpriteAndPosition{
                        .sprite = std::make_unique<AnimatedSprite>(
                            _treeTexture.raw(), 8, 16),
                        .position = spawn.location,
                    });
                break;
        }
    });

    subscribe<Move>(worldEvents, [this] (const auto& move) {
        if (auto it = _sprites.find(move.entity); it != _sprites.end()) {
            it->second.position = move.location;
            it->second.sprite->velocity(move.velocity);
        }

        if (move.entity == _focusEntity) {
            _focusPosition = move.location;
        }
    });

    sdlCheck(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS));
    check(IMG_Init(IMG_INIT_PNG) == IMG_INIT_PNG);

    _window = sdlCheck(SDL_CreateWindow(
        config().windowTitle.c_str(),
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        config().windowWidth,
        config().windowHeight,
        0));

    _camera.screenSize = {config().windowWidth, config().windowHeight};

    _renderer = sdlCheck(SDL_CreateRenderer(
        _window,
        0,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));

    _heroTexture = Texture(_renderer, SOURCE_ROOT / "assets" / "hero.png");
    _treeTexture = Texture(_renderer, SOURCE_ROOT / "assets"/ "tree.png");
    _grassTexture = Texture(_renderer, SOURCE_ROOT / "assets" / "grass.png");
}

View::~View()
{
    SDL_DestroyRenderer(_renderer);
    SDL_DestroyWindow(_window);

    IMG_Quit();
    SDL_Quit();
}

void View::update(double delta)
{
    for (auto& [e, spriteAndPosition] : _sprites) {
        spriteAndPosition.sprite->update(delta);
    }

    if (_focusEntity) {
        auto v = _focusPosition - _camera.center;
        float distance = length(_camera.center - _focusPosition);
        float advance = 8.f * distance * delta + 1.f * delta;
        if (advance < distance) {
            _camera.center += v * advance / distance;
        } else {
            _camera.center = _focusPosition;
        }
    }
}

void View::present()
{
    sdlCheck(SDL_SetRenderDrawColor(_renderer, 50, 50, 50, 255));
    sdlCheck(SDL_RenderClear(_renderer));

    for (const auto& [e, spriteAndPosition] : _sprites) {
        const auto& [sprite, position] = spriteAndPosition;
        SDL_FRect targetRect =
            _camera.rect(position, sprite->width(), sprite->height());
        sdlCheck(SDL_RenderCopyF(
            _renderer, sprite->texture(), sprite->frame(), &targetRect));
    }

    SDL_RenderPresent(_renderer);
}
