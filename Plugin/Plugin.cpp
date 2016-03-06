/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2015 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
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


#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <EmbeddedResources.h>
#include <boost/filesystem.hpp>

#include "../Orthanc/Core/OrthancException.h"
#include "../Orthanc/Core/Toolbox.h"
#include "ViewerToolbox.h"
#include "ViewerPrefetchPolicy.h"
#include "DecodedImageAdapter.h"
#include "SeriesInformationAdapter.h"
#include "../Orthanc/Plugins/Samples/GdcmDecoder/GdcmDecoderCache.h"
#include "../Orthanc/Core/Toolbox.h"


static OrthancPluginContext* context_ = NULL;



class CacheContext
{
private:
  class DynamicString : public Orthanc::IDynamicObject
  {
  private:
    std::string  value_;

  public:
    DynamicString(const char* value) : value_(value)
    {
    }
    
    const std::string& GetValue() const
    {
      return value_;
    }
  };

  Orthanc::FilesystemStorage  storage_;
  Orthanc::SQLite::Connection  db_;

  std::auto_ptr<OrthancPlugins::CacheManager>  cache_;
  std::auto_ptr<OrthancPlugins::CacheScheduler>  scheduler_;

  Orthanc::SharedMessageQueue  newInstances_;
  bool stop_;
  boost::thread newInstancesThread_;
  OrthancPlugins::GdcmDecoderCache  decoder_;

  static void NewInstancesThread(CacheContext* cache)
  {
    while (!cache->stop_)
    {
      std::auto_ptr<Orthanc::IDynamicObject> obj(cache->newInstances_.Dequeue(100));
      if (obj.get() != NULL)
      {
        const std::string& instanceId = dynamic_cast<DynamicString&>(*obj).GetValue();

        // On the reception of a new instance, indalidate the parent series of the instance
        std::string uri = "/instances/" + std::string(instanceId);
        Json::Value instance;
        if (OrthancPlugins::GetJsonFromOrthanc(instance, context_, uri))
        {
          std::string seriesId = instance["ParentSeries"].asString();
          cache->GetScheduler().Invalidate(OrthancPlugins::CacheBundle_SeriesInformation, seriesId);
        }
      }
    }
  }

public:
  CacheContext(const std::string& path) : storage_(path), stop_(false)
  {
    boost::filesystem::path p(path);
    db_.Open((p / "cache.db").string());

    cache_.reset(new OrthancPlugins::CacheManager(db_, storage_));
    //cache_->SetSanityCheckEnabled(true);  // For debug

    scheduler_.reset(new OrthancPlugins::CacheScheduler(*cache_, 100));

    newInstancesThread_ = boost::thread(NewInstancesThread, this);
  }

  ~CacheContext()
  {
    stop_ = true;
    if (newInstancesThread_.joinable())
    {
      newInstancesThread_.join();
    }

    scheduler_.reset(NULL);
    cache_.reset(NULL);
  }

  OrthancPlugins::CacheScheduler& GetScheduler()
  {
    return *scheduler_;
  }

  void SignalNewInstance(const char* instanceId)
  {
    newInstances_.Enqueue(new DynamicString(instanceId));
  }

  OrthancPlugins::GdcmDecoderCache&  GetDecoder()
  {
    return decoder_;
  }
};



static CacheContext* cache_ = NULL;



static OrthancPluginErrorCode OnChangeCallback(OrthancPluginChangeType changeType,
                                               OrthancPluginResourceType resourceType,
                                               const char* resourceId)
{
  try
  {
    if (changeType == OrthancPluginChangeType_NewInstance &&
        resourceType == OrthancPluginResourceType_Instance)
    {
      cache_->SignalNewInstance(resourceId);
    }

    return OrthancPluginErrorCode_Success;
  }
  catch (std::runtime_error& e)
  {
    OrthancPluginLogError(context_, e.what());
    return OrthancPluginErrorCode_Success;  // Ignore error
  }
}



