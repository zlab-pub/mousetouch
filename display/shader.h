#ifndef DISPLAY_SHADER_H_
#define DISPLAY_SHADER_H_

#include "common.h"

namespace npnx {

class Shader {
public:
  Shader();
  ~Shader();
  
  int LoadShader(const char * vertexShaderPath, const char *fragmentShaderPath);
  
  void Use();

public:
  unsigned int mShader = 0;

private:
  bool mLoaded = false;
};

}

#endif // !DISPLAY_SHADER_H_