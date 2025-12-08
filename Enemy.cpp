#include "Enemy.h"
#include <iostream>

// --- Constructor ---
Enemy::Enemy(const std::string& modelPath, const std::string& idleAnimPath,
             const std::string& walkAnimPath, const std::string& attackAnimPath,
             const std::string& dyingAnimPath, glm::vec3 startPosition)
    : m_Model(modelPath), // Initialize Model
      // Initialize Animations (must happen after m_Model)
      m_IdleAnimation(idleAnimPath, &m_Model),
      m_WalkAnimation(walkAnimPath, &m_Model),
      m_AttackAnimation(attackAnimPath, &m_Model),
      m_DyingAnimation(dyingAnimPath, &m_Model),
      // Initialize Animator (must happen after animations)
      m_Animator(&m_IdleAnimation),
      // Initialize all other member variables
      position(startPosition),
      health(100.0f),
      isAlive(true),
      m_State(IDLE),
      m_Forward(0.0f, 0.0f, -1.0f),
      m_MoveSpeed(2.0f),
      m_Yaw(0.0f),
      m_YawSpeed(100.0f),
      m_PlayDyingAnim(false),
      m_BlendAmount(0.0f),
      m_BlendRate(0.01f)
{

}

// --- Public Methods ---

void Enemy::Update(float deltaTime, GLFWwindow* window)
{
    if (!isAlive && m_State != IDLE_DYING) return;

    UpdateAnimationState(window);
    UpdateAnimator(deltaTime);
}

void Enemy::Draw(Shader& shader)
{
    auto transforms = m_Animator.GetFinalBoneMatrices();
    for (int i = 0; i < transforms.size(); ++i)
        shader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);

    shader.setMat4("model", GetModelMatrix());
    m_Model.Draw(shader);
}

void Enemy::TakeDamage(float damage)
{
    if (!isAlive) return;

    health -= damage;
    std::cout << "Hit an enemy! Enemy HP: " << health << std::endl;

    if (health <= 0.0f) {
        health = 0.0f;
        std::cout << "Enemy Defeated!" << std::endl;
        m_PlayDyingAnim = true;
    }
}

glm::mat4 Enemy::GetModelMatrix()
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
    model = glm::rotate(model, glm::radians(0.0f + m_Yaw), glm::vec3(0.0f, 1.0f, 0.0f));
    return model;
}

bool Enemy::CheckAttackBoxCollision(const glm::vec3& playerPos, float playerScale)
{
    float playerMinX = playerPos.x - playerScale;
    float playerMaxX = playerPos.x + playerScale;
    float playerMinY = playerPos.y - playerScale;
    float playerMaxY = playerPos.y + playerScale * 2.0f;
    float playerMinZ = playerPos.z - playerScale;
    float playerMaxZ = playerPos.z + playerScale;

    glm::vec3 corners[8] = {
        glm::vec3(-ENEMY_HITBOX_WIDTH / 2, -ENEMY_HITBOX_HEIGHT / 2, -ENEMY_HITBOX_DEPTH / 2) + ENEMY_HITBOX_OFFSET,
        glm::vec3(ENEMY_HITBOX_WIDTH / 2, -ENEMY_HITBOX_HEIGHT / 2, -ENEMY_HITBOX_DEPTH / 2) + ENEMY_HITBOX_OFFSET,
        glm::vec3(ENEMY_HITBOX_WIDTH / 2,  ENEMY_HITBOX_HEIGHT / 2, -ENEMY_HITBOX_DEPTH / 2) + ENEMY_HITBOX_OFFSET,
        glm::vec3(-ENEMY_HITBOX_WIDTH / 2,  ENEMY_HITBOX_HEIGHT / 2, -ENEMY_HITBOX_DEPTH / 2) + ENEMY_HITBOX_OFFSET,
        glm::vec3(-ENEMY_HITBOX_WIDTH / 2, -ENEMY_HITBOX_HEIGHT / 2,  ENEMY_HITBOX_DEPTH / 2) + ENEMY_HITBOX_OFFSET,
        glm::vec3(ENEMY_HITBOX_WIDTH / 2, -ENEMY_HITBOX_HEIGHT / 2,  ENEMY_HITBOX_DEPTH / 2) + ENEMY_HITBOX_OFFSET,
        glm::vec3(ENEMY_HITBOX_WIDTH / 2,  ENEMY_HITBOX_HEIGHT / 2,  ENEMY_HITBOX_DEPTH / 2) + ENEMY_HITBOX_OFFSET,
        glm::vec3(-ENEMY_HITBOX_WIDTH / 2,  ENEMY_HITBOX_HEIGHT / 2,  ENEMY_HITBOX_DEPTH / 2) + ENEMY_HITBOX_OFFSET
    };

    glm::mat4 enemyAttackModel = GetModelMatrix();

    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 worldCorner = enemyAttackModel * glm::vec4(corners[i], 1.0f);
        if (worldCorner.x >= playerMinX && worldCorner.x <= playerMaxX &&
            worldCorner.y >= playerMinY && worldCorner.y <= playerMaxY &&
            worldCorner.z >= playerMinZ && worldCorner.z <= playerMaxZ)
        {
            return true;
        }
    }

    return false;
}


