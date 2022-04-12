#ifndef DISPLAY_IMAGEUTILS_H_
#define DISPLAY_IMAGEUTILS_H_

#include <cstdint>

unsigned int makeTexture(unsigned char *buffer, int width, int height, int channel = 4);
unsigned int makeTextureFromImage(const char *imagepath);

template <typename T>
void generateRandomArray(T *array, const size_t size, const int lb = 0, const int ub = 255)
{
  NPNX_ASSERT(lb <= ub);
  for (size_t i = 0; i < size; i++)
  {
    array[i] = (T)((i % (ub + 1 - lb) + lb));
  }
}

void generateFBO(unsigned int &fbo, unsigned int &texColorBuffer);

int readImageFlipped(const char *imagepath, uint8_t * buffer, size_t size, int & width, int & height, int & channel);

#endif // !DISPLAY_IMAGEUTILS_H_ 