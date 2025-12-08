#ifndef ENEMY_H
#define ENEMY_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <learnopengl/model_animation.h>
#include <learnopengl/animator.h>
#include <learnopengl/shader_m.h>
#include <GLFW/glfw3.h>
#include <string>

#include "AnimState.h"

class Enemy {
public:
    // Public Attributes
    glm::vec3 position;
    float health;
    bool isAlive;

    // Constructor
    Enemy(const std::string& modelPath, const std::string& idleAnimPath,
          const std::string& walkAnimPath, const std::string& attackAnimPath,
          const std::string& dyingAnimPath, glm::vec3 startPosition);

    // Public Methods
    void Update(float deltaTime, GLFWwindow* window);
    void Draw(Shader& shader);
    void TakeDamage(float damage);
    
    // Getters for main game logic
    AnimState GetState() const { return m_State; }
    float GetAnimCurrentTime() const { return m_Animator.m_CurrentTime; }
    glm::mat4 GetModelMatrix();
    bool CheckAttackBoxCollision(const glm::vec3& playerPos, float playerScale);


private:
    // Model and Animations
    Model m_Model;
    Animation m_IdleAnimation;
    Animation m_WalkAnimation;
    Animation m_AttackAnimation;
    Animation m_DyingAnimation;
    Animator m_Animator;

    // State and Movement
    AnimState m_State;
    glm::vec3 m_Forward;
    float m_MoveSpeed;
    float m_Yaw;
    float m_YawSpeed;
    bool m_PlayDyingAnim; 
    float m_BlendAmount;
    float m_BlendRate;

    // Hitbox Constants
    const float ENEMY_HITBOX_WIDTH = 1.0f;
    const float ENEMY_HITBOX_HEIGHT = 1.5f;
    const float ENEMY_HITBOX_DEPTH = 1.0f;
    const glm::vec3 ENEMY_HITBOX_OFFSET = glm::vec3(0.0f, 1.0f, 1.0f);

    // Private Helper Methods
    void UpdateAnimationState(GLFWwindow* window);
    void UpdateAnimator(float deltaTime);
};

#endif // ENEMY_H