// --- Private Methods ---

void Enemy::UpdateAnimator(float deltaTime)
{
    m_Animator.UpdateAnimation(deltaTime);
}

void Enemy::UpdateAnimationState(GLFWwindow* window)
{
    switch (m_State) {
    case IDLE:
        if ((glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)) {
            m_BlendAmount = 0.0f;
            m_Animator.PlayAnimation(&m_IdleAnimation, &m_WalkAnimation, m_Animator.m_CurrentTime, 0.0f, m_BlendAmount);
            m_State = IDLE_WALK;
        }
        else if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
            m_BlendAmount = 0.0f;
            m_Animator.PlayAnimation(&m_IdleAnimation, &m_AttackAnimation, m_Animator.m_CurrentTime, 0.0f, m_BlendAmount);
            m_State = IDLE_ATTACK;
        }
        else if ((glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) || m_PlayDyingAnim) {
            m_BlendAmount = 0.0f;
            m_Animator.PlayAnimation(&m_IdleAnimation, &m_DyingAnimation, m_Animator.m_CurrentTime, 0.0f, m_BlendAmount);
            m_State = IDLE_DYING;
            m_PlayDyingAnim = false;
        }
        break;
    case IDLE_WALK:
        m_BlendAmount += m_BlendRate;
        m_BlendAmount = fmod(m_BlendAmount, 1.0f);
        m_Animator.PlayAnimation(&m_IdleAnimation, &m_WalkAnimation, m_Animator.m_CurrentTime, m_Animator.m_CurrentTime2, m_BlendAmount);
        if (m_BlendAmount > 0.9f) {
            m_BlendAmount = 0.0f;
            float startTime = m_Animator.m_CurrentTime2;
            m_Animator.PlayAnimation(&m_WalkAnimation, NULL, startTime, 0.0f, m_BlendAmount);
            m_State = WALK;
        }
        break;
    case WALK:
        m_Animator.PlayAnimation(&m_WalkAnimation, NULL, m_Animator.m_CurrentTime, m_Animator.m_CurrentTime2, m_BlendAmount);
        if ((glfwGetKey(window, GLFW_KEY_I) != GLFW_PRESS)) {
            m_State = WALK_IDLE;
        }
        break;
    case WALK_IDLE:
        m_BlendAmount += m_BlendRate;
        m_BlendAmount = fmod(m_BlendAmount, 1.0f);
        m_Animator.PlayAnimation(&m_WalkAnimation, &m_IdleAnimation, m_Animator.m_CurrentTime, m_Animator.m_CurrentTime2, m_BlendAmount);
        if (m_BlendAmount > 0.9f) {
            m_BlendAmount = 0.0f;
            float startTime = m_Animator.m_CurrentTime2;
            m_Animator.PlayAnimation(&m_IdleAnimation, NULL, startTime, 0.0f, m_BlendAmount);
            m_State = IDLE;
        }
        break;
    case IDLE_ATTACK:
        m_BlendAmount += m_BlendRate;
        m_BlendAmount = fmod(m_BlendAmount, 1.0f);
        m_Animator.PlayAnimation(&m_IdleAnimation, &m_AttackAnimation, m_Animator.m_CurrentTime, m_Animator.m_CurrentTime2, m_BlendAmount);
        if (m_BlendAmount > 0.9f) {
            m_BlendAmount = 0.0f;
            float startTime = m_Animator.m_CurrentTime2;
            m_Animator.PlayAnimation(&m_AttackAnimation, NULL, startTime, 0.0f, m_BlendAmount);
            m_State = ATTACK_IDLE;
        }
        break;
    case ATTACK_IDLE:
        if (m_Animator.m_CurrentTime > 0.7f) {
            m_BlendAmount += m_BlendRate;
            m_BlendAmount = fmod(m_BlendAmount, 1.0f);
            m_Animator.PlayAnimation(&m_AttackAnimation, &m_IdleAnimation, m_Animator.m_CurrentTime, m_Animator.m_CurrentTime2, m_BlendAmount);
            if (m_BlendAmount > 0.9f) {
                m_BlendAmount = 0.0f;
                float startTime = m_Animator.m_CurrentTime2;
                m_Animator.PlayAnimation(&m_IdleAnimation, NULL, startTime, 0.0f, m_BlendAmount);
                m_State = IDLE;
            }
        }
        else {
            // Attacking
        }
        break;
    case IDLE_DYING:
        m_BlendAmount += m_BlendRate;
        m_BlendAmount = fmod(m_BlendAmount, 1.0f);
        m_Animator.PlayAnimation(&m_IdleAnimation, &m_DyingAnimation, m_Animator.m_CurrentTime, m_Animator.m_CurrentTime2, m_BlendAmount);
        if (m_BlendAmount > 0.9f) {
            m_BlendAmount = 0.0f;
            float startTime = m_Animator.m_CurrentTime2;
            m_Animator.PlayAnimation(&m_DyingAnimation, NULL, startTime, 0.0f, m_BlendAmount);
            isAlive = false;
        }
        break;
    }
}