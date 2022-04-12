#include <string>
#include "imageUtils.h"
#include "textrectlayer.h"
#include "renderer.h"

using namespace npnx;

TextRectLayer::TextRectLayer(float left, float bottom, float right, float top, float z_index, bool centrosymmetric):
  RectLayer(left, bottom, right, top, z_index),
  mLeft(left),
  mBottom(bottom),
  mRight(right),
  mTop(top),
  mCentrosymmetric(centrosymmetric),
  content("128")
{
    for (int i = 0; i < 10; i++) {
        std::string pos_texture_path = "text_";
        pos_texture_path += std::to_string(i);
        pos_texture_path += ".png";
        this->mTexture.push_back(makeTextureFromImage(NPNX_FETCH_DATA(pos_texture_path)));
    }
}

int TextRectLayer::DrawGL(const int nbFrames) 
{
  NPNX_ASSERT(!mTexture.empty());

  float tmpTransX = mTransX+mATransX;
  glUniform1f(glGetUniformLocation(mParent->mDefaultShader->mShader, "xTrans"), mTransX+mATransX);
  glUniform1f(glGetUniformLocation(mParent->mDefaultShader->mShader, "yTrans"), mTransY+mATransY);

  if (mCentrosymmetric) {
    glUniform1i(glGetUniformLocation(mParent->mDefaultShader->mShader, "centrosymmetric"), 1);
  }
  glActiveTexture(GL_TEXTURE0);

  int len = content.length();
  for (int i = 2; i >= 0; i--)
  {
      if (i < len) {
          glUniform1f(glGetUniformLocation(mParent->mDefaultShader->mShader, "xTrans"), tmpTransX);
          glBindTexture(GL_TEXTURE_2D, mTexture[content.at(len-i-1) - '0']);
          glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(mEBOOffset * sizeof(GLuint)));
      }
      tmpTransX += 0.07f;
  }

  if (mCentrosymmetric) {
    glUniform1i(glGetUniformLocation(mParent->mDefaultShader->mShader, "centrosymmetric"), 0);
  }

  glUniform1f(glGetUniformLocation(mParent->mDefaultShader->mShader, "xTrans"), 0.0f);
  glUniform1f(glGetUniformLocation(mParent->mDefaultShader->mShader, "yTrans"), 0.0f);
  return 0;
}

bool TextRectLayer::isInside(double x, double y, const int nbFrames) {
  if (!visibleCallback(nbFrames)) return false;
  double dx = x - mTransX - mATransX;
  double dy = y - mTransY - mATransY;
  if (mCentrosymmetric) {
    dx = x + mTransX + mATransX;
    dy = y + mTransX + mATransY;
  }
  return dx >= mLeft && dx <=mRight && dy >= mBottom && dy <=mTop;
}