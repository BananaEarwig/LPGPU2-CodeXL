//==============================================================================
/// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file   vktImageRenderer.h
/// \brief  Header file for VktImageRenderer.
///         This class helps to render Vulkan images into RGBA8 CPU buffers.
//==============================================================================

#ifndef __VKT_IMAGE_RENDERER_H__
#define __VKT_IMAGE_RENDERER_H__

#include "../Util/vktUtil.h"
#include "../../../Common/SaveImage.h"

// Disable some warnings set off by LunarG-provided code
#if AMDT_BUILD_TARGET == AMDT_WINDOWS_OS
    #pragma warning (push)
    #pragma warning (disable : 4100)
    #pragma warning (disable : 4458)
#elif AMDT_BUILD_TARGET == AMDT_LINUX_OS
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-parameter"
#else
    #error Unknown build target! No valid value for AMDT_BUILD_TARGET.
#endif

#include <GlslangToSpv.h>

// pop the warning suppression pragmas
#if AMDT_BUILD_TARGET == AMDT_WINDOWS_OS
    #pragma warning (pop)
    #pragma warning (disable : 4505)
#elif AMDT_BUILD_TARGET == AMDT_LINUX_OS
    #pragma GCC diagnostic pop
#endif

/// Bytes per pixel in render target
static const UINT BytesPerPixel = 4;

//-----------------------------------------------------------------------------
/// Define possible measurement types.
//-----------------------------------------------------------------------------
struct UniformBuffer
{
    UINT rtWidth;  ///< Tell the shader the width of the RT
    UINT flipX;    ///< Flip along X axis
    UINT flipY;    ///< Flip along Y axis
};

//-----------------------------------------------------------------------------
/// Define possible measurement types.
//-----------------------------------------------------------------------------
struct VktImageRendererConfig
{
    VkPhysicalDevice physicalDevice; ///< The renderer's physical device
    VkDevice         device;         ///< The renderer's device
    VkQueue          queue;          ///< The queue used by the renderer
};

//-----------------------------------------------------------------------------
/// Define possible measurement types.
//-----------------------------------------------------------------------------
struct CaptureAssets
{
    VkImage        internalRT;     ///< Image backing our RT
    VkDeviceMemory internalRTMem;  ///< Memory backing our RT
    VkImageView    internalRTView; ///< View of our RT
    VkFramebuffer  frameBuffer;    ///< Our RT
    VkImageView    srcImageView;   ///< View pointing to the app's RT
    VkBuffer       uniformBuf;     ///< Uniform buffer used to pass data to our shader
    VkDeviceMemory uniformBufMem;  ///< Memory backing our uniform buffer
    VkBuffer       storageBuf;     ///< Storage buffer
    VkDeviceMemory storageBufMem;  ///< Memory backing our storage buffer
};

//-----------------------------------------------------------------------------
/// Define image capture settings.
//-----------------------------------------------------------------------------
struct ImgCaptureSettings
{
    bool          enable;      ///< Designate that image capture should be enabled
    VkImage       srcImage;    ///< The source image to capture
    VkImageLayout prevState;   ///< The source image's previous state
    UINT          srcWidth;    ///< The source image width
    UINT          srcHeight;   ///< The source image height
    UINT          dstWidth;    ///< The destination RT width
    UINT          dstHeight;   ///< The destination RT height
    bool          flipX;       ///< Whether we want to flip the x-axis
    bool          flipY;       ///< Whether we want to flip the y-axis
};

//-----------------------------------------------------------------------------
/// This class is used to capture a screen shot of an app's frame buffer.
//-----------------------------------------------------------------------------
class VktImageRenderer
{
public:
    static VktImageRenderer* Create(const VktImageRendererConfig& config);

    static void CorrectSizeForApsectRatio(
        UINT  srcWidth,
        UINT  srcHeight,
        UINT& dstWidth,
        UINT& dstHeight);

    ~VktImageRenderer();

    VkResult CaptureImage(ImgCaptureSettings& settings, CpuImage* pImgOut);

    VkCommandBuffer PrepCmdBuf(
        VkImage        srcImage,
        VkImageLayout  prevState,
        UINT           dstWidth,
        UINT           dstHeight,
        CaptureAssets& assets);

    VkResult CreateCaptureAssets(VkImage srcImage, UINT newWidth, UINT newHeight, bool flipX, bool flipY, CaptureAssets& assets);

    void FreeCaptureAssets(CaptureAssets& assets);

    VkResult FetchResults(UINT width, UINT height, CaptureAssets& assets, CpuImage* pImgOut);

private:
    VktImageRenderer();

    VkResult Init(const VktImageRendererConfig& config);

    VkResult InitShaders(
        VkDevice                         device,
        VkPipelineShaderStageCreateInfo* pShaderStages,
        const char*                      pVertShaderText,
        const char*                      pFragShaderText);

    void ChangeImageLayout(VkImage image, VkImageAspectFlags aspectMask, VkImageLayout prevLayout, VkImageLayout newLayout);

    VkResult MemTypeFromProps(UINT typeBits, VkFlags reqsMask, UINT* pTypeIdx);

    VkResult AllocBindImageMem(VkImage* pImage, VkDeviceMemory* pMem, VkDeviceSize* pMemSize);
    VkResult AllocBindBufferMem(VkDescriptorBufferInfo& bufferInfo, VkBuffer* pBuf, VkDeviceMemory* pMem, VkDeviceSize* pMemSize);

    void InitResources(TBuiltInResource& resources);
    EShLanguage FindLanguage(const VkShaderStageFlagBits shaderType);
    VkResult GLSLtoSPV(const VkShaderStageFlagBits shaderType, const char* pShaderStr, std::vector<UINT>& spirv);

    /// Configuration for this renderer
    VktImageRendererConfig m_config;

    /// Physical device properties
    VkPhysicalDeviceMemoryProperties m_memProps;

    /// Instance dispatch table
    VkLayerInstanceDispatchTable* m_pInstanceDT;

    /// Device dispatch table
    VkLayerDispatchTable* m_pDeviceDT;

    /// This renderer's command pool
    VkCommandPool m_cmdPool;

    /// This renderer's command buffer
    VkCommandBuffer m_cmdBuf;

    /// This renderer's render pass
    VkRenderPass m_renderPass;

    /// This renderer's descriptor set pool
    VkDescriptorPool m_descPool;

    /// This renderer's descriptor set layout
    VkDescriptorSetLayout m_descSetLayout;

    /// This renderer's descriptor set
    VkDescriptorSet m_descSet;

    /// The PSO layout used by the renderer
    VkPipelineLayout m_pipelineLayout;

    /// The PSO cache used by the renderer
    VkPipelineCache m_pipelineCache;

    /// The PSO used by the renderer
    VkPipeline m_pipeline;

    /// Texture sampler for the src image
    VkSampler m_sampler;
};

#endif // __VKT_IMAGE_RENDERER_H__