template <enum OrthancPlugins::CacheBundle bundle>
static OrthancPluginErrorCode ServeCache(OrthancPluginRestOutput* output,
                                         const char* url,
                                         const OrthancPluginHttpRequest* request)
{
  try
  {
    if (request->method != OrthancPluginHttpMethod_Get)
    {
      OrthancPluginSendMethodNotAllowed(context_, output, "GET");
      return OrthancPluginErrorCode_Success;
    }

    const std::string id = request->groups[0];
    std::string content;

    if (cache_->GetScheduler().Access(content, bundle, id))
    {
      OrthancPluginAnswerBuffer(context_, output, content.c_str(), content.size(), "application/json");
    }
    else
    {
      OrthancPluginSendHttpStatusCode(context_, output, 404);
    }

    return OrthancPluginErrorCode_Success;
  }
  catch (Orthanc::OrthancException& e)
  {
    OrthancPluginLogError(context_, e.What());
    return OrthancPluginErrorCode_Plugin;
  }
  catch (std::runtime_error& e)
  {
    OrthancPluginLogError(context_, e.what());
    return OrthancPluginErrorCode_Plugin;
  }
  catch (boost::bad_lexical_cast&)
  {
    OrthancPluginLogError(context_, "Bad lexical cast");
    return OrthancPluginErrorCode_Plugin;
  }
}




#if ORTHANC_STANDALONE == 0
static OrthancPluginErrorCode ServeWebViewer(OrthancPluginRestOutput* output,
                                             const char* url,
                                             const OrthancPluginHttpRequest* request)
{
  if (request->method != OrthancPluginHttpMethod_Get)
  {
    OrthancPluginSendMethodNotAllowed(context_, output, "GET");
    return OrthancPluginErrorCode_Success;
  }

  const std::string path = std::string(WEB_VIEWER_PATH) + std::string(request->groups[0]);
  const char* mime = OrthancPlugins::GetMimeType(path);
  
  std::string s;
  try
  {
    Orthanc::Toolbox::ReadFile(s, path);
    const char* resource = s.size() ? s.c_str() : NULL;
    OrthancPluginAnswerBuffer(context_, output, resource, s.size(), mime);
  }
  catch (Orthanc::OrthancException&)
  {
    std::string s = "Inexistent file in served folder: " + path;
    OrthancPluginLogError(context_, s.c_str());
    OrthancPluginSendHttpStatusCode(context_, output, 404);
  }

  return OrthancPluginErrorCode_Success;
}

#endif



template <enum Orthanc::EmbeddedResources::DirectoryResourceId folder>
static OrthancPluginErrorCode ServeEmbeddedFolder(OrthancPluginRestOutput* output,
                                                  const char* url,
                                                  const OrthancPluginHttpRequest* request)
{
  if (request->method != OrthancPluginHttpMethod_Get)
  {
    OrthancPluginSendMethodNotAllowed(context_, output, "GET");
    return OrthancPluginErrorCode_Success;
  }

  std::string path = "/" + std::string(request->groups[0]);
  const char* mime = OrthancPlugins::GetMimeType(path);

  try
  {
    std::string s;
    Orthanc::EmbeddedResources::GetDirectoryResource(s, folder, path.c_str());

    const char* resource = s.size() ? s.c_str() : NULL;
    OrthancPluginAnswerBuffer(context_, output, resource, s.size(), mime);

    return OrthancPluginErrorCode_Success;
  }
  catch (Orthanc::OrthancException& e)
  {
    std::string s = "Unknown static resource in plugin: " + std::string(request->groups[0]);
    OrthancPluginLogError(context_, s.c_str());
    OrthancPluginSendHttpStatusCode(context_, output, 404);
    return OrthancPluginErrorCode_Success;
  }
}



static OrthancPluginErrorCode IsStableSeries(OrthancPluginRestOutput* output,
                                             const char* url,
                                             const OrthancPluginHttpRequest* request)
{
  try
  {
    if (request->method != OrthancPluginHttpMethod_Get)
    {
      OrthancPluginSendMethodNotAllowed(context_, output, "GET");
      return OrthancPluginErrorCode_Success;
    }

    const std::string id = request->groups[0];
    Json::Value series;

    if (OrthancPlugins::GetJsonFromOrthanc(series, context_, "/series/" + id) &&
        series.type() == Json::objectValue)
    {
      bool value = (series["IsStable"].asBool() ||
                    series["Status"].asString() == "Complete");
      std::string answer = value ? "true" : "false";
      OrthancPluginAnswerBuffer(context_, output, answer.c_str(), answer.size(), "application/json");
    }
    else
    {
      OrthancPluginSendHttpStatusCode(context_, output, 404);
    }

    return OrthancPluginErrorCode_Success;
  }
  catch (Orthanc::OrthancException& e)
  {
    OrthancPluginLogError(context_, e.What());
    return OrthancPluginErrorCode_Plugin;
  }
  catch (std::runtime_error& e)
  {
    OrthancPluginLogError(context_, e.what());
    return OrthancPluginErrorCode_Plugin;
  }
  catch (boost::bad_lexical_cast&)
  {
    OrthancPluginLogError(context_, "Bad lexical cast");
    return OrthancPluginErrorCode_Plugin;
  }
}


