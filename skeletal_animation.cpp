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
#include "Enemy.h"
#include "Character.h"
#include "Monster.h"
#include "AnimState.h"
#include "Enemy.cpp"
#include "Character.cpp"
#include "Monster.cpp"

#include <iostream>
#include <vector>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double ypos);
void processInput(GLFWwindow* window);
unsigned int loadCubemap(vector<std::string> faces);
void updatePlayerPosition(int direction);
void setupHitbox();
bool checkAABBCollision(const glm::vec3& enemyPos, float enemyScale);
bool AABBCollision(const glm::vec3& pos1, float x1, float y1, float z1, const glm::vec3& pos2, float x2, float y2, float z2);
bool checkAttackCollision(const glm::vec3& targetPos, float targetScale, float attackHitboxWidth, float attackHitboxHeight, float attackHitboxDepth,
    glm::vec3 attackHitboxOffset, const glm::vec3& attackerPos, float attackerYaw);
void damagePlayer(int damage);
unsigned int loadTexture(const char* path); 
void setupQuad(); 


// settings
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 800;

// game manager
int currentLevel = 0;


// camera
Camera camera(glm::vec3(0.0f, 4.0f, 4.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// player
glm::vec3 playerPosition = glm::vec3(0.4f, 1.1f, -0.4f);
glm::vec3 playerForward = glm::vec3(0.0f, 0.0f, -1.0f);
float moveSpeed = 1.4f;
float playerYaw = 0.0f;
float yawSpeed = 180.0f;
enum AnimState charState = IDLE;
int playerHealth = 200;
int playerMaxHealth = 200;
int playerAttack = 40;
int playerDefense = 5;
bool isPlayerAlive = true;
bool playDyingAnim = false;

// merchant
glm::vec3 merchantPosition = glm::vec3(-1.9f, 1.1f, -3.1f);
float merchantYaw = 0.0f;
float merchantYawSpeed = 100.0f;
enum AnimState merchantState = IDLE;
bool isTalkingToMerchant = false;

// attack hitbox
const float HITBOX_WIDTH = 1.2f;
const float HITBOX_HEIGHT = 1.5f;
const float HITBOX_DEPTH = 1.2f;
const glm::vec3 HITBOX_OFFSET = glm::vec3(0.0f, 1.0f, 1.0f);
unsigned int hitboxVAO = 0;
unsigned int hitboxVBO = 0;
bool enableHitboxRender = false;
bool showAttackHitbox = false;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// UI variables
unsigned int quadVAO = 0;
unsigned int quadVBO = 0;
unsigned int texture_textbox, texture_choice_buy, texture_choice_leave, texture_choice_buy_hl, texture_choice_leave_hl, texture_shop;
bool isBuying = false;
int selectedChoice = 0; // 0 = buy, 1 = leave

unsigned int texture_shop_states[3]; // shop1, shop2, shop3
int shopSelection = 0;               // 0 = Left Item, 1 = Middle, 2 = Right
const int ITEM_COST = 30;

bool keyLeftPressed_Shop = false;
bool keyRightPressed_Shop = false;
bool keyCPressed_Shop = false;
bool keyXPressed_Shop = false;

bool keyUpPressed_Menu = false;
bool keyDownPressed_Menu = false;
bool keyCPressed_Menu = false;

// coins UI
int playerCoins = 0;
unsigned int tex_digits[10];
unsigned int tex_blank;
unsigned int tex_coin_icon;

unsigned int createSingleColorTexture(GLubyte r, GLubyte g, GLubyte b, GLubyte a = 255) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    GLubyte data[] = { r, g, b, a }; // R, G, B, Alpha
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return textureID;
}

bool inventory[3] = { false, false, false }; 
int additionalAtk = 0;      
float additionalSpeed = 0.0f; 

unsigned int tex_inventory_box;
unsigned int tex_items[3];

// HP bar
void RenderHPBar(Shader& uiShader, glm::vec3 worldPos, float currentHp, float maxHp,
    glm::mat4 view, glm::mat4 projection,
    unsigned int bgTexture, unsigned int fillTexture, unsigned int quadVAO)
{
    glm::vec3 headPos = worldPos + glm::vec3(0.0f, 1.2f, 0.0f);

    glm::vec4 clipSpacePos = projection * view * glm::vec4(headPos, 1.0);

    if (clipSpacePos.w <= 0.0f) return; 
    glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;

    float x = ((ndcSpacePos.x + 1.0f) / 2.0f) * SCR_WIDTH;
    float y = ((ndcSpacePos.y + 1.0f) / 2.0f) * SCR_HEIGHT;

    float barWidth = 70.0f;
    float barHeight = 10.0f;
    float hpPercentage = currentHp / maxHp;
    if (hpPercentage < 0.0f) hpPercentage = 0.0f;

    float startX = x - (barWidth / 2.0f);

    // Draw Background (Black)
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(startX, y, 0.0f));
    model = glm::scale(model, glm::vec3(barWidth, barHeight, 1.0f));

    uiShader.setMat4("model", model);
    glBindTexture(GL_TEXTURE_2D, bgTexture);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Draw Health (Red) 
    float fillWidth = barWidth * hpPercentage;

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(startX, y, 0.0f));
    model = glm::scale(model, glm::vec3(fillWidth, barHeight, 1.0f));

    uiShader.setMat4("model", model);
    glBindTexture(GL_TEXTURE_2D, fillTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}

