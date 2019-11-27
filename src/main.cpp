#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"

#include <cstdint>
#include <cstdio>
#include <cassert>

using u32 = uint32_t;

static const u32 FRAME_RATE = 60;
static constexpr float TICKS_PER_FRAME = 1000 / (float)FRAME_RATE;

class SDL
{
private:

  SDL_Window* window;
  SDL_Renderer* renderer;
  
  SDL_Texture* textureUI;
  TTF_Font* font;

  bool willQuit;
  u32 ticks;


public:
  SDL() : window(nullptr), renderer(nullptr), textureUI(nullptr), font(nullptr),
  willQuit(false), ticks(0)
  {

  }

  bool init();
  void deinit();
  void capFPS();
  bool loadData();
  
  void loop();
  void render();
  void handleEvents();


  void blit(SDL_Texture* texture, int dx, int dy);
};

bool SDL::init()
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

void SDL::loop()
{
  while (!willQuit)
  {
    render();
    handleEvents();

    capFPS();
  }
}

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
  TTF_CloseFont(font);

  TTF_Quit();
  IMG_Quit();
  
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

void SDL::blit(SDL_Texture* texture, int dx, int dy)
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

#ifdef _WIN32
#define PREFIX  "../../../"
#else
#define PREFIX ""
#endif

bool SDL::loadData()
{
  SDL_Surface* surfaceUI = IMG_Load("data/ui.png");

  if (!surfaceUI)
  {
    printf("Error while loading ui.png: %s\n", IMG_GetError());
    return false;
  }
 
  textureUI = SDL_CreateTextureFromSurface(renderer, surfaceUI);
  SDL_FreeSurface(surfaceUI);

  font = TTF_OpenFont(PREFIX "data/FreeSans.ttf", 16);

  if (!font)
  {
    printf("Error while loading font: %s\n", TTF_GetError());
    return false;
  }

  return true;
}

void SDL::render()
{
  SDL_RenderClear(renderer);

  SDL_Rect rect = { 0, 0, 16, 16 };
  SDL_Rect rect2 = { 10, 10, 16, 16 };



  SDL_RenderCopy(renderer, textureUI, &rect, &rect2);


  SDL_Color color = { 0, 0, 0, 255 };
  SDL_Surface* surface = TTF_RenderText_Blended(font, "This is a nice test.", color);
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
  blit(texture, 10, 10);

  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);

  SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[])
{
  SDL sdl;

  if (!sdl.init())
    return -1;
  
  if (!sdl.loadData())
  {
    printf("Error while loading and initializing data.\n");
    sdl.deinit();
    return -1;
  }
  
  sdl.loop();
  sdl.deinit();

  return 0;
}
