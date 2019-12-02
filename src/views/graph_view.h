#include "view_manager.h"

void WuDrawLine(float x0, float y0, float x1, float y1,
  const std::function<void(int x, int y, float brightess)>& plot) {
  auto ipart = [](float x) -> int {return int(std::floor(x)); };
  auto round = [](float x) -> float {return std::round(x); };
  auto fpart = [](float x) -> float {return x - std::floor(x); };
  auto rfpart = [=](float x) -> float {return 1 - fpart(x); };

  const bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    std::swap(x0, y0);
    std::swap(x1, y1);
  }
  if (x0 > x1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  const float dx = x1 - x0;
  const float dy = y1 - y0;
  const float gradient = (dx == 0) ? 1 : dy / dx;

  int xpx11;
  float intery;
  {
    const float xend = round(x0);
    const float yend = y0 + gradient * (xend - x0);
    const float xgap = rfpart(x0 + 0.5);
    xpx11 = int(xend);
    const int ypx11 = ipart(yend);
    if (steep) {
      plot(ypx11, xpx11, rfpart(yend) * xgap);
      plot(ypx11 + 1, xpx11, fpart(yend) * xgap);
    }
    else {
      plot(xpx11, ypx11, rfpart(yend) * xgap);
      plot(xpx11, ypx11 + 1, fpart(yend) * xgap);
    }
    intery = yend + gradient;
  }

  int xpx12;
  {
    const float xend = round(x1);
    const float yend = y1 + gradient * (xend - x1);
    const float xgap = rfpart(x1 + 0.5);
    xpx12 = int(xend);
    const int ypx12 = ipart(yend);
    if (steep) {
      plot(ypx12, xpx12, rfpart(yend) * xgap);
      plot(ypx12 + 1, xpx12, fpart(yend) * xgap);
    }
    else {
      plot(xpx12, ypx12, rfpart(yend) * xgap);
      plot(xpx12, ypx12 + 1, fpart(yend) * xgap);
    }
  }

  if (steep) {
    for (int x = xpx11 + 1; x < xpx12; x++) {
      plot(ipart(intery), x, rfpart(intery));
      plot(ipart(intery) + 1, x, fpart(intery));
      intery += gradient;
    }
  }
  else {
    for (int x = xpx11 + 1; x < xpx12; x++) {
      plot(x, ipart(intery), rfpart(intery));
      plot(x, ipart(intery) + 1, fpart(intery));
      intery += gradient;
    }
  }
}

void AALine(int x0, int y0, int x1, int y1, const std::function<void(int x, int y, float brightess)>& plot)
{
  int addr = (y0 * 320 + x0) * 1;
  int dx = x1 - x0;
  int dy = y1 - y0;
  int du, dv, u, v, uincr, vincr;
  /* By switching to (u,v), we combine all eight octants */
  if (abs(dx) > abs(dy))
  {
    /* Note: If this were actual C, these integers would be lost
     * at the closing brace. That's not what I mean to do. Do what
     * I mean. */
    du = abs(dx);
    dv = abs(dy);
    u = x1;
    v = y1;
    uincr = 1;
    vincr = 320;
    if (dx < 0) uincr = -uincr;
    if (dy < 0) vincr = -vincr;
  }
  else
  {
    du = abs(dy);
    dv = abs(dx);
    u = y1;
    v = x1;
    uincr = 320;
    vincr = 1;
    if (dy < 0) uincr = -uincr;
    if (dx < 0) vincr = -vincr;
  }
  int uend = u + 2 * du;
  int d = (2 * dv) - du; /* Initial value as in Bresenham's */
  int incrS = 2 * dv; /* Δd for straight increments */
  int incrD = 2 * (dv - du); /* Δd for diagonal increments */
  int twovdu = 0; /* Numerator of distance; starts at 0 */
  double invD = 1.0 / (2.0*sqrt(du*du + dv * dv)); /* Precomputed inverse denominator */
  double invD2du = 2.0 * (du*invD); /* Precomputed constant */
  do
  {
    /* Note: this pseudocode doesn't ensure that the address is
     * valid, or that it even represents a pixel on the same side of
     * the screen as the adjacent pixel */
    plot(addr % 320, addr / 320, twovdu*invD);
    plot((addr+vincr) % 320, (addr + vincr) / 320, invD2du - twovdu * invD);
    plot((addr - vincr) % 320, (addr - vincr) / 320, invD2du + twovdu * invD);

    if (d < 0)
    {
      /* choose straight (u direction) */
      twovdu = d + du;
      d = d + incrS;
    }
    else
    {
      /* choose diagonal (u+v direction) */
      twovdu = d - du;
      d = d + incrD;
      v = v + 1;
      addr = addr + vincr;
    }
    u = u + 1;
    addr = addr + uincr;
  } while (u < uend);
}

namespace graph
{
  struct point_t
  {
    float x, y;
  };

  struct bounds_t { float min, max; };
  
