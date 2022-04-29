#ifndef DISPLAY_TEXTLAYER_H_
#define DISPLAY_TEXTLAYER_H_

#include "layerobject.h"

namespace npnx {

	class TextRectLayer : public RectLayer {
	public:
		TextRectLayer() : mLeft(0), mBottom(0), mRight(0), mTop(0), mCentrosymmetric(false) {};
		TextRectLayer(float left, float bottom, float right, float top, float z_index, bool centrosymmetric = false);
		virtual ~TextRectLayer() = default;

		virtual int DrawGL(const int nbFrames) override;

		bool isInside(double x, double y, const int nbFrames);
	public:
		float mATransX = 0, mATransY = 0;
		float mTransX = 0, mTransY = 0;
		float mLeft, mBottom, mRight, mTop;
		bool mCentrosymmetric;
		std::string content;
	};

}

#endif

