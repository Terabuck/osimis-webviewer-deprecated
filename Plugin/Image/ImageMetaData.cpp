#include "ImageMetaData.h"

#include <cmath> // for std::pow
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include "../BenchmarkHelper.h"
#include "../../Orthanc/Core/Toolbox.h" // for TokenizeString && StripSpaces
#include "../../Orthanc/Core/Images/ImageProcessing.h" // for GetMinMaxValue
#include "../../Orthanc/Core/OrthancException.h" // for throws

namespace
{
  float GetFloatTag(const Json::Value& dicomTags,
                           const std::string& tagId,
                           float defaultValue);
  bool GetStringTag(std::string& result,
                           const Json::Value& dicomTags,
                           const std::string& tagId);
}

using namespace Orthanc;

ImageMetaData::ImageMetaData()
{
  color = false;
  height = 0;
  width = 0;
  sizeInBytes = 0;

  columnPixelSpacing = 0;
  rowPixelSpacing = 0;

  minPixelValue = 0;
  maxPixelValue = 0;
  slope = 0;
  intercept = 0;
  windowCenter = 0;

  // frontend webviewer related
  isSigned = false;
  stretched = false;
  compression = "raw";

  originalHeight = 0;
  originalWidth = 0;
}

ImageMetaData::ImageMetaData(RawImageContainer* rawImage, const Json::Value& dicomTags)
{
  BENCH(CALCULATE_METADATA)
  ImageAccessor* accessor = rawImage->GetOrthancImageAccessor();

  // define
  // - color
  // - minPixelValue & maxPixelValue (calculated)
  // - windowCenter & windowWidth (default values)
  switch (accessor->GetFormat())
  {
    case PixelFormat_Grayscale8:
    case PixelFormat_Grayscale16:
    case PixelFormat_SignedGrayscale16:
    {
      int64_t a, b;

      color = false;

      // @todo don't process when tag is available
      ImageProcessing::GetMinMaxValue(a, b, *accessor);
      minPixelValue = (a < 0 ? static_cast<int32_t>(a) : 0);
      maxPixelValue = (b > 0 ? static_cast<int32_t>(b) : 1);
      
      windowCenter = static_cast<float>(a + b) / 2.0f;
      
      if (a == b)
      {
        windowWidth = 256.0f;  // Arbitrary value
      }
      else
      {
        windowWidth = static_cast<float>(b - a) / 2.0f;
      }

      break;
    }
    case PixelFormat_RGB24:
    {
      color = true;

      minPixelValue = 0;
      maxPixelValue = 255;
      windowCenter = 127.5f;
      windowWidth = 256.0f;
      break;
    }
    default:
    {
      // @todo throw
    }
  }

  // set width/height
  height = accessor->GetHeight();
  width = accessor->GetWidth();
  
  // set sizeInBytes
  sizeInBytes = accessor->GetSize();

  // set slope/intercept
  slope = GetFloatTag(dicomTags, "0028,1053", 1.0f);
  intercept = GetFloatTag(dicomTags, "0028,1052", 0.0f);

  // set windowCenter & windowWidth (image specific)
  windowCenter = GetFloatTag(dicomTags, "0028,1050", windowCenter * slope + intercept);
  windowWidth = GetFloatTag(dicomTags, "0028,1051", windowWidth * slope);

  // set rowPixelSpacing/columnPixelSpacing
  bool dicomHasPixelSpacing = false;
  std::string pixelSpacing;
  if (GetStringTag(pixelSpacing, dicomTags, "0028,0030"))
  {
    std::vector<std::string> tokens;
    Toolbox::TokenizeString(tokens, pixelSpacing, '\\');

    if (tokens.size() >= 2)
    {
      try
      {
        columnPixelSpacing = boost::lexical_cast<float>(Toolbox::StripSpaces(tokens[1]));
        rowPixelSpacing = boost::lexical_cast<float>(Toolbox::StripSpaces(tokens[0]));
        dicomHasPixelSpacing = true;
      }
      catch (boost::bad_lexical_cast&)
      {
      }
    }
  }

  if (!dicomHasPixelSpacing)
  {
    columnPixelSpacing = 1.0f;
    rowPixelSpacing = 1.0f;
  }

  // frontend webviewer related
  isSigned = (accessor->GetFormat() == PixelFormat_SignedGrayscale16);
  stretched = false;
  compression = "raw";

  originalHeight = accessor->GetHeight();
  originalWidth = accessor->GetWidth();

  BENCH_LOG(IMAGE_WIDTH, width);
  BENCH_LOG(IMAGE_HEIGHT, height);
}

