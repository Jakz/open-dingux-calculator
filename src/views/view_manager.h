#pragma once

#include "sdl_helper.h"

#include "label_cache.h"

#include <array>

namespace ui
{
  class View
  {
  public:
    virtual void render() = 0;
    virtual void handleKeyboardEvent(const SDL_Event& event) = 0;
    virtual void handleMouseEvent(const SDL_Event& event) = 0;
  };

  struct ButtonStyle
  {
    bool pressed;
    bool hovered;
  };

  class ViewManager : public SDL<ViewManager, ViewManager>
  {
  public:
    using view_t = View;
    static const size_t VIEW_COUNT = 2;

  private:
    LabelCache<true> _cache;
    SDL_Texture* textureUI;
    TTF_Font *_font, *_tinyFont;

    std::array<view_t*, 3> views;
    view_t* view;

  public:
    ViewManager();

    bool loadData();

    void handleKeyboardEvent(const SDL_Event& event, bool press);
    void handleMouseEvent(const SDL_Event& event);
    void render();

    void deinit();

    TTF_Font* font() { return _font; }
    TTF_Font* tinyFont() { return _tinyFont; }

    void renderButton(int x, int y, int w, int h, const std::string& label, TTF_Font* font, SDL_Color color, ButtonStyle style);
    void renderButtonBackground(int x, int y, int w, int h, int bx, int by);

    LabelCache<true>* cache() { return &_cache; }
  };
}

