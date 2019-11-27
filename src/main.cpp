#include "SDL.h"

#include <cstdint>

using u32 = uint32_t;

static const u32 FRAME_RATE = 60;
static constexpr float TICKS_PER_FRAME = 1000 / (float)FRAME_RATE;

class SDL
{
private:

  SDL_Window* window;
  SDL_Renderer* renderer;
  
  SDL_Texture* textureUI;

  bool willQuit;
  u32 ticks;


public:
  SDL() : window(nullptr), renderer(nullptr), textureUI(nullptr), willQuit(false), ticks(0)
  {

  }

  bool init();
  void deinit();
  void capFPS();
  void loop();

  void render();
  void handleEvents();

};

bool SDL::init()
{
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    return false;

  // SDL_WINDOW_FULLSCREEN
  window = SDL_CreateWindow("ODCalc v0.1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 200, SDL_WINDOW_OPENGL);
  renderer = SDL_CreateRenderer(window, -1, 0);
}

void SDL::loop()
{
  while (!willQuit)
  {
    render();
    handleEvents();

    capFPS();
  }
}

char titleBuffer[64];

void SDL::capFPS()
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

void SDL::deinit()
{
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_DestroyTexture(textureUI);

  SDL_Quit();
}

void SDL::handleEvents()
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
      if (event.key.keysym.sym == SDLK_ESCAPE)
        willQuit = true;
    }
  }
}

void SDL::render()
{

}

int main(int argc, char* argv[])
{
  SDL sdl;

  sdl.init();
  sdl.loop();
  sdl.deinit();

  return 0;
}