//
//  application.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 20/01/2021.
//

#ifndef application_h
#define application_h

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <array>
#include <optional>
#include <set>

#include "../context/context.hpp"
#include "../debug/debug.hpp"
#include "../buffer/buffer.hpp"
#include "../util/util.hpp"
#include "../device/physicalDevice.hpp"
#include "../device/logicalDevice.hpp"
#include "../pipeline/pipelineBuilder.hpp"
#include "../gltf/GLTFLoader.hpp"
#include "../mesh/vertex.hpp"
#include "../gltf/GLTFObject.hpp"
#include "../ktx/KTXLoader.hpp"
#include "../camera/camera.hpp"
#include "../shader/shader.hpp"
#include "../pipeline/pipeline.hpp"
#include "../commandBuffer/commandBuffer.hpp"

const int WIDTH = 1280;
const int HEIGHT = 720;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct {
    glm::mat4 model;
    glm::mat4 localMatrix;
    glm::mat4 view;
    glm::mat4 projection;
} ubo;

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

class Application {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

protected:
    GLFWwindow* window;
    pvk::Camera* camera;
    pvk::Context* context;

    vk::UniqueDebugUtilsMessengerEXT debugMessenger;
    vk::UniqueInstance instance;
    vk::SurfaceKHR surface;

    vk::PhysicalDevice physicalDevice;
    vk::UniqueDevice device;
    
    vk::PipelineCache pipelineCache;

    vk::Queue graphicsQueue;
    vk::Queue presentQueue;

    vk::SwapchainKHR swapChain;
    std::vector<vk::Image> swapChainImages;
    vk::Format swapChainImageFormat;
    vk::Extent2D swapChainExtent;
    std::vector<vk::ImageView> swapChainImageViews;
    std::vector<vk::Framebuffer> swapChainFramebuffers;
    
    vk::UniqueRenderPass renderPass;

    vk::UniqueCommandPool commandPool;
    
    vk::UniqueImage depthImage;
    vk::UniqueDeviceMemory depthImageMemory;
    vk::UniqueImageView depthImageView;

    std::vector<vk::CommandBuffer, std::allocator<vk::CommandBuffer>> commandBuffers;

    std::vector<vk::Semaphore> imageAvailableSemaphores;
    std::vector<vk::Semaphore> renderFinishedSemaphores;
    std::vector<vk::Fence> inFlightFences;
    size_t currentFrame = 0;
    
    float deltaTime = 0.0f;
    
    bool wPressed = false;
    bool aPressed = false;
    bool sPressed = false;
    bool dPressed = false;
    
    bool initializeMouse;
    bool isMouseActive = false;
    double lastMouseX;
    double lastMouseY;
    double xOffset;
    double yOffset;

    bool framebufferResized = false;
    
    virtual void initialize() = 0;
    virtual void update() = 0;
    virtual void render(pvk::CommandBuffer* commandBuffer) = 0;
    virtual void tearDown() = 0;

