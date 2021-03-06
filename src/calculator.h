#pragma once

#include <algorithm>
#include <unordered_map>

#include <stack>
#include <functional>

#include "precision/iprecision.h"
#include "precision/fprecision.h"

namespace calc
{
  using Value = float_precision;
  
  /*struct Value
  {
    static constexpr size_t MAX_PRECISION = 256;
    
    union
    {
      int_precision i;
      float_precision f;
    };

    enum class Mode { INT, FLOAT } mode;

    Value() : Value(0.0f) { }
    Value(int i) : i(i), mode(Mode::INT) { }
    Value(int_precision i) : i(i), mode(Mode::INT) { }
    Value(double f) : f(f), mode(Mode::FLOAT) { }
    Value(float_precision f) : f(f), mode(Mode::FLOAT) { }

    Value(const Value& other) : mode(other.mode)
    {
      if (mode == Mode::INT)
        i = other.i;
      else if (mode == Mode::FLOAT)
        f = other.f;
    }

    Value(Value&& other) : mode(std::move(other.mode))
    {
      if (mode == Mode::INT)
        i = std::move(other.i);
      else if (mode == Mode::FLOAT)
        f = std::move(other.f);
    }

    ~Value()
    {
      if (mode == Mode::INT)
        (&i)->~int_precision();
      else if (mode == Mode::FLOAT)
        (&f)->~float_precision();
    }
    
    Value& operator=(const Value& other)
    {
      mode = other.mode;

      if (mode == Mode::INT)
        i = other.i;
      else if (mode == Mode::FLOAT)
        f = other.f;

      return *this;
    }

    void toFloat()
    { 
      if (mode == Mode::INT)
      {
        f = float_precision(i.toString(), MAX_PRECISION);
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

    bool isInt() const { return mode == Mode::INT; }

    Value operator+(Value other) const
    {
      if (other.isInt() && !isInt()) other.toFloat();
      if (mode == Mode::INT) return Value(i + other.i);
      else if (mode == Mode::FLOAT) return Value(f + other.f);
      return Value();
    }

    Value operator-(Value other) const
    {
      if (other.isInt() && !isInt()) other.toFloat();
      if (mode == Mode::INT) return Value(i - other.i);
      else if (mode == Mode::FLOAT) return Value(f - other.f);
      return Value();
    }

    Value operator*(Value other) const
    {
      if (other.isInt() && !isInt()) other.toFloat();
      if (mode == Mode::INT) return Value(i * other.i);
      else if (mode == Mode::FLOAT) return Value(f * other.f);
      return Value();
    }

    Value operator/(Value other) const
    {
      if (other.isInt() && !isInt()) other.toFloat();
      if (mode == Mode::INT) return Value(i / other.i);
      else if (mode == Mode::FLOAT) return Value(f / other.f);
      return Value();
    }

    Value& operator+=(Value other)
    {
      if (other.isInt() && !isInt()) other.toFloat();
      if (mode == Mode::INT) i += other.i;
      else if (mode == Mode::FLOAT) f += other.f;
      return *this;
    }

    Value& operator-=(Value other)
    {
      if (other.isInt() && !isInt()) other.toFloat();
      *this = *this - other;
      return *this;
    }

    Value& operator*=(Value other)
    {
      if (other.isInt() && !isInt()) other.toFloat();
      if (mode == Mode::INT) i *= other.i;
      else if (mode == Mode::FLOAT) f *= other.f;
      return *this;
    }

    Value& operator/=(Value other)
    {
      if (other.isInt() && !isInt()) other.toFloat();
      *this = *this / other;
      return *this;
    }

    Value sqrt() const
    {
      assert(mode == Mode::FLOAT);
      return Value(::sqrt(f));
    }

    Value floor() const
    {
      assert(mode == Mode::FLOAT);
      return Value(::floor(f));
    }

    std::string toString() const
    {
      if (mode == Mode::INT) return i.toString();
      else if (mode == Mode::FLOAT) return f.toString();
      return std::string();
    }

    bool operator==(const float_precision& other) const
    {
      assert(mode == Mode::FLOAT && other.mode == Mode::FLOAT);
      return f == other.f;
    }

  };*/
  
  class Calculator
  {
  public:
    using value_t = Value;

    using binary_operator_t = std::function<value_t(value_t, value_t)>;
    using unary_operator_t = std::function<value_t(value_t)>;

  private:
    value_t _value;

    bool _hasMemory;
    value_t _memory;

    std::stack<value_t> _stack;
    std::stack<binary_operator_t> _operators;
    


  public:
    Calculator() : _value(0.0f), _hasMemory(false), _memory(0) { }

    void set(value_t value) { _value = value; }
    const value_t& value() const { return _value; }
    value_t& value() { return _value; }

    void pushValue()
    {
      _stack.push(_value);
    }

    void pushOperator(binary_operator_t op)
    {
      _operators.push(op);
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
