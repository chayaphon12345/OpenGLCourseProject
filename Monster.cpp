#include "Monster.h"
#include <glm/glm.hpp>

Monster::Monster(
    const std::string& modelPath,
    const std::string& idleAnimPath,
    const std::string& walkAnimPath,
    const std::string& attackAnimPath,
    const std::string& dyingAnimPath,
    glm::vec3 startPosition,
    int maxHp,
    int atk,
    int def,
    float m_BlendAmount,
    float m_BlendRate,
    float m_Yaw,
    float m_YawSpeed,
    float m_MoveSpeed,
    float ATTACK_HITBOX_WIDTH,
    float ATTACK_HITBOX_HEIGHT,
    float ATTACK_HITBOX_DEPTH,
    glm::vec3 ATTACK_HITBOX_OFFSET
) 
: Character(
    modelPath, idleAnimPath, walkAnimPath, attackAnimPath, dyingAnimPath,
    startPosition, maxHp, atk, def, 
    m_BlendAmount, m_BlendRate, m_Yaw, m_YawSpeed, m_MoveSpeed,
    ATTACK_HITBOX_WIDTH, ATTACK_HITBOX_HEIGHT, ATTACK_HITBOX_DEPTH,
    ATTACK_HITBOX_OFFSET
)
{
    detectPlayer = false;
    detectRange = 3.0f;
}


void Monster::CheckPlayerDistance(const glm::vec3& playerPos, float deltaTime, bool isPlayerAlive)
{
    if (!isPlayerAlive || m_PlayDyingAnim) {
        detectPlayer = false;
        m_PlayWalkAnim = false;
        m_PlayAttackAnim = false;
        return;
	}
    playerDistance = glm::distance(playerPos, this->position);

    if (playerDistance <= detectRange) {
        detectPlayer = true;
        if (playerDistance <= 0.5f) {
			m_PlayWalkAnim = false;
            m_PlayAttackAnim = true;
        } else {
            m_PlayAttackAnim = false;
			m_PlayWalkAnim = true;
			WalkTowardsPlayer(playerPos, deltaTime);
		}
    } else {
        detectPlayer = false;
    }
}

void Monster::FacePlayer(const glm::vec3& playerPos, float deltaTime, bool isPlayerAlive)
{
	if (!detectPlayer || !isPlayerAlive || m_PlayDyingAnim) return;
    glm::vec3 direction = glm::normalize(playerPos - this->position);
    float targetYaw = glm::degrees(atan2(direction.x, direction.z));
    float yawDiff = targetYaw - GetYaw();
    if (yawDiff > 180.0f) yawDiff -= 360.0f;
    if (yawDiff < -180.0f) yawDiff += 360.0f;
    if (abs(yawDiff) > 1.0f) {
        float yawChange = GetYawSpeed() * (yawDiff > 0 ? 1.0f : -1.0f) * deltaTime;
        if (abs(yawChange) > abs(yawDiff)) {
            SetYaw(targetYaw);
        } else {
            SetYaw(GetYaw() + yawChange);
        }
    }
}

void Monster::WalkTowardsPlayer(const glm::vec3& playerPos, float deltaTime)
{
    if (!detectPlayer || !m_PlayWalkAnim) return;
    glm::vec3 direction = glm::normalize(playerPos - this->position);
    this->position += direction * GetMoveSpeed() * deltaTime;
}