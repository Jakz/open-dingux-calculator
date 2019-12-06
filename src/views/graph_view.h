#pragma once

#include "view_manager.h"


#include <cmath>

namespace graph
{
  template<typename real_t = float>
  struct point_t
  {
    real_t x, y;
    point_t(real_t x, real_t y) : x(x), y(y) { }
  };

  struct bounds_t { float min, max; };

  using function = std::function<float(float)>;
  using coordinate_remapping_function = std::function<point_t<>(point_t<>)>;


  struct coordinate_mapper_builder
  {
    static constexpr double HEIGHT = 240.0;
    static constexpr double WIDTH = 320.0;

    function vertical(float min, float max)
    {
      const float delta = max - min;
      const float ratio = HEIGHT / delta;

      return [ratio, min](double f) { return HEIGHT - (f - min) * ratio; };
    }

    function horizontal(float min, float max)
    {
      const float delta = max - min;
      const float ratio = WIDTH / delta;
      return [ratio, min](double f) { return (f - min) * ratio; };
    }
  };
}

#include "samplers/FunctionSampler1D.h"

#define AT(canvas__, x__, y__) static_cast<color_t*>(canvas__->pixels)[y__*canvas__->w + x__]
#define IS_INSIDE(x__, y__) (x__ >= 0 && x__ < 320 && y__ >= 0 && y__ < 240)

float ipart(float x) { return std::floor(x); }
float fpart(float x) { return x - ipart(x); }
float rfpart(float x) { return 1.0f - fpart(x); }
void pixel(SDL_Surface* canvas, int x, int y, float b, u32 color)
{
  if (IS_INSIDE(x, y))
  {
    int alpha = (int)(255 * b);
    color_t& pixel = AT(canvas, x, y);
    pixel.setRGB(color);
    pixel.a = std::min(alpha + pixel.a, 255);

    /*if (AT(canvas, x, y).a < alpha)
    {
    }*/
  }
}

void draw_line(SDL_Surface* canvas, float x0, float y0, float x1, float y1, u32 color)
{
  bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);

  if (steep)
  {
    std::swap(x0, y0);
    std::swap(x1, y1);
  }

  if (x0 > x1)
  {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  float dx = x1 - x0;
  float dy = y1 - y0;
  float gradient = dy / dx;
  if (dx == 0.0f) gradient = 1.0f;

  float xend = round(x0);
  float yend = y0 + gradient * (xend - x0);
  float xgap = rfpart(x0 + 0.5f);
  int xpxl1 = xend;
  int ypxl1 = ipart(yend);

  if (steep)
  {
    pixel(canvas, ypxl1, xpxl1, rfpart(yend) * xgap, color);
    pixel(canvas, ypxl1+1, xpxl1, fpart(yend) * xgap, color);
  }
  else
  {
    pixel(canvas, xpxl1, ypxl1, rfpart(yend) * xgap, color);
    pixel(canvas, xpxl1, ypxl1+1, fpart(yend) * xgap, color);
  }

  float intery = yend + gradient;

  xend = round(x1);
  yend = y1 + gradient * (xend - x1);
  xgap = fpart(x1 + 0.5);
  int xpxl2 = xend;
  int ypxl2 = ipart(yend);

  if (steep)
  {
    pixel(canvas, ypxl2, xpxl2, rfpart(yend) * xgap, color);
    pixel(canvas, ypxl2 + 1, xpxl2, fpart(yend) * xgap, color);
  }
  else
  {
    pixel(canvas, xpxl2, ypxl2, rfpart(yend) * xgap, color);
    pixel(canvas, xpxl2, ypxl2 + 1, fpart(yend) * xgap, color);
  }

  if (steep)
  {
    for (int x = xpxl1 + 1; x < xpxl2; ++x)
    {
      pixel(canvas, ipart(intery), x, rfpart(intery), color);
      pixel(canvas, ipart(intery)+1, x, fpart(intery), color);
      intery += gradient;
    }
  }
  else
  {
    for (int x = xpxl1 + 1; x < xpxl2; ++x)
    {
      pixel(canvas, x, ipart(intery), rfpart(intery), color);
      pixel(canvas, x, ipart(intery) + 1, fpart(intery), color);
      intery += gradient;
    }
  }

}