    void initWindow() {
        glfwInit();
        
        this->camera = new pvk::Camera(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        glfwSetKeyCallback(this->window, handleKeyboardInput);
        glfwSetCursorPosCallback(this->window, handleMouseInput);
        glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    
    static void handleKeyboardInput(GLFWwindow *window, int key, int scancode, int action, int mods) {
        auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        
        switch (key) {
            case GLFW_KEY_W:
                if (action == GLFW_PRESS) {
                    app->wPressed = true;
                } else if (action == GLFW_RELEASE) {
                    app->wPressed = false;
                }
                break;
                
            case GLFW_KEY_A:
                if (action == GLFW_PRESS) {
                    app->aPressed = true;
                } else if (action == GLFW_RELEASE) {
                    app->aPressed = false;
                }
                break;
                
            case GLFW_KEY_S:
                if (action == GLFW_PRESS) {
                    app->sPressed = true;
                } else if (action == GLFW_RELEASE) {
                    app->sPressed = false;
                }
                break;
                
            case GLFW_KEY_D:
                if (action == GLFW_PRESS) {
                    app->dPressed = true;
                } else if (action == GLFW_RELEASE) {
                    app->dPressed = false;
                }
                break;
                
            default:
                break;
        }
    }
    
    static void handleMouseInput(GLFWwindow *window, double mouseX, double mouseY) {
        auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        
        if (app->initializeMouse) {
            app->lastMouseX = (float) mouseX;
            app->lastMouseY = (float) mouseY;
            app->initializeMouse = false;
        } else {
            app->xOffset = mouseX - app->lastMouseX;
            app->yOffset = app->lastMouseY - mouseY;
            
            app->lastMouseX = mouseX;
            app->lastMouseY = mouseY;
            
            app->isMouseActive = true;
        }
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void initVulkan() {
        createContext();
        createInstance();
        setupDebugCallback();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDepthResources();
        createFramebuffers();
        createCommandPool();
        initialize();
        createCommandBuffers();
        createSyncObjects();
    }

    void mainLoop() {
        double timePrevious = glfwGetTime();
        uint32_t frameCount = 0;

        while (!glfwWindowShouldClose(window)) {
            double timeCurrent = glfwGetTime();
            frameCount++;

            if (timeCurrent - timePrevious >= 1.0)
            {
                std::cout << "FPS: " << frameCount << std::endl;
                std::stringstream ss;
                ss << " [" << frameCount << " FPS]";

                glfwSetWindowTitle(window, ss.str().c_str());

                frameCount = 0;
                timePrevious = timeCurrent;
            }

            glfwPollEvents();
            auto startTime = std::chrono::high_resolution_clock::now();
            drawFrame();
            auto currentTime = std::chrono::high_resolution_clock::now();
            this->deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        }

        device->waitIdle();
    }

    void cleanupSwapChain() {
        for (auto framebuffer : swapChainFramebuffers) {
            device->destroyFramebuffer(framebuffer);
        }

        device->freeCommandBuffers(commandPool.get(), commandBuffers);

        for (auto imageView : swapChainImageViews) {
            device->destroyImageView(imageView);
        }

        device->destroySwapchainKHR(swapChain);
    }

    void cleanup() {
        // NOTE: instance destruction is handled by UniqueInstance, same for device
        cleanupSwapChain();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            device->destroySemaphore(renderFinishedSemaphores[i]);
            device->destroySemaphore(imageAvailableSemaphores[i]);
            device->destroyFence(inFlightFences[i]);
        }
        
        tearDown();
        
        device->destroyPipelineCache(pipelineCache, nullptr);

        // surface is created by glfw, therefore not using a Unique handle
        instance->destroySurfaceKHR(surface);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        device->waitIdle();

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createRenderPass();
        createFramebuffers();
        createCommandBuffers();
    }
    
    void createContext() {
        context = pvk::Context::get();
    }

    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        auto appInfo = vk::ApplicationInfo(
            "Hello Triangle",
            VK_MAKE_VERSION(1, 0, 0),
            "No Engine",
            VK_MAKE_VERSION(1, 0, 0),
            VK_API_VERSION_1_0
        );

        auto extensions = getRequiredExtensions();

        auto createInfo = vk::InstanceCreateInfo(
            vk::InstanceCreateFlags(),
            &appInfo,
            0, nullptr, // enabled layers
            static_cast<uint32_t>(extensions.size()), extensions.data() // enabled extensions
        );

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
            
            vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = getDebugUtilsMessengerCreateInfo();
            createInfo.pNext = &debugMessengerCreateInfo;
        }

        try {
            instance = vk::createInstanceUnique(createInfo, nullptr);
        } catch (vk::SystemError &error) {
            throw std::runtime_error("Failed to create instance.");
        }
    }
    
    static vk::DebugUtilsMessengerCreateInfoEXT getDebugUtilsMessengerCreateInfo() {
        return vk::DebugUtilsMessengerCreateInfoEXT(
            {},
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            &debugCallback
        );
    }

    void setupDebugCallback() {
        if (!enableValidationLayers) return;

        debugMessenger = instance->createDebugUtilsMessengerEXTUnique(
            getDebugUtilsMessengerCreateInfo()
        );
    }

