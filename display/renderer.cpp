#include "renderer.h"
#include "layerobject.h"
#include "shader.h"

using namespace npnx;

Renderer::Renderer(Shader *defaultShader, unsigned int target_FBO):
  mDefaultShader(defaultShader),
  mFBO(target_FBO)
{}

Renderer::~Renderer()
{}

int Renderer::AddLayer(LayerObject *layer)
{
  mLayers[layer->z_index] = layer;
  layer->mParent = this;
  return 0;
}

void Renderer::Initialize()
{
  for(auto iter = mLayers.begin(); iter != mLayers.end(); ++iter) {
    iter->second->Initialize((int)mVBOBuffer.size() / 5, (int)mEBOBuffer.size());
  }

  // NPNX_LOG(mVBOBuffer.size());
  // NPNX_LOG(mEBOBuffer.size());

  // for(auto item: mVBOBuffer) {
  //   NPNX_LOG(item);
  // }

  // for(auto item:mEBOBuffer) {
  //   NPNX_LOG(item);
  // }

  glGenVertexArrays(1, &mVAO);
  glGenBuffers(1, &mVBO);
  glGenBuffers(1, &mEBO);
  glBindVertexArray(mVAO);
  glBindBuffer(GL_ARRAY_BUFFER, mVBO);
  glBufferData(GL_ARRAY_BUFFER, mVBOBuffer.size() * sizeof(float), mVBOBuffer.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, mEBOBuffer.size() * sizeof(GLuint), mEBOBuffer.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
}

void Renderer::Draw(const int nbFrames)
{
  glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
  mDefaultShader->Use();
  for(int i = 0; i < mDefaultTexture.size(); i++) {
    glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(GL_TEXTURE_2D, mDefaultTexture[i]); 
  }
  glBindVertexArray(mVAO);
  for (auto iter = mLayers.begin(); iter != mLayers.end(); ++iter) {
    iter->second->Draw(nbFrames);
  }
}

bool Renderer::Updated(const int nbFrames)
{
  for (auto iter = mLayers.begin(); iter != mLayers.end(); ++iter) {
    if (iter->second->Updated(nbFrames)) return true; 
  }
  return false;
}

void Renderer::FreeLayers() 
{
  for (auto iter:mLayers){
    if (iter.second) {
      delete iter.second;
    }
  }
  mLayers.clear();
}