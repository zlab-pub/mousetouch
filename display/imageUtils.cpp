#include "imageUtils.h"

#include "common.h"
#include <memory>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

unsigned int makeTexture(unsigned char *buffer, int width, int height, int channel)
{
  //only for BGR or BGRA image with 8bit fixed point depth.
  //if you have another format, write another function by copying most of this one.
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  switch (channel)
  {
  case 3:
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, buffer);
    break;
  case 4:
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, buffer);
    break;
  default:
    NPNX_ASSERT(channel != 3 && channel != 4);
    break;
  }
  // glGenerateMipmap(GL_TEXTURE_2D);
  return texture;
}

unsigned int makeTextureFromImage(const char * imagepath){
  cv::Mat img;
  img = cv::imread(imagepath, cv::IMREAD_UNCHANGED);
  NPNX_LOG(imagepath);
  // NPNX_LOG(img.rows);
  // NPNX_LOG(img.cols);
  // NPNX_LOG(img.channels());
  NPNX_ASSERT(img.isContinuous());
  NPNX_ASSERT(img.elemSize1() == 1);
  //unique_ptr only used for simple buffers, do not use it or shared ptr to generate class object, because my ability limitation.
  //  ——npnx
  return makeTexture(img.data, img.cols, img.rows, img.channels());
}

int readImageFlipped(const char *imagepath, uint8_t * buffer, size_t size, int & width, int & height, int & channel) {
	NPNX_LOG(imagepath);
  cv::Mat img(cv::imread(imagepath,cv::IMREAD_COLOR));
  cv::flip(img, img, 0);
  if (!img.isContinuous()) {
    img = img.clone();
  }
  int imgsize = img.cols * img.rows * img.channels();
  if (size<imgsize) {
    return -1;
  } else {
    width = img.cols;
    height = img.rows;
    channel = img.channels();
    memcpy(buffer, img.data, imgsize);
  }
  return imgsize;
}

void generateFBO(unsigned int &fbo, unsigned int &texColorBuffer)
{
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  // generate texture
  glGenTextures(1, &texColorBuffer);
  glBindTexture(GL_TEXTURE_2D, texColorBuffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glBindTexture(GL_TEXTURE_2D, texColorBuffer);
  // attach it to currently bound framebuffer object
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);

  unsigned int rbo;
  glGenRenderbuffers(1, &rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WINDOW_WIDTH, WINDOW_HEIGHT);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  NPNX_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}