#include "CanvasImpl.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include "Canvas.h"

#ifdef __EMSCRIPTEN__
namespace {
constexpr const char* CanvasSelector = "#canvas";
}
#endif

CanvasImpl::CanvasImpl(Canvas* canvas)
{
    this->canvas = canvas;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        throw std::runtime_error(SDL_GetError());
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        throw std::runtime_error(IMG_GetError());
    }

    if (TTF_Init() == -1) {
        throw std::runtime_error(TTF_GetError());
    }

#ifdef __EMSCRIPTEN__
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
#endif

    window = SDL_CreateWindow(
        0,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width, height,
        SDL_WINDOW_SHOWN);

    if (!window) {
        throw std::runtime_error(SDL_GetError());
    }

#ifdef __EMSCRIPTEN__
    renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_SOFTWARE);
#else
    renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED);
#endif

    if (!renderer) {
        throw std::runtime_error(SDL_GetError());
    }

#ifdef __EMSCRIPTEN__
    beginFrame();
#endif

    if (SDL_RenderSetLogicalSize(renderer, width, height) != 0) {
        throw std::runtime_error(SDL_GetError());
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
}

CanvasImpl::~CanvasImpl()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    IMG_Quit();
    TTF_Quit();
}

void CanvasImpl::beginFrame()
{
#ifdef __EMSCRIPTEN__
    double cssWidth = 0.0;
    double cssHeight = 0.0;
    if (emscripten_get_element_css_size(CanvasSelector, &cssWidth, &cssHeight) != EMSCRIPTEN_RESULT_SUCCESS) {
        return;
    }

    if (cssWidth <= 0.0 || cssHeight <= 0.0) {
        return;
    }

    const double resolutionScale = EM_ASM_DOUBLE({
        const scale = Number(Module.gravityDefiedResolutionScale) || 1;
        return Math.min(2, Math.max(0.75, scale));
    });
    const int pixelWidth = std::max(1, static_cast<int>(std::lround(cssWidth * resolutionScale)));
    const int pixelHeight = std::max(1, static_cast<int>(std::lround(cssHeight * resolutionScale)));

    int currentWidth = 0;
    int currentHeight = 0;
    SDL_GetWindowSize(window, &currentWidth, &currentHeight);

    if (currentWidth != pixelWidth || currentHeight != pixelHeight) {
        SDL_SetWindowSize(window, pixelWidth, pixelHeight);
        emscripten_set_canvas_element_size(CanvasSelector, pixelWidth, pixelHeight);
        SDL_RenderSetLogicalSize(renderer, width, height);
    }
#endif
}

void CanvasImpl::repaint()
{
    SDL_RenderPresent(renderer);
}

int CanvasImpl::getWidth()
{
    return width;
}

int CanvasImpl::getHeight()
{
    return height;
}

SDL_Renderer* CanvasImpl::getRenderer()
{
    return renderer;
}

void CanvasImpl::processEvents()
{
    SDL_Event e;

    while (SDL_PollEvent(&e) != 0) {
        switch (e.type) {
        case SDL_QUIT:
            exit(0); // IMPROVE This is a super dumb way to finish the game, but it works
            break;
        case SDL_KEYDOWN: {
            int keyCode = convertKeyCharToKeyCode(e.key.keysym.sym);
            // std::cout << "Key pressed: " << keyCode << std::endl;
            if (keyCode != 0) {
                canvas->publicKeyPressed(keyCode);
            }
        } break;
        case SDL_KEYUP: {
            int sdlCode = e.key.keysym.sym;
            int keyCode = convertKeyCharToKeyCode(sdlCode);
            // std::cout << "Key released: " << keyCode << std::endl;
            if (keyCode != 0) {
                canvas->publicKeyReleased(keyCode);
            } else {
                if (sdlCode == SDLK_ESCAPE) {
                    // std::cout << "ESC released" << std::endl;
                    canvas->pressedEsc();
                }
            }
        } break;
        default:
            break;
        }
    }
}

int CanvasImpl::convertKeyCharToKeyCode(SDL_Keycode keyCode)
{
    switch (keyCode) {
    case SDLK_RETURN:
        return Canvas::Keys::FIRE;
    case SDLK_LEFT:
        return Canvas::Keys::LEFT;
    case SDLK_RIGHT:
        return Canvas::Keys::RIGHT;
    case SDLK_UP:
        return Canvas::Keys::UP;
    case SDLK_DOWN:
        return Canvas::Keys::DOWN;
    default:
        std::cout << "unknown keyEvent: " << keyCode << std::endl;
        return 0;
    }
}

void CanvasImpl::setWindowTitle(const std::string& title)
{
    SDL_SetWindowTitle(window, title.c_str());
}
