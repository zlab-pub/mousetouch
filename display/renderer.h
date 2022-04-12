#ifndef DISPLAY_RENDERER_H_
#define DISPLAY_RENDERER_H_

#include <vector>
#include <map>
#include "common.h"
#include "shader.h"

namespace npnx {

class LayerObject;

class Renderer {
public:
  explicit Renderer(Shader *defaultShader, unsigned int target_FBO);
  ~Renderer();
  
  int AddLayer(LayerObject * layer);
  void Initialize();
  
  bool Updated(const int nbFrames);
  void Draw(const int nbFrames);

  void FreeLayers();
private:
  //we can only add layers when the renderer is not initialized.
  bool mInitialized = false;

public:
  std::map<float, LayerObject *> mLayers;
  std::vector<float> mVBOBuffer;
  std::vector<GLuint> mEBOBuffer;
  unsigned int mVAO, mVBO, mEBO, mFBO;
  Shader *mDefaultShader;
  std::vector<GLuint> mDefaultTexture; //default texture binding for default shader, GL_TEXTUREi is binding to mDefaultTexture[i];
};


}
#endif // !DISPLAY_REDERER_H_