    void createSurface() {
        VkSurfaceKHR rawSurface;
        if (glfwCreateWindowSurface(*instance, window, nullptr, &rawSurface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface.");
        }

        surface = rawSurface;
    }

    void pickPhysicalDevice() {
        physicalDevice = pvk::device::physical::initialize(instance.get(), surface, deviceExtensions);
        pvk::Context::setPhysicalDevice(&physicalDevice);
    }

    void createLogicalDevice() {
        pvk::QueueFamilyIndices indices = pvk::device::physical::findQueueFamilies(physicalDevice, surface);

        device = pvk::device::logical::create(physicalDevice, indices, deviceExtensions, validationLayers, enableValidationLayers);
        pvk::Context::setLogicalDevice(&device);

        graphicsQueue = pvk::Context::getLogicalDevice().getQueue(indices.graphicsFamily.value(), 0);
        presentQueue = pvk::Context::getLogicalDevice().getQueue(indices.presentFamily.value(), 0);
        
        pipelineCache = pvk::Context::getLogicalDevice().createPipelineCache(vk::PipelineCacheCreateInfo());
        pvk::Context::setPipelineCache(&pipelineCache);
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo(
            vk::SwapchainCreateFlagsKHR(),
            surface,
            imageCount,
            surfaceFormat.format,
            surfaceFormat.colorSpace,
            extent,
            1, // imageArrayLayers
            vk::ImageUsageFlagBits::eColorAttachment
        );

        pvk::QueueFamilyIndices indices = pvk::device::physical::findQueueFamilies(physicalDevice, surface);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = vk::SwapchainKHR(nullptr);

        try {
            swapChain = device->createSwapchainKHR(createInfo);
        } catch (vk::SystemError &error) {
            throw std::runtime_error("Failed to create swapchain");
        }

        swapChainImages = device->getSwapchainImagesKHR(swapChain);
        pvk::Context::setSwapchainImages(&swapChainImages);

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            vk::ImageViewCreateInfo createInfo = {};
            createInfo.image = swapChainImages[i];
            createInfo.viewType = vk::ImageViewType::e2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = vk::ComponentSwizzle::eIdentity;
            createInfo.components.g = vk::ComponentSwizzle::eIdentity;
            createInfo.components.b = vk::ComponentSwizzle::eIdentity;
            createInfo.components.a = vk::ComponentSwizzle::eIdentity;
            createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            try {
                swapChainImageViews[i] = device->createImageView(createInfo);
            } catch (vk::SystemError &error) {
                throw std::runtime_error("Failed to create image views");
            }
        }
    }

    void createRenderPass() {
        vk::AttachmentDescription colorAttachment = {};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = vk::SampleCountFlagBits::e1;
        colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
        
        vk::AttachmentDescription depthAttachment = {};
        depthAttachment.format = pvk::util::findSupportedFormat(physicalDevice,
                                                                {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                                                                vk::ImageTiling::eOptimal,
                                                                vk::FormatFeatureFlagBits::eDepthStencilAttachment);;
        depthAttachment.samples = vk::SampleCountFlagBits::e1;
        depthAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
        depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
        depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference colorAttachmentRef = {0, vk::ImageLayout::eColorAttachmentOptimal};
        vk::AttachmentReference depthAttachmentRef = {1, vk::ImageLayout::eDepthStencilAttachmentOptimal};

        vk::SubpassDescription subpass = {};
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        vk::SubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        std::vector<vk::AttachmentDescription> attachments = {colorAttachment, depthAttachment};
        
        vk::RenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.attachmentCount = (uint32_t) attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        try {
            renderPass = device->createRenderPassUnique(renderPassInfo);
        } catch (vk::SystemError &error) {
            throw std::runtime_error("Failed to create render pass");
        }
    }
    
    void createDepthResources() {
        auto format = pvk::util::findSupportedFormat(physicalDevice,
                                                     {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                                                     vk::ImageTiling::eOptimal,
                                                     vk::FormatFeatureFlagBits::eDepthStencilAttachment);
        
        pvk::image::create(swapChainExtent.width, swapChainExtent.height, 1,
                           1, vk::SampleCountFlagBits::e1,
                           format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
                           vk::MemoryPropertyFlagBits::eDeviceLocal,
                           {},
                           depthImage, depthImageMemory);
        
        depthImageView = device->createImageViewUnique(vk::ImageViewCreateInfo{
            {},
            depthImage.get(),
            vk::ImageViewType::e2D,
            format,
            {},
            vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1},
        });
    }
    
    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            std::array<vk::ImageView, 2> attachments = {swapChainImageViews[i], depthImageView.get()};

            try {
                vk::FramebufferCreateInfo frameBufferCreateInfo = {vk::FramebufferCreateFlags(),
                    renderPass.get(), static_cast<uint32_t> (attachments.size()), attachments.data(),
                    swapChainExtent.width, swapChainExtent.height, 1};
                swapChainFramebuffers[i] = device->createFramebuffer(frameBufferCreateInfo);
            } catch (vk::SystemError &err) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }
    void createCommandPool() {
        pvk::QueueFamilyIndices queueFamilyIndices = pvk::device::physical::findQueueFamilies(physicalDevice, surface);

        vk::CommandPoolCreateInfo poolInfo = {};
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        try {
            commandPool = device->createCommandPoolUnique(poolInfo);
            pvk::Context::setCommandPool(&commandPool);
        } catch (vk::SystemError &error) {
            throw std::runtime_error("Failed to create command pool");
        }
    }
    
