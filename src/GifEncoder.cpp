//
//  GifEncoder.cpp
//  GifEncoder
//
//  Created by Jesus Gollonet on 3/20/11.

#include "GifEncoder.h"
#include <iostream>

// for some reason we're not seeing this from freeimage
#define DWORD uint32_t
#define BYTE uint8_t

//ofEvent<string>	ofxGifEncoder::GIF_SAVE_FINISHED;

GifEncoder::GifEncoder() {
}

void GifEncoder::setup(int _w, int _h, float _frameDuration, int _nColors) {
	if (_nColors < 2 || _nColors > 256) {
		CI_LOG_V("ofxGifEncoder: nColors must be between 2 and 256. set to 256");
		nColors = 256;
	}
	w = _w;
	h = _h;

	frameDuration = _frameDuration;
	nColors = _nColors;
	bitsPerPixel = 0;
	nChannels = 0;
	ditherMode = GIF_DITHER_NONE;
	bSaving = false;
	greenScreenColor = Color(0, 255, 0);
}

//--------------------------------------------------------------
GifEncoder::GifFrame * GifEncoder::createGifFrame(unsigned char * px, int _w, int _h, int _bitsPerPixel, float _duration) {
	GifFrame * gf = new GifFrame();
	gf->pixels = px;
	gf->width = _w;
	gf->height = _h;
	gf->duration = _duration;
	gf->bitsPerPixel = _bitsPerPixel;
	return gf;
}

void GifEncoder::addFrame(ci::gl::Texture2dRef img, float _duration) {

	if (img->getWidth() != w || img->getHeight() != h) {
		CI_LOG_V("GifEncoder::addFrame image dimensions don't match, skipping frame");
		return;
	}
	string filename = toString(getElapsedFrames()) + ".jpg";
	fs::path fr = getAssetPath("") / filename;
    Surface8u pixels(img->createSource(), SurfaceConstraintsDefault(), false);
    
//    pixels.setChannelOrder(SurfaceChannelOrder(SurfaceChannelOrder::RGB));
    
//    writeImage(writeFile(fr), pixels);

    const int32_t nChannels = pixels.getPixelBytes();
//    CI_LOG_V(nChannels);
//
//    unsigned char * px = new unsigned char[w * h * nChannels];
//
//    int pxNum = 0;
//    for (int32_t y = 0; y < pixels.getHeight(); ++y) {
//        for (int32_t x = 0; x < pixels.getWidth(); ++x) {
//            px[pxNum++] = *(unsigned char*)pixels.getDataRed(ivec2(x, y));
//            px[pxNum++] = *(unsigned char*)pixels.getDataGreen(ivec2(x, y));
//            px[pxNum++] = *(unsigned char*)pixels.getDataBlue(ivec2(x, y));
//        }
//    }
    
	addFrame(pixels.getData(), w, h, 8 * nChannels,  _duration);
	
}

void GifEncoder::addFrame(unsigned char *px, int _w, int _h, int _bitsPerPixel, float duration) {
	if (_w != w || _h != h) {
		CI_LOG_V("GifEncoder::addFrame image dimensions don't match, skipping frame");
		return;
	}

	float tempDuration = duration;
	if (tempDuration == 0.f) tempDuration = frameDuration;

	nChannels = 0;
	switch (_bitsPerPixel) {
	case 8:     nChannels = 1;      break;
	case 24:    nChannels = 3;      break;
	case 32:    nChannels = 4;      break;
	default:
		CI_LOG_V("bitsPerPixel should be 8, 24 or 32. skipping frame");
		return;
		break;
	}

     unsigned char * temp = new unsigned char[w * h * nChannels];
    memcpy(temp, px, w * h * nChannels);
	GifFrame * gifFrame = GifEncoder::createGifFrame(temp, w, h, _bitsPerPixel, tempDuration);
	frames.push_back(gifFrame);
}

void GifEncoder::setNumColors(int _nColors) {
	nColors = _nColors;
}

void GifEncoder::setDitherMode(int _ditherMode) {
	ditherMode = _ditherMode;
}

void GifEncoder::setFrameDuration(float _duration) {
	for (int i = 0; i < frames.size(); i++) {
		frames[i]->duration = _duration;
	}
}

//--------------------------------------------------------------
void GifEncoder::save(string _fileName) {
	if (bSaving) {
		CI_LOG_V("GifEncoder is already saving. wait for GIF_SAVE_FINISHED");
		return;
	}
	bSaving = true;
	fileName = _fileName;
	doSave();
}

void GifEncoder::doSave() {
	// create a multipage bitmap
//    fs::path fr = getAssetPath("") / fileName;
	FIMULTIBITMAP *multi = FreeImage_OpenMultiBitmap(FIF_GIF, fileName.c_str(), TRUE, FALSE);
	for (int i = 0; i < frames.size(); i++) {
		GifFrame * currentFrame = frames[i];
		processFrame(currentFrame, multi);
		//CI_LOG_I(i);
	}
	FreeImage_CloseMultiBitmap(multi);
//    CI_LOG_I("done");
	bSaving = false;
}

void GifEncoder::calculatePalette(FIBITMAP * bmp) {
	RGBQUAD *pal = FreeImage_GetPalette(bmp);

	for (int i = 0; i < 256; i++) {
		palette.push_back(Color(pal[i].rgbRed, pal[i].rgbGreen, pal[i].rgbBlue));
		CI_LOG_V(palette.at(i));
	}
}