  using function = std::function<float(float)>;
  using coordinate_remapping_function = std::function<point_t(point_t)>;

  
  struct coordinate_mapper_builder
  {
    static constexpr float HEIGHT = 240.0f;
    static constexpr float WIDTH = 320.0f;

    function vertical(float min, float max)
    {
      const float delta = max - min;
      const float ratio = HEIGHT / delta;

      return [ratio, min](float f) { return HEIGHT - (f - min) * ratio; };
    }

    function horizontal(float min, float max)
    {
      const float delta = max - min;
      const float ratio = WIDTH / delta;
      return [ratio, min](float f) { return (f - min) * ratio; };
    }
  };
}

namespace ui
{
  class GraphView : public View
  {
  private:
    ViewManager* manager;

    SDL_Surface* canvas;
    SDL_Surface* aaCanvas;
    SDL_Texture* texture;

  public:
    GraphView(ViewManager* manager);

    void render();
    void handleKeyboardEvent(const SDL_Event& event);
    void handleMouseEvent(const SDL_Event& event);

    void renderFunction(std::function<float(float)> func);
  };

  GraphView::GraphView(ViewManager* manager) : manager(manager), canvas(nullptr)
  { 
  }

#define AT(canvas__, x__, y__) static_cast<u32*>(canvas__->pixels)[y__*canvas__->w + x__]

  void GraphView::renderFunction(graph::function func)
  {
    const int WIDTH = 320;
    const int HEIGHT = 240;
    
    const graph::bounds_t horizontalBounds = { -5.0, 5.0f };
    const graph::bounds_t verticalBounds = { -2.0f, 2.0f };

    graph::function vertical = graph::coordinate_mapper_builder().vertical(verticalBounds.min, verticalBounds.max);
    graph::function horizontal = graph::coordinate_mapper_builder().horizontal(horizontalBounds.min, horizontalBounds.max);
    
    canvas = SDL_CreateRGBSurface(0, WIDTH, HEIGHT, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    aaCanvas = SDL_CreateRGBSurface(0, WIDTH, HEIGHT, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);

    u32* pixels = static_cast<u32*>(canvas->pixels);
    u32 x = 10, y = 15;


    const float dx = (horizontalBounds.max - horizontalBounds.min) / (WIDTH*100.0f);
    for (float fx = horizontalBounds.min; fx <= horizontalBounds.max; fx += 0.005f)
    {
      float fy = func(fx);

      for (s32 pw = 0; pw < 1; ++pw)
        for (s32 ph = 0; ph < 1; ++ph)
        {
          int px = horizontal(pw + fx), py = vertical(ph + fy);

          if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT)
          {
            float brigthness = 1.0f - 0.3f*(std::abs(pw - 1) + std::abs(ph - 1));

            u32 oldColor = pixels[py*canvas->w + px];

            u32 a = (oldColor & 0xff000000) >> 24;

            //pixels[py*canvas->w + px] = SDL_MapRGBA(canvas->format, 255, 0, 0, std::max((int)((brigthness * 255)*0.5f + a), 255));
            pixels[py*canvas->w + px] = SDL_MapRGBA(canvas->format, 255, 0, 0, 255);
          }
        }
    }

    for (int x = 0; x < WIDTH; ++x)
      for (int y = 0; y < HEIGHT; ++y)
      {
        if (AT(aaCanvas, x, y) == 0x00000000)
        {
          int count = 0;
          for (int dx = -1; dx <= 1; ++dx)
            for (int dy = -1; dy <= 1; ++dy)
            {
              if (dx != 0 || dy != 0)
              {
                int fx = x + dx, fy = y + dy;
                if (fx > 0 && fx < WIDTH && fy > 0 && fy < HEIGHT)
                {
                  if (AT(canvas, fx, fy) == 0xffff0000)
                  {
                    ++count;
                    break;
                  }
                }
              }
            }

          count = std::min(count, 5);
          if (count)
            AT(aaCanvas, x, y) = SDL_MapRGBA(canvas->format, 255, 0, 0, count*(255/5.0f));
        } 
      }

    for (int x = 0; x < WIDTH; ++x)
      for (int y = 0; y < HEIGHT; ++y)
      {
        if (AT(canvas, x, y) == 0x00000000)
          AT(canvas, x, y) = AT(aaCanvas, x, y);
      }

    texture = SDL_CreateTextureFromSurface(manager->getRenderer(), canvas);
  }

  void GraphView::render()
  {
    auto* renderer = manager->getRenderer();

    if (!canvas)
      renderFunction([](float x) { return std::sin(x); });
   
    
    //SDL_SetRenderDrawColor(renderer, 255, 250, 237, 255);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer, texture, nullptr, nullptr);

    SDL_RenderPresent(renderer);
  }

  void GraphView::handleKeyboardEvent(const SDL_Event& event)
  {

  }

  void GraphView::handleMouseEvent(const SDL_Event& event)
  {

  }
}
