#include <algorithm>
#include <unordered_map>

#include <stack>
#include <functional>

#include "precision/iprecision.h"
#include "precision/fprecision.h"

namespace calc
{
  struct Value
  {
    static constexpr size_t MAX_PRECISION = 256;
    
    union
    {
      int_precision i;
      float_precision f;
    };

    enum class Mode { INT, FLOAT } mode;

    void toFloat()
    { 
      if (mode == Mode::INT)
      {
        f = float_precision(i.toString(), i.size()*2);
      }
    }

    void toInt()
    {
      if (mode == Mode::FLOAT)
      {
        i = f.to_int_precision();
        mode = Mode::INT;
      }
    }
  };
  
  class Calculator
  {
  public:
    using value_t = double;

    using binary_operator_t = std::function<value_t(value_t, value_t)>;
    using unary_operator_t = std::function<value_t(value_t)>;

  private:
    value_t _value;

    bool _hasMemory;
    value_t _memory;

    std::stack<value_t> _stack;
    std::stack<binary_operator_t> _operators;
    
    std::string _strValue;
    bool _willRestartValue;

    enum class PointMode
    {
      INTEGRAL,
      POINT,
      AFTER_POINT
    } _pointMode;

  public:
    Calculator() : _willRestartValue(false), _value(0), _hasMemory(false), _memory(0) { }

    void set(value_t value) { _value = value; }
    value_t value() const { return _value; }

    void point()
    {
      _pointMode = PointMode::POINT;
    }

    void digit(int digit)
    {
      if (_willRestartValue)
      {
        _stack.push(_value);
        _value = digit;

        if (_pointMode == PointMode::POINT)
        {
          _value /= 10;
          _pointMode = PointMode::AFTER_POINT;
        }

        _willRestartValue = false;
      }
      else
      {
        if (_pointMode == PointMode::INTEGRAL)
        {
          _value *= 10;
          _value += digit;
        }
        else if (_pointMode == PointMode::POINT)
        {
          _value += digit / 10.0;
        }
        else if (_pointMode == PointMode::AFTER_POINT)
        {

        }
      }

    }

    void pushValue()
    {
      _stack.push(_value);
    }

    void pushOperator(binary_operator_t op)
    {
      _operators.push(op);
      _willRestartValue = true;
    }

    void apply(const unary_operator_t& op)
    {
      _value = op(_value);
    }

    void applyFromStack()
    {
      if (!_operators.empty() && !_stack.empty())
      {
        auto op = _operators.top();
        _operators.pop();
        _value = op(_stack.top(), _value);
        _stack.pop();
      }
    }

    void clearStacks()
    {
      _stack = std::stack<value_t>();
      _operators = std::stack<binary_operator_t>();
    }

    void clearMemory()
    {
      _hasMemory = false;
      _value = 0;
    }

    void saveMemory()
    {
      _hasMemory = true;
      _memory = _value;
    }

    void setMemory(value_t value)
    {
      _hasMemory = true;
      _memory = value;
    }

    bool hasMemory() const { return _hasMemory; }
    value_t memory() const { return _memory; }
  };
}
