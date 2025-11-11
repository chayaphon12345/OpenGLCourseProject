#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/animator.h>
#include <learnopengl/model_animation.h>



#include <iostream>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadCubemap(vector<std::string> faces);
void updatePlayerPosition(int direction);
void setupHitbox();
bool checkAABBCollision(const glm::mat4& playerAttackModel, const glm::vec3& enemyPos, float enemyScale);
bool checkEnemyAttackCollision(const glm::mat4& enemyAttackModel, const glm::vec3& playerPos, float playerScale);
void damageEnemy(float damage);
void damagePlayer(float damage);

enum AnimState {
	IDLE = 1,
	IDLE_ATTACK,
	ATTACK_IDLE,
	IDLE_KICK,
	KICK_IDLE,
	IDLE_WALK,
	WALK_IDLE,
	WALK,
	IDLE_WALKBACK,
	WALKBACK_IDLE,
	WALKBACK,
	IDLE_TURN,
	TURN_IDLE,
	IDLE_DYING,
	IDLE_TALK,
	TALK,
	TALK_IDLE
};

// settings
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 800;

// camera
Camera camera(glm::vec3(0.0f, 4.0f, 4.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// player
glm::vec3 playerPosition = glm::vec3(0.4f, 1.1f, -0.4f);
glm::vec3 playerForward = glm::vec3(0.0f, 0.0f, -1.0f);
float moveSpeed = 1.2f;
float playerYaw = 0.0f;
float yawSpeed = 150.0f;
enum AnimState charState = IDLE;
float playerHealth = 100.0f;
bool isPlayerAlive = true;
bool playDyingAnim = false;

// enemy
glm::vec3 enemyPosition = glm::vec3(0.0f, 1.1f, -14.0f);
glm::vec3 enemyForward = glm::vec3(0.0f, 0.0f, -1.0f);
float enemyMoveSpeed = 2.0f;
float enemyYaw = 0.0f;
float enemyYawSpeed = 100.0f;
enum AnimState enemyState = IDLE;
float enemyHealth = 100.0f;
bool isEnemyAlive = true;
bool playEnemyDyingAnim = false;

// merchant
glm::vec3 merchantPosition = glm::vec3(-1.9f, 1.1f, -3.1f);
float merchantYaw = 0.0f;
float merchantYawSpeed = 100.0f;
enum AnimState merchantState = IDLE;
bool isTalkingToMerchant = false;

// attack hitbox
const float HITBOX_WIDTH = 1.0f;
const float HITBOX_HEIGHT = 1.5f;
const float HITBOX_DEPTH = 1.0f;
const glm::vec3 HITBOX_OFFSET = glm::vec3(0.0f, 1.0f, 1.0f);
unsigned int hitboxVAO = 0;
unsigned int hitboxVBO = 0;

// enemy attack hitbox
const float ENEMY_HITBOX_WIDTH = 1.0f;
const float ENEMY_HITBOX_HEIGHT = 1.5f;
const float ENEMY_HITBOX_DEPTH = 1.0f;
const glm::vec3 ENEMY_HITBOX_OFFSET = glm::vec3(0.0f, 1.0f, 1.0f); // Offset is forward relative to enemy forward

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
	stbi_set_flip_vertically_on_load(true);

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile shaders
	// -------------------------
	Shader ourShader("anim_model.vs", "anim_model.fs");
	Shader mapShader("map.vs", "map.fs");
	Shader skyboxShader("6.1.skybox.vs", "6.1.skybox.fs");
	Shader hitboxShader("hitbox.vs", "hitbox.fs");


	// load models
	// -----------
	// idle 3.3, walk 2.06, run 0.83, attack 1.03, kick 1.6
	Model ourModel(FileSystem::getPath("resources/objects/mixamo/knight/model/model.dae"));
	Model mapModel(FileSystem::getPath("resources/objects/map/dungeon/source/DungeonBlend/DungeonBlend/dungeon_v11.obj"));
	Model enemyModel(FileSystem::getPath("resources/objects/mixamo/monster/model/model.dae"));
	Model merchantModel(FileSystem::getPath("resources/objects/mixamo/merchant/Model/Model.dae"));
	Animation idleAnimation(FileSystem::getPath("resources/objects/mixamo/knight/Idle/Idle.dae"), &ourModel);
	Animation walkAnimation(FileSystem::getPath("resources/objects/mixamo/knight/Walking/Walking.dae"), &ourModel);
	Animation walkBackAnimation(FileSystem::getPath("resources/objects/mixamo/knight/WalkBack/WalkBack.dae"), &ourModel);
	Animation runAnimation(FileSystem::getPath("resources/objects/mixamo/knight/Running/Running.dae"), &ourModel);
	Animation attackAnimation(FileSystem::getPath("resources/objects/mixamo/knight/Slash/Slash.dae"), &ourModel);
	Animation kickAnimation(FileSystem::getPath("resources/objects/mixamo/knight/SwordKick/SwordKick.dae"), &ourModel);
	Animation turnAnimation(FileSystem::getPath("resources/objects/mixamo/knight/Turn/Turn.dae"), &ourModel);
	Animation dyingAnimation(FileSystem::getPath("resources/objects/mixamo/knight/Death/Death.dae"), &ourModel);
	Animation enemyIdleAnimation(FileSystem::getPath("resources/objects/mixamo/monster/Idle/Idle.dae"), &enemyModel);
	Animation enemyWalkAnimation(FileSystem::getPath("resources/objects/mixamo/monster/Walk/Walk.dae"), &enemyModel);
	Animation enemyAttackAnimation(FileSystem::getPath("resources/objects/mixamo/monster/Attack/Attack.dae"), &enemyModel);
	Animation enemyDyingAnimation(FileSystem::getPath("resources/objects/mixamo/monster/Dying/Dying.dae"), &enemyModel);
	Animation merchantIdleAnimation(FileSystem::getPath("resources/objects/mixamo/merchant/Idle/Idle.dae"), &merchantModel);
	Animation merchantTalkAnimation(FileSystem::getPath("resources/objects/mixamo/merchant/Talking/Talking.dae"), &merchantModel);
	Animator animator(&idleAnimation);
	Animator enemyAnimator(&enemyIdleAnimation);
	Animator merchantAnimator(&merchantIdleAnimation);
	float blendAmount = 0.0f;
	float blendRate = 0.055f;
	float enemyBlendAmount = 0.0f;
	float enemyBlendRate = 0.005f;
	float merchantBlendAmount = 0.0f;
	float merchantBlendRate = 0.03f;

	setupHitbox();

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);
		if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
			animator.PlayAnimation(&idleAnimation, NULL, 0.0f, 0.0f, 0.0f);
		if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
			animator.PlayAnimation(&walkAnimation, NULL, 0.0f, 0.0f, 0.0f);
		if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
			animator.PlayAnimation(&attackAnimation, NULL, 0.0f, 0.0f, 0.0f);
		if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
			animator.PlayAnimation(&kickAnimation, NULL, 0.0f, 0.0f, 0.0f);
		if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
			animator.PlayAnimation(&turnAnimation, NULL, 0.0f, 0.0f, 0.0f);
		/*if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
			isTalkingToMerchant = !isTalkingToMerchant;*/


		switch (charState) {
		case IDLE:
			if ((glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)) {
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &walkAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_WALK;
			}
			else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &walkBackAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_WALKBACK;
			}
			else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &attackAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_ATTACK;
			}
			else if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &kickAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_KICK;
			}
			else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &turnAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_TURN;
			}
			else if ((glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) || playDyingAnim) {
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &dyingAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_DYING;
			}
			//printf("idle \n");
			break;
		case IDLE_WALK:
			blendAmount += blendRate;
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&idleAnimation, &walkAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&walkAnimation, NULL, startTime, 0.0f, blendAmount);
				charState = WALK;
			}
			//printf("idle_walk \n");
			break;
		case WALK:
			animator.PlayAnimation(&walkAnimation, NULL, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if ((glfwGetKey(window, GLFW_KEY_W) != GLFW_PRESS) && (glfwGetKey(window, GLFW_KEY_A) != GLFW_PRESS) && (glfwGetKey(window, GLFW_KEY_D) != GLFW_PRESS)) {
				charState = WALK_IDLE;
			}
			//printf("walking\n");
			break;
		case WALK_IDLE:
			blendAmount += blendRate;
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&walkAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
				charState = IDLE;
			}
			//printf("walk_idle \n");
			break;
		case IDLE_WALKBACK:
			blendAmount += blendRate;
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&idleAnimation, &walkBackAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&walkBackAnimation, NULL, startTime, 0.0f, blendAmount);
				charState = WALKBACK;
			}
			//printf("idle_walkback \n");
			break;
		case WALKBACK:
			animator.PlayAnimation(&walkBackAnimation, NULL, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if ((glfwGetKey(window, GLFW_KEY_S) != GLFW_PRESS)) {
				charState = WALKBACK_IDLE;
			}
			//printf("walking_back\n");
			break;
		case WALKBACK_IDLE:
			blendAmount += blendRate;
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&walkBackAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
				charState = IDLE;
			}
			//printf("walkback_idle \n");
			break;
		case IDLE_TURN:
			blendAmount += blendRate;
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&idleAnimation, &turnAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&turnAnimation, NULL, startTime, 0.0f, blendAmount);
				charState = TURN_IDLE;
			}
			//printf("idle_turn\n");
			break;
		case TURN_IDLE:
			if (animator.m_CurrentTime > 0.7f) {
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&turnAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					//playerYaw -= 90.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = IDLE;
				}
				//printf("turn_idle \n");
			}
			else {
				// turning
				//printf("turning \n");
			}
			break;
		case IDLE_ATTACK:
			blendAmount += blendRate;
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&idleAnimation, &attackAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&attackAnimation, NULL, startTime, 0.0f, blendAmount);
				charState = ATTACK_IDLE;
			}
			//printf("idle_attack\n");
			break;
		case ATTACK_IDLE:
			if (animator.m_CurrentTime > 0.7f) {
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&attackAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = IDLE;
				}
				//printf("attack_idle \n");
			}
			else {
				// attacking
				//printf("attacking \n");
			}
			break;
		case IDLE_KICK:
			blendAmount += blendRate;
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&idleAnimation, &kickAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&kickAnimation, NULL, startTime, 0.0f, blendAmount);
				charState = KICK_IDLE;
			}
			//printf("idle_kick\n");
			break;
		case KICK_IDLE:
			if (animator.m_CurrentTime > 1.0f) {
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&kickAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = IDLE;
				}
				//printf("kick_idle \n");
			}
			else {
				// kicking
				//printf("kicking \n");
			}
			break;
		case IDLE_DYING:
			blendAmount += (blendRate - 0.052);
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&idleAnimation, &dyingAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&dyingAnimation, NULL, startTime, 0.0f, blendAmount);
				isPlayerAlive = false;
			}
			break;
		}

		animator.UpdateAnimation(deltaTime);


		glm::mat4 playerAttackModel = glm::mat4(1.0f);
		playerAttackModel = glm::translate(playerAttackModel, playerPosition);
		playerAttackModel = glm::scale(playerAttackModel, glm::vec3(.5f, .5f, .5f));
		playerAttackModel = glm::rotate(playerAttackModel, glm::radians(180.0f + playerYaw), glm::vec3(0.0f, 1.0f, 0.0f));

		bool showHitbox = false;
		float enemyScale = 0.5f;

		static bool attackHitPerformed = false;
		static bool kickHitPerformed = false;

		if (charState == ATTACK_IDLE && animator.m_CurrentTime > 0.3f && animator.m_CurrentTime < 0.6f) {
			showHitbox = true;
			if (!attackHitPerformed) {
				if (isEnemyAlive && checkAABBCollision(playerAttackModel, enemyPosition, enemyScale)) {
					damageEnemy(40.0f);
					attackHitPerformed = true;
				}
			}
		}
		if (charState == ATTACK_IDLE && animator.m_CurrentTime > 0.7f) {
			attackHitPerformed = false;
		}

		if (charState == KICK_IDLE && animator.m_CurrentTime > 0.3f && animator.m_CurrentTime < 0.6f) {
			showHitbox = true;
			if (!kickHitPerformed) {
				if (isEnemyAlive && checkAABBCollision(playerAttackModel, enemyPosition, enemyScale)) {
					damageEnemy(20.0f);
					kickHitPerformed = true;
				}
			}
		}
		if (charState == KICK_IDLE && animator.m_CurrentTime > 0.7f) {
			kickHitPerformed = false;
		}


		switch (enemyState) {
		case IDLE:
			if ((glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)) {
				enemyBlendAmount = 0.0f;
				enemyAnimator.PlayAnimation(&enemyIdleAnimation, &enemyWalkAnimation, enemyAnimator.m_CurrentTime, 0.0f, enemyBlendAmount);
				enemyState = IDLE_WALK;
			}
			else if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
				enemyBlendAmount = 0.0f;
				enemyAnimator.PlayAnimation(&enemyIdleAnimation, &enemyAttackAnimation, enemyAnimator.m_CurrentTime, 0.0f, enemyBlendAmount);
				enemyState = IDLE_ATTACK;
			}
			else if ((glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) || playEnemyDyingAnim) {
				enemyBlendAmount = 0.0f;
				enemyAnimator.PlayAnimation(&enemyIdleAnimation, &enemyDyingAnimation, enemyAnimator.m_CurrentTime, 0.0f, enemyBlendAmount);
				enemyState = IDLE_DYING;
			}
			break;
		case IDLE_WALK:
			enemyBlendAmount += enemyBlendRate;
			enemyBlendAmount = fmod(enemyBlendAmount, 1.0f);
			enemyAnimator.PlayAnimation(&enemyIdleAnimation, &enemyWalkAnimation, enemyAnimator.m_CurrentTime, enemyAnimator.m_CurrentTime2, enemyBlendAmount);
			if (enemyBlendAmount > 0.9f) {
				enemyBlendAmount = 0.0f;
				float startTime = enemyAnimator.m_CurrentTime2;
				enemyAnimator.PlayAnimation(&enemyWalkAnimation, NULL, startTime, 0.0f, enemyBlendAmount);
				enemyState = WALK;
			}
			break;
		case WALK:
			enemyAnimator.PlayAnimation(&enemyWalkAnimation, NULL, enemyAnimator.m_CurrentTime, enemyAnimator.m_CurrentTime2, enemyBlendAmount);
			if ((glfwGetKey(window, GLFW_KEY_I) != GLFW_PRESS)) {
				enemyState = WALK_IDLE;
			}
			break;
		case WALK_IDLE:
			enemyBlendAmount += enemyBlendRate;
			enemyBlendAmount = fmod(enemyBlendAmount, 1.0f);
			enemyAnimator.PlayAnimation(&enemyWalkAnimation, &enemyIdleAnimation, enemyAnimator.m_CurrentTime, enemyAnimator.m_CurrentTime2, enemyBlendAmount);
			if (enemyBlendAmount > 0.9f) {
				enemyBlendAmount = 0.0f;
				float startTime = enemyAnimator.m_CurrentTime2;
				enemyAnimator.PlayAnimation(&enemyIdleAnimation, NULL, startTime, 0.0f, enemyBlendAmount);
				enemyState = IDLE;
			}
			break;
		case IDLE_ATTACK:
			enemyBlendAmount += enemyBlendRate;
			enemyBlendAmount = fmod(enemyBlendAmount, 1.0f);
			enemyAnimator.PlayAnimation(&enemyIdleAnimation, &enemyAttackAnimation, enemyAnimator.m_CurrentTime, enemyAnimator.m_CurrentTime2, enemyBlendAmount);
			if (enemyBlendAmount > 0.9f) {
				enemyBlendAmount = 0.0f;
				float startTime = enemyAnimator.m_CurrentTime2;
				enemyAnimator.PlayAnimation(&enemyAttackAnimation, NULL, startTime, 0.0f, enemyBlendAmount);
				enemyState = ATTACK_IDLE;
			}
			break;
		case ATTACK_IDLE:
			if (enemyAnimator.m_CurrentTime > 0.7f) {
				enemyBlendAmount += enemyBlendRate;
				enemyBlendAmount = fmod(enemyBlendAmount, 1.0f);
				enemyAnimator.PlayAnimation(&enemyAttackAnimation, &enemyIdleAnimation, enemyAnimator.m_CurrentTime, enemyAnimator.m_CurrentTime2, enemyBlendAmount);
				if (enemyBlendAmount > 0.9f) {
					enemyBlendAmount = 0.0f;
					float startTime = enemyAnimator.m_CurrentTime2;
					enemyAnimator.PlayAnimation(&enemyIdleAnimation, NULL, startTime, 0.0f, enemyBlendAmount);
					enemyState = IDLE;
				}
				printf("attack_idle \n");
			}
			else {
			}
			break;
		case IDLE_DYING:
			enemyBlendAmount += enemyBlendRate;
			enemyBlendAmount = fmod(enemyBlendAmount, 1.0f);
			enemyAnimator.PlayAnimation(&enemyIdleAnimation, &enemyDyingAnimation, enemyAnimator.m_CurrentTime, enemyAnimator.m_CurrentTime2, enemyBlendAmount);
			if (enemyBlendAmount > 0.9f) {
				enemyBlendAmount = 0.0f;
				float startTime = enemyAnimator.m_CurrentTime2;
				enemyAnimator.PlayAnimation(&enemyDyingAnimation, NULL, startTime, 0.0f, enemyBlendAmount);
				isEnemyAlive = false;
			}
			break;
		}

		enemyAnimator.UpdateAnimation(deltaTime);

		glm::mat4 enemyAttackModel = glm::mat4(1.0f);
		enemyAttackModel = glm::translate(enemyAttackModel, enemyPosition);
		enemyAttackModel = glm::scale(enemyAttackModel, glm::vec3(.5f, .5f, .5f));
		// Enemy rotation: enemyYaw is 0.0f, the model is loaded "forward" so no extra 180 deg
		enemyAttackModel = glm::rotate(enemyAttackModel, glm::radians(0.0f + enemyYaw), glm::vec3(0.0f, 1.0f, 0.0f));

		bool showEnemyHitbox = false;
		float playerScale = 0.5f; // Player's scale
		static bool enemyAttackHitPerformed = false;

		// Check collision when enemy is in the attack phase of the ATTACK_IDLE state
		if (enemyState == ATTACK_IDLE && enemyAnimator.m_CurrentTime > 0.3f && enemyAnimator.m_CurrentTime < 0.6f) {
			showEnemyHitbox = true;
			if (!enemyAttackHitPerformed) {
				if (isPlayerAlive && checkEnemyAttackCollision(enemyAttackModel, playerPosition, playerScale)) {
					damagePlayer(150.0f); // Adjust damage as needed
					enemyAttackHitPerformed = true;
				}
			}
		}
		// Reset the hit flag after the attack animation window
		if (enemyState == ATTACK_IDLE && enemyAnimator.m_CurrentTime > 0.7f) {
			enemyAttackHitPerformed = false;
		}


		switch (merchantState) {
		case IDLE:
			if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
				merchantBlendAmount = 0.0f;
				merchantAnimator.PlayAnimation(&merchantIdleAnimation, &merchantTalkAnimation, merchantAnimator.m_CurrentTime, 0.0f, merchantBlendAmount);
				merchantState = IDLE_TALK;
				isTalkingToMerchant = true;
			}
			break;
		case IDLE_TALK:
			merchantBlendAmount += merchantBlendRate;
			merchantBlendAmount = fmod(merchantBlendAmount, 1.0f);
			merchantAnimator.PlayAnimation(&merchantIdleAnimation, &merchantTalkAnimation, merchantAnimator.m_CurrentTime, merchantAnimator.m_CurrentTime2, merchantBlendAmount);
			if (merchantBlendAmount > 0.9f) {
				merchantBlendAmount = 0.0f;
				float startTime = merchantAnimator.m_CurrentTime2;
				merchantAnimator.PlayAnimation(&merchantTalkAnimation, NULL, startTime, 0.0f, merchantBlendAmount);
				merchantState = TALK;
			}
			break;
		case TALK:
			merchantAnimator.PlayAnimation(&merchantTalkAnimation, NULL, merchantAnimator.m_CurrentTime, merchantAnimator.m_CurrentTime2, merchantBlendAmount);
			if ((glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) || !isTalkingToMerchant) {
				merchantState = TALK_IDLE;
				isTalkingToMerchant = false;
			}
			break;
		case TALK_IDLE:
			merchantBlendAmount += merchantBlendRate;
			merchantBlendAmount = fmod(merchantBlendAmount, 1.0f);
			merchantAnimator.PlayAnimation(&merchantTalkAnimation, &merchantIdleAnimation, merchantAnimator.m_CurrentTime, merchantAnimator.m_CurrentTime2, merchantBlendAmount);
			if (merchantBlendAmount > 0.9f) {
				merchantBlendAmount = 0.0f;
				float startTime = merchantAnimator.m_CurrentTime2;
				merchantAnimator.PlayAnimation(&merchantIdleAnimation, NULL, startTime, 0.0f, merchantBlendAmount);
				merchantState = IDLE;
			}
			break;
		}

		merchantAnimator.UpdateAnimation(deltaTime);



		// render
		// ------
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 playerRotationMatrix = glm::mat4(1.0f);
		playerRotationMatrix = glm::rotate(playerRotationMatrix, glm::radians(playerYaw), glm::vec3(0, 1, 0));

		playerForward = glm::normalize(glm::vec3(playerRotationMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

		// don't forget to enable shader before setting uniforms
		ourShader.use();

		// view/projection transformations
		float aspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;
		float orthoScale = 4.0f;
		glm::mat4 projection = glm::ortho(-orthoScale * aspect, orthoScale * aspect, -orthoScale, orthoScale, -50.0f, 50.0f);
		glm::vec3 camTarget = playerPosition;
		glm::vec3 camPos = camTarget + glm::vec3(5.0f, 5.0f, 5.0f);
		glm::mat4 view = glm::lookAt(camPos, camTarget, glm::vec3(0.0f, 1.0f, 0.0f));


		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", view);

		// render the loaded model
		glm::mat4 model = glm::mat4(1.0f);

		// Draw the player
		if (isPlayerAlive) {
			auto transforms = animator.GetFinalBoneMatrices();
			for (int i = 0; i < transforms.size(); ++i)
				ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);

			model = glm::translate(model, playerPosition);
			model = glm::scale(model, glm::vec3(.5f, .5f, .5f));
			model = glm::rotate(model, glm::radians(180.0f + playerYaw), glm::vec3(0.0f, 1.0f, 0.0f));
			ourShader.setMat4("model", model);
			ourModel.Draw(ourShader);
		}

		//ourShader.use();

		//auto enemyTransforms = enemyAnimator.GetFinalBoneMatrices();
		//for (int i = 0; i < enemyTransforms.size(); ++i)
		//	ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", enemyTransforms[i]);

		//model = glm::mat4(1.0f);
		//model = glm::translate(model, enemyPosition); // translate it down so it's at the center of the scene
		//model = glm::scale(model, glm::vec3(.5f, .5f, .5f));	// it's a bit too big for our scene, so scale it down
		//model = glm::rotate(model, glm::radians(0.0f + enemyYaw), glm::vec3(0.0f, 1.0f, 0.0f));
		//ourShader.setMat4("model", model);
		//enemyModel.Draw(ourShader);


		// Draw the enemy
		ourShader.use();

		if (isEnemyAlive) {
			auto enemyTransforms = enemyAnimator.GetFinalBoneMatrices();
			for (int i = 0; i < enemyTransforms.size(); ++i)
				ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", enemyTransforms[i]);

			model = glm::mat4(1.0f);
			model = glm::translate(model, enemyPosition);
			model = glm::scale(model, glm::vec3(.5f, .5f, .5f));
			model = glm::rotate(model, glm::radians(0.0f + enemyYaw), glm::vec3(0.0f, 1.0f, 0.0f));
			ourShader.setMat4("model", model);
			enemyModel.Draw(ourShader);
		}


		// Draw the merchant
		ourShader.use();

		auto merchantTransforms = merchantAnimator.GetFinalBoneMatrices();
		for (int i = 0; i < merchantTransforms.size(); ++i)
			ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", merchantTransforms[i]);

		model = glm::mat4(1.0f);
		model = glm::translate(model, merchantPosition);
		model = glm::scale(model, glm::vec3(.5f, .5f, .5f));
		model = glm::rotate(model, glm::radians(90.0f + merchantYaw), glm::vec3(0.0f, 1.0f, 0.0f));
		ourShader.setMat4("model", model);
		merchantModel.Draw(ourShader);


		ourShader.use();

		if (isTalkingToMerchant) {
			auto merchantTalkTransforms = merchantAnimator.GetFinalBoneMatrices();
			for (int i = 0; i < merchantTalkTransforms.size(); ++i)
				ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", merchantTalkTransforms[i]);

			glm::mat4 straightFrontView = camera.GetViewMatrix();
			model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(-2.5, -1.75, -0.55));
			model = glm::scale(model, glm::vec3(4.5f, 4.5f, 4.5f));
			model = glm::rotate(model, glm::radians(25.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			ourShader.setMat4("view", straightFrontView);
			ourShader.setMat4("model", model);
			merchantModel.Draw(ourShader);
		}

		// Draw the map
		mapShader.use();
		mapShader.setMat4("projection", projection);
		mapShader.setMat4("view", view);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		mapShader.setMat4("model", model);
		mapModel.Draw(mapShader);


		if (showHitbox) {
			hitboxShader.use();
			hitboxShader.setMat4("projection", projection);
			hitboxShader.setMat4("view", view);

			// Apply the attack box offset and player transform
			glm::mat4 hitboxModel = playerAttackModel;
			hitboxModel = glm::translate(hitboxModel, HITBOX_OFFSET);

			hitboxShader.setMat4("model", hitboxModel);

			//glDisable(GL_DEPTH_TEST); // Draw on top for visibility
			glLineWidth(5.0f); // Make the wireframe thick
			glBindVertexArray(hitboxVAO);
			// Render as GL_LINES (each index pair is a line segment)
			glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
			glLineWidth(1.0f);
			//glEnable(GL_DEPTH_TEST);
		}
		

		if (showEnemyHitbox) {
			hitboxShader.use();
			hitboxShader.setMat4("projection", projection);
			hitboxShader.setMat4("view", view);

			glm::mat4 enemyHitboxModel = enemyAttackModel;
			enemyHitboxModel = glm::translate(enemyHitboxModel, ENEMY_HITBOX_OFFSET);

			//// Calculate the enemy's attack model matrix (same as used for collision)
			//glm::mat4 enemyHitboxModel = glm::mat4(1.0f);
			//enemyHitboxModel = glm::translate(enemyHitboxModel, enemyPosition);
			//enemyHitboxModel = glm::scale(enemyHitboxModel, glm::vec3(.5f, .5f, .5f)); // Enemy model scale
			//enemyHitboxModel = glm::rotate(enemyHitboxModel, glm::radians(0.0f + enemyYaw), glm::vec3(0.0f, 1.0f, 0.0f));

			//// Apply the enemy attack box offset
			//enemyHitboxModel = glm::translate(enemyHitboxModel, ENEMY_HITBOX_OFFSET);

			hitboxShader.setMat4("model", enemyHitboxModel);

			glLineWidth(5.0f); // Make the wireframe thick
			glBindVertexArray(hitboxVAO);
			glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
			glLineWidth(1.0f);
		}

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		updatePlayerPosition(1);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		updatePlayerPosition(-1);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		playerYaw += yawSpeed * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		playerYaw -= yawSpeed * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
		playerYaw = 0.0f;
	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
		playerYaw += 90.0f * deltaTime;
	/*if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		playerPosition += glm::vec3(0.0f, 1.0f, 0.0f) * 2.0f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		playerPosition += glm::vec3(0.0f, -1.0f, 0.0f) * 2.0f * deltaTime;*/
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	//camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

unsigned int loadCubemap(vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}


void updatePlayerPosition(int direction)
{
	if (direction == 1) {
		playerPosition += playerForward * float(direction) * moveSpeed * deltaTime;

	}
	else if (direction == -1) {
		playerPosition += playerForward * float(direction) * moveSpeed * deltaTime;

	}
}


void setupHitbox()
{
	float vertices[] = {
		// front face
		-HITBOX_WIDTH / 2, -HITBOX_HEIGHT / 2, -HITBOX_DEPTH / 2,
		 HITBOX_WIDTH / 2, -HITBOX_HEIGHT / 2, -HITBOX_DEPTH / 2,
		 HITBOX_WIDTH / 2,  HITBOX_HEIGHT / 2, -HITBOX_DEPTH / 2,
		-HITBOX_WIDTH / 2,  HITBOX_HEIGHT / 2, -HITBOX_DEPTH / 2,

		// back face
		-HITBOX_WIDTH / 2, -HITBOX_HEIGHT / 2,  HITBOX_DEPTH / 2,
		 HITBOX_WIDTH / 2, -HITBOX_HEIGHT / 2,  HITBOX_DEPTH / 2,
		 HITBOX_WIDTH / 2,  HITBOX_HEIGHT / 2,  HITBOX_DEPTH / 2,
		-HITBOX_WIDTH / 2,  HITBOX_HEIGHT / 2,  HITBOX_DEPTH / 2,
	};

	unsigned int indices[] = {
		// front
		0, 1, 1, 2, 2, 3, 3, 0,
		// back
		4, 5, 5, 6, 6, 7, 7, 4,
		// side
		0, 4, 1, 5, 2, 6, 3, 7
	};

	glGenVertexArrays(1, &hitboxVAO);
	glGenBuffers(1, &hitboxVBO);
	unsigned int hitboxEBO;
	glGenBuffers(1, &hitboxEBO);

	glBindVertexArray(hitboxVAO);

	glBindBuffer(GL_ARRAY_BUFFER, hitboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hitboxEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}


bool checkAABBCollision(const glm::mat4& playerAttackModel, const glm::vec3& enemyPos, float enemyScale)
{
	float enemyMinX = enemyPos.x - enemyScale;
	float enemyMaxX = enemyPos.x + enemyScale;
	float enemyMinY = enemyPos.y - enemyScale;
	float enemyMaxY = enemyPos.y + enemyScale * 2.0f;
	float enemyMinZ = enemyPos.z - enemyScale;
	float enemyMaxZ = enemyPos.z + enemyScale;

	glm::vec3 corners[8] = {
		glm::vec3(-HITBOX_WIDTH / 2, -HITBOX_HEIGHT / 2, -HITBOX_DEPTH / 2) + HITBOX_OFFSET,
		glm::vec3(HITBOX_WIDTH / 2, -HITBOX_HEIGHT / 2, -HITBOX_DEPTH / 2) + HITBOX_OFFSET,
		glm::vec3(HITBOX_WIDTH / 2,  HITBOX_HEIGHT / 2, -HITBOX_DEPTH / 2) + HITBOX_OFFSET,
		glm::vec3(-HITBOX_WIDTH / 2,  HITBOX_HEIGHT / 2, -HITBOX_DEPTH / 2) + HITBOX_OFFSET,

		glm::vec3(-HITBOX_WIDTH / 2, -HITBOX_HEIGHT / 2,  HITBOX_DEPTH / 2) + HITBOX_OFFSET,
		glm::vec3(HITBOX_WIDTH / 2, -HITBOX_HEIGHT / 2,  HITBOX_DEPTH / 2) + HITBOX_OFFSET,
		glm::vec3(HITBOX_WIDTH / 2,  HITBOX_HEIGHT / 2,  HITBOX_DEPTH / 2) + HITBOX_OFFSET,
		glm::vec3(-HITBOX_WIDTH / 2,  HITBOX_HEIGHT / 2,  HITBOX_DEPTH / 2) + HITBOX_OFFSET
	};

	for (int i = 0; i < 8; ++i)
	{
		glm::vec4 worldCorner = playerAttackModel * glm::vec4(corners[i], 1.0f);

		// Check if any world-space corner is inside the enemy's AABB
		if (worldCorner.x >= enemyMinX && worldCorner.x <= enemyMaxX &&
			worldCorner.y >= enemyMinY && worldCorner.y <= enemyMaxY &&
			worldCorner.z >= enemyMinZ && worldCorner.z <= enemyMaxZ)
		{
			return true;
		}
	}

	return false;
}

bool checkEnemyAttackCollision(const glm::mat4& enemyAttackModel, const glm::vec3& playerPos, float playerScale)
{
	// Player's AABB (Similar to how enemy AABB was defined in checkAABBCollision)
	float playerMinX = playerPos.x - playerScale;
	float playerMaxX = playerPos.x + playerScale;
	float playerMinY = playerPos.y - playerScale;
	float playerMaxY = playerPos.y + playerScale * 2.0f;
	float playerMinZ = playerPos.z - playerScale;
	float playerMaxZ = playerPos.z + playerScale;

	// Enemy Attack Hitbox corners (Local space)
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

	for (int i = 0; i < 8; ++i)
	{
		// Transform local corner to world space using the enemy attack model
		glm::vec4 worldCorner = enemyAttackModel * glm::vec4(corners[i], 1.0f);

		// Check if any world-space corner is inside the player's AABB
		if (worldCorner.x >= playerMinX && worldCorner.x <= playerMaxX &&
			worldCorner.y >= playerMinY && worldCorner.y <= playerMaxY &&
			worldCorner.z >= playerMinZ && worldCorner.z <= playerMaxZ)
		{
			return true;
		}
	}

	return false;
}

void damageEnemy(float damage)
{
	if (!isEnemyAlive) return;

	enemyHealth -= damage;
	std::cout << "Hit an enemy! Enemy HP: " << enemyHealth << std::endl;

	if (enemyHealth <= 0.0f) {
		enemyHealth = 0.0f;
		std::cout << "Enemy Defeated!" << std::endl;
		playEnemyDyingAnim = true;
	}
}

void damagePlayer(float damage)
{
	if (!isPlayerAlive) return;

	playerHealth -= damage;
	std::cout << "Player hit! Player HP: " << playerHealth << std::endl;

	if (playerHealth <= 0.0f) {
		playerHealth = 0.0f;
		std::cout << "Player Defeated!" << std::endl;
		playDyingAnim = true;
	}
}