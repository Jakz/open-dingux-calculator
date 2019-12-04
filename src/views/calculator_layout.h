#pragma once

#include "common.h"

#include "SDL.h"
#include "calculator.h"

#include <functional>
#include <cassert>

namespace gfx
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
    using lambda_t = std::function<void(calc::Calculator&)>;

    std::string label;
    SDL_Rect position;
    SDL_Rect gfx;
    SDL_Color color;
    lambda_t lambda;

  public:
    Button(std::string label, lambda_t lambda, SDL_Rect position, SDL_Rect gfx, SDL_Color color) : label(label), position(position), gfx(gfx), color(color), lambda(lambda) { }
    Button(std::string label, lambda_t lambda, SDL_Rect position, SDL_Rect gfx) : label(label), position(position), gfx(gfx), color({ 255, 255, 255 }), lambda(lambda) { }
  };

  struct ButtonSpec
  {
    std::string label;
    int x, y, w, h;
    SDL_Color color;
    Button::lambda_t lambda;

    ButtonSpec(std::string label, int x, int y, int w, int h, Button::lambda_t lambda) : label(label), x(x), y(y), w(w), h(h), color({ 255, 255, 255 }), lambda(lambda) { }
    ButtonSpec(std::string label, int x, int y, int w, int h, SDL_Color color, Button::lambda_t lambda) : label(label), x(x), y(y), w(w), h(h), color(color), lambda(lambda) { }
  };

  class DigitInputManager
  {
  private:
    bool _willRestartValue;
    bool _afterPointMode;
    s32 _afterPointDigits;

  public:
    DigitInputManager() : _willRestartValue(false), _afterPointMode(false), _afterPointDigits(0) { }

    
    void updateValue(int digit, calc::Calculator& calc)
    {
      if (_willRestartValue)
      {
        calc.pushValue();
        calc.set(0);
      }

      if (!_afterPointMode)
      {
        calc.value().exponent(calc.value().exponent() + 1);
        calc.value() += digit;
      }
      else
      {
        float_precision f = digit;
        f.exponent(-_afterPointDigits - 1);
        calc.value() += f;
        ++_afterPointDigits;
      }

      _willRestartValue = false;
    }

    void pointMode() { _afterPointMode = true; }
    void restartValue() { _willRestartValue = true; }

    void resetPointMode()
    {
      _afterPointMode = false;
      _afterPointDigits = 0;
    }

    void reset()
    {
      resetPointMode();
      _willRestartValue = true;
    }
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
      delete[] _grid;
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

        _buttons.emplace_back(button.label, button.lambda, SDL_Rect{ button.x, button.y, button.w, button.h }, SDL_Rect{ btx, bty, bw, bh }, button.color);
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

    void select(data_t::const_iterator it) { _selected = it; }
    bool hasSelection() const { return selected() != end(); }
    data_t::const_iterator selected() const { return _selected; }

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

  enum ValueRenderMode
  {
    DECIMAL
  };

  class LayoutHelper
  {
  protected:
    DigitInputManager digits;

  protected:
    void addNumberGrid(std::vector<ButtonSpec>& buttons, int bx, int by, int bw, int bh)
    {
      buttons.emplace_back(ButtonSpec{ "0", bx, by + 3 * bh, bw, bh, [this](calc::Calculator& c) { digits.updateValue(0, c); } });
      buttons.emplace_back(ButtonSpec{ "00", bx + bw, by + 3 * bh, bw, bh, [this](calc::Calculator& c) { digits.updateValue(0, c); digits.updateValue(0, c); } });
      buttons.emplace_back(ButtonSpec{ ".", bx + bw * 2, by + 3 * bh, bw, bh, [this](calc::Calculator& c) { digits.pointMode(); } });

      buttons.emplace_back(ButtonSpec{ "1", bx, by + 2 * bh, bw, bh, [this](calc::Calculator& c) { digits.updateValue(1, c); } });
      buttons.emplace_back(ButtonSpec{ "2", bx + bw, by + 2 * bh, bw, bh, [this](calc::Calculator& c) { digits.updateValue(2, c); } });
      buttons.emplace_back(ButtonSpec{ "3", bx + bw * 2, by + 2 * bh, bw, bh, [this](calc::Calculator& c) { digits.updateValue(3, c); } });

      buttons.emplace_back(ButtonSpec{ "4", bx, by + 1 * bh, bw, bh, [this](calc::Calculator& c) { digits.updateValue(4, c); } });
      buttons.emplace_back(ButtonSpec{ "5", bx + bw, by + 1 * bh, bw, bh, [this](calc::Calculator& c) { digits.updateValue(5, c); } });
      buttons.emplace_back(ButtonSpec{ "6", bx + bw * 2, by + 1 * bh, bw, bh, [this](calc::Calculator& c) { digits.updateValue(6, c); } });

      buttons.emplace_back(ButtonSpec{ "7", bx, by, bw, bh, [this](calc::Calculator& c) { digits.updateValue(7, c); } });
      buttons.emplace_back(ButtonSpec{ "8", bx + bw, by, bw, bh, [this](calc::Calculator& c) { digits.updateValue(8, c); } });
      buttons.emplace_back(ButtonSpec{ "9", bx + bw * 2, by, bw, bh, [this](calc::Calculator& c) { digits.updateValue(9, c); } });
    }

  public:
    using value_t = calc::Calculator::value_t;

    void renderValue(char* dest, size_t length, ValueRenderMode mode, const calc::Calculator::value_t& value)
    {
      float_precision i;
      auto f = modf(value, &i);
      
      if (value.get_mantissa().length() == value.exponent() - 1)
      {
        sprintf(dest, "%s", i.to_int_precision().toString().c_str());
      }
      else
        sprintf(dest, "%s", value.toPrecision(std::min(value.get_mantissa().length() + (value.exponent() < 0 ? 1 : 0), 10ULL)).c_str());


        
      /*value_t truncated = trunc(value);
      if (truncated == value)
        sprintf(dest, "%.0f", value);
      else
        sprintf(dest, "%f", value);*/
    }

  };

  class EasyLayout : public Layout, public LayoutHelper
  {
  private:
    static constexpr int BS = 2;
  public:
    EasyLayout() : Layout(18, 70, 20, 14, 2)
    {
      Button::lambda_t empty = [](calc::Calculator&) {};


      std::vector<ButtonSpec> buttons;

      buttons.push_back({ "√", 0, 4, 2, 2, { 200, 200, 200 }, [](calc::Calculator& c) { c.apply([](value_t v) { return sqrt(v); }); } });
      buttons.push_back({ "C", 0, 6, 2, 2, { 200, 50, 50 }, [this](calc::Calculator& c) {
        c.set(0); c.clearStacks(); digits.resetPointMode();
      } });
      buttons.push_back({ "AC", 0, 8, 2, 2, { 200, 50, 50 }, [this](calc::Calculator& c) { c.set(0); c.clearStacks(); c.clearMemory(); digits.resetPointMode(); } });

      buttons.push_back({ "MC", 0, 0, 2, 2, { 200, 200, 200 }, [](calc::Calculator& c) { c.clearMemory(); } });
      buttons.push_back({ "MR", 2, 0, 2, 2, { 200, 200, 200 }, [](calc::Calculator& c) { if (c.hasMemory()) c.set(c.memory()); } });
      buttons.push_back({ "M-", 4, 0, 2, 2, { 200, 200, 200 }, [](calc::Calculator& c) { c.setMemory(c.memory() - c.value()); } });
      buttons.push_back({ "M+", 6, 0, 2, 2, { 200, 200, 200 }, [](calc::Calculator& c) { c.setMemory(c.memory() + c.value()); } });
      buttons.push_back({ "MS", 0, 2, 2, 2, { 200, 200, 200 }, [](calc::Calculator& c) { c.setMemory(c.value()); } });

      buttons.push_back({ "÷", 9, 0, 2, 2, { 200, 200, 200 }, [this](calc::Calculator& c) { c.pushOperator([](value_t v1, value_t v2) { return v1 / v2; }); digits.reset(); } });
      buttons.push_back({ "×", 11, 0, 2, 2, { 200, 200, 200 }, [this](calc::Calculator& c) { c.pushOperator([](value_t v1, value_t v2) { return v1 * v2; }); digits.reset(); } });
      buttons.push_back({ "-", 11, 2, 2, 2, { 200, 200, 200 }, [this](calc::Calculator& c) { c.pushOperator([](value_t v1, value_t v2) { return v1 - v2; }); digits.reset(); } });
      buttons.push_back({ "+", 11, 4, 2, 4, { 200, 200, 200 }, [this](calc::Calculator& c) { c.pushOperator([](value_t v1, value_t v2) { return v1 + v2; }); digits.reset(); } });
      buttons.push_back({ "=", 11, 8, 2, 2, { 200, 200, 200 }, [this](calc::Calculator& c) { c.applyFromStack(); digits.reset(); } });


      LayoutHelper::addNumberGrid(buttons, 2, 2, 3, 2);
      initialize(buttons);
    }
  };
}
