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

struct ButtonStyle
{
  bool pressed;
  bool hovered;
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

  void handleKeyboardEvent(const SDL_Event& event, bool press);
  void render();

  void deinit()
  {
    FC_FreeFont(font);
    SDL_DestroyTexture(textureUI);
    SDL::deinit();
  }

  void renderButton(int x, int y, int w, int h, const std::string& label, SDL_Color color, ButtonStyle style);
  void renderButtonBackground(int x, int y, int w, int h, int bx, int by);
};

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
  struct GridPosition
  {
    u16 x, y;

    struct hash_t
    {
      size_t operator()(const GridPosition& pos) const
      {
        return pos.x << 16 | pos.y;
      }
    };
  };
  
  

  class Button
  {
  public:
    std::string label;
    SDL_Rect position;
    SDL_Rect gfx;
    SDL_Color color;

  public:
    Button(std::string label, SDL_Rect position, SDL_Rect gfx, SDL_Color color) : label(label), position(position), gfx(gfx), color(color) { }
    Button(std::string label, SDL_Rect position, SDL_Rect gfx) : label(label), position(position), gfx(gfx), color({ 255, 255, 255 }) { }
  };

  struct ButtonSpec
  {
    std::string label;
    int x, y, w, h;
    SDL_Color color;

    ButtonSpec(std::string label, int x, int y, int w, int h) : label(label), x(x), y(y), w(w), h(h), color({ 255, 255, 255 }) { }
    ButtonSpec(std::string label, int x, int y, int w, int h, SDL_Color color) : label(label), x(x), y(y), w(w), h(h), color(color) { }
  };

  class Layout
  {
  private:
    int bx, by, cw, ch, m;
    int gw, gh;

    using data_t = std::vector<Button>;
    data_t _buttons;
    GridPosition _selectedPosition;
    data_t::const_iterator _selected;
    data_t::const_iterator* _grid;
    
    inline data_t::const_iterator& grid(int x, int y)
    { 
      assert(x < gw && y < gh);
      return _grid[y*gw + x];
    }

  public:
    Layout(int bx, int by, int cw, int ch, int m) : bx(bx), by(by), cw(cw), ch(ch), m(m), _grid(nullptr)
    {

    }

    ~Layout()
    {
      delete [] _grid;
    }

    void initialize(const std::vector<ButtonSpec>& buttons)
    {
      /* reserve to avoid invalidation of iterators */
      _buttons.reserve(buttons.size());

      gw = 0, gh = 0;

      for (const ButtonSpec& button : buttons)
      {
        int btx = bx + button.x * (cw + m);
        int bty = by + button.y * (ch + m);
        int bw = cw * button.w + m * (button.w - 1);
        int bh = ch * button.h + m * (button.h - 1);

        _buttons.emplace_back(button.label, SDL_Rect{ button.x, button.y, button.w, button.h }, SDL_Rect{ btx, bty, bw, bh }, button.color);
        gw = std::max(button.x + button.w, gw);
        gh = std::max(button.y + button.h, gh);
      }

      /* initialize grid to invalid values */
      _grid = new data_t::const_iterator[gw*gh];
      std::fill(_grid, _grid + (gw * gh), _buttons.end());

      /* populate grid with corresponding buttons for easy navigation */
      for (auto it = _buttons.begin(); it != _buttons.end(); ++it)
      {
        const auto& button = *it;
        for (int x = 0; x < button.position.w; ++x)
          for (int y = 0; y < button.position.h; ++y)
            grid(button.position.x + x, button.position.y + y) = it;
      }

      _selected = grid(0, 0);
      _selectedPosition = { 0, 0 };
    }

    data_t::const_iterator selected() { return _selected; }
    data_t::const_iterator begin() const { return _buttons.begin(); }
    data_t::const_iterator end() const { return _buttons.end(); }

    void hoverNext(int dx, int dy)
    {
      GridPosition current = _selectedPosition;

      if (dx)
      {
        while ((current.x < gw - 1 && dx > 0) || (current.x > 0 && dx < 0))
        {
          current.x += dx;
          const auto& it = grid(current.x, current.y);
          if (it != _buttons.end() && it != _selected)
          {
            _selected = it;
            _selectedPosition = current;
            return;
          }
        }
      }
      else if (dy)
      {
        while ((current.y < gh - 1 && dy > 0) || (current.y > 0 && dy < 0))
        {
          current.y += dy;
          const auto& it = grid(current.x, current.y);
          if (it != _buttons.end() && it != _selected)
          {
            _selected = it;
            _selectedPosition = current;
            return;
          }
        }
      }
    }
  };

  class LayoutHelper
  {
  protected:
    void addNumberGrid(std::vector<ButtonSpec>& buttons, int bx, int by, int bw, int bh)
    {
      buttons.emplace_back(ButtonSpec{ "0", bx, by + 3 * bh, bw, bh });
      buttons.emplace_back(ButtonSpec{ "00", bx + bw, by + 3 * bh, bw, bh });
      buttons.emplace_back(ButtonSpec{ ".", bx + bw * 2, by + 3 * bh, bw, bh });

      buttons.emplace_back(ButtonSpec{ "1", bx, by + 2 * bh, bw, bh });
      buttons.emplace_back(ButtonSpec{ "2", bx + bw, by + 2 * bh, bw, bh });
      buttons.emplace_back(ButtonSpec{ "3", bx + bw*2, by + 2 * bh, bw, bh });

      buttons.emplace_back(ButtonSpec{ "4", bx, by + 1 * bh, bw, bh });
      buttons.emplace_back(ButtonSpec{ "5", bx + bw, by + 1 * bh, bw, bh });
      buttons.emplace_back(ButtonSpec{ "6", bx + bw * 2, by + 1 * bh, bw, bh });

      buttons.emplace_back(ButtonSpec{ "7", bx, by, bw, bh });
      buttons.emplace_back(ButtonSpec{ "8", bx + bw, by, bw, bh });
      buttons.emplace_back(ButtonSpec{ "9", bx + bw * 2, by, bw, bh });
    }

  };

  class EasyLayout : public Layout, private LayoutHelper
  {
  private:
    static constexpr int BS = 2;
  public:
    EasyLayout() : Layout(18, 48, 20, 12, 2)
    { 
      std::vector<ButtonSpec> buttons;
      buttons.push_back({ "%", 0, 2, 2, 2, { 200, 200, 200 } });
      buttons.push_back({ "√", 0, 4, 2, 2, { 200, 200, 200 } });
      buttons.push_back({ "C", 0, 6, 2, 2, { 200, 50, 50 } });
      buttons.push_back({ "AC", 0, 8, 2, 2, { 200, 50, 50 } });


      buttons.push_back({ "MC", 0, 0, 2, 2, { 200, 200, 200 } });
      buttons.push_back({ "MR", 2, 0, 2, 2, { 200, 200, 200 } });
      buttons.push_back({ "M-", 4, 0, 2, 2, { 200, 200, 200 } });
      buttons.push_back({ "M+", 6, 0, 2, 2, { 200, 200, 200 } });


      buttons.push_back({ "÷", 9, 0, 2, 2, { 200, 200, 200 } });
      buttons.push_back({ "×", 11, 0, 2, 2, { 200, 200, 200 } });
      buttons.push_back({ "-", 11, 2, 2, 2, { 200, 200, 200 } });
      buttons.push_back({ "+", 11, 4, 2, 4, { 200, 200, 200 } });
      buttons.push_back({ "=", 11, 8, 2, 2, { 200, 200, 200 } });


      LayoutHelper::addNumberGrid(buttons, 2, 2, 3, 2);
      initialize(buttons);
    }
  };
}

