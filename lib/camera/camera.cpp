//
//  camera.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 18/01/2021.
//

#include "camera.hpp"

namespace pvk {
    Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : front(glm::vec3(0.0f, 0.0f, -1.0f)), movementSpeed(SPEED), sensitivity(SENSITIVITY), zoom(ZOOM) {
        this->position = position;
        this->worldUp = up;
        this->yaw = yaw;
        this->pitch = pitch;
        
        this->calculateCameraVectors();
    };
    
    void Camera::update(CameraMovement direction, float deltaTime) {
        float velocity = this->movementSpeed * deltaTime;
        
        if (direction == FORWARD)
            this->position += this->front * velocity;
        if (direction == BACKWARD)
            this->position -= this->front * velocity;
        if (direction == LEFT)
            this->position -= this->right * velocity;
        if (direction == RIGHT)
            this->position += this->right * velocity;
    }
    
    void Camera::update(float xOffset, float yOffset, float deltaTime) {
        xOffset *= this->sensitivity;
        yOffset *= this->sensitivity;
        
        this->yaw += xOffset;
        this->pitch += yOffset;
        
//        this->pitch = std::fmaxf(-89.0f, std::fminf(this->pitch, 89.0f));
        
        this->calculateCameraVectors();
    }
    
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(this->position, this->position + this->front, this->up);
    }
    
    void Camera::calculateCameraVectors() {
        glm::vec3 _front {
            cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
            sin(glm::radians(pitch)),
            sin(glm::radians(yaw)) * cos(glm::radians(pitch)),
        };
        
        this->front = glm::normalize(_front);
        this->right = glm::normalize(glm::cross(this->front, this->worldUp));
        this->up = glm::normalize(glm::cross(this->right, this->front));
    }
}