int GifEncoder::getClosestToGreenScreenPaletteColorIndex() {
	CI_LOG_V("computing closest palette color");

	float minDistance = 100000;
	int closestIndex = 0;
	vec3 greenScreenVec(greenScreenColor.r, greenScreenColor.g, greenScreenColor.b);
	for (int i = 0; i < palette.size(); i++) {

		vec3 currentVec(palette.at(i).r, palette.at(i).g, palette.at(i).b);
		/* TODO: float currentDistance = currentVec.distance(greenScreenVec);
		if (currentDistance < minDistance){
			minDistance = currentDistance;*/
		closestIndex = i;
		//}
	}
	return closestIndex;

}

void GifEncoder::processFrame(GifFrame * frame, FIMULTIBITMAP *multi) {
    FIBITMAP * bmp = NULL;
    // we need to swap the channels if we're on little endian (see ofImage::swapRgb);
    
    
    if (frame->bitsPerPixel ==32){
        CI_LOG_V("image is transparent!");
        frame = convertTo24BitsWithGreenScreen(frame);
    }
    
//#ifdef TARGET_LITTLE_ENDIAN
    swapRgb(frame);
//#endif
    
    
    // from here on, we can only deal with 24 bits
    
    // get the pixel data
    bmp    = FreeImage_ConvertFromRawBits(
                                          frame->pixels,
                                          frame->width,
                                          frame->height,
                                          frame->width*(frame->bitsPerPixel/8),
                                          frame->bitsPerPixel,
                                          0, 0, 0, true // in of006 this (topdown) had to be false.
                                          );
    
//    FIBITMAP * bmpConverted;
    
//#ifdef TARGET_LITTLE_ENDIAN
    swapRgb(frame);
//#endif
    
    FIBITMAP * quantizedBmp = NULL;
    FIBITMAP * ditheredBmp  = NULL;
    FIBITMAP * processedBmp = NULL;
    
    
    quantizedBmp = FreeImage_ColorQuantizeEx(bmp, FIQ_WUQUANT, nColors);
    processedBmp = quantizedBmp;
    
    if (nChannels == 4){
        calculatePalette(processedBmp);
        FreeImage_SetTransparentIndex(processedBmp,getClosestToGreenScreenPaletteColorIndex());
    }
    
    
    
    // dithering :)
    if(ditherMode > GIF_DITHER_NONE) {
        ditheredBmp = FreeImage_Dither(processedBmp, (FREE_IMAGE_DITHER)ditherMode);
        processedBmp = ditheredBmp;
    }
    
    DWORD frameDuration = (DWORD) (frame->duration * 1000.f);
    
    // clear any animation metadata used by this dib as we’ll adding our own ones
    FreeImage_SetMetadata(FIMD_ANIMATION, processedBmp, NULL, NULL);
    // add animation tags to dib[frameNum]
    FITAG *tag = FreeImage_CreateTag();
    if(tag) {
        FreeImage_SetTagKey(tag, "FrameTime");
        FreeImage_SetTagType(tag, FIDT_LONG);
        FreeImage_SetTagCount(tag, 1);
        FreeImage_SetTagLength(tag, 4);
        FreeImage_SetTagValue(tag, &frameDuration);
        FreeImage_SetMetadata(FIMD_ANIMATION, processedBmp, FreeImage_GetTagKey(tag), tag);
        FreeImage_DeleteTag(tag);
    }
    
    FreeImage_AppendPage(multi, processedBmp);
    
    // clear freeimage stuff
    if(bmp          != NULL) FreeImage_Unload(bmp);
    if(quantizedBmp != NULL) FreeImage_Unload(quantizedBmp);
    if(ditheredBmp  != NULL) FreeImage_Unload(ditheredBmp);
    
    // no need to unload processedBmp, as it points to either of the above
}

GifEncoder::GifFrame * GifEncoder::convertTo24BitsWithGreenScreen(GifFrame * frame) {
	Color otherColor(0, 255, 0);

	int width = frame->width;
	int height = frame->height;

	unsigned char * newPixels = new unsigned char[width * height * 3];

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			ColorA c = ColorA(
				frame->pixels[(j * width + i) * 4 + 0],
				frame->pixels[(j * width + i) * 4 + 1],
				frame->pixels[(j * width + i) * 4 + 2],
				frame->pixels[(j * width + i) * 4 + 3]
			);

			float normalAlpha = c.a / 255.f;
			float inverseAlpha = 1.f - normalAlpha;

			Color newColor = Color(
				c.r * normalAlpha + (otherColor.r * inverseAlpha),
				c.g * normalAlpha + (otherColor.g * inverseAlpha),
				c.b * normalAlpha + (otherColor.b *inverseAlpha)
			);

			newPixels[(j * width + i) * 3 + 0] = newColor.r;
			newPixels[(j * width + i) * 3 + 1] = newColor.g;
			newPixels[(j * width + i) * 3 + 2] = newColor.b;
		}

	}

	GifFrame * newFrame = GifEncoder::createGifFrame(newPixels, width, height, 24, frame->duration);
	return newFrame;
}


// from ofimage
//----------------------------------------------------
void GifEncoder::swapRgb(GifFrame * pix) {
	if (pix->bitsPerPixel != 8) {
		int sizePixels = pix->width * pix->height;
		int cnt = 0;
		unsigned char temp;
		int byteCount = pix->bitsPerPixel / 8;

		while (cnt < sizePixels) {
			temp = pix->pixels[cnt*byteCount];
			pix->pixels[cnt*byteCount] = pix->pixels[cnt*byteCount + 2];
			pix->pixels[cnt*byteCount + 2] = temp;
			cnt++;
		}
	}
}

void GifEncoder::exit() {

}

void GifEncoder::reset() {
	if (bSaving) {
		CI_LOG_V("GifEncoder is saving. wait for GIF_SAVE_FINISHED to reset");
		return;
	}
	for (int i = 0; i < frames.size(); i++) {
		delete frames[i]->pixels;
		delete frames[i];
	}
	frames.clear();
}


