#include "scene.hpp"

#include "config.hpp"
#include "world.hpp"

#include "build-info.hpp"

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

StaticSprite::StaticSprite(const Texture& texture, int w, int h, int scale)
    : _texture(&texture)
    , _width(w)
    , _height(h)
    , _scale(scale)
    , _rect(SDL_Rect{.x = 0, .y = 0, .w = w, .h = h})
    , _targetRect(SDL_FRect{
        .x = 0, .y = 0, .w = 1.f * _width * _scale, .h = 1.f * _height * _scale})
{
}

void StaticSprite::position(float x, float y)
{
    _targetRect.x = x - _width * _scale / 2.f;
    _targetRect.y = y - _height * _scale / 2.f;
}

void StaticSprite::update(double delta)
{
}

const SDL_Rect* StaticSprite::frame() const
{
    return &_rect;
}

const SDL_FRect* StaticSprite::targetRect() const
{
    return &_targetRect;
}

SDL_Texture* StaticSprite::texture() const
{
    return _texture->raw();;
}

DirectionalSprite::DirectionalSprite(
        const Texture& texture, int w, int h, int scale)
    : _texture(&texture)
{
    check(_texture->height() == h * 8);
    check(_texture->width() % w == 0);

    _frameCount = _texture->width() / w;
    _width = w;
    _height = h;
    _scale = scale;
    _targetRect = {
        .x = 0,
        .y = 0,
        .w = 1.f * _width * _scale,
        .h = 1.f * _height * _scale
    };

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

const SDL_FRect* DirectionalSprite::targetRect() const
{
    return &_targetRect;
}

SDL_Texture* DirectionalSprite::texture() const
{
    return _texture->raw();
}

void DirectionalSprite::position(float x, float y)
{
    _targetRect.x = x - _width * _scale / 2.f;
    _targetRect.y = y - _height * _scale / 2.f;
}

void DirectionalSprite::velocity(float vx, float vy)
{
    auto heroSpeed = std::sqrt(vx * vx + vy * vy);

    if (heroSpeed > 0) {
        if (1.5 * std::abs(vx) > std::abs(vy)) {
            if (vx > 0) {
                _animatedDirection = AnimatedDirection::Right;
            } else {
                _animatedDirection = AnimatedDirection::Left;
            }
        } else {
            if (vy > 0) {
                _animatedDirection = AnimatedDirection::Up;
            } else {
                _animatedDirection = AnimatedDirection::Down;
            }
        }
    }

    if (heroSpeed < _minWalkSpeed) {
        _animatedMotion = AnimatedMotion::Stand;
    } else {
        _animatedMotion = AnimatedMotion::Walk;
    }
}

void DirectionalSprite::update(double delta)
{
    int ticks = _metronome.ticks(delta);
    _currentFrameIndex = (_currentFrameIndex + ticks) % _frameCount;
}

View::View()
{
    sdlCheck(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS));
    check(IMG_Init(IMG_INIT_PNG) == IMG_INIT_PNG);

    _window = sdlCheck(SDL_CreateWindow(
        config().windowTitle.c_str(),
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        config().windowWidth,
        config().windowHeight,
        0));

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

void View::addHero(thing::Entity e)
{
    _movingSprites.emplace(e, DirectionalSprite{_heroTexture, 16, 16, 10});
}

void View::addTree(thing::Entity e)
{
    _staticSprites.emplace(e, StaticSprite{_treeTexture, 8, 16, 10});
}

void View::update(const thing::EntityManager& ecs, double delta)
{
    for (auto& [e, sprite] : _staticSprites) {
        const auto& position = ecs.component<WorldLocation>(e).position;
        sprite.position(position.x, -position.y);
        sprite.update(delta);
    }

    for (auto& [e, sprite] : _movingSprites) {
        const auto& position = ecs.component<WorldLocation>(e).position;
        const auto& velocity = ecs.component<WorldMovement>(e).velocity;

        sprite.position(position.x, -position.y);
        sprite.velocity(velocity.x, velocity.y);
        sprite.update(delta);
    }
}

void View::present()
{
    sdlCheck(SDL_SetRenderDrawColor(_renderer, 50, 50, 50, 255));
    sdlCheck(SDL_RenderClear(_renderer));

    for (const auto& [e, sprite] : _staticSprites) {
        sdlCheck(SDL_RenderCopyF(
            _renderer,
            sprite.texture(),
            sprite.frame(),
            sprite.targetRect()));
    }

    for (const auto& [e, sprite] : _movingSprites) {
        sdlCheck(SDL_RenderCopyF(
            _renderer,
            sprite.texture(),
            sprite.frame(),
            sprite.targetRect()));
    }

    SDL_RenderPresent(_renderer);
}
