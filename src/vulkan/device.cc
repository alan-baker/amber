// Copyright 2018 The Amber Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/vulkan/device.h"

#include <cassert>
#include <cstring>
#include <memory>
#include <set>
#include <vector>

#include "src/make_unique.h"
#include "src/vulkan/log.h"

namespace amber {
namespace vulkan {
namespace {

const char* const kRequiredValidationLayers[] = {
#ifdef __ANDROID__
    // Note that the order of enabled layers is important. It is
    // based on Android NDK Vulkan document.
    "VK_LAYER_GOOGLE_threading",      "VK_LAYER_LUNARG_parameter_validation",
    "VK_LAYER_LUNARG_object_tracker", "VK_LAYER_LUNARG_core_validation",
    "VK_LAYER_GOOGLE_unique_objects",
#else   // __ANDROID__
    "VK_LAYER_LUNARG_standard_validation",
#endif  // __ANDROID__
};

const size_t kNumberOfRequiredValidationLayers =
    sizeof(kRequiredValidationLayers) / sizeof(const char*);

const char* kExtensionForValidationLayer = "VK_EXT_debug_report";

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flag,
                                             VkDebugReportObjectTypeEXT,
                                             uint64_t,
                                             size_t,
                                             int32_t,
                                             const char* layerPrefix,
                                             const char* msg,
                                             void*) {
  std::string flag_message;
  switch (flag) {
    case VK_DEBUG_REPORT_ERROR_BIT_EXT:
      flag_message = "[ERROR]";
      break;
    case VK_DEBUG_REPORT_WARNING_BIT_EXT:
      flag_message = "[WARNING]";
      break;
    default:
      flag_message = "[UNKNOWN]";
      break;
  }

  LogError(flag_message + " validation layer (" + layerPrefix + "):\n" + msg);
  return VK_FALSE;
}

VkPhysicalDeviceFeatures RequestedFeatures(
    const std::vector<Feature>& required_features) {
  VkPhysicalDeviceFeatures requested_features = VkPhysicalDeviceFeatures();
  for (const auto& feature : required_features) {
    switch (feature) {
      case Feature::kRobustBufferAccess:
        requested_features.robustBufferAccess = VK_TRUE;
        break;
      case Feature::kFullDrawIndexUint32:
        requested_features.fullDrawIndexUint32 = VK_TRUE;
        break;
      case Feature::kImageCubeArray:
        requested_features.imageCubeArray = VK_TRUE;
        break;
      case Feature::kIndependentBlend:
        requested_features.independentBlend = VK_TRUE;
        break;
      case Feature::kGeometryShader:
        requested_features.geometryShader = VK_TRUE;
        break;
      case Feature::kTessellationShader:
        requested_features.tessellationShader = VK_TRUE;
        break;
      case Feature::kSampleRateShading:
        requested_features.sampleRateShading = VK_TRUE;
        break;
      case Feature::kDualSrcBlend:
        requested_features.dualSrcBlend = VK_TRUE;
        break;
      case Feature::kLogicOp:
        requested_features.logicOp = VK_TRUE;
        break;
      case Feature::kMultiDrawIndirect:
        requested_features.multiDrawIndirect = VK_TRUE;
        break;
      case Feature::kDrawIndirectFirstInstance:
        requested_features.drawIndirectFirstInstance = VK_TRUE;
        break;
      case Feature::kDepthClamp:
        requested_features.depthClamp = VK_TRUE;
        break;
      case Feature::kDepthBiasClamp:
        requested_features.depthBiasClamp = VK_TRUE;
        break;
      case Feature::kFillModeNonSolid:
        requested_features.fillModeNonSolid = VK_TRUE;
        break;
      case Feature::kDepthBounds:
        requested_features.depthBounds = VK_TRUE;
        break;
      case Feature::kWideLines:
        requested_features.wideLines = VK_TRUE;
        break;
      case Feature::kLargePoints:
        requested_features.largePoints = VK_TRUE;
        break;
      case Feature::kAlphaToOne:
        requested_features.alphaToOne = VK_TRUE;
        break;
      case Feature::kMultiViewport:
        requested_features.multiViewport = VK_TRUE;
        break;
      case Feature::kSamplerAnisotropy:
        requested_features.samplerAnisotropy = VK_TRUE;
        break;
      case Feature::kTextureCompressionETC2:
        requested_features.textureCompressionETC2 = VK_TRUE;
        break;
      case Feature::kTextureCompressionASTC_LDR:
        requested_features.textureCompressionASTC_LDR = VK_TRUE;
        break;
      case Feature::kTextureCompressionBC:
        requested_features.textureCompressionBC = VK_TRUE;
        break;
      case Feature::kOcclusionQueryPrecise:
        requested_features.occlusionQueryPrecise = VK_TRUE;
        break;
      case Feature::kPipelineStatisticsQuery:
        requested_features.pipelineStatisticsQuery = VK_TRUE;
        break;
      case Feature::kVertexPipelineStoresAndAtomics:
        requested_features.vertexPipelineStoresAndAtomics = VK_TRUE;
        break;
      case Feature::kFragmentStoresAndAtomics:
        requested_features.fragmentStoresAndAtomics = VK_TRUE;
        break;
      case Feature::kShaderTessellationAndGeometryPointSize:
        requested_features.shaderTessellationAndGeometryPointSize = VK_TRUE;
        break;
      case Feature::kShaderImageGatherExtended:
        requested_features.shaderImageGatherExtended = VK_TRUE;
        break;
      case Feature::kShaderStorageImageExtendedFormats:
        requested_features.shaderStorageImageExtendedFormats = VK_TRUE;
        break;
      case Feature::kShaderStorageImageMultisample:
        requested_features.shaderStorageImageMultisample = VK_TRUE;
        break;
      case Feature::kShaderStorageImageReadWithoutFormat:
        requested_features.shaderStorageImageReadWithoutFormat = VK_TRUE;
        break;
      case Feature::kShaderStorageImageWriteWithoutFormat:
        requested_features.shaderStorageImageWriteWithoutFormat = VK_TRUE;
        break;
      case Feature::kShaderUniformBufferArrayDynamicIndexing:
        requested_features.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
        break;
      case Feature::kShaderSampledImageArrayDynamicIndexing:
        requested_features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
        break;
      case Feature::kShaderStorageBufferArrayDynamicIndexing:
        requested_features.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;
        break;
      case Feature::kShaderStorageImageArrayDynamicIndexing:
        requested_features.shaderStorageImageArrayDynamicIndexing = VK_TRUE;
        break;
      case Feature::kShaderClipDistance:
        requested_features.shaderClipDistance = VK_TRUE;
        break;
      case Feature::kShaderCullDistance:
        requested_features.shaderCullDistance = VK_TRUE;
        break;
      case Feature::kShaderFloat64:
        requested_features.shaderFloat64 = VK_TRUE;
        break;
      case Feature::kShaderInt64:
        requested_features.shaderInt64 = VK_TRUE;
        break;
      case Feature::kShaderInt16:
        requested_features.shaderInt16 = VK_TRUE;
        break;
      case Feature::kShaderResourceResidency:
        requested_features.shaderResourceResidency = VK_TRUE;
        break;
      case Feature::kShaderResourceMinLod:
        requested_features.shaderResourceMinLod = VK_TRUE;
        break;
      case Feature::kSparseBinding:
        requested_features.sparseBinding = VK_TRUE;
        break;
      case Feature::kSparseResidencyBuffer:
        requested_features.sparseResidencyBuffer = VK_TRUE;
        break;
      case Feature::kSparseResidencyImage2D:
        requested_features.sparseResidencyImage2D = VK_TRUE;
        break;
      case Feature::kSparseResidencyImage3D:
        requested_features.sparseResidencyImage3D = VK_TRUE;
        break;
      case Feature::kSparseResidency2Samples:
        requested_features.sparseResidency2Samples = VK_TRUE;
        break;
      case Feature::kSparseResidency4Samples:
        requested_features.sparseResidency4Samples = VK_TRUE;
        break;
      case Feature::kSparseResidency8Samples:
        requested_features.sparseResidency8Samples = VK_TRUE;
        break;
      case Feature::kSparseResidency16Samples:
        requested_features.sparseResidency16Samples = VK_TRUE;
        break;
      case Feature::kSparseResidencyAliased:
        requested_features.sparseResidencyAliased = VK_TRUE;
        break;
      case Feature::kVariableMultisampleRate:
        requested_features.variableMultisampleRate = VK_TRUE;
        break;
      case Feature::kInheritedQueries:
        requested_features.inheritedQueries = VK_TRUE;
        break;
      case Feature::kFramebuffer:
      case Feature::kDepthStencil:
      case Feature::kFenceTimeout:
      case Feature::kUnknown:
        break;
    }
  }
  return requested_features;
}

bool AreAllRequiredFeaturesSupported(
    const VkPhysicalDeviceFeatures& available_features,
    const std::vector<Feature>& required_features) {
  if (required_features.empty())
    return true;

  for (const auto& feature : required_features) {
    switch (feature) {
      case Feature::kRobustBufferAccess:
        if (available_features.robustBufferAccess == VK_FALSE)
          return false;
        break;
      case Feature::kFullDrawIndexUint32:
        if (available_features.fullDrawIndexUint32 == VK_FALSE)
          return false;
        break;
      case Feature::kImageCubeArray:
        if (available_features.imageCubeArray == VK_FALSE)
          return false;
        break;
      case Feature::kIndependentBlend:
        if (available_features.independentBlend == VK_FALSE)
          return false;
        break;
      case Feature::kGeometryShader:
        if (available_features.geometryShader == VK_FALSE)
          return false;
        break;
      case Feature::kTessellationShader:
        if (available_features.tessellationShader == VK_FALSE)
          return false;
        break;
      case Feature::kSampleRateShading:
        if (available_features.sampleRateShading == VK_FALSE)
          return false;
        break;
      case Feature::kDualSrcBlend:
        if (available_features.dualSrcBlend == VK_FALSE)
          return false;
        break;
      case Feature::kLogicOp:
        if (available_features.logicOp == VK_FALSE)
          return false;
        break;
      case Feature::kMultiDrawIndirect:
        if (available_features.multiDrawIndirect == VK_FALSE)
          return false;
        break;
      case Feature::kDrawIndirectFirstInstance:
        if (available_features.drawIndirectFirstInstance == VK_FALSE)
          return false;
        break;
      case Feature::kDepthClamp:
        if (available_features.depthClamp == VK_FALSE)
          return false;
        break;
      case Feature::kDepthBiasClamp:
        if (available_features.depthBiasClamp == VK_FALSE)
          return false;
        break;
      case Feature::kFillModeNonSolid:
        if (available_features.fillModeNonSolid == VK_FALSE)
          return false;
        break;
      case Feature::kDepthBounds:
        if (available_features.depthBounds == VK_FALSE)
          return false;
        break;
      case Feature::kWideLines:
        if (available_features.wideLines == VK_FALSE)
          return false;
        break;
      case Feature::kLargePoints:
        if (available_features.largePoints == VK_FALSE)
          return false;
        break;
      case Feature::kAlphaToOne:
        if (available_features.alphaToOne == VK_FALSE)
          return false;
        break;
      case Feature::kMultiViewport:
        if (available_features.multiViewport == VK_FALSE)
          return false;
        break;
      case Feature::kSamplerAnisotropy:
        if (available_features.samplerAnisotropy == VK_FALSE)
          return false;
        break;
      case Feature::kTextureCompressionETC2:
        if (available_features.textureCompressionETC2 == VK_FALSE)
          return false;
        break;
      case Feature::kTextureCompressionASTC_LDR:
        if (available_features.textureCompressionASTC_LDR == VK_FALSE)
          return false;
        break;
      case Feature::kTextureCompressionBC:
        if (available_features.textureCompressionBC == VK_FALSE)
          return false;
        break;
      case Feature::kOcclusionQueryPrecise:
        if (available_features.occlusionQueryPrecise == VK_FALSE)
          return false;
        break;
      case Feature::kPipelineStatisticsQuery:
        if (available_features.pipelineStatisticsQuery == VK_FALSE)
          return false;
        break;
      case Feature::kVertexPipelineStoresAndAtomics:
        if (available_features.vertexPipelineStoresAndAtomics == VK_FALSE)
          return false;
        break;
      case Feature::kFragmentStoresAndAtomics:
        if (available_features.fragmentStoresAndAtomics == VK_FALSE)
          return false;
        break;
      case Feature::kShaderTessellationAndGeometryPointSize:
        if (available_features.shaderTessellationAndGeometryPointSize ==
            VK_FALSE)
          return false;
        break;
      case Feature::kShaderImageGatherExtended:
        if (available_features.shaderImageGatherExtended == VK_FALSE)
          return false;
        break;
      case Feature::kShaderStorageImageExtendedFormats:
        if (available_features.shaderStorageImageExtendedFormats == VK_FALSE)
          return false;
        break;
      case Feature::kShaderStorageImageMultisample:
        if (available_features.shaderStorageImageMultisample == VK_FALSE)
          return false;
        break;
      case Feature::kShaderStorageImageReadWithoutFormat:
        if (available_features.shaderStorageImageReadWithoutFormat == VK_FALSE)
          return false;
        break;
      case Feature::kShaderStorageImageWriteWithoutFormat:
        if (available_features.shaderStorageImageWriteWithoutFormat == VK_FALSE)
          return false;
        break;
      case Feature::kShaderUniformBufferArrayDynamicIndexing:
        if (available_features.shaderUniformBufferArrayDynamicIndexing ==
            VK_FALSE)
          return false;
        break;
      case Feature::kShaderSampledImageArrayDynamicIndexing:
        if (available_features.shaderSampledImageArrayDynamicIndexing ==
            VK_FALSE)
          return false;
        break;
      case Feature::kShaderStorageBufferArrayDynamicIndexing:
        if (available_features.shaderStorageBufferArrayDynamicIndexing ==
            VK_FALSE)
          return false;
        break;
      case Feature::kShaderStorageImageArrayDynamicIndexing:
        if (available_features.shaderStorageImageArrayDynamicIndexing ==
            VK_FALSE)
          return false;
        break;
      case Feature::kShaderClipDistance:
        if (available_features.shaderClipDistance == VK_FALSE)
          return false;
        break;
      case Feature::kShaderCullDistance:
        if (available_features.shaderCullDistance == VK_FALSE)
          return false;
        break;
      case Feature::kShaderFloat64:
        if (available_features.shaderFloat64 == VK_FALSE)
          return false;
        break;
      case Feature::kShaderInt64:
        if (available_features.shaderInt64 == VK_FALSE)
          return false;
        break;
      case Feature::kShaderInt16:
        if (available_features.shaderInt16 == VK_FALSE)
          return false;
        break;
      case Feature::kShaderResourceResidency:
        if (available_features.shaderResourceResidency == VK_FALSE)
          return false;
        break;
      case Feature::kShaderResourceMinLod:
        if (available_features.shaderResourceMinLod == VK_FALSE)
          return false;
        break;
      case Feature::kSparseBinding:
        if (available_features.sparseBinding == VK_FALSE)
          return false;
        break;
      case Feature::kSparseResidencyBuffer:
        if (available_features.sparseResidencyBuffer == VK_FALSE)
          return false;
        break;
      case Feature::kSparseResidencyImage2D:
        if (available_features.sparseResidencyImage2D == VK_FALSE)
          return false;
        break;
      case Feature::kSparseResidencyImage3D:
        if (available_features.sparseResidencyImage3D == VK_FALSE)
          return false;
        break;
      case Feature::kSparseResidency2Samples:
        if (available_features.sparseResidency2Samples == VK_FALSE)
          return false;
        break;
      case Feature::kSparseResidency4Samples:
        if (available_features.sparseResidency4Samples == VK_FALSE)
          return false;
        break;
      case Feature::kSparseResidency8Samples:
        if (available_features.sparseResidency8Samples == VK_FALSE)
          return false;
        break;
      case Feature::kSparseResidency16Samples:
        if (available_features.sparseResidency16Samples == VK_FALSE)
          return false;
        break;
      case Feature::kSparseResidencyAliased:
        if (available_features.sparseResidencyAliased == VK_FALSE)
          return false;
        break;
      case Feature::kVariableMultisampleRate:
        if (available_features.variableMultisampleRate == VK_FALSE)
          return false;
        break;
      case Feature::kInheritedQueries:
        if (available_features.inheritedQueries == VK_FALSE)
          return false;
        break;
      case Feature::kFramebuffer:
      case Feature::kDepthStencil:
      case Feature::kFenceTimeout:
      case Feature::kUnknown:
        break;
    }
  }

  return true;
}

bool AreAllExtensionsSupported(
    const std::vector<std::string>& available_extensions,
    const std::vector<std::string>& required_extensions) {
  if (required_extensions.empty())
    return true;

  std::set<std::string> required_extension_set(required_extensions.begin(),
                                               required_extensions.end());
  for (const auto& extension : available_extensions) {
    required_extension_set.erase(extension);
  }

  return required_extension_set.empty();
}

}  // namespace

Device::Device() = default;

Device::Device(VkInstance instance,
               VkPhysicalDevice physical_device,
               const VkPhysicalDeviceFeatures& available_features,
               const std::vector<std::string>& available_extensions,
               uint32_t queue_family_index,
               VkDevice device,
               VkQueue queue)
    : instance_(instance),
      physical_device_(physical_device),
      available_physical_device_features_(available_features),
      available_physical_device_extensions_(available_extensions),
      queue_family_index_(queue_family_index),
      device_(device),
      queue_(queue),
      destroy_device_(false) {}

Device::~Device() = default;

void Device::Shutdown() {
  if (destroy_device_) {
    ptrs_.vkDestroyDevice(device_, nullptr);
    ptrs_.vkDestroyDebugReportCallbackEXT(instance_, callback_, nullptr);
    ptrs_.vkDestroyInstance(instance_, nullptr);
  }
}

Result Device::LoadVulkanGlobalPointers(
    PFN_vkGetInstanceProcAddr getInstanceProcAddr) {
#define AMBER_VK_FUNC(func)
#define AMBER_VK_GLOBAL_FUNC(func)                             \
  if (!(ptrs_.func = reinterpret_cast<PFN_##func>(             \
            getInstanceProcAddr(VK_NULL_HANDLE, #func)))) {    \
    return Result("Vulkan: Unable to load " #func " pointer"); \
  }
#include "src/vulkan/vk-funcs.inc"
#undef AMBER_VK_GLOBAL_FUNC
#undef AMBER_VK_FUNC

  return {};
}

Result Device::LoadVulkanPointers(
    PFN_vkGetInstanceProcAddr getInstanceProcAddr) {
#define AMBER_VK_GLOBAL_FUNC(func)
#define AMBER_VK_FUNC(func)                                    \
  if (!(ptrs_.func = reinterpret_cast<PFN_##func>(             \
            getInstanceProcAddr(instance_, #func)))) {         \
    return Result("Vulkan: Unable to load " #func " pointer"); \
  }
#include "src/vulkan/vk-funcs.inc"  // NOLINT(build/include)
#undef AMBER_VK_FUNC
#undef AMBER_VK_GLOBAL_FUNC

  return {};
}

Result Device::Initialize(PFN_vkGetInstanceProcAddr getInstanceProcAddr,
                          const std::vector<Feature>& required_features,
                          const std::vector<std::string>& required_extensions) {
  Result r = LoadVulkanGlobalPointers(getInstanceProcAddr);
  if (!r.IsSuccess())
    return r;

  if (destroy_device_) {
    r = CreateInstance();
    if (!r.IsSuccess())
      return r;
  }

  r = LoadVulkanPointers(getInstanceProcAddr);
  if (!r.IsSuccess())
    return r;

  if (destroy_device_) {
    r = CreateDebugReportCallback();
    if (!r.IsSuccess())
      return r;

    r = ChoosePhysicalDevice(required_features, required_extensions);
    if (!r.IsSuccess())
      return r;

    r = CreateDevice(required_features, required_extensions);
    if (!r.IsSuccess())
      return r;

    ptrs_.vkGetDeviceQueue(device_, queue_family_index_, 0, &queue_);
    if (queue_ == VK_NULL_HANDLE)
      return Result("Vulkan::Calling vkGetDeviceQueue Fail");
  } else {
    if (!AreAllRequiredFeaturesSupported(available_physical_device_features_,
                                         required_features)) {
      return Result(
          "Vulkan: Device::Initialize given physical device does not support "
          "required features");
    }

    if (!AreAllExtensionsSupported(available_physical_device_extensions_,
                                   required_extensions)) {
      return Result(
          "Vulkan: Device::Initialize given physical device does not support "
          "required extensions");
    }
  }

  ptrs_.vkGetPhysicalDeviceProperties(physical_device_,
                                      &physical_device_properties_);

  ptrs_.vkGetPhysicalDeviceMemoryProperties(physical_device_,
                                            &physical_memory_properties_);

  return {};
}

bool Device::ChooseQueueFamilyIndex(const VkPhysicalDevice& physical_device) {
  uint32_t count;
  std::vector<VkQueueFamilyProperties> properties;

  ptrs_.vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count,
                                                 nullptr);
  properties.resize(count);

  ptrs_.vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count,
                                                 properties.data());

  for (uint32_t i = 0; i < count; ++i) {
    if (properties[i].queueFlags &
        (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
      queue_family_index_ = i;
      return true;
    }
  }

  return false;
}

Result Device::CreateInstance() {
  VkApplicationInfo app_info = VkApplicationInfo();
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.apiVersion = VK_MAKE_VERSION(1, 0, 0);

  Result r = AreAllValidationLayersSupported();
  if (!r.IsSuccess())
    return r;

  if (!AreAllValidationExtensionsSupported())
    return Result("Vulkan: extensions of validation layers are not supported");

  VkInstanceCreateInfo instance_info = VkInstanceCreateInfo();
  instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_info.pApplicationInfo = &app_info;
  instance_info.enabledLayerCount = kNumberOfRequiredValidationLayers;
  instance_info.ppEnabledLayerNames = kRequiredValidationLayers;
  instance_info.enabledExtensionCount = 1U;
  instance_info.ppEnabledExtensionNames = &kExtensionForValidationLayer;

  if (ptrs_.vkCreateInstance(&instance_info, nullptr, &instance_) != VK_SUCCESS)
    return Result("Vulkan::Calling vkCreateInstance Fail");

  return {};
}

Result Device::CreateDebugReportCallback() {
  VkDebugReportCallbackCreateInfoEXT info =
      VkDebugReportCallbackCreateInfoEXT();
  info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
  info.pfnCallback = debugCallback;

  if (ptrs_.vkCreateDebugReportCallbackEXT(instance_, &info, nullptr,
                                           &callback_) != VK_SUCCESS) {
    return Result("Vulkan: vkCreateDebugReportCallbackEXT fail");
  }
  return {};
}

Result Device::ChoosePhysicalDevice(
    const std::vector<Feature>& required_features,
    const std::vector<std::string>& required_extensions) {
  uint32_t count;
  std::vector<VkPhysicalDevice> physical_devices;

  if (ptrs_.vkEnumeratePhysicalDevices(instance_, &count, nullptr) !=
      VK_SUCCESS) {
    return Result("Vulkan::Calling vkEnumeratePhysicalDevices Fail");
  }

  physical_devices.resize(count);
  if (ptrs_.vkEnumeratePhysicalDevices(instance_, &count,
                                       physical_devices.data()) != VK_SUCCESS) {
    return Result("Vulkan::Calling vkEnumeratePhysicalDevices Fail");
  }

  for (uint32_t i = 0; i < count; ++i) {
    VkPhysicalDeviceFeatures available_features = VkPhysicalDeviceFeatures();
    ptrs_.vkGetPhysicalDeviceFeatures(physical_devices[i], &available_features);
    if (!AreAllRequiredFeaturesSupported(available_features,
                                         required_features)) {
      continue;
    }

    if (!AreAllExtensionsSupported(GetAvailableExtensions(physical_devices[i]),
                                   required_extensions)) {
      continue;
    }

    if (ChooseQueueFamilyIndex(physical_devices[i])) {
      physical_device_ = physical_devices[i];
      return {};
    }
  }

  return Result("Vulkan::No physical device supports Vulkan");
}

Result Device::CreateDevice(
    const std::vector<Feature>& required_features,
    const std::vector<std::string>& required_extensions) {
  VkDeviceQueueCreateInfo queue_info = VkDeviceQueueCreateInfo();
  const float priorities[] = {1.0f};
  queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_info.queueFamilyIndex = queue_family_index_;
  queue_info.queueCount = 1;
  queue_info.pQueuePriorities = priorities;

  VkDeviceCreateInfo info = VkDeviceCreateInfo();
  info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  info.pQueueCreateInfos = &queue_info;
  info.queueCreateInfoCount = 1;
  // TODO(jaebaek): Enable layers
  VkPhysicalDeviceFeatures requested_features =
      RequestedFeatures(required_features);
  info.pEnabledFeatures = &requested_features;

  std::vector<const char*> enabled_extensions;
  for (size_t i = 0; i < required_extensions.size(); ++i)
    enabled_extensions.push_back(required_extensions[i].c_str());
  info.enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size());
  info.ppEnabledExtensionNames = enabled_extensions.data();

  if (ptrs_.vkCreateDevice(physical_device_, &info, nullptr, &device_) !=
      VK_SUCCESS) {
    return Result("Vulkan::Calling vkCreateDevice Fail");
  }

  return {};
}

std::vector<std::string> Device::GetAvailableExtensions(
    const VkPhysicalDevice& physical_device) {
  std::vector<std::string> available_extensions;
  uint32_t available_extension_count = 0;
  std::vector<VkExtensionProperties> available_extension_properties;

  if (ptrs_.vkEnumerateDeviceExtensionProperties(physical_device, nullptr,
                                                 &available_extension_count,
                                                 nullptr) != VK_SUCCESS) {
    return available_extensions;
  }

  if (available_extension_count == 0)
    return available_extensions;

  available_extension_properties.resize(available_extension_count);
  if (ptrs_.vkEnumerateDeviceExtensionProperties(
          physical_device, nullptr, &available_extension_count,
          available_extension_properties.data()) != VK_SUCCESS) {
    return available_extensions;
  }

  for (const auto& property : available_extension_properties)
    available_extensions.push_back(property.extensionName);

  return available_extensions;
}

Result Device::AreAllValidationLayersSupported() {
  std::vector<VkLayerProperties> available_layer_properties;
  uint32_t available_layer_count = 0;
  if (ptrs_.vkEnumerateInstanceLayerProperties(&available_layer_count,
                                               nullptr) != VK_SUCCESS) {
    return Result("Vulkan: vkEnumerateInstanceLayerProperties fail");
  }
  available_layer_properties.resize(available_layer_count);
  if (ptrs_.vkEnumerateInstanceLayerProperties(
          &available_layer_count, available_layer_properties.data()) !=
      VK_SUCCESS) {
    return Result("Vulkan: vkEnumerateInstanceLayerProperties fail");
  }

  std::set<std::string> required_layer_set(
      kRequiredValidationLayers,
      &kRequiredValidationLayers[kNumberOfRequiredValidationLayers]);
  for (const auto& property : available_layer_properties) {
    required_layer_set.erase(property.layerName);
  }

  if (required_layer_set.empty())
    return {};

  std::string missing_layers;
  for (const auto& missing_layer : required_layer_set)
    missing_layers = missing_layers + missing_layer + ",\n\t\t";
  return Result("Vulkan: missing validation layers:\n\t\t" + missing_layers);
}

bool Device::AreAllValidationExtensionsSupported() {
  for (const auto& layer : kRequiredValidationLayers) {
    uint32_t available_extension_count = 0;
    std::vector<VkExtensionProperties> extension_properties;

    if (ptrs_.vkEnumerateInstanceExtensionProperties(
            layer, &available_extension_count, nullptr) != VK_SUCCESS) {
      return false;
    }
    extension_properties.resize(available_extension_count);
    if (ptrs_.vkEnumerateInstanceExtensionProperties(
            layer, &available_extension_count, extension_properties.data()) !=
        VK_SUCCESS) {
      return false;
    }

    for (const auto& ext : extension_properties) {
      if (!strcmp(kExtensionForValidationLayer, ext.extensionName))
        return true;
    }
  }

  return false;
}

}  // namespace vulkan
}  // namespace amber
