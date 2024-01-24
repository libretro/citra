// Copyright 2023 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <limits>
#include "common/logging/log.h"
#include "common/microprofile.h"
#include "common/settings.h"
#include "citra_libretro/vulkan/vk_swapchain.h"
#include "citra_libretro/environment.h"

namespace LibRetro {
namespace Vulkan {

static const retro_hw_render_interface_vulkan* vulkan;

#define VULKAN_MAX_SWAPCHAIN_IMAGES 8
struct VkSwapchainKHR_T
{
  uint32_t count;
  struct
  {
    VkImage handle;
    VkDeviceMemory memory;
    retro_vulkan_image retro_image;
  } images[VULKAN_MAX_SWAPCHAIN_IMAGES];
  std::mutex mutex;
  std::condition_variable condVar;
  int current_index;
};
static VkSwapchainKHR_T chain;

Swapchain::Swapchain(const ::Vulkan::Instance& instance_, u32 width, u32 height, vk::SurfaceKHR surface_)
    : instance{instance_}, surface{surface_} {
    vulkan = LibRetro::GetHWRenderInterfaceVulkan();
    FindPresentFormat();
    Create(width, height, surface);
}

Swapchain::~Swapchain() {
    Destroy();
}

bool Swapchain::MemoryTypeFromProperties(uint32_t typeBits, VkFlags requirements_mask, uint32_t* typeIndex) {
  VkPhysicalDeviceMemoryProperties memory_properties = instance.GetPhysicalDevice().getMemoryProperties();
  // Search memtypes to find first index with those properties
  for (uint32_t i = 0; i < 32; i++) {
    if ((typeBits & 1) == 1) {
      // Type is available, does it match user properties?
      if ((memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
        *typeIndex = i;
        return true;
      }
    }
    typeBits >>= 1;
  }
  // No memory types matched, return failure
  return false;
}

void Swapchain::Create(u32 width_, u32 height_, vk::SurfaceKHR surface_) {
    width = width_;
    height = height_;
    surface = surface_;
    needs_recreation = false;

    Destroy();

    SetPresentMode();
    SetSurfaceProperties();

    const std::array queue_family_indices = {
        instance.GetGraphicsQueueFamilyIndex(),
        instance.GetPresentQueueFamilyIndex(),
    };

    const bool exclusive = queue_family_indices[0] == queue_family_indices[1];
    const u32 queue_family_indices_count = exclusive ? 1u : 2u;
    const vk::SharingMode sharing_mode =
        exclusive ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;
    const vk::SwapchainCreateInfoKHR swapchain_info = {
        .surface = surface,
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment |
                      vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
        .imageSharingMode = sharing_mode,
        .queueFamilyIndexCount = queue_family_indices_count,
        .pQueueFamilyIndices = queue_family_indices.data(),
        .preTransform = transform,
        .compositeAlpha = composite_alpha,
        .presentMode = present_mode,
        .clipped = true,
        .oldSwapchain = nullptr,
    };

    uint32_t swapchain_mask = vulkan->get_sync_index_mask(vulkan->handle);

    chain.count = 0;
    while (swapchain_mask) {
        chain.count++;
        swapchain_mask >>= 1;
    }
    assert(chain.count <= VULKAN_MAX_SWAPCHAIN_IMAGES);

    for (uint32_t i = 0; i < chain.count; i++) {
        {
            vk::ImageCreateInfo info{
                .flags = vk::ImageCreateFlagBits::eMutableFormat,
                .imageType = vk::ImageType::e2D,
                .format = swapchain_info.imageFormat,
                .extent = {
                    .width = swapchain_info.imageExtent.width,
                    .height = swapchain_info.imageExtent.height,
                    .depth = 1,
                },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = vk::SampleCountFlagBits::e1,
                .tiling = vk::ImageTiling::eOptimal,
                .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst |
                        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eColorAttachment,

                .initialLayout = vk::ImageLayout::eUndefined,
            };

            chain.images[i].handle = instance.GetDevice().createImage(info);
        }

        vk::MemoryRequirements memreq = instance.GetDevice().getImageMemoryRequirements(chain.images[i].handle);

        vk::MemoryAllocateInfo alloc{
            .allocationSize = memreq.size
        };
/*
        VkMemoryDedicatedAllocateInfoKHR dedicated{
            VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR};
        if (DEDICATED_ALLOCATION)
        {
            alloc.pNext = &dedicated;
            dedicated.image = chain.images[i].handle;
        }
*/
        MemoryTypeFromProperties(memreq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                &alloc.memoryTypeIndex);
        chain.images[i].memory = instance.GetDevice().allocateMemory(alloc);
        instance.GetDevice().bindImageMemory(chain.images[i].handle, chain.images[i].memory, 0);

        chain.images[i].retro_image.create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        chain.images[i].retro_image.create_info.image = chain.images[i].handle;
        chain.images[i].retro_image.create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        chain.images[i].retro_image.create_info.format = static_cast<VkFormat>(swapchain_info.imageFormat);
        chain.images[i].retro_image.create_info.components = {
            VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A};
        chain.images[i].retro_image.create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        chain.images[i].retro_image.create_info.subresourceRange.layerCount = 1;
        chain.images[i].retro_image.create_info.subresourceRange.levelCount = 1;

        vk::Result res = instance.GetDevice().createImageView(reinterpret_cast<const vk::ImageViewCreateInfo *>(&chain.images[i].retro_image.create_info), nullptr,
                            reinterpret_cast<vk::ImageView *>(&chain.images[i].retro_image.image_view));
        if (res != vk::Result::eSuccess) {
            LOG_CRITICAL(Render_Vulkan, "Unable to create image view - {}", res);
        }

        chain.images[i].retro_image.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    chain.current_index = -1;

    SetupImages();
    RefreshSemaphores();
}

bool Swapchain::AcquireNextImage() {
    vulkan->wait_sync_index(vulkan->handle);
    image_index = vulkan->get_sync_index(vulkan->handle);
    return true;
}

void Swapchain::Present() {
    std::unique_lock<std::mutex> lock(chain.mutex);
    chain.current_index = image_index;

    vulkan->set_image(vulkan->handle, &chain.images[image_index].retro_image,
                    0, nullptr, vulkan->queue_index);

    chain.condVar.notify_all();

    frame_index = (frame_index + 1) % image_count;
}

void Swapchain::FindPresentFormat() {
    const auto formats = instance.GetPhysicalDevice().getSurfaceFormatsKHR(surface);

    // If there is a single undefined surface format, the device doesn't care, so we'll just use
    // RGBA.
    if (formats[0].format == vk::Format::eUndefined) {
        surface_format.format = vk::Format::eR8G8B8A8Unorm;
        surface_format.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
        return;
    }

    // Try to find a suitable format.
    for (const vk::SurfaceFormatKHR& sformat : formats) {
        vk::Format format = sformat.format;
        if (format != vk::Format::eR8G8B8A8Unorm && format != vk::Format::eB8G8R8A8Unorm) {
            continue;
        }

        surface_format.format = format;
        surface_format.colorSpace = sformat.colorSpace;
        return;
    }

    UNREACHABLE_MSG("Unable to find required swapchain format!");
}

void Swapchain::SetPresentMode() {
    const auto modes = instance.GetPhysicalDevice().getSurfacePresentModesKHR(surface);
    const bool use_vsync = Settings::values.use_vsync_new.GetValue();
    const auto find_mode = [&modes](vk::PresentModeKHR requested) {
        const auto it =
            std::find_if(modes.begin(), modes.end(),
                         [&requested](vk::PresentModeKHR mode) { return mode == requested; });

        return it != modes.end();
    };

    present_mode = vk::PresentModeKHR::eFifo;
    const bool has_immediate = find_mode(vk::PresentModeKHR::eImmediate);
    const bool has_mailbox = find_mode(vk::PresentModeKHR::eMailbox);
    if (!has_immediate && !has_mailbox) {
        LOG_WARNING(Render_Vulkan, "Forcing Fifo present mode as no alternatives are available");
        return;
    }

    // If the user has disabled vsync use immediate mode for the least latency.
    // This may have screen tearing.
    if (!use_vsync) {
        present_mode =
            has_immediate ? vk::PresentModeKHR::eImmediate : vk::PresentModeKHR::eMailbox;
        return;
    }
    // If vsync is enabled attempt to use mailbox mode in case the user wants to speedup/slowdown
    // the game. If mailbox is not available use immediate and warn about it.
    if (use_vsync && Settings::values.frame_limit.GetValue() > 100) {
        present_mode = has_mailbox ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eImmediate;
        if (!has_mailbox) {
            LOG_WARNING(
                Render_Vulkan,
                "Vsync enabled while frame limiting and no mailbox support, expect tearing");
        }
        return;
    }
}

void Swapchain::SetSurfaceProperties() {
    const vk::SurfaceCapabilitiesKHR capabilities =
        instance.GetPhysicalDevice().getSurfaceCapabilitiesKHR(surface);

    extent = capabilities.currentExtent;
    if (capabilities.currentExtent.width == std::numeric_limits<u32>::max()) {
        extent.width = std::max(capabilities.minImageExtent.width,
                                std::min(capabilities.maxImageExtent.width, width));
        extent.height = std::max(capabilities.minImageExtent.height,
                                 std::min(capabilities.maxImageExtent.height, height));
    }

    // Select number of images in swap chain, we prefer one buffer in the background to work on
    image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) {
        image_count = std::min(image_count, capabilities.maxImageCount);
    }

    // Prefer identity transform if possible
    transform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    if (!(capabilities.supportedTransforms & transform)) {
        transform = capabilities.currentTransform;
    }

    // Opaque is not supported everywhere.
    composite_alpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    if (!(capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque)) {
        composite_alpha = vk::CompositeAlphaFlagBitsKHR::eInherit;
    }
}

void Swapchain::Destroy() {
    vk::Device device = instance.GetDevice();
    for (u32 i = 0; i < image_count; i++) {
        device.destroySemaphore(image_acquired[i]);
        device.destroySemaphore(present_ready[i]);
    }
    image_acquired.clear();
    present_ready.clear();
}

void Swapchain::RefreshSemaphores() {
    const vk::Device device = instance.GetDevice();
    image_acquired.resize(image_count);
    present_ready.resize(image_count);

    for (vk::Semaphore& semaphore : image_acquired) {
        semaphore = device.createSemaphore({});
    }
    for (vk::Semaphore& semaphore : present_ready) {
        semaphore = device.createSemaphore({});
    }
}

void Swapchain::SetupImages() {
    images.clear();
    images.resize(chain.count);
    for (uint32_t i = 0; i < chain.count; ++i) {
        images[i] = chain.images[i].handle;
    }
    image_count = static_cast<u32>(images.size());
}

} // namespace Vulkan
} // namespace LibRetro