namespace ui
{
  static constexpr int WIDTH = 320;
  static constexpr int HEIGHT = 240;

  struct RenderEnvironment
  {
    struct { graph::bounds_t hor, ver; } bounds;
    struct { graph::function hor, ver; } mapper;
  };

  class RenderedFunction
  {
    mutable SDL_Surface* _canvas;
    mutable SDL_Texture* _texture;
    mutable bool _dirty;
    graph::function _function;
    u32 _color;

    void refineFunction(std::list<graph::point_t<>>& points) const
    {
      std::list<graph::point_t<>>::iterator prev = points.begin(), it = points.begin();
      ++it;
      for ( ; it != points.end(); ++prev, ++it)
      {
        /*if (prev->x < -1.0f && it->x > -1.0f)
        {
          it = points.insert(it, { -1.0f, +INFINITY });
          prev = points.insert(it, { -1.0f, -INFINITY });;
          it = std::next(prev);
        }
        if (prev->x < 1.0f && it->x > 1.0f)
        {
          it = points.insert(it, { 1.0f, -INFINITY });
          prev = points.insert(it, { 1.0f, +INFINITY });;
          it = std::next(prev);
        }*/

        if (isinf(it->y))
        {
          it->y = std::copysignf(INFINITY, prev->y);
          it = points.insert(it, { it->x, std::copysign(INFINITY, std::next(it)->y) });
          ++it;
        }
        
      }
    }

    void repaint(const RenderEnvironment& env) const
    {
      using value_list_t = std::list<graph::point_t<>>;
      FunctionSampler1D::SampleFunctionParams params;
      params.InitialPoints = 200;
      params.RangeThreshold = (env.bounds.ver.max - env.bounds.ver.min) / (HEIGHT*10);
      params.MaxRecursion = 50;
      value_list_t values;

      FunctionSampler1D::SampleFunction(_function, env.bounds.hor.min, env.bounds.hor.max, params, values);
      refineFunction(values);

      u32* pixels = static_cast<u32*>(_canvas->pixels);

      constexpr int LIMIT = HEIGHT * 3;
      constexpr float ASYMPTOTE_THRESHOLD = HEIGHT;

      bool skipNext = false;
      auto f = values.begin();
      auto s = std::next(values.begin());
      for (; s != values.end(); ++f, ++s)
      {
        if (skipNext)
        {
          skipNext = false;
          continue;
        }

        float fx1 = f->x, fy1 = f->y;
        float fx2 = s->x, fy2 = s->y;
        /*
        if (isinf(fy1)) 
          fy1 = std::copysign(INFINITY, fy2);
        if (isinf(fy2)) 
          fy2 = std::copysign(INFINITY, fy1);
          */

        if (isinf(fy1) && isinf(fy2))
          continue;
        
        float x1 = env.mapper.hor(fx1), y1 = env.mapper.ver(fy1);
        float x2 = env.mapper.hor(fx2), y2 = env.mapper.ver(fy2);

        if (!isinf(fy1) && !isinf(fy2) && (abs(y1 - y2) > ASYMPTOTE_THRESHOLD))
          continue;


        if (y1 > LIMIT) y1 = LIMIT;
        else if (y1 < -LIMIT) y1 = -LIMIT;

        if (y2 > LIMIT) y2 = LIMIT;
        else if (y2 < -LIMIT) y2 = -LIMIT;

        //TODO: should check that there is no intersection
        /*if (!IS_INSIDE(x1, y1) && !IS_INSIDE(x2, y2))
          continue;
          */

        draw_line(_canvas, x1, y1, x2, y2, _color);

        /*if (IS_INSIDE((int)x1, (int)y1))
          AT(_canvas, (int)x1, (int)y1) = 0xff000000;*/
      }
    }

  public:
    RenderedFunction(graph::function function, u32 color) : _canvas(nullptr), _texture(nullptr), _dirty(true), _function(function), _color(color)
    {

    }