static OrthancPluginErrorCode DecodeImageCallback(OrthancPluginImage** target,
                                                  const void* dicom,
                                                  const uint32_t size,
                                                  uint32_t frameIndex)
{
  try
  {
    std::auto_ptr<OrthancPlugins::OrthancImageWrapper> image;

#if 0
    // Do not use the cache
    OrthancPlugins::GdcmImageDecoder decoder(dicom, size);
    image.reset(new OrthancPlugins::OrthancImageWrapper(context_, decoder, frameIndex));
#else
    using namespace OrthancPlugins;
    image.reset(cache_->GetDecoder().Decode(context_, dicom, size, frameIndex));
#endif

    *target = image->Release();

    return OrthancPluginErrorCode_Success;
  }
  catch (Orthanc::OrthancException& e)
  {
    *target = NULL;

    std::string s = "Cannot decode image using GDCM: " + std::string(e.What());
    OrthancPluginLogError(context_, s.c_str());
    return OrthancPluginErrorCode_Plugin;
  }
  catch (std::runtime_error& e)
  {
    *target = NULL;

    std::string s = "Cannot decode image using GDCM: " + std::string(e.what());
    OrthancPluginLogError(context_, s.c_str());
    return OrthancPluginErrorCode_Plugin;
  }
}


extern "C"
{
  ORTHANC_PLUGINS_API int32_t OrthancPluginInitialize(OrthancPluginContext* context)
  {
    using namespace OrthancPlugins;

    context_ = context;
    OrthancPluginLogWarning(context_, "Initializing the Web viewer");


    /* Check the version of the Orthanc core */
    if (OrthancPluginCheckVersion(context_) == 0)
    {
      char info[1024];
      sprintf(info, "Your version of Orthanc (%s) must be above %d.%d.%d to run this plugin",
              context_->orthancVersion,
              ORTHANC_PLUGINS_MINIMAL_MAJOR_NUMBER,
              ORTHANC_PLUGINS_MINIMAL_MINOR_NUMBER,
              ORTHANC_PLUGINS_MINIMAL_REVISION_NUMBER);
      OrthancPluginLogError(context_, info);
      return -1;
    }

    OrthancPluginSetDescription(context_, "Provides a Web viewer of DICOM series within Orthanc.");


    // Replace the default decoder of DICOM images that is built in Orthanc
    OrthancPluginRegisterDecodeImageCallback(context_, DecodeImageCallback);


    /* By default, use half of the available processing cores for the decoding of DICOM images */
    int decodingThreads = boost::thread::hardware_concurrency() / 2;
    if (decodingThreads == 0)
    {
      decodingThreads = 1;
    }


    try
    {
      /* Read the configuration of the Web viewer */
      Json::Value configuration;
      if (!ReadConfiguration(configuration, context))
      {
        OrthancPluginLogError(context_, "Unable to read the configuration file of Orthanc");
        return -1;
      }

      // By default, the cache of the Web viewer is located inside the
      // "StorageDirectory" of Orthanc
      boost::filesystem::path cachePath = GetStringValue(configuration, "StorageDirectory", ".");
      cachePath /= "WebViewerCache";
      int cacheSize = 100;  // By default, a cache of 100 MB is used

      if (configuration.isMember("WebViewer"))
      {
        std::string key = "CachePath";
        if (!configuration["WebViewer"].isMember(key))
        {
          // For backward compatibility with the initial release of the Web viewer
          key = "Cache";
        }

        cachePath = GetStringValue(configuration["WebViewer"], key, cachePath.string());
        cacheSize = GetIntegerValue(configuration["WebViewer"], "CacheSize", cacheSize);
        decodingThreads = GetIntegerValue(configuration["WebViewer"], "Threads", decodingThreads);
      }

      std::string message = ("Web viewer using " + boost::lexical_cast<std::string>(decodingThreads) + 
                             " threads for the decoding of the DICOM images");
      OrthancPluginLogWarning(context_, message.c_str());

      if (decodingThreads <= 0 ||
          cacheSize <= 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      message = "Storing the cache of the Web viewer in folder: " + cachePath.string();
      OrthancPluginLogWarning(context_, message.c_str());

   
      /* Create the cache */
      cache_ = new CacheContext(cachePath.string());
      CacheScheduler& scheduler = cache_->GetScheduler();


      /* Look for a change in the versions */
      std::string orthancVersion("unknown"), webViewerVersion("unknown");
      bool clear = false;
      if (!scheduler.LookupProperty(orthancVersion, CacheProperty_OrthancVersion) ||
          orthancVersion != std::string(context_->orthancVersion))
      {
        std::string s = ("The version of Orthanc has changed from \"" + orthancVersion + "\" to \"" + 
                         std::string(context_->orthancVersion) + "\": The cache of the Web viewer will be cleared");
        OrthancPluginLogWarning(context_, s.c_str());
        clear = true;
      }

      if (!scheduler.LookupProperty(webViewerVersion, CacheProperty_WebViewerVersion) ||
          webViewerVersion != std::string(ORTHANC_WEBVIEWER_VERSION))
      {
        std::string s = ("The version of the Web viewer plugin has changed from \"" + webViewerVersion + "\" to \"" + 
                         std::string(ORTHANC_WEBVIEWER_VERSION) + "\": The cache of the Web viewer will be cleared");
        OrthancPluginLogWarning(context_, s.c_str());
        clear = true;
      }


      /* Clear the cache if needed */
      if (clear)
      {
        OrthancPluginLogWarning(context_, "Clearing the cache of the Web viewer");
        scheduler.Clear();
        scheduler.SetProperty(CacheProperty_OrthancVersion, context_->orthancVersion);
        scheduler.SetProperty(CacheProperty_WebViewerVersion, ORTHANC_WEBVIEWER_VERSION);
      }
      else
      {
        OrthancPluginLogInfo(context_, "No change in the versions, no need to clear the cache of the Web viewer");
      }


      /* Configure the cache */
      scheduler.RegisterPolicy(new ViewerPrefetchPolicy(context_));
      scheduler.Register(CacheBundle_SeriesInformation, 
                         new SeriesInformationAdapter(context_, scheduler), 1);
      scheduler.Register(CacheBundle_DecodedImage, 
                         new DecodedImageAdapter(context_), decodingThreads);


      /* Set the quotas */
      scheduler.SetQuota(CacheBundle_SeriesInformation, 1000, 0);    // Keep info about 1000 series
      
      message = "Web viewer using a cache of " + boost::lexical_cast<std::string>(cacheSize) + " MB";
      OrthancPluginLogWarning(context_, message.c_str());

      scheduler.SetQuota(CacheBundle_DecodedImage, 0, static_cast<uint64_t>(cacheSize) * 1024 * 1024);
    }
    catch (std::runtime_error& e)
    {
      OrthancPluginLogError(context_, e.what());
      return -1;
    }
    catch (Orthanc::OrthancException& e)
    {
      OrthancPluginLogError(context_, e.What());
      return -1;
    }


    /* Install the callbacks */
    OrthancPluginRegisterRestCallback(context_, "/web-viewer/series/(.*)", ServeCache<CacheBundle_SeriesInformation>);
    OrthancPluginRegisterRestCallback(context_, "/web-viewer/is-stable-series/(.*)", IsStableSeries);
    OrthancPluginRegisterRestCallback(context_, "/web-viewer/instances/(.*)", ServeCache<CacheBundle_DecodedImage>);
    OrthancPluginRegisterRestCallback(context, "/web-viewer/libs/(.*)", ServeEmbeddedFolder<Orthanc::EmbeddedResources::JAVASCRIPT_LIBS>);

#if ORTHANC_STANDALONE == 1
    OrthancPluginRegisterRestCallback(context, "/web-viewer/app/(.*)", ServeEmbeddedFolder<Orthanc::EmbeddedResources::WEB_VIEWER>);
#else
    OrthancPluginRegisterRestCallback(context, "/web-viewer/app/(.*)", ServeWebViewer);
#endif

    OrthancPluginRegisterOnChangeCallback(context, OnChangeCallback);


    /* Extend the default Orthanc Explorer with custom JavaScript */
    std::string explorer;
    Orthanc::EmbeddedResources::GetFileResource(explorer, Orthanc::EmbeddedResources::ORTHANC_EXPLORER);
    OrthancPluginExtendOrthancExplorer(context_, explorer.c_str());

    return 0;
  }


  ORTHANC_PLUGINS_API void OrthancPluginFinalize()
  {
    OrthancPluginLogWarning(context_, "Finalizing the Web viewer");

    if (cache_ != NULL)
    {
      delete cache_;
      cache_ = NULL;
    }
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetName()
  {
    return "osimis-web-viewer";
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetVersion()
  {
    return ORTHANC_WEBVIEWER_VERSION;
  }
}
