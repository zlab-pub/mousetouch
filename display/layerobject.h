#ifndef DISPLAY_LAYEROBJECT_H_
#define DISPLAY_LAYEROBJECT_H_

#include "common.h"
#include <vector>
#include <functional>

namespace npnx {
using std::vector; 

class Renderer;
class LayerObject {
public:
  LayerObject(){};
  explicit LayerObject(float z);
  virtual ~LayerObject() = default;

  virtual int Initialize(const int VBOOffset, const int EBOOffset);
  
  int Draw(const int nbFrames);
  virtual int DrawGL(const int nbFrames) = 0;
  virtual bool Updated(const int nbFrames);

  int defaultInitialize(const int VBOOffset, const int EBOOffset);

public: //avoid the tedious getter and setter functions. 
  vector<float> mVBOBuffer;
  vector<GLuint> mEBOBuffer;
  GLuint mVBOOffset, mEBOOffset;
  std::function<bool(const int)> visibleCallback = [](const int) { return true; };
  std::function<int(const int)> beforeDraw = [] (const int) {return 0;};
  std::function<int(const int)> afterDraw = [] (const int) {return 0;};
  float z_index;
  Renderer *mParent;  
};

class RectLayer: public LayerObject{
public:
  RectLayer(){};
  RectLayer(float left, float bottom, float right, float top, float z_index);
  virtual ~RectLayer() = default;

  virtual int DrawGL(const int nbFrames) override;

public:
  std::function<unsigned int(const int)> textureNoCallback = [] (const int) {return 0;};
  std::vector<GLuint> mTexture;
};

} // namespace npnx

#endif // !DISPLAY_LAYEROBJECT_H_