//
// Created by Christian aan de Wiel on 28/02/2021.
//

#ifndef PVK_MOCKAPPLICATION_HPP
#define PVK_MOCKAPPLICATION_HPP

class MockApplication : public Application {
public:
    MockApplication() = default;
    virtual ~MockApplication() = default;
    MockApplication(const MockApplication &other) = delete;

    MockApplication &operator=(const MockApplication &other) = delete;

    void prepare() {
        initWindow();
        initVulkan();
    }

    void destroy() {
        cleanup();
    }

    vk::RenderPass &getRenderPass() {
        return *this->renderPass;
    }

    vk::Extent2D &getSwapChainExtent() {
        return this->swapChainExtent;
    }

    vk::Queue &getGraphicsQueue() {
        return this->graphicsQueue;
    }

private:
    void initialize() override {}
    void update() override {}
    void render(pvk::CommandBuffer *commandBuffer) override {}
    [[maybe_unused]] void tearDown() override {}
};

#endif //PVK_MOCKAPPLICATION_HPP
