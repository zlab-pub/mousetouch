#include "layerobject.h"
#include "renderer.h"
#include "shader.h"

using namespace npnx;

LayerObject::LayerObject(float z):
mParent(NULL),
z_index(z)
{}

int LayerObject::Initialize(const int VBOOffset, const int EBOOffset) 
{
  return defaultInitialize(VBOOffset, EBOOffset);
}

int LayerObject::defaultInitialize(const int VBOOffset, const int EBOOffset)
{
  for (auto it : mVBOBuffer)
  {
    mParent->mVBOBuffer.push_back(it);
  }
  for (auto it : mEBOBuffer)
  {
    mParent->mEBOBuffer.push_back(VBOOffset + it);
  }
  mVBOOffset = VBOOffset;
  mEBOOffset = EBOOffset;
  return 0;
}

int LayerObject::Draw(const int nbFrames) {
  if (!visibleCallback(nbFrames)) return 0;

  {
    int suc = beforeDraw(nbFrames);
    NPNX_ASSERT(suc == 0);
  }

  this->DrawGL(nbFrames);

  {
    int suc = afterDraw(nbFrames);
    NPNX_ASSERT(suc == 0);
  }
  return 0;
}

bool LayerObject::Updated(const int nbFrames){
  return nbFrames < 5 || (visibleCallback(nbFrames - 1) ^ visibleCallback(nbFrames)) 
    || (visibleCallback(nbFrames - 2) ^ visibleCallback(nbFrames));
}

RectLayer::RectLayer(float left, float bottom, float right, float top, float z)
    : LayerObject(z)
{
  mVBOBuffer.assign({
    left, bottom, 1.0f, 0.0f, 0.0f,
    right, bottom, 1.0f, 1.0f, 0.0f,
    right, top, 1.0f, 1.0f, 1.0f,
    left, top, 1.0f, 0.0f, 1.0f
  });
  mEBOBuffer.assign({
    0,1,2,
    0,2,3
  });
  mTexture.clear();
}

int RectLayer::DrawGL(const int nbFrames)
{

  NPNX_ASSERT(!mTexture.empty());

  // Here we should do the following thing to return to default texture for GL_TEXTUREi for parent renderer.
  // But in our program we guarantte that GL_TEXTURE0 is set for every rect, so we need not to reset it.

  // int prevTextureBinding;
  // glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTextureBinding);
  // ...
  // glBindTexture(GL_TEXTURE_2D, prevTextureBinding);
  // TODO(after the thesis): found the texture type (i.e. GL_TEXTURE_3D) for current GL_TEXTUREi.
  
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mTexture[textureNoCallback(nbFrames)]);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)(mEBOOffset * sizeof(GLuint)));
  
  return 0;
}
