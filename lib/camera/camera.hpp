//
//  camera.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 18/01/2021.
//

#ifndef camera_hpp
#define camera_hpp

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace pvk {
    enum CameraMovement {FORWARD, BACKWARD, LEFT, RIGHT};
    
    // Default parameters
    const float YAW         = -90.0f;
    const float PITCH       =  0.0f;
    const float SPEED       =  5.0f;
    const float SENSITIVITY =  0.05f;
    const float ZOOM        =  45.0f;
    
    class Camera {
    public:
        Camera(glm::vec3 position, glm::vec3 up, float yaw = YAW, float pitch = PITCH);
        void update(CameraMovement direction, float deltaTime);
        void update(float xOffset, float yOffset, float deltaTime);
        glm::mat4 getViewMatrix() const;
        
        glm::vec3 position, front, up{}, right{}, worldUp;
        float yaw, pitch;
        float movementSpeed, sensitivity, zoom;
        
    private:
        void calculateCameraVectors();
    };
}


#endif /* camera_hpp */
