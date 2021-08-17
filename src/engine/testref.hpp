#include <core/type/typetraits.hpp>

namespace eXl
{
  class MyClass
  {
    EXL_REFLECT;
  public:
    int field = 0;
    static int static_field;
    void method();
    static void static_method();
  };
}