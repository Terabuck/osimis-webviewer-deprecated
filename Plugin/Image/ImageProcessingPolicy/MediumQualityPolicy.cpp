#include "MediumQualityPolicy.h"
#include "ImageProcessingPolicy/ResizePolicy.h"
#include "ImageProcessingPolicy/Uint8ConversionPolicy.h"
#include "ImageProcessingPolicy/JpegConversionPolicy.h"
#include "ImageProcessingPolicy/KLVEmbeddingPolicy.h"

MediumQualityPolicy::MediumQualityPolicy()
{
  resampleAndJpegPolicy_.AddPolicy(new ResizePolicy(1000));
  resampleAndJpegPolicy_.AddPolicy(new Uint8ConversionPolicy()); // Does nothing if already 8bit
  resampleAndJpegPolicy_.AddPolicy(new JpegConversionPolicy(100));
  resampleAndJpegPolicy_.AddPolicy(new KLVEmbeddingPolicy());

  // @todo try/catch and clean memory on exception
}

MediumQualityPolicy::~MediumQualityPolicy()
{
}

IImageContainer* MediumQualityPolicy::Apply(IImageContainer* input, ImageMetaData* metaData)
{
  return resampleAndJpegPolicy_.Apply(input, metaData);
}