    void updateUniformBuffers() {
        if (this->wPressed) {
            this->camera->update(pvk::FORWARD, this->deltaTime);
        }
        if (this->aPressed) {
            this->camera->update(pvk::LEFT, this->deltaTime);
        }
        if (sPressed) {
            this->camera->update(pvk::BACKWARD, this->deltaTime);
        }
        if (dPressed) {
            this->camera->update(pvk::RIGHT, this->deltaTime);
        }
        
        if (this->isMouseActive) {
            // Mouse is active, so no need to reset offset variables.
        } else {
            this->xOffset = 0;
            this->yOffset = 0;
        }
        
        this->camera->update(static_cast<float>(this->xOffset), static_cast<float>(this->yOffset), this->deltaTime);
        this->isMouseActive = false;
        
        update();
    }

    void createCommandBuffers() {
        commandBuffers.resize(swapChainFramebuffers.size());

        vk::CommandBufferAllocateInfo allocInfo = {};
        allocInfo.commandPool = commandPool.get();
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        try {
            commandBuffers = device->allocateCommandBuffers(allocInfo);
        } catch (vk::SystemError &error) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        for (size_t i = 0; i < commandBuffers.size(); i++) {
            vk::CommandBufferBeginInfo beginInfo = {};
            beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

            try {
                commandBuffers[i].begin(beginInfo);
            }
            catch (vk::SystemError &error) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            vk::RenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.renderPass = renderPass.get();
            renderPassInfo.framebuffer = swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = vk::Offset2D( 0, 0 );
            renderPassInfo.renderArea.extent = swapChainExtent;

            std::array<vk::ClearValue, 2> clearValues;
            clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f});
            clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
            
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            commandBuffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
            
            auto commandBufferPublic = new pvk::CommandBuffer(&commandBuffers[i], (uint32_t) i);
            
            render(commandBufferPublic);
            
            commandBuffers[i].endRenderPass();