/*
* D-PAD Left - SDLK_LEFT
* D-PAD Right - SDLK_RIGHT
* D-PAD Up - SDLK_UP
* D-PAD Down - SDLK_DOWN
* Y button - SDLK_SPACE
* X button - SDLK_LSHIFT
* A button - SDLK_LCTRL
* B button - SDLK_LALT
* START button - SDLK_RETURN
* SELECT button - SDLK_ESC
* L shoulder - SDLK_TAB
* R shoulder - SDLK_BACKSPACE
* Power slider in up position - SDLK_POWER (not encouraged to map in game, as it's used by the pwswd daemon)
* Power slider in down position - SDLK_PAUSE

*/
namespace calc
{
  class Calculator
  {
  public:
    using value_t = double;

  private:
    value_t _value;

  public:
    void set(value_t value) { _value = value; }

    value_t value() const { return _value; }
  };
}

calc::EasyLayout layout;
calc::Calculator calculator;

bool pressed = false;

void MySDL::handleKeyboardEvent(const SDL_Event& event, bool press)
{
  switch (event.key.keysym.sym)
  {
  case SDLK_ESCAPE:
    SDL::exit();
    break;

  case SDLK_LEFT:
    if (press)
      layout.hoverNext(-1, 0);
    pressed = false;
    break;
  case SDLK_RIGHT:
    if (press)
      layout.hoverNext(1, 0);
    pressed = false;
    break;
  case SDLK_UP:
    if (press)
      layout.hoverNext(0, -1);
    pressed = false;
    break;
  case SDLK_DOWN:
    if (press)
      layout.hoverNext(0, 1);
    pressed = false;
    break;

  case SDLK_LALT:
    pressed = press;
    break;
  }
}

void MySDL::render()
{
  SDL_RenderClear(renderer);

  for (auto it = layout.begin(); it != layout.end(); ++it)
  {
    const auto& button = *it;
    renderButton(button.gfx.x, button.gfx.y, button.gfx.w, button.gfx.h, button.label, button.color, { pressed && it == layout.selected(), it == layout.selected() });
  }

  renderButtonBackground(18, 10, 284, 30, 0, 0);

  static char buffer[512];
  sprintf(buffer, "%f", calculator.value());
  FC_DrawAlign(font, getRenderer(), 284, 25, FC_ALIGN_RIGHT, buffer);

  SDL_RenderPresent(renderer);
}

void MySDL::renderButtonBackground(int x, int y, int w, int h, int bx, int by)
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

void MySDL::renderButton(int x, int y, int w, int h, const std::string& label, SDL_Color color, ButtonStyle style)
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

  int a = FC_GetAscent(font, label.c_str());
  int lh = FC_GetLineHeight(font);
  float tx = x + w / 2 + (style.pressed ? 0.5 : 0);
  float ty = y + h / 2 + (style.pressed ? 0.5 : 0);
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
