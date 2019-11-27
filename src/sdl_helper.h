#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"

#include <cstdint>
#include <cstdio>
#include <cassert>

using u32 = uint32_t;

static const u32 FRAME_RATE = 60;
static constexpr float TICKS_PER_FRAME = 1000 / (float)FRAME_RATE;

template<typename EventHandler, typename Renderer>
class SDL
{
protected:
  EventHandler& eventHandler;
  Renderer& loopRenderer;

  SDL_Window* window;
  SDL_Renderer* renderer;

  bool willQuit;
  u32 ticks;


public:
  SDL(EventHandler& eventHandler, Renderer& loopRenderer) : eventHandler(eventHandler), loopRenderer(loopRenderer), 
    window(nullptr), renderer(nullptr), willQuit(false), ticks(0)
  {

  }

  bool init();
  void deinit();
  void capFPS();
  
  void loop();
  void handleEvents();

  void exit() { willQuit = true; }

  void blit(SDL_Texture* texture, int sx, int sy, int w, int h, int dx, int dy);
  void blit(SDL_Texture* texture, int dx, int dy);
};

template<typename EventHandler, typename Renderer>
bool SDL<typename EventHandler, typename Renderer>::init()
{
  if (SDL_Init(SDL_INIT_EVERYTHING))
  {
    printf("Error on SDL_Init().\n");
    return false;
  }

  if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG)
  {
    printf("Error on IMG_Init().\n");
    return false;
  }
    
  if (TTF_Init())
  {
    printf("Error on TTF_Init().\n");
    return false;
  }
  
  // SDL_WINDOW_FULLSCREEN
#if _WIN32
  window = SDL_CreateWindow("ODCalc v0.1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 200, SDL_WINDOW_OPENGL);
#else
  window = SDL_CreateWindow("ODCalc v0.1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 200, SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN);
#endif
  renderer = SDL_CreateRenderer(window, -1, 0);

  SDL_SetRenderDrawColor(renderer, 236, 232, 228, 255);
  
  return true;
}

template<typename EventHandler, typename Renderer>
void SDL<typename EventHandler, typename Renderer>::loop()
{
  while (!willQuit)
  {
    loopRenderer.render();
    handleEvents();

    capFPS();
  }
}

template<typename EventHandler, typename Renderer>
void SDL<typename EventHandler, typename Renderer>::capFPS()
{
  u32 ticks = SDL_GetTicks();
  u32 elapsed = ticks - SDL::ticks;

  //printf("Ticks: %u, waiting %f ticks, aticks: %u\n", elapsed, TICKS_PER_FRAME - elapsed, Gfx::fticks);

  u32 frameTime = elapsed;

  if (elapsed < TICKS_PER_FRAME)
  {
    SDL_Delay(TICKS_PER_FRAME - elapsed);
    frameTime = TICKS_PER_FRAME;
  }

  SDL::ticks = SDL_GetTicks();
}

template<typename EventHandler, typename Renderer>
void SDL<typename EventHandler, typename Renderer>::deinit()
{
  TTF_Quit();
  IMG_Quit();
  
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  
  SDL_Quit();
}

template<typename EventHandler, typename Renderer>
void SDL<typename EventHandler, typename Renderer>::handleEvents()
{
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
    case SDL_QUIT:
      willQuit = true;
      break;

    case SDL_KEYDOWN:
      eventHandler.handleKeyboardEvent(event);
      break;
    }
  }
}


template<typename EventHandler, typename Renderer>
inline void SDL<typename EventHandler, typename Renderer>::blit(SDL_Texture* texture, int sx, int sy, int w, int h, int dx, int dy)
{
  SDL_Rect from = { sx, sy, w, h };
  SDL_Rect to = { dx, dy, w, h };
  SDL_RenderCopy(renderer, texture, &from, &to);
}


template<typename EventHandler, typename Renderer>
inline void SDL<typename EventHandler, typename Renderer>::blit(SDL_Texture* texture, int dx, int dy)
{
  u32 dummy;
  int dummy2;

  SDL_Rect from = { 0, 0, 0, 0 };
  SDL_Rect to = { dx, dy, 0, 0 };

  SDL_QueryTexture(texture, &dummy, &dummy2, &from.w, &from.h);

  to.w = from.w;
  to.h = from.h;

  SDL_RenderCopy(renderer, texture, &from, &to);
}