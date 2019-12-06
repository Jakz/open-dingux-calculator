#pragma once

#include "view_manager.h"

#include "calculator_layout.h"

namespace ui
{
  template<typename LayoutType>
  class CalculatorView : public ui::ViewManager::view_t
  {
  private:
    ui::ViewManager* gvm;
    calc::Calculator calculator;
    LayoutType layout;
    bool buttonPressed;

  public:
    CalculatorView(ui::ViewManager* gvm) : gvm(gvm), buttonPressed(false), layout(gvm) { }

    void render() override
    {
      auto* renderer = gvm->getRenderer();

      SDL_SetRenderDrawColor(renderer, 236, 232, 228, 255);
      SDL_RenderClear(renderer);

      for (auto it = layout.begin(); it != layout.end(); ++it)
      {
        const auto& button = *it;
        gvm->renderButton(button.gfx.x, button.gfx.y, button.gfx.w, button.gfx.h, button.label, button.font, button.color, { buttonPressed && it == layout.selected(), it == layout.selected() });
      }

      const auto& dbounds = layout.displayBounds();
      gvm->renderButtonBackground(dbounds.x, dbounds.y, dbounds.w, dbounds.h, 0, 0);

      static constexpr u32 VALUE_LABEL_KEY = 123;
      static char buffer[512];
      layout.renderValue(buffer, 512, calculator.value());
      auto texture = gvm->cache()->get(buffer, VALUE_LABEL_KEY, gvm->font());
      SDL_Rect dest = { dbounds.x + dbounds.w - 14, dbounds.y + 5, texture->second.rect.w, texture->second.rect.h };
      SDL_RenderCopy(renderer, texture->second.texture, nullptr, &dest);

      if (calculator.hasMemory())
        gvm->blit(gvm->cache()->texture(), gvm->cache()->get("m", gvm->tinyFont())->second, 20, 18);
    }

    void handleKeyboardEvent(const SDL_Event& event) override
    {
      const bool press = event.type == SDL_KEYDOWN;

      switch (event.key.keysym.sym)
      {
      case SDLK_ESCAPE:
        gvm->exit();
        break;

      case SDLK_LEFT:
        if (press)
          layout.hoverNext(-1, 0);
        buttonPressed = false;
        break;
      case SDLK_RIGHT:
        if (press)
          layout.hoverNext(1, 0);
        buttonPressed = false;
        break;
      case SDLK_UP:
        if (press)
          layout.hoverNext(0, -1);
        buttonPressed = false;
        break;
      case SDLK_DOWN:
        if (press)
          layout.hoverNext(0, 1);
        buttonPressed = false;
        break;
      case SDLK_SPACE:
        calculator.set(0);
        calculator.clearStacks();
        layout.getDigits().resetPointMode();
        break;
      case SDLK_LALT:
        if (!event.key.repeat)
        {
          buttonPressed = press;
          if (press && layout.hasSelection())
            layout.selected()->lambda(calculator);
        }

        break;
      }
    }

    void handleMouseEvent(const SDL_Event& event) override
    {
#if MOUSE_ENABLED
      if (event.button.type == SDL_MOUSEBUTTONDOWN)
      {
        for (auto it = layout.begin(); it != layout.end(); ++it)
        {
          const auto& button = *it;
          SDL_Point position = { event.button.x, event.button.y };
          if (SDL_PointInRect(&position, &button.gfx))
          {
            layout.select(it);
            buttonPressed = true;
            button.lambda(calculator);
          }
        }
      }
      else if (event.button.type == SDL_MOUSEBUTTONUP)
      {
        buttonPressed = false;
      }
#endif
    }
  };
}
