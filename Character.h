#ifndef CHARACTER_H
#define CHARACTER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <learnopengl/model_animation.h>
#include <learnopengl/animator.h>
#include <learnopengl/shader_m.h>
#include <GLFW/glfw3.h>
#include <string>

#include "AnimState.h"

class Character {
public:
	// Public Attributes
	glm::vec3 position;
	bool isAlive;
	bool m_PlayDyingAnim;
	bool m_PlayWalkAnim;
	bool m_PlayAttackAnim;
	bool showAttackHitbox;
	int doAttack = 0;

	// Constructor
	Character(const std::string& modelPath, const std::string& idleAnimPath,
		const std::string& walkAnimPath, const std::string& attackAnimPath,
		const std::string& dyingAnimPath, glm::vec3 startPosition, int maxHp,
		int atk, int def, float m_BlendAmount, float m_BlendRate, float m_Yaw,
		float m_YawSpeed, float m_MoveSpeed, float ATTACK_HITBOX_WIDTH, 
		float ATTACK_HITBOX_HEIGHT, float ATTACK_HITBOX_DEPTH, glm::vec3 ATTACK_HITBOX_OFFSET);

	// Public Methods
	void Update(float deltaTime, GLFWwindow* window, glm::vec3& playerPos);
	void Draw(Shader& shader);
	void TakeDamage(float damage);

	// Getters for main game logic
	AnimState GetState() const { return m_State; }
	float GetAnimCurrentTime() const { return m_Animator.m_CurrentTime; }
	glm::mat4 GetModelMatrix();
	bool checkAttackBoxCollision(const glm::vec3& targetPos, float targetScale, float attackHitboxWidth, float attackHitboxHeight, float attackHitboxDepth,
		glm::vec3 attackHitboxOffset, const glm::vec3& attackerPos, float attackerYaw);
	int GetMaxHP() const { return maxHp; }
	void SetMaxHP(int newMaxHp) {
		if (newMaxHp < 1) newMaxHp = 1;
		maxHp = newMaxHp;
		if (hp > maxHp) hp = maxHp;
	}
	int GetHP() const { return hp; }
	void SetHP(int newHp) {
		if (newHp > maxHp) newHp = maxHp;
		if (newHp < 0) newHp = 0;
		hp = newHp; 
	}
	int GetATK() const { return atk; }
	void SetATK(int newAtk) {
		if (newAtk < 0) newAtk = 0;
		atk = newAtk;
	}
	int GetDEF() const { return def; }
	void SetDEF(int newDef) {
		if (newDef < 0) newDef = 0;
		def = newDef;
	}
	float GetYaw() const { return m_Yaw; }
	void SetYaw(float newYaw) { m_Yaw = newYaw; }
	float GetYawSpeed() const { return m_YawSpeed; }
	float GetMoveSpeed() const { return m_MoveSpeed; }
private:
	int maxHp;
	int hp;
	int atk;
	int def;

	// Model & Animations
	Model m_Model;
	Animation m_IdleAnimation;
	Animation m_WalkAnimation;
	Animation m_AttackAnimation;
	Animation m_DyingAnimation;

	Animator m_Animator;
	AnimState m_State;

	float m_BlendAmount;
	float m_BlendRate;

	// Position & Movement
	glm::vec3 m_Forward;
	float m_Yaw;
	float m_YawSpeed;
	float m_MoveSpeed;
	
	// Hitbox
	float ATTACK_HITBOX_WIDTH = 1.0f;
	float ATTACK_HITBOX_HEIGHT = 1.5f;
	float ATTACK_HITBOX_DEPTH = 1.0f;
	glm::vec3 ATTACK_HITBOX_OFFSET = glm::vec3(0.0f, 1.0f, 1.0f);
	
	// Private Methods
	void UpdateAnimator(float deltaTime);
	void UpdateAnimationState(GLFWwindow* window, glm::vec3& playerPosition);
};

#endif