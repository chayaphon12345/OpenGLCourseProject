#ifndef MONSTER_H
#define MONSTER_H

#include "Character.h"

class Monster : public Character {
public:
    bool detectPlayer = false;
    float detectRange = 3.0f;
	float playerDistance = 0.0f;

    // Constructor 
    Monster(
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
    );

    // New function: Check player distance
    void CheckPlayerDistance(const glm::vec3& playerPos, float deltaTime, bool isPlayerAlive);
	void FacePlayer(const glm::vec3& playerPos, float deltaTime, bool isPlayerAlive);
	void WalkTowardsPlayer(const glm::vec3& playerPos, float deltaTime);
};

#endif