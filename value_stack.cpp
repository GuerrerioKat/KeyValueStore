#include "value_stack.h"
#include "exceptions.h"

ValueStack::ValueStack()
  // TODO: initialize member variable(s) (if necessary)
{
}

ValueStack::~ValueStack()
{
}

bool ValueStack::is_empty() const
{
  return v_stack.empty();
}

void ValueStack::push( const std::string &value )
{
  v_stack.push_back(value);
}

std::string ValueStack::get_top() const
{
  if (is_empty()) {
    throw OperationException("Stack is empty");
  }
  return v_stack.back();
}

void ValueStack::pop()
{
  if (is_empty()) {
    throw OperationException("Stack is empty");
  }
  v_stack.pop_back();
}
