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

#ifndef SRC_VULKAN_DEVICE_H_
#define SRC_VULKAN_DEVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "amber/result.h"
#include "amber/vulkan_header.h"
#include "src/feature.h"

namespace amber {
namespace vulkan {

struct VulkanPtrs {
#define AMBER_VK_FUNC(func) PFN_##func func;
#define AMBER_VK_GLOBAL_FUNC(func) PFN_##func func;
#include "src/vulkan/vk-funcs.inc"
#undef AMBER_VK_GLOBAL_FUNC
#undef AMBER_VK_FUNC
};

class Device {
 public:
  Device();
  Device(VkInstance instance,
         VkPhysicalDevice physical_device,
         const VkPhysicalDeviceFeatures& available_features,
         const std::vector<std::string>& required_extensions,
         uint32_t queue_family_index,
         VkDevice device,
         VkQueue queue);
  ~Device();

  Result Initialize(PFN_vkGetInstanceProcAddr getInstanceProcAddr,
                    const std::vector<Feature>& required_features,
                    const std::vector<std::string>& required_extensions);
  void Shutdown();

  VkInstance GetInstance() const { return instance_; }
  VkPhysicalDevice GetPhysicalDevice() { return physical_device_; }
  VkDevice GetDevice() const { return device_; }
  VkPhysicalDevice GetPhysicalDevice() const { return physical_device_; }
  uint32_t GetQueueFamilyIndex() const { return queue_family_index_; }
  VkQueue GetQueue() const { return queue_; }
  const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() const {
    return physical_device_properties_;
  }
  const VkPhysicalDeviceMemoryProperties& GetPhysicalMemoryProperties() const {
    return physical_memory_properties_;
  }

  const VulkanPtrs* GetPtrs() const { return &ptrs_; }

 private:
  Result LoadVulkanGlobalPointers(PFN_vkGetInstanceProcAddr);
  Result LoadVulkanPointers(PFN_vkGetInstanceProcAddr);
  Result CreateInstance();
  Result CreateDebugReportCallback();

  // Get a physical device by checking if the physical device has a proper
  // queue family, required features, and required extensions. Note that
  // this method calls ChooseQueueFamilyIndex() to check if any queue
  // provided by the physical device supports graphics or compute pipeline
  // and sets |queue_family_index_| for the proper queue family.
  Result ChoosePhysicalDevice(
      const std::vector<Feature>& required_features,
      const std::vector<std::string>& required_extensions);

  // Return true if |physical_device| has a queue family that supports both
  // graphics and compute or only a compute pipeline. If the proper queue
  // family exists, |queue_family_index_| will have the queue family index
  // and flags, respectively. Return false if the proper queue family does
  // not exist.
  bool ChooseQueueFamilyIndex(const VkPhysicalDevice& physical_device);

  // Create a logical device with enabled features |required_features|
  // and enabled extensions|required_extensions|.
  Result CreateDevice(const std::vector<Feature>& required_features,
                      const std::vector<std::string>& required_extensions);

  std::vector<std::string> GetAvailableExtensions(
      const VkPhysicalDevice& physical_device);
  Result AreAllValidationLayersSupported();
  bool AreAllValidationExtensionsSupported();

  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugReportCallbackEXT callback_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkPhysicalDeviceProperties physical_device_properties_;
  VkPhysicalDeviceMemoryProperties physical_memory_properties_;
  VkPhysicalDeviceFeatures available_physical_device_features_;
  std::vector<std::string> available_physical_device_extensions_;
  uint32_t queue_family_index_ = 0;
  VkDevice device_ = VK_NULL_HANDLE;

  VkQueue queue_ = VK_NULL_HANDLE;

  bool destroy_device_ = true;

  VulkanPtrs ptrs_;
};

}  // namespace vulkan

}  // namespace amber

#endif  // SRC_VULKAN_DEVICE_H_
