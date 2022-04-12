#include "shader.h"

#include <GL/glew.h>

#ifdef _WIN32
#include <GL/wglew.h>
#endif

#ifdef __linux__
#include <GL/glxew.h>
#endif

#include <GLFW/glfw3.h>

#include <fstream>
#include <sstream>
#include <string>

#include "common.h"

using namespace npnx;
using namespace std;

Shader::Shader(){
}
Shader::~Shader(){
}
  
int Shader::LoadShader(const char * vertexShaderPath, const char *fragmentShaderPath){
  ifstream vertexFile(vertexShaderPath);
  NPNX_ASSERT(vertexFile);

  ifstream fragmentFile(fragmentShaderPath);
  NPNX_ASSERT(fragmentFile);

  NPNX_LOG(vertexShaderPath);
  NPNX_LOG(fragmentShaderPath);
  
  stringstream vBuffer, fBuffer;
  string vCode, fCode;
  vBuffer << vertexFile.rdbuf();
  fBuffer << fragmentFile.rdbuf();
  
  vCode = vBuffer.str();
  fCode = fBuffer.str();
  
  vertexFile.close();
  fragmentFile.close();

  const char *vCode_c_str = vCode.c_str();
  const char *fCode_c_str = fCode.c_str();

  unsigned int vShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vShader, 1, &vCode_c_str, NULL);
  glCompileShader(vShader);
  int success;
  char log[8192];
  glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(vShader, 8192, NULL, log);
    NPNX_ASSERT_LOG(!"vertex shader error", log);
  }

  unsigned int fShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fShader, 1, &fCode_c_str, NULL);
  glCompileShader(fShader);
  glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(fShader, 8192, NULL, log);
    NPNX_ASSERT_LOG(!"frag shader error", log);
  }

  mShader = glCreateProgram();
  glAttachShader(mShader, vShader);
  glAttachShader(mShader, fShader);
  glLinkProgram(mShader);
  glGetProgramiv(mShader, GL_LINK_STATUS, &success);
  if (!success)
  {
    glGetProgramInfoLog(mShader, 8192, NULL, log);
    NPNX_ASSERT_LOG(!"linking error",log);
  }
  glDeleteShader(vShader);
  glDeleteShader(fShader);

  mLoaded = true;
  return 0;
}

void Shader::Use(){
  NPNX_ASSERT_LOG(mLoaded, "shader is not loaded");
  glUseProgram(mShader);
}
  