    ~RenderedFunction()
    {
      SDL_FreeSurface(_canvas);
      SDL_DestroyTexture(_texture);
    }

    void dirty() { _dirty = true; }

    void render(SDL_Renderer* renderer, const RenderEnvironment& env) const
    {
      if (!_canvas)
        _canvas = SDL_CreateRGBSurface(0, WIDTH, HEIGHT, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);

      if (_dirty)
      {
        SDL_FillRect(_canvas, nullptr, 0);
        repaint(env);

        if (_texture)
          SDL_DestroyTexture(_texture);

        _texture = SDL_CreateTextureFromSurface(renderer, _canvas);
        SDL_BlendMode blendMode = SDL_BLENDMODE_BLEND;
        SDL_GetTextureBlendMode(_texture, &blendMode);
        _dirty = false;
      }

      SDL_RenderCopy(renderer, _texture, nullptr, nullptr);

    }
  };

  class GraphView : public View
  {
  private:
    ViewManager* gvm;

    void drawAxes();
    void drawTicks();

    RenderEnvironment env;
    std::vector<RenderedFunction> functions;

  public:
    GraphView(ViewManager* gvm);

    void dirty();
    void render();
    void handleKeyboardEvent(const SDL_Event& event);
    void handleMouseEvent(const SDL_Event& event);

    void setBounds(graph::bounds_t hor, graph::bounds_t ver);
  };

  GraphView::GraphView(ViewManager* gvm) : gvm(gvm)
  {
    float ratio = HEIGHT / (float)WIDTH;
    float value = 20.0f;
    setBounds({ -value, value }, { -value * ratio, value * ratio });
    //functions.emplace_back([](float x) { return log(x); }, 0x00ff0000);
    //functions.emplace_back([](float x) { return 1/(x*x); }, 0x00ff0000);

    //functions.emplace_back([](float x) { return sin(x)*3 + cos(2*x)*4; }, 0x0000ff00);
    //functions.emplace_back([](float x) { return x*x; }, 0x000000ff);
    //functions.emplace_back([](float x) { return x * x * x; }, 0x00ff0000);
    //functions.emplace_back([](float x) { return tan(x); }, 0x00ff8000);
    functions.emplace_back([](float x) { return (x*x)/(x*x-1); }, 0x00ff8000);

  }

  void GraphView::setBounds(graph::bounds_t hor, graph::bounds_t ver)
  {
    env.bounds.hor = hor;
    env.bounds.ver = ver;

    env.mapper.hor = graph::coordinate_mapper_builder().horizontal(env.bounds.hor.min, env.bounds.hor.max);
    env.mapper.ver = graph::coordinate_mapper_builder().vertical(env.bounds.ver.min, env.bounds.ver.max);
  }

  void GraphView::dirty()
  {
    for (auto& function : functions)
      function.dirty();
  }