            try {
                commandBuffers[i].end();
            } catch (vk::SystemError &error) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        try {
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                imageAvailableSemaphores[i] = device->createSemaphore({});
                renderFinishedSemaphores[i] = device->createSemaphore({});
                inFlightFences[i] = device->createFence({vk::FenceCreateFlagBits::eSignaled});
            }
        } catch (vk::SystemError &error) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }

    void drawFrame() {
        device->waitForFences(1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

        uint32_t imageIndex;
        try {
            vk::ResultValue result = device->acquireNextImageKHR(swapChain, std::numeric_limits<uint64_t>::max(),
                imageAvailableSemaphores[currentFrame], nullptr);
            imageIndex = result.value;
        } catch (vk::OutOfDateKHRError &error) {
            recreateSwapChain();
            return;
        } catch (vk::SystemError &error) {
            throw std::runtime_error("Failed to acquire swapchain image");
        }
        
        updateUniformBuffers();

        vk::SubmitInfo submitInfo = {};

        vk::Semaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

        vk::Semaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        device->resetFences(1, &inFlightFences[currentFrame]);

        try {
            graphicsQueue.submit(submitInfo, inFlightFences[currentFrame]);
        } catch (vk::SystemError &error) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        vk::PresentInfoKHR presentInfo = {};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        vk::SwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        vk::Result resultPresent;
        try {
            resultPresent = presentQueue.presentKHR(presentInfo);
        } catch (vk::OutOfDateKHRError &error) {
            resultPresent = vk::Result::eErrorOutOfDateKHR;
        } catch (vk::SystemError &error) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        if (resultPresent == vk::Result::eSuboptimalKHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
            return;
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    vk::UniqueShaderModule createShaderModule(const std::vector<char>& code) {
        try {
            return device->createShaderModuleUnique({
                vk::ShaderModuleCreateFlags(),
                code.size(),
                reinterpret_cast<const uint32_t*>(code.data())
            });
        } catch (vk::SystemError &error) {
            throw std::runtime_error("failed to create shader module!");
        }
    }

    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
        if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined) {
            return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
        }

        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
        vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
                return availablePresentMode;
            }
            else if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
                bestMode = availablePresentMode;
            }
        }

        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            vk::Extent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    SwapChainSupportDetails querySwapChainSupport(const vk::PhysicalDevice& device) {
        SwapChainSupportDetails details;
        details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
        details.formats = device.getSurfaceFormatsKHR(surface);
        details.presentModes = device.getSurfacePresentModesKHR(surface);

        return details;
    }

    static std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }
    
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {

        std::string message;
        
        message += vk::to_string( static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>( messageSeverity ) ) + ": " +
        vk::to_string( static_cast<vk::DebugUtilsMessageTypeFlagsEXT>( messageTypes ) ) + ":\n";
        message += std::string( "\t" ) + "messageIDName   = <" + pCallbackData->pMessageIdName + ">\n";
        message += std::string( "\t" ) + "messageIdNumber = " + std::to_string( pCallbackData->messageIdNumber ) + "\n";
        message += std::string( "\t" ) + "message         = <" + pCallbackData->pMessage + ">\n";
        if ( 0 < pCallbackData->queueLabelCount )
        {
            message += std::string( "\t" ) + "Queue Labels:\n";
            for ( uint8_t i = 0; i < pCallbackData->queueLabelCount; i++ )
            {
                message += std::string( "\t\t" ) + "labelName = <" + pCallbackData->pQueueLabels[i].pLabelName + ">\n";
            }
        }
        if ( 0 < pCallbackData->cmdBufLabelCount )
        {
            message += std::string( "\t" ) + "CommandBuffer Labels:\n";
            for ( uint8_t i = 0; i < pCallbackData->cmdBufLabelCount; i++ )
            {
                message += std::string( "\t\t" ) + "labelName = <" + pCallbackData->pCmdBufLabels[i].pLabelName + ">\n";
            }
        }
        if ( 0 < pCallbackData->objectCount )
        {
            for ( uint8_t i = 0; i < pCallbackData->objectCount; i++ )
            {
                message += std::string( "\t" ) + "Object " + std::to_string( i ) + "\n";
                message += std::string( "\t\t" ) + "objectType   = " +
                vk::to_string( static_cast<vk::ObjectType>( pCallbackData->pObjects[i].objectType ) ) + "\n";
                message +=
                std::string( "\t\t" ) + "objectHandle = " + std::to_string( pCallbackData->pObjects[i].objectHandle ) + "\n";
                if ( pCallbackData->pObjects[i].pObjectName )
                {
                    message += std::string( "\t\t" ) + "objectName   = <" + pCallbackData->pObjects[i].pObjectName + ">\n";
                }
            }
        }
        
        std::cout << message << std::endl;
        
        return VK_FALSE;
    }

    static bool checkValidationLayerSupport() {
        auto availableLayers = vk::enumerateInstanceLayerProperties();
        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }
};

#endif /* application_h */
