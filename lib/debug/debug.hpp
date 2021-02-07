//
//  validation.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 13/01/2021.
//

#ifndef debug_h
#define debug_h

#include <vulkan/vulkan.hpp>

PFN_vkCreateDebugUtilsMessengerEXT  pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
    VkAllocationCallbacks const* pAllocator) {

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugMessageFunc( VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
                                                VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
                                                VkDebugUtilsMessengerCallbackDataEXT const * pCallbackData,
                                                void * /*pUserData*/ )
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
    
#ifdef _WIN32
    MessageBox( NULL, message.c_str(), "Alert", MB_OK );
#else
    std::cout << message << std::endl;
#endif
    
    return false;
}

bool checkLayers( std::vector<char const *> const & layers, std::vector<vk::LayerProperties> const & properties )
{
    // return true if all layers are listed in the properties
    return std::all_of( layers.begin(), layers.end(), [&properties]( char const * name ) {
        return std::find_if( properties.begin(), properties.end(), [&name]( vk::LayerProperties const & property ) {
            return strcmp( property.layerName, name ) == 0;
        } ) != properties.end();
    } );
}

vk::DebugUtilsMessengerCreateInfoEXT getDebugUtilsMessengerCreateInfo() {
    return vk::DebugUtilsMessengerCreateInfoEXT(
        {},
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        &debugMessageFunc
    );
}

namespace pvk {
    namespace debug {
        void initialize(vk::UniqueInstance &instance) {
            auto procAddrCreate                 = instance->getProcAddr("vkCreateDebugUtilsMessengerEXT");
            auto procAddrDestroy                = instance->getProcAddr("vkDestroyDebugUtilsMessengerEXT");
            pfnVkCreateDebugUtilsMessengerEXT   = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(procAddrCreate);
            pfnVkDestroyDebugUtilsMessengerEXT  = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(procAddrDestroy);
            
            if (!pfnVkCreateDebugUtilsMessengerEXT) {
                std::cout << "GetInstanceProcAddr: Unable to find pfnVkCreateDebugUtilsMessengerEXT function." << std::endl;
                exit(1);
            }
            if (!pfnVkDestroyDebugUtilsMessengerEXT) {
                std::cout << "GetInstanceProcAddr: Unable to find pfnVkDestroyDebugUtilsMessengerEXT function." << std::endl;
                exit(1);
            }
            
            vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                                                                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
            vk::DebugUtilsMessageTypeFlagsEXT     messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                                                                   | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
                                                                   | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
            
            
            auto debugUtilsMessengerEXT                             = vk::DebugUtilsMessengerCreateInfoEXT({}, severityFlags, messageTypeFlags, &debugMessageFunc);
            vk::UniqueDebugUtilsMessengerEXT debugUtilsMessenger    = instance->createDebugUtilsMessengerEXTUnique(debugUtilsMessengerEXT);
        }
    }
}

#endif /* validation_h */
