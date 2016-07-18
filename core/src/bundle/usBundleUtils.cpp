/*=============================================================================

  Library: CppMicroServices

  Copyright (c) The CppMicroServices developers. See the COPYRIGHT
  file at the top-level directory of this distribution and at
  https://github.com/saschazelzer/CppMicroServices/COPYRIGHT .

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=============================================================================*/

#include "usBundleUtils.h"
#include "usBundleUtils_p.h"

#include <usLog.h>
#include <usUtils_p.h>

#ifdef __GNUC__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#elif _WIN32
#include <windows.h>
#endif


namespace us {

namespace {
#ifdef US_BUILD_SHARED_LIBS
  const bool sharedLibMode = true;
#else
  const bool sharedLibMode = false;
#endif
}

namespace BundleUtils
{

#ifdef __GNUC__

std::string GetLibraryPath_impl(void* symbol)
{
  Dl_info info;
  if (dladdr(symbol, &info))
  {
    return info.dli_fname;
  }
  else
  {
    US_DEBUG << "GetLibraryPath_impl() failed for address " << symbol;
  }
  return "";
}

void* GetSymbol_impl(const std::string& bundleName, const std::string& libLocation, const char* symbol)
{
  // Clear the last error message
  dlerror();

  void* selfHandle = nullptr;
  if (!sharedLibMode || bundleName == "main")
  {
    // Get the handle of the executable
    selfHandle = dlopen(0, RTLD_LAZY);
  }
  else
  {
    selfHandle = dlopen(libLocation.c_str(), RTLD_LAZY);
  }

  if (selfHandle)
  {
    void* addr = dlsym(selfHandle, symbol);
    if (!addr)
    {
      const char* dlerrorMsg = dlerror();
      if (dlerrorMsg)
      {
        US_DEBUG << "GetSymbol_impl() failed: " << dlerrorMsg;
      }
    }
    dlclose(selfHandle);
    return addr;
  }
  else
  {
    US_DEBUG << "GetSymbol_impl() dlopen() failed: " << dlerror();
  }
  return nullptr;
}

#elif _WIN32

std::string GetLibraryPath_impl(void *symbol)
{
  HMODULE handle = nullptr;
  BOOL handleError = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                       static_cast<LPCTSTR>(symbol), &handle);
  if (!handleError)
  {
    // Test
    US_DEBUG << "GetLibraryPath_impl():GetBundleHandle() " << GetLastErrorStr();
    return "";
  }

  char bundlePath[512];
  if (GetModuleFileName(handle, bundlePath, 512))
  {
    return bundlePath;
  }

  US_DEBUG << "GetLibraryPath_impl():GetBundleFileName() " << GetLastErrorStr();
  return "";
}

void* GetSymbol_impl(const std::string& bundleName, const std::string& libLocation, const char* symbol)
{
  HMODULE handle = nullptr;
  if (!sharedLibMode || bundleName == "main")
  {
    handle = GetModuleHandle(nullptr);
  }
  else
  {
    handle = GetModuleHandle(libLocation.c_str());
  }

  if (!handle)
  {
    US_DEBUG << "GetSymbol_impl():GetBundleHandle() " << GetLastErrorStr();
    return nullptr;
  }

  void* addr = (void*)GetProcAddress(handle, symbol);
  if (!addr)
  {
    US_DEBUG << "GetSymbol_impl():GetProcAddress(handle," << symbol << ") " << GetLastErrorStr();
  }
  return addr;
}

#else

std::string GetLibraryPath_impl(void*)
{
  return "";
}

void* GetSymbol_impl(const std::string&, const std::string&, const char*)
{
  return nullptr;
}

#endif

std::string GetLibraryPath(void* symbol)
{
  return GetLibraryPath_impl(symbol);
}

void* GetSymbol(const std::string& bundleName, const std::string& libLocation, const char* symbol)
{
  return GetSymbol_impl(bundleName, libLocation, symbol);
}

} // namespace BundleUtils

} // namespace us
