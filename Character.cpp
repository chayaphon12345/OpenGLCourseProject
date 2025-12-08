#include "Character.h"
#include <iostream>

Character::Character(const std::string& modelPath, const std::string& idleAnimPath,
    const std::string& walkAnimPath, const std::string& attackAnimPath,
    const std::string& dyingAnimPath, glm::vec3 startPosition, int maxHp,
    int atk, int def, float m_BlendAmount, float m_BlendRate, float m_Yaw,
    float m_YawSpeed, float m_MoveSpeed, float ATTACK_HITBOX_WIDTH,
    float ATTACK_HITBOX_HEIGHT, float ATTACK_HITBOX_DEPTH, glm::vec3 ATTACK_HITBOX_OFFSET)
    : m_Model(modelPath), // Initialize Model
    // Initialize Animations (must happen after m_Model)
    m_IdleAnimation(idleAnimPath, &m_Model),
    m_WalkAnimation(walkAnimPath, &m_Model),
    m_AttackAnimation(attackAnimPath, &m_Model),
    m_DyingAnimation(dyingAnimPath, &m_Model),
    // Initialize Animator 
    m_Animator(&m_IdleAnimation),
    // Initialize all other member variables
    position(startPosition),
    isAlive(true),
    m_State(IDLE),
    m_Forward(0.0f, 0.0f, -1.0f),
    m_MoveSpeed(m_MoveSpeed),
    m_Yaw(m_Yaw),
    m_YawSpeed(m_YawSpeed),
    m_PlayDyingAnim(false),
	m_PlayAttackAnim(false),
	m_PlayWalkAnim(false),
    m_BlendAmount(m_BlendAmount),
    m_BlendRate(m_BlendRate),
    maxHp(maxHp),
    hp(maxHp),
    atk(atk),
    def(def),
    ATTACK_HITBOX_WIDTH(ATTACK_HITBOX_WIDTH),
    ATTACK_HITBOX_HEIGHT(ATTACK_HITBOX_HEIGHT),
    ATTACK_HITBOX_DEPTH(ATTACK_HITBOX_DEPTH),
    ATTACK_HITBOX_OFFSET(ATTACK_HITBOX_OFFSET),
    showAttackHitbox(false)
{

}

// --- Public Methods ---

void Character::Update(float deltaTime, GLFWwindow* window, glm::vec3& playerPosition)
{
    if (!isAlive && m_State != IDLE_DYING) return;

    UpdateAnimationState(window, playerPosition);
    UpdateAnimator(deltaTime);
}

void Character::Draw(Shader& shader)
{
    auto transforms = m_Animator.GetFinalBoneMatrices();
    for (int i = 0; i < transforms.size(); ++i)
        shader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
    shader.setMat4("model", GetModelMatrix());
    m_Model.Draw(shader);
}

void Character::TakeDamage(float damage)
{
    if (!isAlive) return;
    damage -= def;
    if (damage < 0) damage = 0;
    SetHP(hp - damage);
    std::cout << "Character receive damage! Remain HP: " << hp << std::endl;
    if (hp <= 0) {
        SetHP(0);
        std::cout << "Character has died!" << std::endl;
        m_PlayDyingAnim = true;
    }
}

glm::mat4 Character::GetModelMatrix()
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
    model = glm::rotate(model, glm::radians(0.0f + m_Yaw), glm::vec3(0.0f, 1.0f, 0.0f));
    return model;
}

