#include <cstdio>
#include <map>

#include "lib/application/application.hpp"

class App : public Application {
    pvk::Pipeline *_pipeline;
    pvk::Pipeline *_skyboxPipeline;

    pvk::Shader *_shader;
    pvk::Shader *_skyboxShader;

    pvk::Object *_skyboxObject;
    pvk::Object *_fox;

    pvk::Texture _skyboxTexture;

    struct {
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec3 cameraPosition;
        glm::vec3 lightPosition;
    } uniformBufferObject;

    struct {
        glm::mat4 model;
        glm::mat4 localMatrix;
        glm::mat4 inverseBindMatrices[256];
        float jointCount;
    } bufferObject;

    struct {
        glm::vec4 baseColorFactor;
        float metallicFactor;
        float roughnessFactor;
    } material;

    void initialize() override {
        // Initialize shader
        _shader = new pvk::Shader("/Users/christian/Documents/VulkanHpp/VulkanHpp/shaders/base.vert.spv",
                                  "/Users/christian/Documents/VulkanHpp/VulkanHpp/shaders/base.frag.spv",
                                  {
                                          {0, pvk::DescriptorType::UNIFORM_BUFFER, pvk::ShaderStage::VERTEX_ONLY,         sizeof(uniformBufferObject)},
                                          {1, pvk::DescriptorType::UNIFORM_BUFFER, pvk::ShaderStage::VERTEX_ONLY,         sizeof(bufferObject)},
                                          {2, pvk::DescriptorType::UNIFORM_BUFFER, pvk::ShaderStage::VERTEX_AND_FRAGMENT, sizeof(material)}
                                  });

        _skyboxShader = new pvk::Shader("/Users/christian/Documents/VulkanHpp/VulkanHpp/shaders/skybox_new.vert.spv",
                                        "/Users/christian/Documents/VulkanHpp/VulkanHpp/shaders/skybox_new.frag.spv",
                                        {
                                                {0, pvk::DescriptorType::UNIFORM_BUFFER,         pvk::ShaderStage::VERTEX_ONLY,   sizeof(uniformBufferObject)},
                                                {1, pvk::DescriptorType::UNIFORM_BUFFER,         pvk::ShaderStage::VERTEX_ONLY,   sizeof(bufferObject)},
                                                {2, pvk::DescriptorType::COMBINED_IMAGE_SAMPLER, pvk::ShaderStage::FRAGMENT_ONLY, 0},
                                        });

        // Create pipeline
        _pipeline = new pvk::Pipeline(renderPass.get(), swapChainExtent, _shader);
        _skyboxPipeline = new pvk::Pipeline(renderPass.get(), swapChainExtent, _skyboxShader,
                                            vk::CullModeFlagBits::eFront, false);

        // Load model
        _fox = pvk::Object::createFromGLTF(graphicsQueue,
                                           "/Users/christian/Downloads/walk_robot/scene.gltf");
        _pipeline->registerObject(_fox);

        // Load skybox
        _skyboxObject = pvk::Object::createFromGLTF(graphicsQueue, "/Users/christian/Downloads/data/models/cube.gltf");
        _skyboxTexture = pvk::ktx::load(graphicsQueue, "/Users/christian/Downloads/data/textures/cubemap_space.ktx");
        _skyboxPipeline->registerObject(_skyboxObject);
        _skyboxPipeline->registerTexture(&_skyboxTexture, 2);

        // Finalize pipeline and prepare for rendering
        _pipeline->prepareForRenderStage();
        _skyboxPipeline->prepareForRenderStage();

        uniformBufferObject.view = camera->getViewMatrix();
        uniformBufferObject.projection = glm::perspective(glm::radians(30.0f),
                                                          swapChainExtent.width / (float) swapChainExtent.height, 0.1f,
                                                          1000.0f);
        uniformBufferObject.projection[1][1] *= -1;
        uniformBufferObject.lightPosition = glm::vec3(0.0f, 0.0f, 5.0f);

        auto setMaterial = [](pvk::gltf::Object *object, pvk::gltf::Node *node, vk::DeviceMemory &memory) {
            struct {
                glm::vec4 baseColorFactor = glm::vec4(1.0f);
                float metallicFactor = 0.5f;
                float roughnessFactor = 0.4f;
            } material;
            pvk::buffer::update(memory, sizeof(material), &material);
//            pvk::buffer::update(memory,
//                                sizeof(object->primitiveLookup[node->nodeIndex][0]->material),
//                                &object->primitiveLookup[node->nodeIndex][0]->material);
        };

        _fox->updateUniformBufferPerNode(2, setMaterial);
    }

    void update() override {
        uniformBufferObject.view = camera->getViewMatrix();
        uniformBufferObject.cameraPosition = camera->position;
        uniformBufferObject.lightPosition =
                uniformBufferObject.lightPosition + glm::vec3(0, 0, 100.0f * this->deltaTime);

        _fox->updateUniformBuffer(0, sizeof(uniformBufferObject), &uniformBufferObject);
        _skyboxObject->updateUniformBuffer(0, sizeof(uniformBufferObject), &uniformBufferObject);

        const auto setUniformBufferObject = [](pvk::gltf::Object *object,
                                               pvk::gltf::Node *node,
                                               vk::DeviceMemory &memory) {
            struct {
                glm::mat4 model;
                glm::mat4 localMatrix;
                glm::mat4 inverseBindMatrices[256];
                float jointCount;
            } _bufferObject{};

            _bufferObject.model = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
            _bufferObject.localMatrix = node->getGlobalMatrix();

            auto &inverseBindMatrices = object->inverseBindMatrices;

            if (inverseBindMatrices.empty()) {
                // No need to change the buffer object.
            } else {
                for (size_t i = 0; i < inverseBindMatrices.size(); i++) {
                    _bufferObject.inverseBindMatrices[i] = inverseBindMatrices[i];
                }
            }

            _bufferObject.jointCount = static_cast<float>(inverseBindMatrices.size());

            pvk::buffer::update(memory, sizeof(_bufferObject), &_bufferObject);
        };

        _fox->getAnimations()[0]->update(this->deltaTime);
        _fox->gltfObject->updateJoints();
        _fox->updateUniformBufferPerNode(1, setUniformBufferObject);
        _skyboxObject->updateUniformBufferPerNode(1, setUniformBufferObject);
    }

    void render(pvk::CommandBuffer *commandBuffer) override {
        commandBuffer->bindPipeline(_skyboxPipeline);

        for (auto &node : _skyboxObject->getNodes()) {
            commandBuffer->drawNode(_skyboxObject->gltfObject, node);
        }

        commandBuffer->bindPipeline(_pipeline);

        for (auto &node : _fox->gltfObject->nodeLookup) {
            commandBuffer->drawNode(_fox->gltfObject, node.second);
        }
    }

    void tearDown() override {
        delete _fox;
        delete _skyboxObject;
        delete _pipeline;
        delete _skyboxPipeline;
        delete _shader;
        delete _skyboxShader;
    }
};

int main() {
    App app;

    try {
        app.run();
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
