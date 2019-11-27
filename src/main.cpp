#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"

#include "sdl_helper.h"

class MySDL : public SDL<MySDL, MySDL>
{
private:
  SDL_Texture* textureUI;
  TTF_Font* font;

public:
  MySDL() : SDL(*this, *this), textureUI(nullptr), font(nullptr) { }

  bool loadData();

  void handleKeyboardEvent(const SDL_Event& event);
  void render();

  void deinit()
  {
    TTF_CloseFont(font);
    SDL_DestroyTexture(textureUI);
    SDL::deinit();
  }
};

void MySDL::handleKeyboardEvent(const SDL_Event& event)
{
  if (event.key.keysym.sym == SDLK_ESCAPE)
    SDL::exit();
}

#ifdef _WIN32
#define PREFIX  "../../../"
#else
#define PREFIX ""
#endif

bool MySDL::loadData()
{
  SDL_Surface* surfaceUI = IMG_Load(PREFIX "data/ui.png");

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

void MySDL::render()
{
  SDL_RenderClear(renderer);

  blit(textureUI, 0, 0, 16, 16, 10, 10);

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
  MySDL sdl;

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
