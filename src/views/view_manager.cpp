#include "view_manager.h"

#include "calculator_view.h"
#include "graph_view.h"

ui::ViewManager::ViewManager() : SDL<ui::ViewManager, ui::ViewManager>(*this, *this), textureUI(nullptr), _font(nullptr), _tinyFont(nullptr)
{

}

void ui::ViewManager::deinit()
{
  TTF_CloseFont(_font);
  TTF_CloseFont(_tinyFont);
  SDL_DestroyTexture(textureUI);
  SDL::deinit();
}

bool ui::ViewManager::loadData()
{
#ifdef _WIN32
  const std::string prefix = "../../../";
#else
  const std::string prefix = "";
#endif

  SDL_Surface* surfaceUI = IMG_Load((prefix + "data/ui.png").c_str());

  if (!surfaceUI)
  {
    printf("Error while loading ui.png: %s\n", IMG_GetError());
    return false;
  }

  textureUI = SDL_CreateTextureFromSurface(renderer, surfaceUI);
  SDL_FreeSurface(surfaceUI);

  const std::string fontName = "FreeSans.ttf";


  _font = TTF_OpenFont((prefix + "data/" + fontName).c_str(), 16);
  _tinyFont = TTF_OpenFont((prefix + "data/" + fontName).c_str(), 10);

  if (!_font || !_tinyFont)
  {
    printf("Error while loading font: %s\n", TTF_GetError());
    return false;
  }

  _cache.init(getRenderer(), 128, 128);

  views[0] = new CalculatorView<gfx::EasyLayout>(this);
  views[1] = new CalculatorView<gfx::ScientificLayout>(this);
  views[2] = new GraphView(this);
  view = views[1];

  return true;
}

bool pressed = false;

void ui::ViewManager::handleKeyboardEvent(const SDL_Event& event, bool press)
{
  view->handleKeyboardEvent(event);
}

void ui::ViewManager::handleMouseEvent(const SDL_Event& event)
{
  view->handleMouseEvent(event);
}


void ui::ViewManager::render()
{
  view->render();
}

void ui::ViewManager::renderButtonBackground(int x, int y, int w, int h, int bx, int by)
{
  assert(w >= 16 && h >= 16);

  constexpr int S = 8;
  constexpr int M = 6;
  constexpr int K = 4;

  const int BX = bx;
  const int BY = by;

  blit(textureUI, BX, BY, S, S, x, y); /* top left */
  blit(textureUI, BX + S, BY, S, S, x + w - S, y); /* top right */
  blit(textureUI, BX, BY + S, S, S, x, y + h - S); /* bottom left */
  blit(textureUI, BX + S, BY + S, S, S, x + w - S, y + h - S); /* bottom right */

  blit(textureUI, BX + M, BY, K, S, x + S, y, w - S * 2, S); /* top */
  blit(textureUI, BX + M, BY + S, K, S, x + S, y + h - S, w - S * 2, S); /* bottom */
  blit(textureUI, BX, BY + M, S, K, x, y + S, S, h - S * 2); /* left */
  blit(textureUI, BX + S, BY + M, S, K, x + w - S, y + S, S, h - S * 2); /* right */

  blit(textureUI, BX + M, BY + M, K, K, x + S, y + S, w - S * 2, h - S * 2); /* center */
}

void ui::ViewManager::renderButton(int x, int y, int w, int h, const std::string& label, TTF_Font* font, SDL_Color color, ButtonStyle style)
{
  assert(w >= 16 && h >= 16);

  const int BX = style.pressed ? 16 : 0;
  const int BY = 0;

  SDL_SetTextureColorMod(textureUI, color.r, color.g, color.b);
  renderButtonBackground(x, y, w, h, BX, BY);

  if (style.hovered)
  {
    SDL_SetTextureColorMod(textureUI, 255, 255, 255);
    renderButtonBackground(x, y, w, h, 0, 16);
  }

  const SDL_Rect& rect = _cache.get(label, font)->second;
  blit(_cache.texture(), rect, x + w / 2 - rect.w / 2 + (style.pressed ? 1 : 0), y + h / 2 - rect.h / 2 + (style.pressed ? 1 : 0));
}