ImageMetaData::ImageMetaData(const DicomMap& headerTags, const Json::Value& dicomTags)
{
  BENCH(CALCULATE_METADATA)

  // define color
  std::string photometricInterpretation = dicomTags["PhotometricInterpretation"].asString();
  if (photometricInterpretation == "MONOCHROME1" || photometricInterpretation == "MONOCHROME2") {
    color = false;
  }
  else {
    color = true;
  }


  // set minPixelValue & maxPixelValue
  int bitsStored = boost::lexical_cast<int>(dicomTags["BitsStored"]);
  minPixelValue = 0; // approximative value
  maxPixelValue = std::pow(2, bitsStored); // approximative value

  // set width/height
  width = boost::lexical_cast<uint32_t>(dicomTags["Rows"].asString());
  height = boost::lexical_cast<uint32_t>(dicomTags["Columns"].asString());

  // set sizeInBytes
  int bitsAllocated = boost::lexical_cast<int>(dicomTags["BitsAllocated"]) * (color ? 3 : 1);
  sizeInBytes = width * height * bitsAllocated;

  // set slope/intercept
  slope = GetFloatTag(dicomTags, "0028,1053", 1.0f);
  intercept = GetFloatTag(dicomTags, "0028,1052", 0.0f);

  // set windowCenter & windowWidth (image specific)
  windowCenter = GetFloatTag(dicomTags, "0028,1050", 127.5f * slope + intercept);
  windowWidth = GetFloatTag(dicomTags, "0028,1051", 256.0f * slope);

  // set rowPixelSpacing/columnPixelSpacing
  bool dicomHasPixelSpacing = false;
  std::string pixelSpacing;
  if (GetStringTag(pixelSpacing, dicomTags, "0028,0030"))
  {
    std::vector<std::string> tokens;
    Toolbox::TokenizeString(tokens, pixelSpacing, '\\');

    if (tokens.size() >= 2)
    {
      try
      {
        columnPixelSpacing = boost::lexical_cast<float>(Toolbox::StripSpaces(tokens[1]));
        rowPixelSpacing = boost::lexical_cast<float>(Toolbox::StripSpaces(tokens[0]));
        dicomHasPixelSpacing = true;
      }
      catch (boost::bad_lexical_cast&)
      {
      }
    }
  }

  if (!dicomHasPixelSpacing)
  {
    columnPixelSpacing = 1.0f;
    rowPixelSpacing = 1.0f;
  }

  // frontend webviewer related

  // set signed pixels
  isSigned = boost::lexical_cast<bool>(dicomTags["PixelRepresentation"].asString());

  // set stretched image (16bit -> 8bit dynamic compression)
  stretched = false;

  // Retrieve transfer syntax
  const DicomValue* transfertSyntaxValue = headerTags.TestAndGetValue(0x0002, 0x0010);
  std::string transferSyntax;

  if (transfertSyntaxValue->IsBinary()) {
    throw OrthancException(static_cast<ErrorCode>(OrthancPluginErrorCode_CorruptedFile));
  }
  else if (transfertSyntaxValue == NULL || transfertSyntaxValue->IsNull()) {
    // Set default transfer syntax if not found
    transferSyntax = "1.2.840.10008.1.2";
  }
  else {
    // Stripping spaces should not be required, as this is a UI value
    // representation whose stripping is supported by the Orthanc
    // core, but let's be careful...
    transferSyntax = Toolbox::StripSpaces(transfertSyntaxValue->GetContent());
  }

  // set compression format @todo
  boost::regex regexp("^1\\.2\\.840\\.10008\\.1\\.2\\.4\\.(\\d\\d)$"); // see http://www.dicomlibrary.com/dicom/transfer-syntax/
  boost::cmatch matches;
  if (boost::regex_match(transferSyntax.c_str(), matches, regexp)) {
    switch(boost::lexical_cast<uint32_t>(matches[1])) {
    case 50: // Lossy JPEG 8-bit Image Compression
      compression = "jpeg";
      break;
    case 51: // Lossy JPEG 12-bit Image Compression
      compression = "jpeg";
      break;
    case 57: // JPEG Lossless, Nonhierarchical (Processes 14)
      compression = "jpeg";
      break;
    case 70: // JPEG Lossless, Nonhierarchical, First-Order Prediction (Default Transfer Syntax for Lossless JPEG Image Compression)
      compression = "jpeg";
      break;
    case 80: // JPEG-LS Lossless Image Compression
      compression = "jpeg";
      break;
    case 81: // JPEG-LS Lossy (Near- Lossless) Image Compression
      compression = "jpeg";
      break;
    case 90: // JPEG 2000 Image Compression (Lossless Only)
      compression = "jpeg2000";
      break;
    case 91: // JPEG 2000 Image Compression
      compression = "jpeg2000";
      break;
    case 92: // JPEG 2000 Part 2 Multicomponent Image Compression (Lossless Only)
      compression = "jpeg2000";
      break;
    case 93: // JPEG 2000 Part 2 Multicomponent Image Compression
      compression = "jpeg2000";
      break;

    case 94: // JPIP Referenced
      compression = "jpip";
      break;
    case 95: // JPIP Referenced Deflate
      compression = "jpip";
      break;

    default:
      assert(true);
      break;
    }
  }
  else {
    assert(true);
  }

  // set original height & width
  originalHeight = boost::lexical_cast<uint32_t>(dicomTags["Rows"].asString());
  originalWidth = boost::lexical_cast<uint32_t>(dicomTags["Columns"].asString());

  BENCH_LOG(IMAGE_WIDTH, width);
  BENCH_LOG(IMAGE_HEIGHT, height);
}

namespace {
  float GetFloatTag(const Json::Value& dicomTags,
                           const std::string& tagId,
                           float defaultValue)
  {
    std::string tmp;
    if (GetStringTag(tmp, dicomTags, tagId))
    {
      try
      {
        return boost::lexical_cast<float>(Toolbox::StripSpaces(tmp));
      }
      catch (boost::bad_lexical_cast&)
      {
      }
    }

    return defaultValue;
  }

  bool GetStringTag(std::string& result,
                           const Json::Value& dicomTags,
                           const std::string& tagId)
  {
    if (dicomTags.type() == Json::objectValue &&
        dicomTags.isMember(tagId) &&
        dicomTags[tagId].type() == Json::objectValue &&
        dicomTags[tagId].isMember("Type") &&
        dicomTags[tagId].isMember("Value") &&
        dicomTags[tagId]["Type"].type() == Json::stringValue &&
        dicomTags[tagId]["Value"].type() == Json::stringValue &&
        dicomTags[tagId]["Type"].asString() == "String")
    {
      result = dicomTags[tagId]["Value"].asString();
      return true;
    }        
    else
    {
      return false;
    }
  }
}
