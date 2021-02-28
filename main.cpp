#include <chrono>

#include "lib/application/application.hpp"

class App : public Application
{
public:
    App() = default;
    ~App() = default;

private:
    std::unique_ptr<pvk::Pipeline> _pipeline;
    std::unique_ptr<pvk::Pipeline> _skyboxPipeline;

    std::shared_ptr<pvk::Object> _fox;
    std::shared_ptr<pvk::Object> _skyboxObject;

    std::shared_ptr<pvk::Texture> _skyboxTexture;

    struct
    {
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec3 cameraPosition;
        glm::vec3 lightPosition;
    } uniformBufferObject;

    struct
    {
        glm::mat4 model;
        glm::mat4 localMatrix;
        glm::mat4 inverseBindMatrices[256];
        float jointCount;
    } bufferObject;

    struct
    {
        glm::vec4 baseColorFactor;
        float metallicFactor;
        float roughnessFactor;
    } materialStructure;

    void initialize() override
    {
        _pipeline = pvk::createPipelineFromDefinition(
            "/Users/christian/PVK-Engine/definitions/pbr.json", renderPass.get(), swapChainExtent);
        _skyboxPipeline = pvk::createPipelineFromDefinition(
            "/Users/christian/PVK-Engine/definitions/skybox.json", renderPass.get(), swapChainExtent);

        _pipeline->setUniformBufferSize(0, 0, sizeof(uniformBufferObject));
        _pipeline->setUniformBufferSize(0, 1, sizeof(bufferObject));
        _pipeline->setUniformBufferSize(1, 0, sizeof(materialStructure));

        _skyboxPipeline->setUniformBufferSize(0, 0, sizeof(uniformBufferObject));
        _skyboxPipeline->setUniformBufferSize(0, 1, sizeof(bufferObject));

        // Load model
        auto t1 = std::chrono::high_resolution_clock::now();
        _fox = pvk::Object::createFromGLTF(graphicsQueue, "/Users/christian/Downloads/chaman_ti-pche_3_animations/scene.gltf");
        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        std::cout << "Loading model took " << duration << "ms" << std::endl;

        _pipeline->registerObject(_fox);

        // Load skybox
        _skyboxObject = pvk::Object::createFromGLTF(graphicsQueue, "/Users/christian/Downloads/data/models/cube.gltf");
        _skyboxTexture = pvk::ktx::load(graphicsQueue, "/Users/christian/Downloads/data/textures/cubemap_space.ktx");
        _skyboxPipeline->registerTexture(_skyboxTexture, 0, 2);
        _skyboxPipeline->registerObject(_skyboxObject);

        // Finalize pipeline and prepare for rendering
        _pipeline->prepare();
        _skyboxPipeline->prepare();

        uniformBufferObject.view = camera->getViewMatrix();
        uniformBufferObject.projection =
            glm::perspective(glm::radians(30.0F), swapChainExtent.width / (float)swapChainExtent.height, 0.1F, 1000.0F);
        uniformBufferObject.projection[1][1] *= -1;
        uniformBufferObject.lightPosition = glm::vec3(10.0F, 10.0F, 10.0F);

        auto setMaterial = [](pvk::gltf::Object &object, pvk::gltf::Primitive &primitive, vk::UniqueDeviceMemory &memory) {
            pvk::buffer::update(memory, sizeof(primitive.getMaterial().materialFactor), &primitive.getMaterial().materialFactor);
        };

        _fox->updateUniformBufferPerPrimitive(setMaterial, 1, 0);
    }

    void update() override
    {
        uniformBufferObject.view = camera->getViewMatrix();
        uniformBufferObject.cameraPosition = camera->position;
//        uniformBufferObject.lightPosition += glm::vec3(0, 0, 10.0F * this->deltaTime);

        _fox->updateUniformBuffer(&uniformBufferObject, sizeof(uniformBufferObject), 0, 0);
        _skyboxObject->updateUniformBuffer(&uniformBufferObject, sizeof(uniformBufferObject), 0, 0);

        const auto updateInverseBindMatrices = [](pvk::gltf::Object &object,
                                                  pvk::gltf::Node &node,
                                                  vk::UniqueDeviceMemory &memory) {
            auto &inverseBindMatrices = object.inverseBindMatrices;

            for (size_t i = 0; i < inverseBindMatrices.size(); i++)
            {
                node.bufferObject.inverseBindMatrices[i] = inverseBindMatrices[i];
            }

            node.bufferObject.jointCount = node.skinIndex > -1 ? static_cast<float>(inverseBindMatrices.size()) : 0.0F;
        };

        const auto setUniformBufferObject =
            [](pvk::gltf::Object &object, pvk::gltf::Node &node, vk::UniqueDeviceMemory &memory) {
                node.bufferObject.model = glm::scale(glm::mat4(1.0f), glm::vec3(0.1F));
                node.bufferObject.localMatrix = node.getGlobalMatrix();
                pvk::buffer::update(memory, sizeof(node.bufferObject), &node.bufferObject);
            };

        _fox->getAnimation(0).update(this->deltaTime);
        _fox->gltfObject->updateJoints();
        _fox->updateUniformBufferPerNode(updateInverseBindMatrices, 0, 1);
        _fox->updateUniformBufferPerNode(setUniformBufferObject, 0, 1);
        _skyboxObject->updateUniformBufferPerNode(setUniformBufferObject, 0, 1);
    }

    void render(pvk::CommandBuffer *commandBuffer) override
    {
        for (const auto &node : _skyboxObject->gltfObject->getNodes())
        {
            commandBuffer->drawNode(*_skyboxPipeline, *_skyboxObject->gltfObject, *node.second.lock());
        }

        for (const auto &node : _fox->gltfObject->getNodes())
        {
            commandBuffer->drawNode(*_pipeline, *_fox->gltfObject, *node.second.lock());
        }
    }

    void tearDown() override
    {
    }
};

auto main() -> int
{
    try
    {
        App app;
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    pvk::Context::tearDown();

    return EXIT_SUCCESS;
}