  void GraphView::drawAxes()
  {
    SDL_SetRenderDrawBlendMode(gvm->getRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(gvm->getRenderer(), 0, 0, 0, 60);

    const float LX = 3, LY = 6;

    float sx = env.mapper.hor(0.0f);
    if (sx >= 0 && sx < WIDTH)
    {
      SDL_RenderDrawLine(gvm->getRenderer(), sx, 0, sx, HEIGHT);
      SDL_RenderDrawLine(gvm->getRenderer(), sx - LX, LY, sx, 0);
      SDL_RenderDrawLine(gvm->getRenderer(), sx + LX, LY, sx, 0);
    }

    float sy = env.mapper.ver(0.0f);
    if (sy >= 0 && sy < HEIGHT)
    {
      SDL_RenderDrawLine(gvm->getRenderer(), 0, sy, WIDTH, sy);
      SDL_RenderDrawLine(gvm->getRenderer(), WIDTH - LY, sy - LX, WIDTH - 1, sy);
      SDL_RenderDrawLine(gvm->getRenderer(), WIDTH - LY, sy + LX, WIDTH - 1, sy);
    }
  }

  void GraphView::drawTicks()
  {
    SDL_SetRenderDrawBlendMode(gvm->getRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(gvm->getRenderer(), 0, 0, 0, 60);

    const float LEN = 2;

    float sy = env.mapper.ver(0.0f);

    if (sy >= 0 && sy < HEIGHT)
    {
      for (int sx = (int)env.bounds.hor.min; sx < env.bounds.hor.max; ++sx)
        SDL_RenderDrawLine(gvm->getRenderer(), env.mapper.hor(sx), sy - LEN / 2, env.mapper.hor(sx), sy + LEN / 2);
    }
  }

  void GraphView::render()
  {
    auto* renderer = gvm->getRenderer();

    SDL_SetRenderDrawColor(renderer, 255, 250, 237, 255);
    SDL_RenderClear(renderer);

    for (const auto& function : functions)
      function.render(renderer, env);

    /*auto label = gvm->cache()->get("𝑦 = 3sin(𝑥) + 4cos(2𝑥)", 8000, gvm->tinyFont(), color_t(255, 128, 0));
    gvm->blit(label->second.texture, label->second.rect, 0, 10);*/

    static char buffer[128];
    {
      sprintf(buffer, "%2.2f, %2.2f", env.bounds.hor.min, env.bounds.ver.max);
      auto label = gvm->cache()->get(buffer, 8000, gvm->tinyFont(), color_t(0, 0, 0, 60));
      SDL_SetTextureBlendMode(label->second.texture, SDL_BLENDMODE_BLEND);
      gvm->blit(label->second.texture, label->second.rect, 2, 2);
    }
    {
      sprintf(buffer, "%2.2f, %2.2f", env.bounds.hor.max, env.bounds.ver.min);
      auto label = gvm->cache()->get(buffer, 8000, gvm->tinyFont(), color_t(0, 0, 0, 60));
      SDL_SetTextureBlendMode(label->second.texture, SDL_BLENDMODE_BLEND);
      gvm->blit(label->second.texture, label->second.rect, WIDTH-2-label->second.rect.w, HEIGHT-2-label->second.rect.h);
    }

    drawAxes();
    drawTicks();
  }

  void GraphView::handleKeyboardEvent(const SDL_Event& event)
  {
    if (event.type == SDL_KEYDOWN)
    {
      switch (event.key.keysym.sym)
      {
      case SDLK_LEFT:
      {
        float scale = std::abs(env.bounds.hor.min - env.bounds.hor.max) / 20.0f;
        setBounds({ env.bounds.hor.min - scale, env.bounds.hor.max - scale }, env.bounds.ver);
        dirty();
        break;
      }
      case SDLK_RIGHT:
      {
        float scale = std::abs(env.bounds.hor.min - env.bounds.hor.max) / 20.0f;
        setBounds({ env.bounds.hor.min + scale, env.bounds.hor.max + scale }, env.bounds.ver);
        dirty();
        break;
      }
      case SDLK_UP:
      {
        float scale = std::abs(env.bounds.hor.min - env.bounds.hor.max) / 20.0f;
        setBounds(env.bounds.hor, { env.bounds.ver.min + scale, env.bounds.ver.max + scale });
        dirty();
        break;
      }
      case SDLK_DOWN:
      {
        float scale = std::abs(env.bounds.hor.min - env.bounds.hor.max) / 20.0f;
        setBounds(env.bounds.hor, { env.bounds.ver.min - scale, env.bounds.ver.max - scale });
        dirty();
        break;
      }
      case SDLK_TAB:
      {
        setBounds(
          { env.bounds.hor.min * 1.1f, env.bounds.hor.max * 1.1f },
          { env.bounds.ver.min * 1.1f, env.bounds.ver.max * 1.1f }
        );
        dirty();

        break;
      }
      case SDLK_BACKSPACE:
      {
        setBounds(
          { env.bounds.hor.min * 0.9f, env.bounds.hor.max * 0.9f },
          { env.bounds.ver.min * 0.9f, env.bounds.ver.max * 0.9f }
        );
        dirty();

        break;
      }

      case SDLK_ESCAPE:
        gvm->exit();
        break;
      }
    }
  }

  void GraphView::handleMouseEvent(const SDL_Event& event)
  {

  }
}