bool Character::checkAttackBoxCollision(const glm::vec3& targetPos, float targetScale, float attackHitboxWidth, float attackHitboxHeight, float attackHitboxDepth,
    glm::vec3 attackHitboxOffset, const glm::vec3& attackerPos, float attackerYaw)
{
    float targetMinX = targetPos.x - targetScale;
    float targetMaxX = targetPos.x + targetScale;
    float targetMinY = targetPos.y - targetScale;
    float targetMaxY = targetPos.y + targetScale * 2.0f;
    float targetMinZ = targetPos.z - targetScale;
    float targetMaxZ = targetPos.z + targetScale;

    glm::vec3 corners[8] = {
        glm::vec3(-attackHitboxWidth / 2, -attackHitboxHeight / 2, -attackHitboxDepth / 2) + attackHitboxOffset,
        glm::vec3(attackHitboxWidth / 2, -attackHitboxHeight / 2, -attackHitboxDepth / 2) + attackHitboxOffset,
        glm::vec3(attackHitboxWidth / 2,  attackHitboxHeight / 2, -attackHitboxDepth / 2) + attackHitboxOffset,
        glm::vec3(-attackHitboxWidth / 2,  attackHitboxHeight / 2, -attackHitboxDepth / 2) + attackHitboxOffset,
        glm::vec3(-attackHitboxWidth / 2, -attackHitboxHeight / 2,  attackHitboxDepth / 2) + attackHitboxOffset,
        glm::vec3(attackHitboxWidth / 2, -attackHitboxHeight / 2,  attackHitboxDepth / 2) + attackHitboxOffset,
        glm::vec3(attackHitboxWidth / 2,  attackHitboxHeight / 2,  attackHitboxDepth / 2) + attackHitboxOffset,
        glm::vec3(-attackHitboxWidth / 2,  attackHitboxHeight / 2,  attackHitboxDepth / 2) + attackHitboxOffset
    };

    glm::mat4 attackModel = glm::mat4(1.0f);
    attackModel = glm::translate(attackModel, attackerPos);
    attackModel = glm::scale(attackModel, glm::vec3(.5f, .5f, .5f));
    attackModel = glm::rotate(attackModel, glm::radians(attackerYaw), glm::vec3(0.0f, 1.0f, 0.0f));

    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 worldCorner = attackModel * glm::vec4(corners[i], 1.0f);
        if (worldCorner.x >= targetMinX && worldCorner.x <= targetMaxX &&
            worldCorner.y >= targetMinY && worldCorner.y <= targetMaxY &&
            worldCorner.z >= targetMinZ && worldCorner.z <= targetMaxZ)
        {
            return true;
        }
    }

    return false;
}


// --- Private Methods ---
void Character::UpdateAnimator(float deltaTime)
{
    m_Animator.UpdateAnimation(deltaTime);
}

void Character::UpdateAnimationState(GLFWwindow* window, glm::vec3& playerPosition)
{
    switch (m_State) {
    case IDLE:
        if (((glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) || m_PlayWalkAnim) && !m_PlayDyingAnim) {
            m_BlendAmount = 0.0f;
            m_Animator.PlayAnimation(&m_IdleAnimation, &m_WalkAnimation, m_Animator.m_CurrentTime, 0.0f, m_BlendAmount);
            m_State = IDLE_WALK;
			m_PlayWalkAnim = false;
        }
        else if (((glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) || m_PlayAttackAnim) && !m_PlayDyingAnim) {
            m_BlendAmount = 0.0f;
            m_Animator.PlayAnimation(&m_IdleAnimation, &m_AttackAnimation, m_Animator.m_CurrentTime, 0.0f, m_BlendAmount);
            m_State = IDLE_ATTACK;
			m_PlayAttackAnim = false;
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
        showAttackHitbox = true;
        std::cout << "Monster Current attack time: " << m_Animator.m_CurrentTime << std::endl;
        if (m_Animator.m_CurrentTime > 0.7f) {
            m_BlendAmount += m_BlendRate;
            m_BlendAmount = fmod(m_BlendAmount, 1.0f);
            m_Animator.PlayAnimation(&m_AttackAnimation, &m_IdleAnimation, m_Animator.m_CurrentTime, m_Animator.m_CurrentTime2, m_BlendAmount);
            if (m_BlendAmount > 0.9f) {
                std::cout << "Only One" << std::endl;
                if (checkAttackBoxCollision(playerPosition, 0.5, ATTACK_HITBOX_WIDTH, ATTACK_HITBOX_HEIGHT, ATTACK_HITBOX_DEPTH,
                    ATTACK_HITBOX_OFFSET, position, m_Yaw)) {
					std::cout << "Player hit by monster!" << std::endl;
                    doAttack = atk;
                }
                m_BlendAmount = 0.0f;
                float startTime = m_Animator.m_CurrentTime2;
                m_Animator.PlayAnimation(&m_IdleAnimation, NULL, startTime, 0.0f, m_BlendAmount);
                m_State = IDLE;
                showAttackHitbox = false;
            }
        }
        else {
            // Attacking
        }
        break;
    case IDLE_DYING:
        m_BlendAmount += (m_BlendRate -0.035);
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