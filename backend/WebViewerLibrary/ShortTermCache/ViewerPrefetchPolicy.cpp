/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "ViewerPrefetchPolicy.h"

#include "ViewerToolbox.h"
#include "CacheScheduler.h"

#include <json/value.h>
#include <json/reader.h>
#include "Image/ImageController.h"


static const Json::Value::ArrayIndex PREFETCH_FORWARD = 10;
static const Json::Value::ArrayIndex PREFETCH_BACKWARD = 3;


namespace OrthancPlugins
{
  void ViewerPrefetchPolicy::ApplySeries(std::list<CacheIndex>& toPrefetch,
                                         CacheScheduler& cache,
                                         const std::string& series,
                                         const std::string& content)
  {
    Json::Value json;
    Json::Reader reader;
    if (!reader.parse(content, json) ||
        !json.isMember("Slices"))
    {
      return;
    }

    const Json::Value& instances = json["Slices"];
    if (instances.type() != Json::arrayValue)
    {
      return;
    }

    for (Json::Value::ArrayIndex i = 0; 
         i < instances.size() && i < PREFETCH_FORWARD; 
         i++)
    {
      std::string item = "jpeg95-" + instances[i].asString();
      toPrefetch.push_back(CacheIndex(CacheBundle_DecodedImage, item));
    }
  }


  void ViewerPrefetchPolicy::ApplyInstance(std::list<CacheIndex>& toPrefetch,
                                           CacheScheduler& cache,
                                           const std::string& path)
  {
    std::string instanceId;
    uint32_t frameIndex;
    std::auto_ptr<IImageProcessingPolicy> processingPolicy;

    ImageControllerUrlParser::parseUrlPostfix(path, instanceId, frameIndex, processingPolicy);

    // if the current quality is low, start to prefetch the higher quality:
    std::string currentQuality = processingPolicy->ToString();
    std::string nextQuality;
    if (currentQuality == "low-quality")
    {
      toPrefetch.push_back(CacheIndex(CacheBundle_DecodedImage, instanceId + "/" + boost::lexical_cast<std::string>(frameIndex) + "/medium-quality"));
      toPrefetch.push_back(CacheIndex(CacheBundle_DecodedImage, instanceId + "/" + boost::lexical_cast<std::string>(frameIndex) + "/high-quality"));
    }
    else if (currentQuality == "medium-quality")
      toPrefetch.push_back(CacheIndex(CacheBundle_DecodedImage, instanceId + "/" + boost::lexical_cast<std::string>(frameIndex) + "/high-quality"));

    return;

//    // get the instance information
//    Json::Value instance;
//    if (!GetJsonFromOrthanc(instance, context_, "/instances/" + instanceId) ||
//        !instance.isMember("ParentSeries"))
//    {
//      return;
//    }

//    // get the series information
//    std::string tmp;
//    if (!cache.Access(tmp, CacheBundle_SeriesInformation, instance["ParentSeries"].asString()))
//    {
//      return;
//    }
    
//    Json::Value series;
//    Json::Reader reader;
//    if (!reader.parse(tmp, series) ||
//        !series.isMember("Slices"))
//    {
//      return;
//    }

//    const Json::Value& instances = series["Slices"];

//    if (instances.type() != Json::arrayValue)
//    {
//      return;
//    }

//    Json::Value::ArrayIndex position = 0;
//    while (position < instances.size())
//    {
//      if (instances[position] == instanceAndFrame)
//      {
//        break;
//      }

//      position++;
//    }

//    if (position == instances.size())
//    {
//      return;
//    }

//    for (Json::Value::ArrayIndex i = position;
//         i < instances.size() && i < position + PREFETCH_FORWARD;
//         i++)
//    {
//      std::string item = compression + instances[i].asString();
//      toPrefetch.push_back(CacheIndex(CacheBundle_DecodedImage, item));
//    }

//    for (Json::Value::ArrayIndex i = position;
//         i >= 0 && i > position - PREFETCH_BACKWARD; )
//    {
//      i--;
//      std::string item = compression + instances[i].asString();
//      toPrefetch.push_back(CacheIndex(CacheBundle_DecodedImage, item));
//    }
  }


  void ViewerPrefetchPolicy::Apply(std::list<CacheIndex>& toPrefetch,
                                   CacheScheduler& cache,
                                   const CacheIndex& accessed,
                                   const std::string& content)
  {
    switch (accessed.GetBundle())
    {
      case CacheBundle_SeriesInformation:
        ApplySeries(toPrefetch, cache, accessed.GetItem(), content);
        return;

      case CacheBundle_DecodedImage:
        ApplyInstance(toPrefetch, cache, accessed.GetItem());
        return;

      default:
        return;
    }
  }
}