void RenderCoinUI(Shader& uiShader, int coins, unsigned int* digitTextures, unsigned int blankTex, unsigned int coinTex, unsigned int quadVAO)
{
    // Configuration
    float iconSize = 40.0f;
    float padding = 5.0f;
    float startX = 20.0f;
    float startY = SCR_HEIGHT - 60.0f;

    if (coins > 999) coins = 999;

    int hundreds = (coins / 100) % 10;
    int tens = (coins / 10) % 10;
    int ones = coins % 10;

    unsigned int texH = (coins >= 100) ? digitTextures[hundreds] : blankTex;
    unsigned int texT = (coins >= 10) ? digitTextures[tens] : blankTex;
    unsigned int texO = digitTextures[ones];

    unsigned int texturesToDraw[4] = { texH, texT, texO, coinTex };

    for (int i = 0; i < 4; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        
        float xPos = startX + (i * (iconSize + padding));

        model = glm::translate(model, glm::vec3(xPos, startY, 0.0f));
        model = glm::scale(model, glm::vec3(iconSize, iconSize, 1.0f));

        uiShader.setMat4("model", model);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texturesToDraw[i]);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glBindVertexArray(0);
}

void RenderInventory(Shader& uiShader, unsigned int quadVAO)
{
    float boxSize = 60.0f;
    float padding = 10.0f;

    float totalWidth = (boxSize * 3) + (padding * 2);
    float startX = (SCR_WIDTH - totalWidth) / 2.0f;
    float startY = 20.0f; 

    for (int i = 0; i < 3; i++)
    {
        float x = startX + (i * (boxSize + padding));

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, startY, 0.0f));
        model = glm::scale(model, glm::vec3(boxSize, boxSize, 1.0f));

        uiShader.setMat4("model", model);
        glBindTexture(GL_TEXTURE_2D, tex_inventory_box);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        if (inventory[i])
        {
            float itemSize = boxSize * 0.9f;
            float offset = (boxSize - itemSize) / 2.0f;

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(x + offset, startY + offset, 0.0f));
            model = glm::scale(model, glm::vec3(itemSize, itemSize, 1.0f));

            uiShader.setMat4("model", model);
            glBindTexture(GL_TEXTURE_2D, tex_items[i]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
    glBindVertexArray(0);
}

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

    // Load UI textures

    stbi_set_flip_vertically_on_load(true);

	// Merchant UI textures
    texture_textbox = loadTexture(FileSystem::getPath("resources/ui/image1.png").c_str());
    texture_choice_buy = loadTexture(FileSystem::getPath("resources/ui/image2.png").c_str());
    texture_choice_leave = loadTexture(FileSystem::getPath("resources/ui/image3.png").c_str());
    texture_choice_buy_hl = loadTexture(FileSystem::getPath("resources/ui/image4.png").c_str());
    texture_choice_leave_hl = loadTexture(FileSystem::getPath("resources/ui/image5.png").c_str());
    texture_shop = loadTexture(FileSystem::getPath("resources/ui/image6.png").c_str());

    texture_shop_states[0] = loadTexture(FileSystem::getPath("resources/ui/shop1.png").c_str());
    texture_shop_states[1] = loadTexture(FileSystem::getPath("resources/ui/shop2.png").c_str());
    texture_shop_states[2] = loadTexture(FileSystem::getPath("resources/ui/shop3.png").c_str());

    tex_inventory_box = createSingleColorTexture(255, 255, 255, 200);
    tex_items[0] = loadTexture(FileSystem::getPath("resources/ui/healPotion.png").c_str());
    tex_items[1] = loadTexture(FileSystem::getPath("resources/ui/strPotion.png").c_str());
    tex_items[2] = loadTexture(FileSystem::getPath("resources/ui/spdPotion.png").c_str());

    tex_coin_icon = loadTexture(FileSystem::getPath("resources/ui/coin.png").c_str());
    tex_blank = loadTexture(FileSystem::getPath("resources/ui/blank.png").c_str());
    for (int i = 0; i < 10; i++) {
        string path = "resources/ui/" + std::to_string(i) + ".png";
        tex_digits[i] = loadTexture(FileSystem::getPath(path.c_str()).c_str());
    }

    unsigned int tex_hp_bg = createSingleColorTexture(50, 50, 50);
    unsigned int tex_hp_fill = createSingleColorTexture(200, 0, 0);   
    unsigned int tex_hp_player = createSingleColorTexture(0, 200, 0);

    // Set flip back to true for 3D models
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
    Shader uiShader("ui.vs", "ui.fs");


    // load models
    // -----------
    Model ourModel(FileSystem::getPath("resources/objects/mixamo/knight/model/model.dae"));
    Model mapModel(FileSystem::getPath("resources/objects/map/dungeon/source/DungeonBlend/DungeonBlend/dungeon_v13.obj"));
    Model merchantModel(FileSystem::getPath("resources/objects/mixamo/merchant/Model/Model.dae"));

    Animation idleAnimation(FileSystem::getPath("resources/objects/mixamo/knight/Idle/Idle.dae"), &ourModel);
    Animation walkAnimation(FileSystem::getPath("resources/objects/mixamo/knight/Walking/Walking.dae"), &ourModel);
    Animation walkBackAnimation(FileSystem::getPath("resources/objects/mixamo/knight/WalkBack/WalkBack.dae"), &ourModel);
    Animation runAnimation(FileSystem::getPath("resources/objects/mixamo/knight/Running/Running.dae"), &ourModel);
    Animation attackAnimation(FileSystem::getPath("resources/objects/mixamo/knight/Slash/Slash.dae"), &ourModel);
    Animation kickAnimation(FileSystem::getPath("resources/objects/mixamo/knight/SwordKick/SwordKick.dae"), &ourModel);
    Animation turnAnimation(FileSystem::getPath("resources/objects/mixamo/knight/Turn/Turn.dae"), &ourModel);
    Animation dyingAnimation(FileSystem::getPath("resources/objects/mixamo/knight/Death/Death.dae"), &ourModel);

    Animation merchantIdleAnimation(FileSystem::getPath("resources/objects/mixamo/merchant/Idle/Idle.dae"), &merchantModel);
    Animation merchantTalkAnimation(FileSystem::getPath("resources/objects/mixamo/merchant/Talking/Talking.dae"), &merchantModel);

    Animator animator(&idleAnimation);
    Animator merchantAnimator(&merchantIdleAnimation);

    float blendAmount = 0.0f;
    float blendRate = 0.055f;

    float merchantBlendAmount = 0.0f;
    float merchantBlendRate = 0.03f;

    Monster mon1(
        FileSystem::getPath("resources/objects/mixamo/monster/model/model.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Idle/Idle.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Walk/Walk.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Attack/Attack.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Dying/Dying.dae"),
        glm::vec3(0.4f, 1.1f, -12.0f), // Initial position
        100, // maxHp
        20,  // atk
        5,   // def
        0.0f, // m_BlendAmount
        0.055f, // m_BlendRate
        0.0f, // m_Yaw
        100.0f, // m_YawSpeed
        0.8f, // m_MoveSpeed
        HITBOX_WIDTH,
        HITBOX_HEIGHT,
        HITBOX_DEPTH,
        HITBOX_OFFSET
    );

    Monster mon2(
        FileSystem::getPath("resources/objects/mixamo/monster/model/model.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Idle/Idle.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Walk/Walk.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Attack/Attack.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Dying/Dying.dae"),
        glm::vec3(0.4f, 1.1f, -16.0f), // Initial position
        100, // maxHp
        20,  // atk
        5,   // def
        0.0f, // m_BlendAmount
        0.055f, // m_BlendRate
        0.0f, // m_Yaw
        100.0f, // m_YawSpeed
        0.8f, // m_MoveSpeed
        HITBOX_WIDTH,
        HITBOX_HEIGHT,
        HITBOX_DEPTH,
        HITBOX_OFFSET
    );

    Monster mon3(
        FileSystem::getPath("resources/objects/mixamo/monster/model/model.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Idle/Idle.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Walk/Walk.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Attack/Attack.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Dying/Dying.dae"),
        glm::vec3(0.8f, 1.1f, -13.0f), // Initial position
        100, // maxHp
        20,  // atk
        5,   // def
        0.0f, // m_BlendAmount
        0.055f, // m_BlendRate
        0.0f, // m_Yaw
        100.0f, // m_YawSpeed
        0.8f, // m_MoveSpeed
        HITBOX_WIDTH,
        HITBOX_HEIGHT,
        HITBOX_DEPTH,
        HITBOX_OFFSET
    );

    Monster mon4(
        FileSystem::getPath("resources/objects/mixamo/monster/model/model.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Idle/Idle.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Walk/Walk.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Attack/Attack.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Dying/Dying.dae"),
        glm::vec3(1.0f, 1.1f, -15.0f), // Initial position
        100, // maxHp
        20,  // atk
        5,   // def
        0.0f, // m_BlendAmount
        0.055f, // m_BlendRate
        0.0f, // m_Yaw
        100.0f, // m_YawSpeed
        0.8f, // m_MoveSpeed
        HITBOX_WIDTH,
        HITBOX_HEIGHT,
        HITBOX_DEPTH,
        HITBOX_OFFSET
    );

    Monster mon5(
        FileSystem::getPath("resources/objects/mixamo/monster/model/model.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Idle/Idle.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Walk/Walk.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Attack/Attack.dae"),
        FileSystem::getPath("resources/objects/mixamo/monster/Dying/Dying.dae"),
        glm::vec3(1.8f, 1.1f, -18.0f), // Initial position
        100, // maxHp
        20,  // atk
        5,   // def
        0.0f, // m_BlendAmount
        0.055f, // m_BlendRate
        0.0f, // m_Yaw
        100.0f, // m_YawSpeed
        0.8f, // m_MoveSpeed
        HITBOX_WIDTH,
        HITBOX_HEIGHT,
        HITBOX_DEPTH,
        HITBOX_OFFSET
    );

    vector<Monster> monsters = {};

	monsters.push_back(mon1);
	monsters.push_back(mon2);
	monsters.push_back(mon3);
	monsters.push_back(mon4);
	monsters.push_back(mon5);

    setupHitbox();
    setupQuad();


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


        // player animation state
        // ----------------------
        switch (charState) {
        case IDLE:
            if ((glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) 
                && (glfwGetKey(window, GLFW_KEY_SPACE) != GLFW_PRESS) && !playDyingAnim) {
                blendAmount = 0.0f;
                animator.PlayAnimation(&idleAnimation, &walkAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
                charState = IDLE_WALK;
            }
            else if ((glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) && (glfwGetKey(window, GLFW_KEY_SPACE) != GLFW_PRESS)) {
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
            break;
        case IDLE_WALK:
            blendAmount += blendRate;
            blendAmount = fmod(blendAmount, 1.0f);
            animator.PlayAnimation(&idleAnimation, &walkAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
            if (playDyingAnim) {
                blendAmount = 0.0f;
                float startTime = animator.m_CurrentTime2;
                animator.PlayAnimation(&walkAnimation, &dyingAnimation, startTime, 0.0f, blendAmount);
                charState = IDLE_DYING;
            }
			else
            if (blendAmount > 0.9f) {
                blendAmount = 0.0f;
                float startTime = animator.m_CurrentTime2;
                animator.PlayAnimation(&walkAnimation, NULL, startTime, 0.0f, blendAmount);
                charState = WALK;
            }
            break;
        case WALK:
            animator.PlayAnimation(&walkAnimation, NULL, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
            if ((glfwGetKey(window, GLFW_KEY_W) != GLFW_PRESS) && (glfwGetKey(window, GLFW_KEY_A) != GLFW_PRESS) && (glfwGetKey(window, GLFW_KEY_D) != GLFW_PRESS) || playDyingAnim) {
                charState = WALK_IDLE;
            }
            break;
        case WALK_IDLE:
            blendAmount += blendRate;
            blendAmount = fmod(blendAmount, 1.0f);
            animator.PlayAnimation(&walkAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
            if (playDyingAnim) {
                blendAmount = 0.0f;
                float startTime = animator.m_CurrentTime2;
                animator.PlayAnimation(&idleAnimation, &dyingAnimation, startTime, 0.0f, blendAmount);
                charState = IDLE_DYING;
			}
            if (blendAmount > 0.9f) {
                blendAmount = 0.0f;
                float startTime = animator.m_CurrentTime2;
                animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
                charState = IDLE;
            }
            break;
        case IDLE_WALKBACK:
            blendAmount += blendRate;
            blendAmount = fmod(blendAmount, 1.0f);
            animator.PlayAnimation(&idleAnimation, &walkBackAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
            if (playDyingAnim) {
                blendAmount = 0.0f;
                float startTime = animator.m_CurrentTime2;
                animator.PlayAnimation(&walkBackAnimation, &dyingAnimation, startTime, 0.0f, blendAmount);
                charState = IDLE_DYING;
			}
            if (blendAmount > 0.9f) {
                blendAmount = 0.0f;
                float startTime = animator.m_CurrentTime2;
                animator.PlayAnimation(&walkBackAnimation, NULL, startTime, 0.0f, blendAmount);
                charState = WALKBACK;
            }
            break;
        case WALKBACK:
            animator.PlayAnimation(&walkBackAnimation, NULL, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
            if ((glfwGetKey(window, GLFW_KEY_S) != GLFW_PRESS) || playDyingAnim) {
                charState = WALKBACK_IDLE;
            }
            break;
        case WALKBACK_IDLE:
            blendAmount += blendRate;
            blendAmount = fmod(blendAmount, 1.0f);
            animator.PlayAnimation(&walkBackAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
            if (playDyingAnim) {
                blendAmount = 0.0f;
                float startTime = animator.m_CurrentTime2;
                animator.PlayAnimation(&idleAnimation, &dyingAnimation, startTime, 0.0f, blendAmount);
                charState = IDLE_DYING;
            }
            if (blendAmount > 0.9f) {
                blendAmount = 0.0f;
                float startTime = animator.m_CurrentTime2;
                animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
                charState = IDLE;
            }
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
            break;
        case TURN_IDLE:
            if (animator.m_CurrentTime > 0.7f) {
                blendAmount += blendRate;
                blendAmount = fmod(blendAmount, 1.0f);
                animator.PlayAnimation(&turnAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
                if (blendAmount > 0.9f) {
                    blendAmount = 0.0f;
                    float startTime = animator.m_CurrentTime2;
                    animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
                    charState = IDLE;
                }
            }
            else {
                // turning
            }
            break;
        case IDLE_ATTACK:
            blendAmount += blendRate;
            blendAmount = fmod(blendAmount, 1.0f);
            animator.PlayAnimation(&idleAnimation, &attackAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
            if (playDyingAnim) {
                blendAmount = 0.0f;
                float startTime = animator.m_CurrentTime2;
                animator.PlayAnimation(&attackAnimation, &dyingAnimation, startTime, 0.0f, blendAmount);
                charState = IDLE_DYING;
            }
            if (blendAmount > 0.9f) {
                blendAmount = 0.0f;
                float startTime = animator.m_CurrentTime2;
                animator.PlayAnimation(&attackAnimation, NULL, startTime, 0.0f, blendAmount);
                charState = ATTACK_IDLE;
            }
            break;
        case ATTACK_IDLE:
            showAttackHitbox = true;
			std::cout << "Player Current attack time: " << animator.m_CurrentTime << std::endl;
            if (animator.m_CurrentTime > 0.7f) {
                blendAmount += blendRate;
                blendAmount = fmod(blendAmount, 1.0f);
                animator.PlayAnimation(&attackAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
                if (blendAmount > 0.9f) {
					std::cout << "Only 1" << std::endl;
                    for (int i = 0, n = monsters.size(); i < n; i++) {
                        if(monsters[i].isAlive && checkAttackCollision(monsters[i].position, 0.5, HITBOX_WIDTH, HITBOX_HEIGHT, HITBOX_DEPTH,HITBOX_OFFSET, playerPosition, playerYaw)) {
                            float totalDamage = (float)(playerAttack + additionalAtk);

                            monsters[i].TakeDamage(totalDamage);
                            std::cout << "Hit Monster! Damage: " << totalDamage << std::endl;
							std::cout << "Monster " << i << " hit!" << std::endl;
							std::cout << "Monster HP: " << monsters[i].GetHP() << std::endl;
                            if (monsters[i].GetHP() <= 0) {
                                std::cout << "Monster " << i << " defeated!" << std::endl;
								playerCoins += 30;
							}
						}
                    }
                    blendAmount = 0.0f;
                    float startTime = animator.m_CurrentTime2;
                    animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
                    charState = IDLE;
					showAttackHitbox = false;
                }
            }
            else {
                // attacking
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
            }
            else {
                // kicking
            }
            break;
        case IDLE_DYING:
            blendAmount += (blendRate - 0.035);
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

		// characters update
        for (int i = 0, n = monsters.size(); i < n; i++) {
			monsters[i].CheckPlayerDistance(playerPosition, deltaTime, isPlayerAlive);
			monsters[i].FacePlayer(playerPosition, deltaTime, isPlayerAlive);
            monsters[i].doAttack = 0;
			monsters[i].Update(deltaTime, window, playerPosition);
            if (monsters[i].doAttack > 0){
				damagePlayer(monsters[i].doAttack);
                std::cout << "Player hit by monster! Player HP remain " << playerHealth << std::endl;
			}
		}

        // Merchant
        static bool keyMPressed_merchant = false;
        switch (merchantState) {
        case IDLE:
            if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !keyMPressed_merchant) {
                keyMPressed_merchant = true;
                merchantBlendAmount = 0.0f;
                merchantAnimator.PlayAnimation(&merchantIdleAnimation, &merchantTalkAnimation, merchantAnimator.m_CurrentTime, 0.0f, merchantBlendAmount);
                merchantState = IDLE_TALK;
                isTalkingToMerchant = true;
                isBuying = false;
                selectedChoice = 0;
            }
            else if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE) {
                keyMPressed_merchant = false;
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

            if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !keyMPressed_merchant) {
                keyMPressed_merchant = true;
                isTalkingToMerchant = false;
                isBuying = false;
                merchantState = TALK_IDLE;
            }
            else if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE) {
                keyMPressed_merchant = false;
            }

            if (!isTalkingToMerchant) {
                isBuying = false; 
                merchantState = TALK_IDLE;
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
                keyMPressed_merchant = false;
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


		// Draw monsters
		ourShader.use();
		for (int i = 0, n = monsters.size(); i < n; i++) {
			if (monsters[i].isAlive) {
				monsters[i].Draw(ourShader);
			}
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

        // Draw hitboxes
        if (enableHitboxRender && showAttackHitbox) {
			std::cout << "Drawing Player Hitbox" << std::endl;
            hitboxShader.use();
            hitboxShader.setMat4("projection", projection);
            hitboxShader.setMat4("view", view);

            glm::mat4 playerAttackModel = glm::mat4(1.0f);
            playerAttackModel = glm::translate(playerAttackModel, playerPosition);
            playerAttackModel = glm::scale(playerAttackModel, glm::vec3(.5f, .5f, .5f));
            playerAttackModel = glm::rotate(playerAttackModel, glm::radians(180.0f + playerYaw), glm::vec3(0.0f, 1.0f, 0.0f));

            glm::mat4 hitboxModel = playerAttackModel;
            hitboxModel = glm::translate(hitboxModel, HITBOX_OFFSET);

            hitboxShader.setMat4("model", hitboxModel);

            glDisable(GL_DEPTH_TEST);
            glLineWidth(5.0f);
            glBindVertexArray(hitboxVAO);
            glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            glLineWidth(1.0f);
            glEnable(GL_DEPTH_TEST);
        }

        for (int i = 0, n = monsters.size(); i < n; i++) {
            if (enableHitboxRender && monsters[i].showAttackHitbox) {
                std::cout << "Drawing Monster " << i << " Hitbox" << std::endl;
                hitboxShader.use();
                hitboxShader.setMat4("projection", projection);
                hitboxShader.setMat4("view", view);
                glm::mat4 monsterAttackModel = glm::mat4(1.0f);
                monsterAttackModel = glm::translate(monsterAttackModel, monsters[i].position);
                monsterAttackModel = glm::scale(monsterAttackModel, glm::vec3(.5f, .5f, .5f));
                monsterAttackModel = glm::rotate(monsterAttackModel, glm::radians(monsters[i].GetYaw()), glm::vec3(0.0f, 1.0f, 0.0f));
                glm::mat4 hitboxModel = monsterAttackModel;
                hitboxModel = glm::translate(hitboxModel, HITBOX_OFFSET);
                hitboxShader.setMat4("model", hitboxModel);
                glDisable(GL_DEPTH_TEST);
                glLineWidth(5.0f);
                glBindVertexArray(hitboxVAO);
                glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
                glLineWidth(1.0f);
				glEnable(GL_DEPTH_TEST);
            }
        }


        // UI Rendering
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        uiShader.use();
        glm::mat4 uiProjection = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);
        uiShader.setMat4("projection", uiProjection);
        uiShader.setInt("screenTexture", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(quadVAO);

        // Draw Inventory
        RenderInventory(uiShader, quadVAO);

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(quadVAO);

        RenderCoinUI(uiShader, playerCoins, tex_digits, tex_blank, tex_coin_icon, quadVAO);

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(quadVAO);

        for (int i = 0; i < monsters.size(); i++) {
            if (monsters[i].isAlive) {
                RenderHPBar(uiShader,
                    monsters[i].position,
                    (float)monsters[i].GetHP(),
                    100.0f,
                    view, projection,
                    tex_hp_bg, tex_hp_fill, quadVAO);
            }
        }

        if (isPlayerAlive) {
            RenderHPBar(uiShader,
                playerPosition,
                (float)playerHealth,
                (float)playerMaxHealth,
                view, projection,
                tex_hp_bg, tex_hp_player, quadVAO);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(quadVAO);

        if (isTalkingToMerchant)
        {

            if (isBuying)
            {
                // Draw Shop Menu (shop1, shop2, or shop3)
                float shopWidth = 700.0f;
                float shopHeight = 450.0f;
                float shopX = (SCR_WIDTH - shopWidth) / 2.0f;
                float shopY = (SCR_HEIGHT - shopHeight) / 2.0f;

                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(shopX, shopY, 0.0f));
                model = glm::scale(model, glm::vec3(shopWidth, shopHeight, 1.0f));
                uiShader.setMat4("model", model);

                // BIND THE TEXTURE BASED ON SELECTION
                glBindTexture(GL_TEXTURE_2D, texture_shop_states[shopSelection]);

                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
            else
            {
                float boxWidth = 800.0f;
                float boxHeight = 200.0f;
                float boxX = (SCR_WIDTH - boxWidth) / 2.0f;
                float boxY = 50.0f;

                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(boxX, boxY, 0.0f));
                model = glm::scale(model, glm::vec3(boxWidth, boxHeight, 1.0f));
                uiShader.setMat4("model", model);

                glBindTexture(GL_TEXTURE_2D, texture_textbox);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                // Draw Choices (top-right of text box)
                float choiceWidth = 150.0f;
                float choiceHeight = 50.0f;
                float choiceX = boxX + boxWidth - choiceWidth - 30.0f;
                float choiceY_top = boxY + boxHeight - choiceHeight - 70.0f;
                float choiceY_bottom = choiceY_top - choiceHeight - 10.0f;

                // Draw Choice 1 (Buy)
                unsigned int buyTexture = (selectedChoice == 0) ? texture_choice_buy_hl : texture_choice_buy;
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(choiceX, choiceY_top, 0.0f));
                model = glm::scale(model, glm::vec3(choiceWidth, choiceHeight, 1.0f));
                uiShader.setMat4("model", model);

                glBindTexture(GL_TEXTURE_2D, buyTexture);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                // Draw Choice 2 (Leave)
                unsigned int leaveTexture = (selectedChoice == 1) ? texture_choice_leave_hl : texture_choice_leave;
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(choiceX, choiceY_bottom, 0.0f));
                model = glm::scale(model, glm::vec3(choiceWidth, choiceHeight, 1.0f));
                uiShader.setMat4("model", model);

                glBindTexture(GL_TEXTURE_2D, leaveTexture);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);


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

    // Only process movement if NOT talking to the merchant
    if (!isTalkingToMerchant)
    {
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
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            playerCoins++;

        static bool key1Pressed = false;
        static bool key2Pressed = false;
        static bool key3Pressed = false;

        // USE ITEM 1: Heal 20 HP
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && !key1Pressed) {
            key1Pressed = true;
            if (inventory[0]) {
                playerHealth += 20;
                if (playerHealth > playerMaxHealth) playerHealth = playerMaxHealth;
                inventory[0] = false;
                std::cout << "Used Potion! HP: " << playerHealth << std::endl;
            }
        }
        else if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE) key1Pressed = false;

        // USE ITEM 2: +20 Attack
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && !key2Pressed) {
            key2Pressed = true;
            if (inventory[1]) {
                additionalAtk = 20;
                inventory[1] = false;
                std::cout << "Attack Boosted!" << std::endl;
            }
        }
        else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE) key2Pressed = false;

        // USE ITEM 3: +0.4 Speed
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && !key3Pressed) {
            key3Pressed = true;
            if (inventory[2]) {
                additionalSpeed = 0.4f;
                inventory[2] = false;
                std::cout << "Speed Boosted!" << std::endl;
            }
        }
        else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE) key3Pressed = false;
    }

    // Merchant Interaction
    if (isTalkingToMerchant)
    {
        // ----------------------------------------------------
        // STATE 1: SHOPPING MENU (Inside the shop)
        // ----------------------------------------------------
        if (isBuying)
        {
            // 1. Navigation (Left)
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && !keyLeftPressed_Shop) {
                keyLeftPressed_Shop = true;
                shopSelection--;
                if (shopSelection < 0) shopSelection = 0;
            }
            else if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_RELEASE) {
                keyLeftPressed_Shop = false;
            }

            // 2. Navigation (Right)
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && !keyRightPressed_Shop) {
                keyRightPressed_Shop = true;
                shopSelection++;
                if (shopSelection > 2) shopSelection = 2;
            }
            else if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_RELEASE) {
                keyRightPressed_Shop = false;
            }

            // 3. Buy Item (C)
            if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !keyCPressed_Shop) {
                keyCPressed_Shop = true;

                if (!inventory[shopSelection] && playerCoins >= ITEM_COST) {
                    playerCoins -= ITEM_COST;
                    inventory[shopSelection] = true;
                    std::cout << "Bought Item " << (shopSelection + 1) << std::endl;
                }
                else if (inventory[shopSelection]) {
                    std::cout << "You already have this item!" << std::endl;
                }
                else {
                    std::cout << "Not enough coins!" << std::endl;
                }
            }
            else if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
                keyCPressed_Shop = false;
            }

            // 4. Exit Shop (X)
            if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS && !keyXPressed_Shop) {
                keyXPressed_Shop = true;
                isBuying = false;
            }
            else if (glfwGetKey(window, GLFW_KEY_X) == GLFW_RELEASE) {
                keyXPressed_Shop = false;
            }
        }
        // ----------------------------------------------------
        // STATE 2: CHOICE MENU (Buy / Leave)
        // ----------------------------------------------------
        else
        {
            // Up
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && !keyUpPressed_Menu) {
                keyUpPressed_Menu = true;
                selectedChoice = 0;
            }
            else if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE) keyUpPressed_Menu = false;

            // Down
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && !keyDownPressed_Menu) {
                keyDownPressed_Menu = true;
                selectedChoice = 1;
            }
            else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE) keyDownPressed_Menu = false;

            // Confirm (C) - ENTERING THE SHOP
            if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !keyCPressed_Menu) {
                keyCPressed_Menu = true;

                if (selectedChoice == 0) {
                    isBuying = true;
                    shopSelection = 0;
                    keyCPressed_Shop = true;
                }
                else {
                    isTalkingToMerchant = false;
                }
            }
            else if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
                keyCPressed_Menu = false;
            }
        }
    }

}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
}
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
    // Calculate total speed
    float currentSpeed = moveSpeed + additionalSpeed;

    if (direction == 1) {
        playerPosition += playerForward * float(direction) * currentSpeed * deltaTime;
    }
    else if (direction == -1) {
        playerPosition += playerForward * float(direction) * currentSpeed * deltaTime;
    }
}

