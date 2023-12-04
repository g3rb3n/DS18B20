#ifndef _Return_H_
#define _Return_H_

#include <inttypes.h>

template <typename T>
class Return
{
  private:
    T t;
    uint8_t _code = 0;

  public:
    Return(T v)
    :
    t(v)
    {     
    }
    
    T& operator=(const T& v)
    {
      t = v;
      return *this;
    }
    
    operator T()
    {
      return t;
    }
    
    bool valid()
    {
      return _code == 0;
    }
    
    uint8_t code()
    {
      return _code;
    }
    
    static Return error(uint8_t code)
    {
      Return<T> ret(0);
      ret._code = code;
      return ret;
    }
};

#endif
