#include <cstdio>
#include <chrono>

#include "lib/application/application.hpp"

class App : public Application {
    pvk::Pipeline *_pipeline;
    pvk::Pipeline *_skyboxPipeline;

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
        _pipeline = pvk::createPipelineFromDefinition("/Users/christian/PVK-Engine/definitions/pbr.json",
                                                      renderPass.get(),
                                                      swapChainExtent);
        _skyboxPipeline = pvk::createPipelineFromDefinition("/Users/christian/PVK-Engine/definitions/skybox.json",
                                                            renderPass.get(),
                                                            swapChainExtent);

        _pipeline->setUniformBufferSize(0, 0, sizeof(uniformBufferObject));
        _pipeline->setUniformBufferSize(0, 1, sizeof(bufferObject));
        _pipeline->setUniformBufferSize(0, 2, sizeof(material));

        _skyboxPipeline->setUniformBufferSize(0, 0, sizeof(uniformBufferObject));
        _skyboxPipeline->setUniformBufferSize(0, 1, sizeof(bufferObject));

        // Load model
        auto t1 = std::chrono::high_resolution_clock::now();
        _fox = pvk::Object::createFromGLTF(graphicsQueue, "/Users/christian/Downloads/walk_robot/scene.gltf");
        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();
        std::cout << "Loading model took " << duration << "ms";

        _pipeline->registerObject(_fox);
        _pipeline->registerTexture(_fox->gltfObject->materials[0]->baseColorTexture, 3);
        _pipeline->registerTexture(_fox->gltfObject->materials[0]->occlusionTexture, 4);
        _pipeline->registerTexture(_fox->gltfObject->materials[0]->metallicRoughnessTexture, 5);

        // Load skybox
        _skyboxObject = pvk::Object::createFromGLTF(graphicsQueue, "/Users/christian/Downloads/data/models/cube.gltf");
        _skyboxTexture = pvk::ktx::load(graphicsQueue, "/Users/christian/Downloads/data/textures/cubemap_space.ktx");
        _skyboxPipeline->registerObject(_skyboxObject);
        _skyboxPipeline->registerTexture(&_skyboxTexture, 2);

        // Finalize pipeline and prepare for rendering
        _pipeline->prepare();
        _skyboxPipeline->prepare();

        uniformBufferObject.view = camera->getViewMatrix();
        uniformBufferObject.projection = glm::perspective(glm::radians(30.0f),
                                                          swapChainExtent.width / (float) swapChainExtent.height, 0.1f,
                                                          1000.0f);
        uniformBufferObject.projection[1][1] *= -1;
        uniformBufferObject.lightPosition = glm::vec3(0.0f, 0.0f, 5.0f);

        auto setMaterial = [](pvk::gltf::Object *object, pvk::gltf::Node *node, vk::DeviceMemory &memory) {
            struct {
                glm::vec4 baseColorFactor = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                float metallicFactor = 0.0;
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
        uniformBufferObject.lightPosition += glm::vec3(0, 0, 10.0f * this->deltaTime);

        _fox->updateUniformBuffer(0, sizeof(uniformBufferObject), &uniformBufferObject);
        _skyboxObject->updateUniformBuffer(0, sizeof(uniformBufferObject), &uniformBufferObject);

        const auto setUniformBufferObject = [](pvk::gltf::Object *object,
                                               pvk::gltf::Node *node,
                                               vk::DeviceMemory &memory) {
            node->bufferObject.model = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
            node->bufferObject.localMatrix = node->getGlobalMatrix();

            auto &inverseBindMatrices = object->inverseBindMatrices;

            if (inverseBindMatrices.empty()) {
                // No need to change the buffer object.
            } else {
                for (size_t i = 0; i < inverseBindMatrices.size(); i++) {
                    node->bufferObject.inverseBindMatrices[i] = inverseBindMatrices[i];
                }
            }

            if (node->skinIndex > -1) {
                node->bufferObject.jointCount = static_cast<float>(inverseBindMatrices.size());
            } else {
                node->bufferObject.jointCount = 0.0f;
            }

            pvk::buffer::update(memory, sizeof(node->bufferObject), &node->bufferObject);
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
