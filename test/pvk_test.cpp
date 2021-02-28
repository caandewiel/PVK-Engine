#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <ostream>
#include <sstream>
#include <vector>

#include "../lib/application/application.hpp"
#include "MockApplication.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-result"

namespace {
    std::unique_ptr<MockApplication> application;
}

class VulkanEnvironment : public ::testing::Environment {
public:
    explicit VulkanEnvironment() {
        application = std::make_unique<MockApplication>();
        application->prepare();
    }

    ~VulkanEnvironment() {
        application->destroy();
        application.release();
    }
};

TEST(PipelineParserTest, parseEmptyJson) {
    std::ostringstream filePathStream;
    filePathStream << std::filesystem::current_path().c_str() << "/../test/data/emptyPipeline.json";

    EXPECT_ANY_THROW(pvk::parseDescriptorSets(pvk::parseDefinition(filePathStream.str())).empty());
}

TEST(PipelineParserTest, parseJson) {
    std::ostringstream filePathStream;
    filePathStream << std::filesystem::current_path().c_str() << "/../test/data/test.json";

    auto descriptorSets = pvk::parseDescriptorSets(pvk::parseDefinition(filePathStream.str()));

    EXPECT_EQ(descriptorSets.size(), 1);

    auto &descriptorSet = descriptorSets[0];
    EXPECT_EQ(descriptorSet.get()->visibility, pvk::Pipeline::DescriptorSetVisibility::NODE);

    auto &bindings = descriptorSet->bindings;
    EXPECT_EQ(bindings.size(), 3);
    EXPECT_EQ(bindings[0]->shaderStageFlags, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
    EXPECT_EQ(bindings[1]->shaderStageFlags, vk::ShaderStageFlagBits::eVertex);
    EXPECT_EQ(bindings[2]->shaderStageFlags, vk::ShaderStageFlagBits::eFragment);
    EXPECT_EQ(bindings[0]->descriptorType, vk::DescriptorType::eUniformBuffer);
    EXPECT_EQ(bindings[1]->descriptorType, vk::DescriptorType::eUniformBuffer);
    EXPECT_EQ(bindings[2]->descriptorType, vk::DescriptorType::eCombinedImageSampler);
}

TEST(PipelineParserTest, parseFullPipeline) {
    std::ostringstream filePathStream;
    filePathStream << std::filesystem::current_path().c_str() << "/../test/data/test.json";

    auto pipeline = pvk::createPipelineFromDefinition(filePathStream.str(), application->getRenderPass(), application->getSwapChainExtent());
    application.get();
}

TEST(GLTFTest, parseCubeGLTF) {
    std::ostringstream filePathStream;
    filePathStream << std::filesystem::current_path().c_str() << "/../test/data/cube.glb";

    auto object = pvk::GLTFLoader::loadObject(application->getGraphicsQueue(), filePathStream.str());
    EXPECT_EQ(object->vertices.size(), 24);
    EXPECT_EQ(object->indices.size(), 36);
    EXPECT_EQ(object->getNodes().size(), 1);
    EXPECT_EQ(object->getNumberOfPrimitives(), 1);
}

TEST(GLTFTest, singleNodeGlobalMatrixIsEqualToLocalMatrix) {
    std::ostringstream filePathStream;
    filePathStream << std::filesystem::current_path().c_str() << "/../test/data/cube.glb";

    auto object = pvk::GLTFLoader::loadObject(application->getGraphicsQueue(), filePathStream.str());
    EXPECT_EQ(object->getNodeByIndex(0).getGlobalMatrix(), object->getNodeByIndex(0).getLocalMatrix());
}

TEST(GLTFTest, multipleNodeGlobalMatrixIsNotEqualToLocalMatrix) {
    std::ostringstream filePathStream;
    filePathStream << std::filesystem::current_path().c_str() << "/../test/data/joints.glb";

    auto object = pvk::GLTFLoader::loadObject(application->getGraphicsQueue(), filePathStream.str());
    EXPECT_TRUE(object->getNodes().size() > 1);
    const auto &rootNode = object->getNodeByIndex(0);
    const auto &childNode = object->getNodeByIndex(1);

    EXPECT_EQ(rootNode.getGlobalMatrix(), rootNode.getLocalMatrix());
    EXPECT_NE(childNode.getGlobalMatrix(), childNode.getLocalMatrix());
    EXPECT_EQ(rootNode.getGlobalMatrix(), childNode.getGlobalMatrix());
    EXPECT_EQ(rootNode.getLocalMatrix() * childNode.getLocalMatrix(), rootNode.getGlobalMatrix());
}

TEST(GLTFTest, parseRiggedFigure) {
    std::ostringstream filePathStream;
    filePathStream << std::filesystem::current_path().c_str() << "/../test/data/joints.glb";

    auto object = pvk::GLTFLoader::loadObject(application->getGraphicsQueue(), filePathStream.str());
    EXPECT_EQ(object->vertices.size(), 370);
    EXPECT_EQ(object->indices.size(), 768);
    EXPECT_EQ(object->getNodes().size(), 22);
    EXPECT_EQ(object->skinLookup.size(), 1);
    EXPECT_EQ(object->animations.size(), 1);

    auto &skin = object->skinLookup[0];
    EXPECT_EQ(skin->jointsIndices.size(), 19);
    EXPECT_EQ(skin->inverseBindMatrices.size(), 19);

    auto &animation = object->animations[0];
    EXPECT_EQ(animation->channels.size(), 57);
    EXPECT_EQ(animation->samplers.size(), 57);
    EXPECT_EQ(animation->startTime, 0.0F);
    EXPECT_EQ(animation->endTime, 1.25F);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new VulkanEnvironment);

    auto ret = RUN_ALL_TESTS();

    pvk::Context::tearDown();

    return ret;
}
#pragma clang diagnostic pop