void setupHitbox()
{
    float vertices[] = {
        -HITBOX_WIDTH / 2, -HITBOX_HEIGHT / 2, -HITBOX_DEPTH / 2,
         HITBOX_WIDTH / 2, -HITBOX_HEIGHT / 2, -HITBOX_DEPTH / 2,
         HITBOX_WIDTH / 2,  HITBOX_HEIGHT / 2, -HITBOX_DEPTH / 2,
        -HITBOX_WIDTH / 2,  HITBOX_HEIGHT / 2, -HITBOX_DEPTH / 2,
        -HITBOX_WIDTH / 2, -HITBOX_HEIGHT / 2,  HITBOX_DEPTH / 2,
         HITBOX_WIDTH / 2, -HITBOX_HEIGHT / 2,  HITBOX_DEPTH / 2,
         HITBOX_WIDTH / 2,  HITBOX_HEIGHT / 2,  HITBOX_DEPTH / 2,
        -HITBOX_WIDTH / 2,  HITBOX_HEIGHT / 2,  HITBOX_DEPTH / 2,
    };
    unsigned int indices[] = {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
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

bool AABBCollision(const glm::vec3& pos1, float x1, float y1, float z1,
    const glm::vec3& pos2, float x2, float y2, float z2)
{
    float minX1 = pos1.x - x1;
    float maxX1 = pos1.x + x1;
    float minY1 = pos1.y - y1;
    float maxY1 = pos1.y + y1;
    float minZ1 = pos1.z - z1;
    float maxZ1 = pos1.z + z1;

    float minX2 = pos2.x - x2;
    float maxX2 = pos2.x + x2;
    float minY2 = pos2.y - y2;
    float maxY2 = pos2.y + y2;
    float minZ2 = pos2.z - z2;
    float maxZ2 = pos2.z + z2;

    return (maxX1 >= minX2 && maxX2 >= minX1) &&
        (maxY1 >= minY2 && maxY2 >= minY1) &&
        (maxZ1 >= minZ2 && maxZ2 >= minZ1);
}

bool checkAABBCollision(const glm::vec3& enemyPos, float enemyScale)
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

    glm::mat4 playerAttackModel = glm::mat4(1.0f);
    playerAttackModel = glm::translate(playerAttackModel, playerPosition);
    playerAttackModel = glm::scale(playerAttackModel, glm::vec3(.5f, .5f, .5f));
    playerAttackModel = glm::rotate(playerAttackModel, glm::radians(180.0f + playerYaw), glm::vec3(0.0f, 1.0f, 0.0f));

    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 worldCorner = playerAttackModel * glm::vec4(corners[i], 1.0f);
        if (worldCorner.x >= enemyMinX && worldCorner.x <= enemyMaxX &&
            worldCorner.y >= enemyMinY && worldCorner.y <= enemyMaxY &&
            worldCorner.z >= enemyMinZ && worldCorner.z <= enemyMaxZ)
        {
            return true;
        }
    }
    return false;
}

bool checkAttackCollision(const glm::vec3& targetPos, float targetScale, float attackHitboxWidth, float attackHitboxHeight, float attackHitboxDepth,
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
    attackModel = glm::rotate(attackModel, glm::radians(180.0f + attackerYaw), glm::vec3(0.0f, 1.0f, 0.0f));

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

void damagePlayer(int damage)
{
    if (!isPlayerAlive) return;
	damage -= playerDefense;
	if (damage < 0) damage = 0;
    playerHealth -= damage;
    std::cout << "Player hit! Player HP: " << playerHealth << std::endl;

    if (playerHealth <= 0) {
        playerHealth = 0;
        std::cout << "Player Defeated!" << std::endl;
        playDyingAnim = true;
    }
}

unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}
void setupQuad()
{
    float quadVertices[] = {
        // pos       // tex
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}
