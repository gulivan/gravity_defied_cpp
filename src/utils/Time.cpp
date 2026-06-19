#include "Time.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <chrono>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace Time {
int64_t currentTimeMillis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
}

void sleep(int64_t ms)
{
#ifdef __EMSCRIPTEN__
    emscripten_sleep(static_cast<unsigned int>(std::max<int64_t>(ms, 0)));
#else
    SDL_Delay(ms);
#endif
}
}
