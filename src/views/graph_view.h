#include "view_manager.h"

#include "samplers/FunctionSampler1D.h"

#include <cmath>

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
#define IS_INSIDE(x__, y__) (x__ >= 0 && x__ < 320 && y__ >= 0 && y__ < 240)

  void line(SDL_Surface* canvas, float x0, float y0, float x1, float y1, u32 color)
  {
    int i;
    double x = x1 - x0;
    double y = y1 - y0;
    double length = sqrt(x*x + y * y);
    double addx = x / length;
    double addy = y / length;
    x = x0;
    y = y0;

    {
      for (i = 0; i < length; i += 1) {

        if (x >= 0 && x < 320 && y >= 0 && y < 240)
          AT(canvas, (int)x, (int)y) = color;
        x += addx;
        y += addy;
      }
    }
  }


  void GraphView::renderFunction(graph::function func)
  {
    const int WIDTH = 320;
    const int HEIGHT = 240;

    const graph::bounds_t horizontalBounds = { -5.0, 5.0f };
    const graph::bounds_t verticalBounds = { -10.0f, 10.0f };

    graph::function vertical = graph::coordinate_mapper_builder().vertical(verticalBounds.min, verticalBounds.max);
    graph::function horizontal = graph::coordinate_mapper_builder().horizontal(horizontalBounds.min, horizontalBounds.max);
    
    canvas = SDL_CreateRGBSurface(0, WIDTH, HEIGHT, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    aaCanvas = SDL_CreateRGBSurface(0, WIDTH, HEIGHT, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);


    using value_list_t = std::list<std::pair<double, double>>;
    FunctionSampler1D::SampleFunctionParams params;
    params.InitialPoints = 100;
    params.MaxRecursion = 20;
    value_list_t values;
    std::function<double(double)> funcd = [&func](double x) { return (double)func(x); };
    FunctionSampler1D::SampleFunction(funcd, -5, 5, params, values);



    u32* pixels = static_cast<u32*>(canvas->pixels);
    u32 x = 10, y = 15;

    auto f = values.begin();
    auto s = std::next(values.begin());
    for ( ; s != values.end(); ++f, ++s )
    {
      int x1 = std::roundf(horizontal(f->first)), y1 = std::roundf(vertical(f->second));
      int x2 = std::roundf(horizontal(s->first)), y2 = std::roundf(vertical(s->second));
      line(canvas, x1, y1, x2, y2, 0xffff0000);

      if (IS_INSIDE(x1, y1))
        AT(canvas, x1, y1) = 0xff000000;

      if (IS_INSIDE(x2, y2))
        AT(canvas, x2, y2) = 0xff000000;
    }

    /*const float dx = (horizontalBounds.max - horizontalBounds.min) / (WIDTH*100.0f);
    for (float fx = horizontalBounds.min; fx <= horizontalBounds.max; fx += dx)
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
    }*/

    /*
    for (int x = 0; x < WIDTH; ++x)
      for (int y = 0; y < HEIGHT; ++y)
      {
        int r = 0, g = 0, b = 0, a = 0;
        for (int dx = -2; dx <= 2; ++dx)
          for (int dy = -2; dy <= 2; ++dy)
          {
            if (dx != 0 || dy != 0)
            {
              int fx = x + dx, fy = y + dy;
              if (fx > 0 && fx < WIDTH && fy > 0 && fy < HEIGHT)
              {
                int distance = std::abs(dx) + std::abs(dy);
                float coefficient = 1.0f;
                switch (distance) {
                case 1: coefficient = 0.20f; break;
                case 2: coefficient = 0.08f; break;
                case 3: coefficient = 0.03f; break;
                case 4: coefficient = 0.01f; break;
                default: assert(false);
                };

                const u32 color = AT(canvas, fx, fy);
                r += ((color & 0x00ff0000) >> 16) * coefficient;
                g += (color & 0x0000ff00) >> 8;
                b += color & 0x000000ff;
                a += ((color & 0xff000000) >> 24) * coefficient;
              }
            }
          }

        if (r + g + b + a)
          AT(aaCanvas, x, y) = SDL_MapRGBA(aaCanvas->format, 255, 0, 0, a);
      }
     
    for (int x = 0; x < WIDTH; ++x)
      for (int y = 0; y < HEIGHT; ++y)
      {
        if (AT(canvas, x, y) == 0x00000000 && AT(aaCanvas, x, y) != 0)
          AT(canvas, x, y) += AT(aaCanvas, x, y);
      }
    */

    texture = SDL_CreateTextureFromSurface(manager->getRenderer(), canvas);
  }

  void GraphView::render()
  {
    auto* renderer = manager->getRenderer();

    if (!canvas)
      renderFunction([](float x) { return sin(x)*3; });
   
    
    SDL_SetRenderDrawColor(renderer, 255, 250, 237, 255);
    //SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  }

  void GraphView::handleKeyboardEvent(const SDL_Event& event)
  {

  }

  void GraphView::handleMouseEvent(const SDL_Event& event)
  {

  }
}
