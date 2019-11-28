#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"

#include "sdl_helper.h"
#include "sdl_fontcache.h"

#include <algorithm>
#include <unordered_map>

class LabelCache
{
public:
  using map_t = std::unordered_map<std::string, SDL_Rect>;

private:
  int _w, _h;

  int _currentY, _currentX;
  int _maxY;

  TTF_Font* _font;
  SDL_Texture* _texture;
  map_t _cache;

  map_t::const_iterator compute(const std::string& text)
  {
    SDL_Surface* surface = TTF_RenderText_Blended(_font, text.c_str(), { 0, 0, 0, 255 });
    
    /* if it doesn't fit current row start a new one */
    if (_currentX + surface->w > _w)
    {
      _currentY = _maxY;
      _currentX = 0;
    }

    if (surface->w > _w)
      assert(false);

    int newY = _currentY + surface->h;

    /* if there is not enough vertical space there's no way */
    if (newY > _h)
      assert(false);

    /* update new column max */
    _maxY = std::max(_maxY, newY);

    auto pair = _cache.emplace(std::make_pair(text, SDL_Rect{ _currentX, _currentY, surface->w, surface->h }));

    _currentX += surface->w;

    u32 format;
    SDL_QueryTexture(_texture, &format, nullptr, nullptr, nullptr);
    assert(surface->format->format == format);

    void* pixels;
    int pitch;
    SDL_LockTexture(_texture, nullptr, &pixels, &pitch);
    SDL_LockSurface(surface);

    



    return pair.first;
  }

public:
  LabelCache() : _font(nullptr), _texture(nullptr), _w(0), _h(0), _maxY(0), _currentY(0), _currentX(0)
  {

  }

  void init(TTF_Font* font, SDL_Renderer* renderer, int w, int h)
  {
    _font = font;
    //_renderer = renderer;
    _w = w;
    _h = h;

    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);

    _texture = SDL_CreateTexture(renderer, info.texture_formats[0], SDL_TEXTUREACCESS_STATIC, w, h);
  }


  map_t::const_iterator get(const std::string& text)
  {
    map_t::const_iterator it = _cache.find(text);

    if (it == _cache.end())
      it = compute(text);
    
    return it;
  }
  



  ~LabelCache()
  {
    SDL_DestroyTexture(_texture);
  }
};

enum class ButtonStyle
{
  NORMAL,
  PRESSED,
};

class MySDL : public SDL<MySDL, MySDL>
{
private:
  LabelCache labelCache;
  SDL_Texture* textureUI;
  FC_Font* font;

public:
  MySDL() : SDL(*this, *this), textureUI(nullptr), font(nullptr) { }

  bool loadData();

  void handleKeyboardEvent(const SDL_Event& event);
  void render();

  void deinit()
  {
    FC_FreeFont(font);
    SDL_DestroyTexture(textureUI);
    SDL::deinit();
  }

  void renderButton(int x, int y, int w, int h, const std::string& label, ButtonStyle style);
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


#include "sdl_fontcache.h"

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

  font = FC_CreateFont();
  FC_LoadFont(font, getRenderer(), PREFIX "data/FreeSans.ttf", 16, FC_MakeColor(0, 0, 0, 255), TTF_STYLE_NORMAL);

  if (!font)
  {
    printf("Error while loading font: %s\n", TTF_GetError());
    return false;
  }

  return true;
}

#include <vector>

namespace calc
{
  class Button
  {
  public:
    std::string label;
    int x, y;
    int w, h;

  public:
    Button(std::string label, int x, int y, int w, int h) : label(label), x(x), y(y), w(w), h(h) { }
  };

  class Layout
  {
  private:
    std::vector<Button> buttons;

  public:
    Layout(int bx, int by, int cw, int ch, int m, const std::initializer_list<Button>& buttons)
    {
      for (const Button button : buttons)
      {
        int btx = bx + button.x * (cw + m);
        int bty = by + button.y * (ch + m);
        int bw = cw * button.w + m * (button.w - 1);
        int bh = ch * button.h + m * (button.h - 1);

        this->buttons.emplace_back(button.label, btx, bty, bw, bh);
      }
    }

    decltype(buttons)::const_iterator begin() const { return buttons.begin(); }
    decltype(buttons)::const_iterator end() const { return buttons.end(); }
  };

  class EasyLayout : public Layout
  {
  public:
    EasyLayout() : Layout(
      20, 20, 24, 24, 2,
      {
        Button("0", 0, 0, 1, 1),
        Button("1", 0, 1, 1, 1),

        Button("+", 1, 0, 1, 2)
      }
    ) { }
  };
}

calc::EasyLayout layout;


void MySDL::render()
{
  SDL_RenderClear(renderer);

  for (const auto& button : layout)
  {
    renderButton(button.x, button.y, button.w, button.h, button.label, ButtonStyle::NORMAL);
  }

  SDL_RenderPresent(renderer);
}

void MySDL::renderButton(int x, int y, int w, int h, const std::string& label, ButtonStyle style)
{
  assert(w >= 16 && h >= 16);

  constexpr int S = 8;
  constexpr int M = 6;
  constexpr int K = 4;

  const int BX = style == ButtonStyle::PRESSED ? 16 : 0;
  const int BY = 0;

  blit(textureUI, BX, BY, S, S, x, y); /* top left */
  blit(textureUI, BX + S, BY, S, S, x + w - S, y); /* top right */
  blit(textureUI, BX, S, BY + S, S, x, y + h - S); /* bottom left */
  blit(textureUI, BX + S, BY + S, S, S, x + w - S, y + h - S); /* bottom right */

  blit(textureUI, BX + M, BY, K, S, x + S, y, w - S * 2, S); /* top */
  blit(textureUI, BX + M, BY + S, K, S, x + S, y + h - S, w - S * 2, S); /* bottom */
  blit(textureUI, BX, M, BY + S, K, x, y + S, S, h - S * 2); /* left */
  blit(textureUI, BX + S, BY + M, S, K, x + w - S, y + S, S, h - S * 2); /* right */

  blit(textureUI, BX + M, BY + M, K, K, x + S, y + S, w - S * 2, h - S * 2); /* center */


  int a = FC_GetAscent(font, label.c_str());
  int lh = FC_GetLineHeight(font);
  float tx = x + w / 2 + (style == ButtonStyle::PRESSED ? 0.5 : 0);
  float ty = y + h / 2 + (style == ButtonStyle::PRESSED ? 0.5 : 0);
  FC_DrawAlign(font, getRenderer(), tx, ty - a + lh / 2, FC_ALIGN_CENTER, label.c_str());
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
