#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
// =========================================================Gazme stat
enum GameState { INTRO, SHOP, INSTRUCTIONS, PLAYING, LEVEL_COMPLETE, GAME_OVER, WIN };
GameState gameState = INTRO;
bool paused = false;
int menuSelection = 0;
//============================================================== Shop
int shopTab = 0;
int shopMode = 0;
int selectedAttack = 0;
int selectedDefense = 0;
float attackBonus[5] = {0.0f, 0.15f, 0.30f, 0.50f, 0.80f};
int attackPrice[5] = {0, 300, 600, 1000, 1500};
bool attackOwned[5] = {true, false, false, false, false};
float defenseAbsorb[5] = {0.30f, 0.50f, 0.70f, 0.90f, 1.00f};
int defensePrice[5] = {200, 400, 600, 800, 1200};
bool defenseOwned[5] = {true, false, false, false, false};
int equippedAttack = 0;
int equippedDefense = 0;
float shieldCapacity = 0.0f;
const int MAX_LEVEL = 20;
// ============================================================player
float playerX = -0.72f;
float playerY = -0.68f;
float playerSize = 0.055f;
float playerSpeed = 0.045f;
int health = 100;
int score = 0;
int fireCooldownTicks = 0;
float facingX = 1.0f, facingY = 0.0f;
bool firing = false;
float muzzleFlashTimer = 0.0f;
// ============================================================level
int level = 1;
bool keyCollected = false;
bool doorOpen = false;
int timeLeft = 60;
float keyX = 0.65f, keyY = 0.55f;
float doorX = 0.75f, doorY = -0.65f;
// ============================================================WALLS / ENEMIES / TRAPS
struct Wall { float x, y, width, height; };
const int MAX_WALLS = 60;
Wall walls[MAX_WALLS];
int wallCount = 0;

struct Enemy
{
    float x, y, size, speed;
    float health;
    int type;
    bool alive;
    float dashTimer;
    bool dashing;
};
const int MAX_ENEMIES = 30;
Enemy enemies[MAX_ENEMIES];
int enemyCount = 0;

struct Trap { float x, y, width, height; bool active; };
const int MAX_TRAPS = 30;
Trap traps[MAX_TRAPS];
int trapCount = 0;
// =============================================================BULLETS
struct Bullet { float x, y, vx, vy; bool active; };
const int MAX_BULLETS = 40;
Bullet bullets[MAX_BULLETS];
int bulletCount = 0;
const float BULLET_SPEED = 0.055f;
const float BULLET_DAMAGE = 10.0f;
// ============================================================BOSS
float bossX = 0.55f, bossY = 0.35f, bossSize = 0.15f;
int bossHealth = 15;
int bossMaxHealth = 15;
bool bossAlive = true;
bool isBossLevel(int lvl) { return lvl % 5 == 0; }
// ============================================================PARTICLES  (impact bursts, torch embers, key sparkle)
struct Particle { float x, y, vx, vy, life, maxLife, r, g, b; };
const int MAX_PARTICLES = 200;
Particle particles[MAX_PARTICLES];
int particleCount = 0;

void spawnParticles(float x, float y, float r, float g, float b, int count)
{
    for (int i = 0; i < count && particleCount < MAX_PARTICLES; i++)
    {
        float angle = (float)(rand() % 628) / 100.0f;
        float speed = 0.004f + (rand() % 100) / 4000.0f;

        particles[particleCount].x = x;
        particles[particleCount].y = y;
        particles[particleCount].vx = cos(angle) * speed;
        particles[particleCount].vy = sin(angle) * speed;
        particles[particleCount].life = 0.6f + (rand() % 40) / 100.0f;
        particles[particleCount].maxLife = particles[particleCount].life;
        particles[particleCount].r = r;
        particles[particleCount].g = g;
        particles[particleCount].b = b;

        particleCount++;
    }
}

void updateParticles()
{
    for (int i = 0; i < particleCount; i++)
    {
        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        particles[i].vy -= 0.0006f;
        particles[i].life -= 0.08f;

        if (particles[i].life <= 0.0f)
        {
            particles[i] = particles[particleCount - 1];
            particleCount--;
            i--;
        }
    }
}

void drawParticles()
{
    glPointSize(3.0f);
    glBegin(GL_POINTS);

    for (int i = 0; i < particleCount; i++)
    {
        float t = particles[i].life / particles[i].maxLife;
        glColor4f(particles[i].r, particles[i].g, particles[i].b, t);
        glVertex2f(particles[i].x, particles[i].y);
    }

    glEnd();
    glPointSize(1.0f);
}
// ============================================================SCREEN SHAKE / DAMAGE
float shakeTimer = 0.0f;
float shakeStrength = 0.0f;
float flashTimer = 0.0f;
void triggerShake(float strength)
{
    shakeTimer = 0.25f;
    shakeStrength = strength;
}

void triggerFlash()
{
    flashTimer = 0.35f;
}

float animationTime = 0.0f;
// ============================================================ DRAW TExt
void drawText(float x, float y, const char* text, void* font = GLUT_BITMAP_HELVETICA_18)
{
    glRasterPos2f(x, y);
    for (int i = 0; text[i] != '\0'; i++)
        glutBitmapCharacter(font, text[i]);
}
// ============================================================// PRIMITIVES

void drawRectangle(float x, float y, float width, float height)
{
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void drawCircle(float radius)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0f, 0.0f);
    for (float a = 0.0f; a <= 6.283f; a += 0.08f)
        glVertex2f(radius * cos(a), radius * sin(a));
    glEnd();
}
// simple faux-lighting: two overlapping circles, a darker base and a and lighter offset highlight, gives characters some depth without shaders
void drawShadedCircle(float radius, float r, float g, float b)
{
    glColor3f(r * 0.55f, g * 0.55f, b * 0.55f);
    drawCircle(radius);

    glColor3f(r, g, b);
    drawCircle(radius * 0.82f);

    glPushMatrix();
    glTranslatef(-radius * 0.28f, radius * 0.28f, 0.0f);
    glColor3f(fmin(r * 1.35f, 1.0f), fmin(g * 1.35f, 1.0f), fmin(b * 1.35f, 1.0f));
    drawCircle(radius * 0.35f);
    glPopMatrix();
}

bool collision(float x1, float y1, float size1, float x2, float y2, float size2)
{
    return x1 + size1 > x2 - size2 && x1 - size1 < x2 + size2 &&
           y1 + size1 > y2 - size2 && y1 - size1 < y2 + size2;
}
// ============================================================// ADD HELPERS
void addWall(float x, float y, float width, float height)
{
    if (wallCount >= MAX_WALLS) return;
    walls[wallCount++] = { x, y, width, height };
}
void addEnemy(float x, float y, float speed, int type)
{
    if (enemyCount >= MAX_ENEMIES) return;

    Enemy e;
    e.x = x; e.y = y;
    e.size = (type == 2) ? 0.065f : 0.055f;   // type 2 is a heavier brute, same 100 hp though
    e.speed = speed;
    e.health = 100.0f;                        // every enemy dies to exactly 10 bullets
    e.alive = true;
    e.type = type;
    e.dashTimer = (float)(rand() % 30) / 10.0f;
    e.dashing = false;

    enemies[enemyCount++] = e;
}

void addTrap(float x, float y, float width, float height)
{
    if (trapCount >= MAX_TRAPS) return;
    traps[trapCount++] = { x, y, width, height, true };
}

float randRange(float lo, float hi)
{
    return lo + ((float)(rand() % 1000) / 1000.0f) * (hi - lo);
}

bool circleHitsAnyWall(float x, float y, float size)
{
    for (int i = 0; i < wallCount; i++)
    {
        float left = walls[i].x - size, right = walls[i].x + walls[i].width + size;
        float bottom = walls[i].y - size, top = walls[i].y + walls[i].height + size;

        if (x > left && x < right && y > bottom && y < top)
            return true;
    }
    return false;
}

bool playerHitsWall(float x, float y)
{
    return circleHitsAnyWall(x, y, playerSize);
}
// ============================================================// FLOOR

void drawFloor()
{
    glColor3f(0.10f, 0.08f, 0.12f);
    drawRectangle(-1.0f, -1.0f, 2.0f, 2.0f);

    glColor3f(0.16f, 0.13f, 0.18f);
    glBegin(GL_LINES);
    for (float x = -1.0f; x <= 1.0f; x += 0.10f) { glVertex2f(x, -1.0f); glVertex2f(x, 1.0f); }
    for (float y = -1.0f; y <= 1.0f; y += 0.10f) { glVertex2f(-1.0f, y); glVertex2f(1.0f, y); }
    glEnd();

    // corner vignette - four dark triangles, cheap fake ambient occlusion
    glColor4f(0.0f, 0.0f, 0.0f, 0.35f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-1.0f, 1.0f); glVertex2f(-0.55f, 1.0f); glVertex2f(-1.0f, 0.55f);
    glVertex2f(1.0f, 1.0f);  glVertex2f(0.55f, 1.0f);  glVertex2f(1.0f, 0.55f);
    glVertex2f(-1.0f, -1.0f);glVertex2f(-0.55f, -1.0f);glVertex2f(-1.0f, -0.55f);
    glVertex2f(1.0f, -1.0f); glVertex2f(0.55f, -1.0f); glVertex2f(1.0f, -0.55f);
    glEnd();
}
// ============================================================// WALLS
void drawWalls()
{
    for (int i = 0; i < wallCount; i++)
    {
        float x = walls[i].x, y = walls[i].y, w = walls[i].width, h = walls[i].height;

        glColor3f(0.25f, 0.23f, 0.28f);
        drawRectangle(x, y, w, h);

        glColor3f(0.40f, 0.37f, 0.43f);
        drawRectangle(x, y + h - 0.012f, w, 0.012f);

        glColor3f(0.12f, 0.11f, 0.14f);
        glBegin(GL_LINES);
        glVertex2f(x, y + h / 2);
        glVertex2f(x + w, y + h / 2);
        glEnd();
    }
}
// ============================================================// PLAYER
void drawPlayer()
{
    glPushMatrix();
    glTranslatef(playerX, playerY, 0.0f);

    glColor4f(0.02f, 0.02f, 0.02f, 0.5f);
    glPushMatrix();
    glTranslatef(0.0f, -0.065f, 0.0f);
    glScalef(1.3f, 0.35f, 1.0f);
    drawCircle(0.055f);
    glPopMatrix();

    glColor3f(0.08f, 0.28f, 0.75f);
    drawRectangle(-0.045f, -0.05f, 0.09f, 0.10f);

    glColor3f(0.35f, 0.45f, 0.65f);
    drawRectangle(-0.038f, -0.03f, 0.076f, 0.065f);

    glPushMatrix();
    glTranslatef(0.0f, 0.075f, 0.0f);
    drawShadedCircle(0.042f, 0.75f, 0.50f, 0.30f);
    glPopMatrix();

    glColor3f(0.15f, 0.16f, 0.20f);
    glBegin(GL_POLYGON);
    glVertex2f(-0.045f, 0.075f);
    glVertex2f(0.045f, 0.075f);
    glVertex2f(0.030f, 0.115f);
    glVertex2f(-0.030f, 0.115f);
    glEnd();

    // cooldown ring - fills back up so the player can see when they can
    // fire again (tougher combat: shooting isn't free/instant every frame)
    if (fireCooldownTicks > 0)
    {
        float t = 1.0f - (float)fireCooldownTicks / 2.0f;
        glColor4f(1.0f, 0.85f, 0.2f, 0.8f);
        glBegin(GL_LINE_STRIP);
        for (float a = -1.5708f; a <= -1.5708f + t * 6.283f; a += 0.15f)
            glVertex2f(0.07f * cos(a), 0.13f + 0.07f * sin(a));
        glEnd();
    }

    // small gun barrel pointing in the aim/facing direction
    float aimAngle = atan2(facingY, facingX) * 57.2958f;
    glPushMatrix();
    glRotatef(aimAngle, 0.0f, 0.0f, 1.0f);
    glColor3f(0.25f, 0.25f, 0.28f);
    drawRectangle(0.03f, -0.01f, 0.07f, 0.02f);

    if (muzzleFlashTimer > 0.0f)
    {
        glColor4f(1.0f, 0.85f, 0.2f, muzzleFlashTimer / 0.15f);
        glPushMatrix();
        glTranslatef(0.11f, 0.0f, 0.0f);
        drawCircle(0.025f);
        glPopMatrix();
    }
    glPopMatrix();

    glPopMatrix();
}
// ============================================================ KEY
void drawKey()
{
    if (keyCollected) return;

    glPushMatrix();
    glTranslatef(keyX, keyY, 0.0f);
    glTranslatef(0.0f, sin(animationTime * 5.0f) * 0.008f, 0.0f);
    glRotatef(animationTime * 40.0f, 0.0f, 0.0f, 1.0f);

    // soft glow halo
    glColor4f(1.0f, 0.85f, 0.2f, 0.25f + 0.1f * sin(animationTime * 6.0f));
    drawCircle(0.065f);

    glColor3f(1.0f, 0.75f, 0.05f);
    glBegin(GL_LINE_LOOP);
    for (float a = 0; a <= 6.283f; a += 0.1f)
        glVertex2f(0.035f * cos(a), 0.035f * sin(a));
    glEnd();

    drawRectangle(0.02f, -0.01f, 0.13f, 0.02f);
    drawRectangle(0.12f, -0.03f, 0.025f, 0.05f);

    glPopMatrix();
}
// ============================================================// DOOR
void drawDoor()
{
    glPushMatrix();
    glTranslatef(doorX, doorY, 0.0f);

    glColor3f(0.35f, 0.20f, 0.08f);
    drawRectangle(-0.085f, -0.14f, 0.17f, 0.28f);

    if (doorOpen)
    {
        float pulse = 0.15f + 0.1f * sin(animationTime * 4.0f);
        glColor4f(0.05f, 0.70f, 0.25f, 0.3f + pulse);
        drawRectangle(-0.11f, -0.17f, 0.22f, 0.34f);
        glColor3f(0.05f, 0.70f, 0.25f);
    }
    else
    {
        glColor3f(0.25f, 0.07f, 0.08f);
    }

    drawRectangle(-0.065f, -0.12f, 0.13f, 0.24f);

    glColor3f(1.0f, 0.75f, 0.1f);
    glPushMatrix();
    glTranslatef(0.035f, 0.0f, 0.0f);
    drawCircle(0.009f);
    glPopMatrix();

    glPopMatrix();
}

// ============================================================// TRAPS
void drawTraps()
{
    for (int i = 0; i < trapCount; i++)
    {
        if (!traps[i].active) continue;

        float x = traps[i].x, y = traps[i].y, w = traps[i].width, h = traps[i].height;

        glColor3f(0.30f, 0.08f, 0.08f);
        drawRectangle(x, y, w, h);

        glColor3f(0.80f, 0.80f, 0.85f);
        for (int j = 0; j < 3; j++)
        {
            float px = x + 0.02f + j * 0.04f;
            float bob = 0.006f * sin(animationTime * 6.0f + j);

            glBegin(GL_TRIANGLES);
            glVertex2f(px, y + 0.02f + bob);
            glVertex2f(px + 0.02f, y + h - 0.02f + bob);
            glVertex2f(px + 0.04f, y + 0.02f + bob);
            glEnd();
        }
    }
}

// ============================================================// ENEMY
void drawEnemy(int i)
{
    if (!enemies[i].alive) return;

    glPushMatrix();
    glTranslatef(enemies[i].x, enemies[i].y, 0.0f);

    glColor4f(0.02f, 0.02f, 0.02f, 0.5f);
    glPushMatrix();
    glTranslatef(0.0f, -0.06f, 0.0f);
    glScalef(1.3f, 0.35f, 1.0f);
    drawCircle(enemies[i].size);
    glPopMatrix();

    float r, g, b;
    if (enemies[i].type == 1)      { r = 0.65f; g = 0.05f; b = 0.08f; }
    else if (enemies[i].type == 2) { r = 0.55f; g = 0.10f; b = 0.65f; }
    else                            { r = 0.85f; g = 0.45f; b = 0.05f; } // type 3 dasher: orange

    if (enemies[i].dashing) { r = fmin(r * 1.5f, 1.0f); g = fmin(g * 1.5f, 1.0f); }

    drawShadedCircle(enemies[i].size, r, g, b);

    glColor3f(0.25f, 0.12f, 0.08f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.035f, 0.035f); glVertex2f(-0.06f, 0.09f); glVertex2f(-0.005f, 0.055f);
    glVertex2f(0.035f, 0.035f);  glVertex2f(0.06f, 0.09f);  glVertex2f(0.005f, 0.055f);
    glEnd();

    glColor3f(1.0f, 0.9f, 0.1f);
    glPushMatrix(); glTranslatef(-0.018f, 0.015f, 0.0f); drawCircle(0.008f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.018f, 0.015f, 0.0f); drawCircle(0.008f); glPopMatrix();

    glColor3f(0.15f, 0.15f, 0.15f);
    drawRectangle(-0.06f, 0.085f, 0.12f, 0.012f);
    glColor3f(0.9f, 0.05f, 0.05f);
    drawRectangle(-0.06f, 0.085f, 0.12f * enemies[i].health / 100.0f, 0.012f);

    glPopMatrix();
}

// ============================================================ BOSS
void drawBoss()
{
    if (!bossAlive) return;

    bool enraged = bossHealth <= bossMaxHealth / 3;

    glPushMatrix();
    glTranslatef(bossX, bossY, 0.0f);

    float auraPulse = 1.2f + 0.08f * sin(animationTime * (enraged ? 10.0f : 4.0f));
    glColor4f(enraged ? 0.55f : 0.25f, 0.0f, enraged ? 0.05f : 0.35f, 0.6f);
    glPushMatrix();
    glScalef(auraPulse, auraPulse, 1.0f);
    drawCircle(bossSize);
    glPopMatrix();

    drawShadedCircle(bossSize, enraged ? 0.75f : 0.45f, 0.0f, enraged ? 0.05f : 0.65f);

    glColor3f(0.85f, 0.65f, 0.20f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.08f, 0.08f); glVertex2f(-0.12f, 0.18f); glVertex2f(-0.02f, 0.12f);
    glVertex2f(0.08f, 0.08f);  glVertex2f(0.12f, 0.18f);  glVertex2f(0.02f, 0.12f);
    glEnd();

    glColor3f(1.0f, 0.05f, 0.05f);
    glPushMatrix(); glTranslatef(-0.045f, 0.035f, 0.0f); drawCircle(0.018f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.045f, 0.035f, 0.0f); drawCircle(0.018f); glPopMatrix();

    glColor3f(0.15f, 0.15f, 0.15f);
    drawRectangle(-0.16f, 0.19f, 0.32f, 0.025f);
    glColor3f(0.9f, 0.05f, 0.05f);
    float hpWidth = 0.32f * bossHealth / (float)bossMaxHealth;
    drawRectangle(-0.16f, 0.19f, hpWidth, 0.025f);

    glPopMatrix();
}

// ============================================================ TORCH
void drawTorch(float x, float y)
{
    glPushMatrix();
    glTranslatef(x, y, 0.0f);

    glColor3f(0.30f, 0.15f, 0.05f);
    drawRectangle(-0.008f, -0.08f, 0.016f, 0.10f);

    float flame = 0.035f + 0.008f * sin(animationTime * 8.0f);

    glColor3f(1.0f, 0.25f, 0.02f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-flame, 0.02f); glVertex2f(0.0f, 0.10f); glVertex2f(flame, 0.02f);
    glEnd();

    glColor3f(1.0f, 0.75f, 0.05f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-flame * 0.5f, 0.02f); glVertex2f(0.0f, 0.075f); glVertex2f(flame * 0.5f, 0.02f);
    glEnd();

    glPopMatrix();
}

// ============================================================ BULLETS
void fireBullet()
{
    if (bulletCount >= MAX_BULLETS) return;

    Bullet b;
    b.x = playerX + facingX * 0.06f;
    b.y = playerY + facingY * 0.06f;
    b.vx = facingX * BULLET_SPEED;
    b.vy = facingY * BULLET_SPEED;
    b.active = true;

    bullets[bulletCount++] = b;

    firing = true;
    muzzleFlashTimer = 0.15f;
}

void updateBullets()
{
    for (int i = 0; i < bulletCount; i++)
    {
        if (!bullets[i].active) { bullets[i] = bullets[bulletCount - 1]; bulletCount--; i--; continue; }

        bullets[i].x += bullets[i].vx;
        bullets[i].y += bullets[i].vy;

        // out of bounds
        if (bullets[i].x < -1.0f || bullets[i].x > 1.0f || bullets[i].y < -1.0f || bullets[i].y > 1.0f)
        {
            bullets[i].active = false;
            continue;
        }

        // wall hit
        bool hitWall = false;
        for (int w = 0; w < wallCount; w++)
        {
            float left = walls[w].x, right = walls[w].x + walls[w].width;
            float bottom = walls[w].y, top = walls[w].y + walls[w].height;

            if (bullets[i].x > left && bullets[i].x < right && bullets[i].y > bottom && bullets[i].y < top)
            { hitWall = true; break; }
        }
        if (hitWall)
        {
            spawnParticles(bullets[i].x, bullets[i].y, 0.7f, 0.7f, 0.7f, 5);
            bullets[i].active = false;
            continue;
        }

        // enemy hit - bullets only ever damage enemies, never the player
        for (int e = 0; e < enemyCount; e++)
        {
            if (!enemies[e].alive) continue;

            if (collision(bullets[i].x, bullets[i].y, 0.012f, enemies[e].x, enemies[e].y, enemies[e].size))
            {
                enemies[e].health -= BULLET_DAMAGE;   // exactly -10%
                bullets[i].active = false;
                spawnParticles(bullets[i].x, bullets[i].y, 0.9f, 0.6f, 0.2f, 6);

                if (enemies[e].health <= 0.0f)
                {
                    enemies[e].alive = false;
                    score += 100;
                    spawnParticles(enemies[e].x, enemies[e].y, 0.9f, 0.2f, 0.2f, 16);
                }
                break;
            }
        }
        if (!bullets[i].active) continue;

        // boss hit
        if (isBossLevel(level) && bossAlive)
        {
            if (collision(bullets[i].x, bullets[i].y, 0.012f, bossX, bossY, bossSize))
            {
                bossHealth--;
                bullets[i].active = false;
                spawnParticles(bullets[i].x, bullets[i].y, 0.8f, 0.1f, 0.8f, 8);

                if (bossHealth <= 0)
                {
                    bossHealth = 0;
                    bossAlive = false;
                    score += 1000;
                    doorOpen = true;
                    spawnParticles(bossX, bossY, 0.9f, 0.7f, 0.1f, 40);
                }
            }
        }
    }
}

void drawBullets()
{
    glColor3f(1.0f, 0.9f, 0.3f);
    for (int i = 0; i < bulletCount; i++)
    {
        if (!bullets[i].active) continue;

        glBegin(GL_LINES);
        glVertex2f(bullets[i].x - bullets[i].vx * 1.5f, bullets[i].y - bullets[i].vy * 1.5f);
        glVertex2f(bullets[i].x, bullets[i].y);
        glEnd();

        glPushMatrix();
        glTranslatef(bullets[i].x, bullets[i].y, 0.0f);
        drawCircle(0.011f);
        glPopMatrix();
    }
}

// ============================================================ MINIMAP  (top-right corner: player, key, door, enemies)
void drawMinimap()
{
    float mx = 0.72f, my = 0.72f, scale = 0.20f;

    glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
    drawRectangle(mx - scale, my - scale, scale * 2, scale * 2);

    glColor3f(0.5f, 0.5f, 0.55f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(mx - scale, my - scale); glVertex2f(mx + scale, my - scale);
    glVertex2f(mx + scale, my + scale); glVertex2f(mx - scale, my + scale);
    glEnd();

    glPointSize(5.0f);
    glBegin(GL_POINTS);

    glColor3f(0.2f, 0.6f, 1.0f);
    glVertex2f(mx + playerX * scale, my + playerY * scale);

    if (!keyCollected)
    {
        glColor3f(1.0f, 0.8f, 0.1f);
        glVertex2f(mx + keyX * scale, my + keyY * scale);
    }

    glColor3f(doorOpen ? 0.1f : 0.8f, doorOpen ? 0.8f : 0.1f, 0.1f);
    glVertex2f(mx + doorX * scale, my + doorY * scale);

    glColor3f(0.9f, 0.1f, 0.1f);
    for (int i = 0; i < enemyCount; i++)
        if (enemies[i].alive)
            glVertex2f(mx + enemies[i].x * scale, my + enemies[i].y * scale);

    glEnd();
    glPointSize(1.0f);
}

// ============================================================ PROCEDURAL LEVEL GENERATION  (levels 6-20)
void proceduralLevel(int lvl)
{
    // Difficulty ramps with level: more enemies, more traps, faster & enemy base speed, tighter timer. Enemy speed is constant within a level.
    int baseEnemies = 4 + lvl / 3;
    if (baseEnemies > 12) baseEnemies = 12;

    int baseTraps = 3 + lvl / 3;
    if (baseTraps > 10) baseTraps = 10;
    // Enemy speed increases by 0.5x (50%) each level. & Within the same level, every enemy keeps a constant speed.
    float enemySpeedBase = 0.032f * pow(1.5f, (float)(lvl - 1));
    // a handful of short obstacle walls, randomly scattered & (kept short deliberately so the level always stays solvable)
    int wallPairs = 3 + lvl / 4;
    for (int i = 0; i < wallPairs; i++)
    {
        float wx = randRange(-0.55f, 0.35f);
        float wy = randRange(-0.55f, 0.55f);

        if (rand() % 2 == 0)
            addWall(wx, wy, 0.05f, randRange(0.25f, 0.55f));
        else
            addWall(wx, wy, randRange(0.25f, 0.55f), 0.05f);
    }

    keyX = randRange(0.35f, 0.70f);
    keyY = randRange(-0.55f, 0.60f);

    doorX = 0.72f;
    doorY = randRange(-0.65f, 0.60f);

    for (int i = 0; i < baseEnemies; i++)
    {
        float ex = randRange(-0.60f, 0.68f);
        float ey = randRange(-0.60f, 0.68f);

        // avoid spawning right on top of the player's start corner
        if (ex < -0.4f && ey < -0.4f) ex += 0.5f;

        int type;
        int roll = rand() % 3;
        if (roll == 0) type = 1;
        else if (roll == 1) type = 2;
        else type = 3; // dasher, introduced by procedural levels for extra bite

        addEnemy(ex, ey, enemySpeedBase, type);
    }

    for (int i = 0; i < baseTraps; i++)
    {
        float tx = randRange(-0.65f, 0.55f);
        float ty = randRange(-0.65f, 0.55f);
        addTrap(tx, ty, 0.12f, 0.12f);
    }

    if (isBossLevel(lvl))
    {
        bossX = 0.55f;
        bossY = 0.40f;
        bossMaxHealth = 15 + (lvl / 5 - 1) * 8;
        bossHealth = bossMaxHealth;

        // fewer regular enemies on boss floors so the boss is the focus
        enemyCount = enemyCount > 3 ? 3 : enemyCount;
    }

    timeLeft = 70 - lvl * 2;
    if (timeLeft < 30) timeLeft = 30;
}

// ============================================================LEVEL CREATION
void createLevel(int newLevel)
{
    level = newLevel;

    wallCount = 0;
    enemyCount = 0;
    trapCount = 0;
    particleCount = 0;

    keyCollected = false;
    doorOpen = false;

    playerX = -0.72f;
    playerY = -0.68f;

    bossAlive = true;
    bossHealth = 15;
    bossMaxHealth = 15;

    // outer walls, every level
    addWall(-0.95f, 0.80f, 1.90f, 0.10f);
    addWall(-0.95f, -0.90f, 1.90f, 0.10f);
    addWall(-0.95f, -0.80f, 0.10f, 1.60f);
    addWall(0.85f, -0.80f, 0.10f, 1.60f);

    if (newLevel == 1)
    {
        addWall(-0.30f, -0.40f, 0.08f, 0.70f);
        addWall(0.10f, 0.10f, 0.50f, 0.08f);
        addWall(-0.65f, 0.35f, 0.30f, 0.06f);
        keyX = 0.60f; keyY = 0.55f;
        doorX = 0.72f; doorY = -0.65f;
        addEnemy(0.45f, 0.35f, 0.030f, 1);
        timeLeft = 55;
    }
    else if (newLevel == 2)
    {
        addWall(-0.45f, -0.20f, 0.08f, 0.80f);
        addWall(-0.05f, 0.00f, 0.08f, 0.65f);
        addWall(0.20f, -0.50f, 0.50f, 0.08f);
        addWall(0.35f, 0.45f, 0.06f, 0.30f);
        keyX = 0.62f; keyY = 0.55f;
        doorX = 0.72f; doorY = -0.65f;
        addEnemy(0.45f, 0.45f, 0.036f, 1);
        addEnemy(0.55f, -0.10f, 0.033f, 1);
        addTrap(-0.70f, 0.30f, 0.12f, 0.12f);
        addTrap(0.55f, 0.55f, 0.12f, 0.12f);
        timeLeft = 52;
    }
    else if (newLevel == 3)
    {
        addWall(-0.50f, -0.30f, 0.08f, 1.0f);
        addWall(-0.20f, 0.20f, 0.60f, 0.08f);
        addWall(0.40f, -0.40f, 0.08f, 0.80f);
        addWall(-0.70f, -0.70f, 0.25f, 0.06f);
        keyX = 0.65f; keyY = 0.55f;
        doorX = 0.72f; doorY = -0.65f;
        addEnemy(0.45f, 0.45f, 0.043f, 1);
        addEnemy(0.30f, -0.10f, 0.040f, 2);
        addEnemy(-0.20f, 0.55f, 0.037f, 1);
        addTrap(-0.70f, 0.20f, 0.12f, 0.12f);
        addTrap(0.10f, -0.60f, 0.12f, 0.12f);
        addTrap(-0.10f, 0.55f, 0.12f, 0.12f);
        timeLeft = 50;
    }
    else if (newLevel == 4)
    {
        addWall(-0.50f, 0.00f, 0.80f, 0.08f);
        addWall(0.00f, -0.50f, 0.08f, 0.90f);
        addWall(0.30f, 0.30f, 0.50f, 0.08f);
        addWall(-0.75f, -0.55f, 0.06f, 0.35f);
        keyX = 0.65f; keyY = -0.30f;
        doorX = 0.72f; doorY = -0.65f;
        addEnemy(0.50f, 0.50f, 0.048f, 1);
        addEnemy(-0.20f, 0.60f, 0.045f, 2);
        addEnemy(0.60f, -0.20f, 0.050f, 1);
        addEnemy(-0.40f, -0.20f, 0.042f, 3);
        addTrap(-0.70f, 0.20f, 0.12f, 0.12f);
        addTrap(-0.30f, -0.70f, 0.12f, 0.12f);
        addTrap(0.20f, 0.60f, 0.12f, 0.12f);
        addTrap(0.60f, 0.55f, 0.12f, 0.12f);
        timeLeft = 48;
    }
    else if (newLevel == 5)
    {
        addWall(-0.40f, -0.20f, 0.08f, 0.90f);
        addWall(0.10f, 0.20f, 0.60f, 0.08f);
        addWall(0.40f, -0.50f, 0.08f, 0.80f);
        addWall(-0.75f, 0.15f, 0.06f, 0.35f);
        keyX = 0.65f; keyY = -0.65f;
        doorX = 0.72f; doorY = 0.60f;
        bossX = 0.55f; bossY = 0.40f;
        bossMaxHealth = 15; bossHealth = 15;
        addEnemy(0.50f, -0.10f, 0.052f, 1);
        addEnemy(-0.20f, 0.55f, 0.048f, 2);
        addEnemy(0.60f, -0.30f, 0.055f, 3);
        addTrap(-0.70f, 0.25f, 0.12f, 0.12f);
        addTrap(0.15f, -0.65f, 0.12f, 0.12f);
        addTrap(0.60f, 0.00f, 0.12f, 0.12f);
        timeLeft = 55;
    }
    else
    {
        proceduralLevel(newLevel);
    }
}

// ============================================================HEALTH BAR / UI
void drawHealthBar()
{
    glColor3f(0.12f, 0.12f, 0.12f);
    drawRectangle(-0.90f, 0.87f, 0.30f, 0.035f);

    float width = 0.30f * health / 100.0f;

    if (health > 60) glColor3f(0.1f, 0.85f, 0.15f);
    else if (health > 30) glColor3f(1.0f, 0.65f, 0.05f);
    else glColor3f(0.9f, 0.05f, 0.05f);

    drawRectangle(-0.90f, 0.87f, width, 0.035f);
}

void drawUI()
{
    char text[100];

    // translucent HUD panel so text stays legible over busy backgrounds
    glColor4f(0.0f, 0.0f, 0.0f, 0.45f);
    drawRectangle(-0.98f, 0.83f, 1.96f, 0.15f);
    glColor3f(0.4f, 0.35f, 0.15f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(-0.98f, 0.83f); glVertex2f(0.98f, 0.83f);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    sprintf(text, "LEVEL %d / %d", level, MAX_LEVEL);
    drawText(-0.90f, 0.94f, text);

    sprintf(text, "SCORE %d", score);
    drawText(-0.50f, 0.94f, text);

    sprintf(text, "TIME %d", timeLeft);
    drawText(0.10f, 0.94f, text);

    drawHealthBar();

    if (isBossLevel(level))
    {
        glColor3f(1.0f, 0.75f, 0.1f);
        drawText(-0.15f, 0.94f, "BOSS", GLUT_BITMAP_HELVETICA_18);
    }

    if (isBossLevel(level) && bossAlive)
    {
        glColor3f(1.0f, 0.3f, 0.3f);
        sprintf(text, "BOSS HP %d / %d", bossHealth, bossMaxHealth);
        drawText(0.28f, -0.92f, text);
    }
}

// ============================================================SCREENS INTRO / MAIN MENU VISUALS
void drawIntroBackground()
{
    // Deep dungeon background
    glColor3f(0.025f, 0.015f, 0.045f);
    drawRectangle(-1.0f, -1.0f, 2.0f, 2.0f);

    // Back wall
    glColor3f(0.08f, 0.035f, 0.11f);
    drawRectangle(-0.88f, -0.78f, 1.76f, 1.58f);

    // Horizontal stone courses
    glColor3f(0.13f, 0.06f, 0.16f);
    for (float y = -0.70f; y <= 0.70f; y += 0.13f)
    {
        glBegin(GL_LINES);
        glVertex2f(-0.88f, y);
        glVertex2f(0.88f, y);
        glEnd();
    }

    // Vertical stone seams
    glColor3f(0.095f, 0.04f, 0.12f);
    for (int row = 0; row < 11; row++)
    {
        float y = -0.64f + row * 0.13f;
        float offset = (row % 2 == 0) ? -0.08f : 0.0f;

        for (float x = -0.82f + offset; x < 0.90f; x += 0.22f)
        {
            glBegin(GL_LINES);
            glVertex2f(x, y);
            glVertex2f(x, y + 0.13f);
            glEnd();
        }
    }

    // Large dungeon doorway
    glColor3f(0.035f, 0.02f, 0.045f);
    glBegin(GL_POLYGON);
    glVertex2f(-0.68f, -0.78f);
    glVertex2f(-0.68f, 0.03f);
    glVertex2f(-0.62f, 0.22f);
    glVertex2f(-0.48f, 0.39f);
    glVertex2f(-0.25f, 0.49f);
    glVertex2f(0.25f, 0.49f);
    glVertex2f(0.48f, 0.39f);
    glVertex2f(0.62f, 0.22f);
    glVertex2f(0.68f, 0.03f);
    glVertex2f(0.68f, -0.78f);
    glEnd();

    // Door inner glow / stone arch
    glColor3f(0.24f, 0.07f, 0.28f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(-0.64f, -0.75f);
    glVertex2f(-0.64f, 0.02f);
    glVertex2f(-0.57f, 0.20f);
    glVertex2f(-0.44f, 0.34f);
    glVertex2f(-0.23f, 0.43f);
    glVertex2f(0.23f, 0.43f);
    glVertex2f(0.44f, 0.34f);
    glVertex2f(0.57f, 0.20f);
    glVertex2f(0.64f, 0.02f);
    glVertex2f(0.64f, -0.75f);
    glEnd();

    // Door vertical slabs
    glColor3f(0.10f, 0.035f, 0.12f);
    for (float x = -0.52f; x <= 0.52f; x += 0.13f)
    {
        glBegin(GL_LINES);
        glVertex2f(x, -0.74f);
        glVertex2f(x, 0.20f);
        glEnd();
    }

    // Floor
    glColor3f(0.055f, 0.035f, 0.065f);
    drawRectangle(-1.0f, -0.98f, 2.0f, 0.22f);

    glColor3f(0.13f, 0.07f, 0.15f);
    for (float x = -1.0f; x < 1.0f; x += 0.16f)
    {
        glBegin(GL_LINES);
        glVertex2f(x, -0.98f);
        glVertex2f(x + 0.08f, -0.76f);
        glEnd();
    }

    // Purple side banners
    glColor3f(0.20f, 0.025f, 0.24f);
    glBegin(GL_POLYGON);
    glVertex2f(-0.96f, 0.72f);
    glVertex2f(-0.76f, 0.72f);
    glVertex2f(-0.79f, -0.10f);
    glVertex2f(-0.87f, -0.20f);
    glVertex2f(-0.96f, -0.05f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(0.76f, 0.72f);
    glVertex2f(0.96f, 0.72f);
    glVertex2f(0.96f, -0.05f);
    glVertex2f(0.87f, -0.20f);
    glVertex2f(0.79f, -0.10f);
    glEnd();

    // Banner emblems
    glColor3f(0.48f, 0.10f, 0.50f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.86f, 0.46f);
    glVertex2f(-0.92f, 0.34f);
    glVertex2f(-0.80f, 0.34f);
    glEnd();

    glBegin(GL_TRIANGLES);
    glVertex2f(0.86f, 0.46f);
    glVertex2f(0.80f, 0.34f);
    glVertex2f(0.92f, 0.34f);
    glEnd();

    // Hanging chains
    glColor3f(0.18f, 0.15f, 0.20f);

float chainX[4] = {-0.92f, -0.72f, 0.72f, 0.92f};

for (int i = 0; i < 4; i++)
{
    float x = chainX[i];

    for (float y = 0.96f; y > 0.58f; y -= 0.055f)
    {
        glBegin(GL_LINE_LOOP);

        glVertex2f(x - 0.012f, y);
        glVertex2f(x + 0.012f, y + 0.015f);
        glVertex2f(x + 0.008f, y + 0.035f);
        glVertex2f(x - 0.008f, y + 0.020f);

        glEnd();
    }
}

// End of drawIntroBackground()
}

void drawMenuTorch(float x, float y)
{
    glPushMatrix();
    glTranslatef(x, y, 0.0f);

    // Stone stand
    glColor3f(0.16f, 0.14f, 0.18f);
    drawRectangle(-0.055f, -0.09f, 0.11f, 0.08f);
    drawRectangle(-0.035f, -0.01f, 0.07f, 0.07f);

    // Torch handle
    glColor3f(0.28f, 0.13f, 0.045f);
    drawRectangle(-0.012f, 0.02f, 0.024f, 0.12f);

    // Outer flame
    float flameSize = 0.052f + 0.008f * sin(animationTime * 8.0f);

    glColor3f(0.95f, 0.18f, 0.02f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-flameSize, 0.13f);
    glVertex2f(0.0f, 0.27f);
    glVertex2f(flameSize, 0.13f);
    glEnd();

    // Inner flame
    glColor3f(1.0f, 0.72f, 0.08f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-flameSize * 0.45f, 0.13f);
    glVertex2f(0.0f, 0.23f);
    glVertex2f(flameSize * 0.45f, 0.13f);
    glEnd();

    glPopMatrix();
}

void drawMenuCrystal()
{
    glPushMatrix();
    glTranslatef(0.0f, 0.68f, 0.0f);

    float pulse = 0.88f + 0.12f * sin(animationTime * 4.0f);

    glColor4f(0.55f, 0.10f, 0.95f, 0.18f * pulse);
    drawCircle(0.12f);

    glColor3f(0.55f, 0.12f, 0.85f);
    glBegin(GL_POLYGON);
    glVertex2f(0.0f, 0.12f);
    glVertex2f(0.075f, 0.03f);
    glVertex2f(0.045f, -0.08f);
    glVertex2f(-0.045f, -0.08f);
    glVertex2f(-0.075f, 0.03f);
    glEnd();

    glColor3f(0.88f, 0.50f, 1.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(0.0f, 0.12f);
    glVertex2f(0.075f, 0.03f);
    glVertex2f(0.0f, 0.00f);
    glEnd();

    glPopMatrix();
}

void drawMenuButton(float y, const char* label, bool selected)
{
    float x = -0.27f;
    float w = 0.54f;
    float h = 0.085f;

    // Button shadow
    glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
    drawRectangle(x + 0.012f, y - 0.012f, w, h);

    if (selected)
    {
        glColor4f(0.35f, 0.08f, 0.50f, 0.90f);
        drawRectangle(x, y, w, h);

        glColor3f(0.78f, 0.28f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w - 0.018f, y + h);
        glVertex2f(x + 0.018f, y + h);
        glEnd();
    }
    else
    {
        glColor4f(0.055f, 0.025f, 0.075f, 0.92f);
        drawRectangle(x, y, w, h);

        glColor3f(0.27f, 0.12f, 0.32f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w - 0.018f, y + h);
        glVertex2f(x + 0.018f, y + h);
        glEnd();
    }

    // Small decorative diamonds
    glColor3f(0.85f, 0.55f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(x + 0.025f, y + h * 0.50f);
    glVertex2f(x + 0.035f, y + h * 0.62f);
    glVertex2f(x + 0.045f, y + h * 0.50f);
    glVertex2f(x + 0.035f, y + h * 0.38f);

    glVertex2f(x + w - 0.045f, y + h * 0.50f);
    glVertex2f(x + w - 0.035f, y + h * 0.62f);
    glVertex2f(x + w - 0.025f, y + h * 0.50f);
    glVertex2f(x + w - 0.035f, y + h * 0.38f);
    glEnd();

    glColor3f(1.0f, 0.88f, 0.68f);
    drawText(x + 0.19f, y + 0.026f, label, GLUT_BITMAP_HELVETICA_18);
}


void drawShopItemCard(float x, float y, float w, float h,
                      const char* title, const char* stat,
                      const char* priceText, bool unlocked, bool equipped,
                      bool selected, bool attackCard)
{
    // Shadow
    glColor4f(0.0f, 0.0f, 0.0f, 0.65f);
    drawRectangle(x + 0.012f, y - 0.012f, w, h);

    // Card body
    if (selected)
    {
        if (attackCard)
            glColor3f(0.17f, 0.055f, 0.23f);
        else
            glColor3f(0.16f, 0.045f, 0.055f);
    }
    else
    {
        glColor3f(0.045f, 0.025f, 0.06f);
    }
    drawRectangle(x, y, w, h);

    // Border
    if (selected)
        glColor3f(0.85f, 0.48f, 0.15f);
    else if (unlocked)
        glColor3f(0.34f, 0.18f, 0.40f);
    else
        glColor3f(0.20f, 0.16f, 0.20f);

    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w - 0.012f, y + h);
    glVertex2f(x + 0.012f, y + h);
    glEnd();

    // Item number / title
    glColor3f(0.92f, 0.86f, 0.72f);
    drawText(x + 0.018f, y + h - 0.045f, title, GLUT_BITMAP_HELVETICA_12);

    // Weapon / shield icon
    glPushMatrix();
    glTranslatef(x + w * 0.50f, y + h * 0.54f, 0.0f);

    if (attackCard)
    {
        // Simple stylized gun
        glColor3f(unlocked ? 0.62f : 0.23f, unlocked ? 0.62f : 0.23f, unlocked ? 0.68f : 0.25f);
        drawRectangle(-0.055f, 0.015f, 0.11f, 0.025f);
        drawRectangle(-0.025f, -0.025f, 0.035f, 0.04f);
        glColor3f(0.85f, 0.55f, 0.18f);
        drawRectangle(0.045f, 0.022f, 0.035f, 0.012f);
    }
    else
    {
        // Simple shield
        glColor3f(unlocked ? 0.55f : 0.24f, unlocked ? 0.38f : 0.20f, unlocked ? 0.15f : 0.18f);
        glBegin(GL_POLYGON);
        glVertex2f(0.0f, 0.065f);
        glVertex2f(0.060f, 0.040f);
        glVertex2f(0.048f, -0.045f);
        glVertex2f(0.0f, -0.075f);
        glVertex2f(-0.048f, -0.045f);
        glVertex2f(-0.060f, 0.040f);
        glEnd();
        glColor3f(0.75f, 0.58f, 0.25f);
        drawRectangle(-0.008f, -0.045f, 0.016f, 0.09f);
    }
    glPopMatrix();

    // Stat
    glColor3f(0.35f, 0.95f, 0.30f);
    drawText(x + 0.018f, y + 0.075f, stat, GLUT_BITMAP_HELVETICA_12);

    // Status / price
    if (equipped)
    {
        glColor3f(0.30f, 1.0f, 0.35f);
        drawText(x + 0.018f, y + 0.028f, "EQUIPPED", GLUT_BITMAP_HELVETICA_12);
    }
    else if (unlocked)
    {
        glColor3f(0.90f, 0.68f, 0.18f);
        drawText(x + 0.018f, y + 0.028f, priceText, GLUT_BITMAP_HELVETICA_12);
    }
    else
    {
        glColor3f(0.55f, 0.52f, 0.56f);
        drawText(x + 0.018f, y + 0.028f, priceText, GLUT_BITMAP_HELVETICA_12);

        // Lock
        glColor3f(0.78f, 0.60f, 0.20f);
        drawRectangle(x + w - 0.055f, y + 0.022f, 0.030f, 0.030f);
        glColor3f(0.05f, 0.03f, 0.05f);
        drawRectangle(x + w - 0.048f, y + 0.030f, 0.016f, 0.022f);
    }
}

void drawShopTopFrame()
{
    glColor3f(0.08f, 0.025f, 0.11f);
    drawRectangle(-0.93f, 0.76f, 1.86f, 0.19f);

    glColor3f(0.43f, 0.16f, 0.50f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.93f, 0.76f);
    glVertex2f(0.93f, 0.76f);
    glVertex2f(0.90f, 0.95f);
    glVertex2f(-0.90f, 0.95f);
    glEnd();

    glColor3f(0.96f, 0.76f, 0.25f);
    drawText(-0.12f, 0.82f, "SHOP", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(0.70f, 0.48f, 0.78f);
    drawText(-0.23f, 0.775f, "UPGRADE YOUR POWER", GLUT_BITMAP_HELVETICA_12);
}

void drawShopSideTab(float y, const char* label, bool selected, bool attack)
{
    if (selected)
    {
        if (attack) glColor3f(0.22f, 0.06f, 0.30f);
        else glColor3f(0.25f, 0.045f, 0.055f);
    }
    else
        glColor3f(0.045f, 0.03f, 0.055f);

    drawRectangle(-0.92f, y, 0.25f, 0.13f);

    if (selected)
        glColor3f(0.88f, attack ? 0.35f : 0.18f, attack ? 1.0f : 0.20f);
    else
        glColor3f(0.30f, 0.24f, 0.32f);

    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.92f, y);
    glVertex2f(-0.67f, y);
    glVertex2f(-0.68f, y + 0.13f);
    glVertex2f(-0.91f, y + 0.13f);
    glEnd();

    glColor3f(0.90f, 0.84f, 0.76f);
    drawText(-0.86f, y + 0.047f, label, GLUT_BITMAP_HELVETICA_18);
}

void drawShopDetailPanel()
{
    float x = 0.72f;
    float y = -0.61f;
    float w = 0.25f;
    float h = 1.27f;

    glColor3f(0.035f, 0.018f, 0.045f);
    drawRectangle(x, y, w, h);

    glColor3f(0.38f, shopTab == 0 ? 0.14f : 0.08f, shopTab == 0 ? 0.50f : 0.10f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();

    if (shopTab == 0)
    {
        const char* names[5] = {
            "PISTOL", "RAPID SMG", "ASSAULT RIFLE",
            "PLASMA RIFLE", "CHAOS CANNON"
        };

        int idx = selectedAttack;
        char buf[80];

        glColor3f(0.72f, 0.38f, 0.90f);
        drawText(x + 0.045f, y + h - 0.07f, "SELECTED WEAPON", GLUT_BITMAP_HELVETICA_12);

        glColor3f(0.96f, 0.78f, 0.30f);
        drawText(x + 0.06f, y + h - 0.13f, names[idx], GLUT_BITMAP_HELVETICA_18);

        glPushMatrix();
        glTranslatef(x + w * 0.50f, y + h - 0.30f, 0.0f);
        glColor3f(0.65f, 0.65f, 0.70f);
        drawRectangle(-0.065f, 0.02f, 0.13f, 0.03f);
        drawRectangle(-0.025f, -0.025f, 0.035f, 0.045f);
        glColor3f(0.85f, 0.55f, 0.18f);
        drawRectangle(0.05f, 0.027f, 0.04f, 0.012f);
        glPopMatrix();

        sprintf(buf, "DAMAGE +%d%%", (int)(attackBonus[idx] * 100.0f));
        glColor3f(0.35f, 1.0f, 0.30f);
        drawText(x + 0.055f, y + h - 0.48f, buf, GLUT_BITMAP_HELVETICA_12);

        if (attackOwned[idx])
        {
            glColor3f(0.30f, 1.0f, 0.35f);
            drawText(x + 0.075f, y + 0.30f,
                     idx == equippedAttack ? "EQUIPPED" : "OWNED",
                     GLUT_BITMAP_HELVETICA_18);
        }
        else
        {
            sprintf(buf, "PRICE: %d", attackPrice[idx]);
            glColor3f(1.0f, 0.72f, 0.20f);
            drawText(x + 0.065f, y + 0.33f, buf, GLUT_BITMAP_HELVETICA_12);

            glColor3f(0.75f, 0.48f, 0.20f);
            drawRectangle(x + 0.045f, y + 0.17f, 0.16f, 0.075f);

            glColor3f(1.0f, 0.90f, 0.65f);
            drawText(x + 0.092f, y + 0.195f, "BUY", GLUT_BITMAP_HELVETICA_18);
        }

        glColor3f(0.68f, 0.50f, 0.74f);
        drawText(x + 0.025f, y + 0.07f,
                 "Press ENTER to", GLUT_BITMAP_HELVETICA_12);
        drawText(x + 0.025f, y + 0.035f,
                 "buy / equip", GLUT_BITMAP_HELVETICA_12);
    }
    else
    {
        const char* names[5] = {
            "BASIC SHIELD", "REINFORCED SHIELD", "ADVANCED SHIELD",
            "ENERGY SHIELD", "ULTIMATE SHIELD"
        };

        int idx = selectedDefense;
        char buf[80];

        glColor3f(0.95f, 0.30f, 0.22f);
        drawText(x + 0.045f, y + h - 0.07f, "SELECTED SHIELD", GLUT_BITMAP_HELVETICA_12);

        glColor3f(0.96f, 0.78f, 0.30f);
        drawText(x + 0.045f, y + h - 0.13f, names[idx], GLUT_BITMAP_HELVETICA_12);

        glPushMatrix();
        glTranslatef(x + w * 0.50f, y + h - 0.31f, 0.0f);
        glColor3f(0.55f, 0.38f, 0.18f);
        glBegin(GL_POLYGON);
        glVertex2f(0.0f, 0.075f);
        glVertex2f(0.07f, 0.045f);
        glVertex2f(0.055f, -0.055f);
        glVertex2f(0.0f, -0.09f);
        glVertex2f(-0.055f, -0.055f);
        glVertex2f(-0.07f, 0.045f);
        glEnd();
        glPopMatrix();

        sprintf(buf, "ABSORBS %d%%", (int)(defenseAbsorb[idx] * 100.0f));
        glColor3f(0.35f, 1.0f, 0.30f);
        drawText(x + 0.06f, y + h - 0.49f, buf, GLUT_BITMAP_HELVETICA_12);

        if (defenseOwned[idx])
        {
            glColor3f(0.30f, 1.0f, 0.35f);
            drawText(x + 0.075f, y + 0.30f,
                     idx == equippedDefense ? "EQUIPPED" : "OWNED",
                     GLUT_BITMAP_HELVETICA_18);
        }
        else
        {
            sprintf(buf, "PRICE: %d", defensePrice[idx]);
            glColor3f(1.0f, 0.72f, 0.20f);
            drawText(x + 0.065f, y + 0.33f, buf, GLUT_BITMAP_HELVETICA_12);

            glColor3f(0.75f, 0.48f, 0.20f);
            drawRectangle(x + 0.045f, y + 0.17f, 0.16f, 0.075f);

            glColor3f(1.0f, 0.90f, 0.65f);
            drawText(x + 0.092f, y + 0.195f, "BUY", GLUT_BITMAP_HELVETICA_18);
        }

        glColor3f(0.78f, 0.48f, 0.48f);
        drawText(x + 0.018f, y + 0.07f,
                 "Shield breaks after", GLUT_BITMAP_HELVETICA_12);
        drawText(x + 0.018f, y + 0.035f,
                 "absorbing damage.", GLUT_BITMAP_HELVETICA_12);
    }
}

void drawShopScreen()
{
    glClear(GL_COLOR_BUFFER_BIT);

    drawIntroBackground();

    // Dark shop overlay
    glColor4f(0.01f, 0.005f, 0.015f, 0.80f);
    drawRectangle(-1.0f, -1.0f, 2.0f, 2.0f);

    drawShopTopFrame();

    char scoreText[80];
    sprintf(scoreText, "SCORE: %d", score);
    glColor3f(1.0f, 0.78f, 0.25f);
    drawText(0.55f, 0.85f, scoreText, GLUT_BITMAP_HELVETICA_18);

    // ========================================================STAGE 1: CATEGORY SELECTION
    // UP   = Attack,DOWN = Defense,ENTER = Enter selected category,B = Back
    if (shopMode == 0)
    {
        glColor3f(0.75f, 0.52f, 0.84f);
        drawText(-0.25f, 0.64f, "SELECT CATEGORY", GLUT_BITMAP_TIMES_ROMAN_24);

        // Attack card
        bool attackSelected = (shopTab == 0);

        if (attackSelected)
            glColor3f(0.22f, 0.055f, 0.30f);
        else
            glColor3f(0.045f, 0.025f, 0.055f);

        drawRectangle(-0.55f, 0.16f, 1.10f, 0.22f);

        glColor3f(attackSelected ? 0.90f : 0.32f,
                  attackSelected ? 0.35f : 0.22f,
                  attackSelected ? 1.00f : 0.38f);

        glBegin(GL_LINE_LOOP);
        glVertex2f(-0.55f, 0.16f);
        glVertex2f(0.55f, 0.16f);
        glVertex2f(0.55f, 0.38f);
        glVertex2f(-0.55f, 0.38f);
        glEnd();

        glColor3f(0.95f, 0.84f, 0.70f);
        drawText(-0.15f, 0.24f, "ATTACK", GLUT_BITMAP_TIMES_ROMAN_24);

        // Defense card
        bool defenseSelected = (shopTab == 1);

        if (defenseSelected)
            glColor3f(0.30f, 0.055f, 0.065f);
        else
            glColor3f(0.045f, 0.025f, 0.055f);

        drawRectangle(-0.55f, -0.15f, 1.10f, 0.22f);

        glColor3f(defenseSelected ? 1.00f : 0.32f,
                  defenseSelected ? 0.30f : 0.22f,
                  defenseSelected ? 0.25f : 0.25f);

        glBegin(GL_LINE_LOOP);
        glVertex2f(-0.55f, -0.15f);
        glVertex2f(0.55f, -0.15f);
        glVertex2f(0.55f, 0.07f);
        glVertex2f(-0.55f, 0.07f);
        glEnd();

        glColor3f(0.95f, 0.84f, 0.70f);
        drawText(-0.17f, -0.07f, "DEFENSE", GLUT_BITMAP_TIMES_ROMAN_24);

        glColor3f(0.80f, 0.68f, 0.48f);
        drawText(-0.30f, -0.38f,
                 "UP / DOWN = SELECT    ENTER = OPEN",
                 GLUT_BITMAP_HELVETICA_18);

        glColor3f(0.55f, 0.35f, 0.62f);
        drawText(-0.23f, -0.48f,
                 "B = BACK TO MENU",
                 GLUT_BITMAP_HELVETICA_12);

        glFlush();
        return;
    }

    // ========================================================STAGE 2: ITEM SELECTION
    // LEFT / RIGHT = item,ENTER = buy/equip,UP = Attack category,DOWN = Defense category,B = back to category screen
    drawShopSideTab(0.51f, "ATTACK", shopTab == 0, true);
    drawShopSideTab(0.35f, "DEFENSE", shopTab == 1, false);

    glColor3f(0.72f, 0.45f, 0.82f);
    if (shopTab == 0)
        drawText(-0.88f, 0.18f, "WEAPONS", GLUT_BITMAP_HELVETICA_18);
    else
    {
        glColor3f(0.88f, 0.30f, 0.25f);
        drawText(-0.88f, 0.18f, "SHIELDS", GLUT_BITMAP_HELVETICA_18);
    }

    float cardX = -0.61f;
    float cardY = -0.08f;
    float cardW = 0.255f;
    float cardH = 0.56f;
    float gap = 0.012f;

    const char* attackNames[5] = {
        "1. PISTOL", "2. RAPID SMG", "3. ASSAULT RIFLE",
        "4. PLASMA RIFLE", "5. CHAOS CANNON"
    };

    const char* defenseNames[5] = {
        "1. BASIC SHIELD", "2. REINFORCED SHIELD", "3. ADVANCED SHIELD",
        "4. ENERGY SHIELD", "5. ULTIMATE SHIELD"
    };

    for (int i = 0; i < 5; i++)
    {
        float x = cardX + i * (cardW + gap);

        char stat[40];
        char price[40];

        if (shopTab == 0)
        {
            sprintf(stat, "DAMAGE +%d%%", (int)(attackBonus[i] * 100.0f));

            if (i == 0)
                sprintf(price, "FREE / STARTER");
            else
                sprintf(price, "%d SCORE", attackPrice[i]);

            drawShopItemCard(
                x, cardY, cardW, cardH,
                attackNames[i], stat, price,
                attackOwned[i], i == equippedAttack,
                i == selectedAttack, true
            );
        }
        else
        {
            sprintf(stat, "ABSORB %d%%", (int)(defenseAbsorb[i] * 100.0f));

            if (i == 0)
                sprintf(price, "FREE / STARTER");
            else
                sprintf(price, "%d SCORE", defensePrice[i]);

            drawShopItemCard(
                x, cardY, cardW, cardH,
                defenseNames[i], stat, price,
                defenseOwned[i], i == equippedDefense,
                i == selectedDefense, false
            );
        }
    }

    glColor3f(0.80f, 0.68f, 0.48f);
    drawText(-0.60f, -0.70f,
             "LEFT / RIGHT = SELECT ITEM     ENTER = BUY / EQUIP",
             GLUT_BITMAP_HELVETICA_12);

    glColor3f(0.62f, 0.42f, 0.70f);
    drawText(-0.44f, -0.79f,
             "B = CATEGORIES     UP / DOWN = SWITCH CATEGORY",
             GLUT_BITMAP_HELVETICA_12);

    drawShopDetailPanel();

    glFlush();
}


void drawIntro()
{
    glClear(GL_COLOR_BUFFER_BIT);

    drawIntroBackground();

    drawMenuTorch(-0.72f, -0.43f);
    drawMenuTorch(0.72f, -0.43f);

    drawMenuCrystal();

    // Main title
    glColor3f(0.92f, 0.90f, 0.86f);
    drawText(-0.40f, 0.48f, "DUNGEON CRAWLER", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(0.90f, 0.52f, 0.12f);
    drawText(-0.27f, 0.37f, "THE DARK REALM", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(0.63f, 0.35f, 0.72f);
    drawText(-0.26f, 0.29f, "SURVIVE THE DUNGEON");

    // Menu
    drawMenuButton(0.10f, "START GAME", menuSelection == 0);
    drawMenuButton(-0.01f, "SHOP", menuSelection == 1);
    drawMenuButton(-0.12f, "INSTRUCTIONS", menuSelection == 2);
    drawMenuButton(-0.23f, "EXIT GAME", menuSelection == 3);

    glColor3f(0.72f, 0.64f, 0.76f);
    drawText(-0.43f, -0.47f, "UP / DOWN = SELECT     ENTER = CONFIRM");

    glColor3f(0.50f, 0.25f, 0.60f);
    drawText(-0.35f, -0.62f, "20 LEVELS  |  4 BOSSES  |  NO MERCY");

    glColor3f(0.58f, 0.40f, 0.72f);
    drawText(-0.37f, -0.82f, "SURVIVE THE DUNGEON. DEFEAT THE DARKNESS.");

    glFlush();
}


void drawInstructions()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.75f, 0.55f, 0.15f);
    drawText(-0.30f, 0.50f, "HOW TO PLAY");

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(-0.50f, 0.28f, "W / A / X / D  -  MOVE (also aims)");
    drawText(-0.50f, 0.16f, "SPACE           -  SHOOT (10% dmg/hit)");
    drawText(-0.50f, 0.04f, "P               -  PAUSE");
    drawText(-0.50f, -0.08f, "R               -  RESTART");
    drawText(-0.50f, -0.20f, "Collect the key and open the door");
    drawText(-0.50f, -0.32f, "Enemies chase fast - dodge and shoot");
    drawText(-0.50f, -0.44f, "Enemy speed rises by 50% each level");

    glColor3f(0.9f, 0.2f, 0.2f);
    drawText(-0.35f, -0.65f, "PRESS ENTER");

    glFlush();
}

void drawLevelComplete()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // ========================================================LEVEL COMPLETE - DUNGEON STYLE VICTORY SCREEN
    // Dark layered background
    glColor3f(0.018f, 0.012f, 0.030f);
    drawRectangle(-1.0f, -1.0f, 2.0f, 2.0f);

    glColor3f(0.055f, 0.025f, 0.075f);
    drawRectangle(-0.92f, -0.86f, 1.84f, 1.72f);

    // Stone wall lines
    glColor3f(0.10f, 0.045f, 0.13f);
    for (float y = -0.75f; y <= 0.75f; y += 0.14f)
    {
        glBegin(GL_LINES);
        glVertex2f(-0.92f, y);
        glVertex2f(0.92f, y);
        glEnd();
    }

    glColor3f(0.075f, 0.032f, 0.10f);
    for (int row = 0; row < 11; row++)
    {
        float y = -0.68f + row * 0.14f;
        float offset = (row % 2 == 0) ? -0.10f : 0.0f;

        for (float x = -0.86f + offset; x < 0.95f; x += 0.24f)
        {
            glBegin(GL_LINES);
            glVertex2f(x, y);
            glVertex2f(x, y + 0.14f);
            glEnd();
        }
    }

    // Large central magical arch
    glColor3f(0.025f, 0.012f, 0.035f);
    glBegin(GL_POLYGON);
    glVertex2f(-0.70f, -0.72f);
    glVertex2f(-0.70f, 0.00f);
    glVertex2f(-0.64f, 0.22f);
    glVertex2f(-0.50f, 0.39f);
    glVertex2f(-0.28f, 0.49f);
    glVertex2f(0.28f, 0.49f);
    glVertex2f(0.50f, 0.39f);
    glVertex2f(0.64f, 0.22f);
    glVertex2f(0.70f, 0.00f);
    glVertex2f(0.70f, -0.72f);
    glEnd();

    glColor3f(0.25f, 0.08f, 0.30f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(-0.65f, -0.70f);
    glVertex2f(-0.65f, 0.00f);
    glVertex2f(-0.59f, 0.19f);
    glVertex2f(-0.45f, 0.34f);
    glVertex2f(-0.25f, 0.44f);
    glVertex2f(0.25f, 0.44f);
    glVertex2f(0.45f, 0.34f);
    glVertex2f(0.59f, 0.19f);
    glVertex2f(0.65f, 0.00f);
    glVertex2f(0.65f, -0.70f);
    glEnd();

    // Animated purple magical glow
    float pulse = 0.5f + 0.5f * sin(animationTime * 4.0f);

    glColor4f(0.55f, 0.12f, 0.90f, 0.10f + pulse * 0.06f);
    drawCircle(0.42f);

    glColor4f(0.70f, 0.25f, 1.0f, 0.08f + pulse * 0.05f);
    drawCircle(0.30f);

    // Central victory seal
    glPushMatrix();
    glTranslatef(0.0f, 0.12f, 0.0f);
    glRotatef(animationTime * 12.0f, 0.0f, 0.0f, 1.0f);

    glColor3f(0.55f, 0.30f, 0.75f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 8; i++)
    {
        float a = i * 6.283185f / 8.0f + 0.392699f;
        glVertex2f(0.19f * cos(a), 0.19f * sin(a));
    }
    glEnd();

    glColor3f(0.85f, 0.58f, 0.15f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 8; i++)
    {
        float a = i * 6.283185f / 8.0f;
        glVertex2f(0.145f * cos(a), 0.145f * sin(a));
    }
    glEnd();

    glPopMatrix();

    // Shield / check mark
    glColor3f(0.78f, 0.50f, 0.12f);
    glBegin(GL_POLYGON);
    glVertex2f(0.0f, 0.30f);
    glVertex2f(0.105f, 0.25f);
    glVertex2f(0.085f, 0.08f);
    glVertex2f(0.0f, -0.02f);
    glVertex2f(-0.085f, 0.08f);
    glVertex2f(-0.105f, 0.25f);
    glEnd();

    glColor3f(0.20f, 0.10f, 0.24f);
    glBegin(GL_LINES);
    glVertex2f(-0.055f, 0.15f);
    glVertex2f(-0.015f, 0.105f);
    glVertex2f(-0.015f, 0.105f);
    glVertex2f(0.065f, 0.21f);
    glEnd();

    // Decorative floating sparks
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    glColor3f(0.90f, 0.65f, 0.20f);

    for (int i = 0; i < 18; i++)
    {
        float a = i * 0.92f;
        float radius = 0.30f + 0.035f * sin(animationTime * 2.0f + i);
        float px = cos(a + animationTime * 0.25f) * radius;
        float py = 0.10f + sin(a + animationTime * 0.25f) * radius * 0.65f;

        if (fabs(px) < 0.22f && py > -0.02f && py < 0.30f)
            continue;

        glVertex2f(px, py);
    }

    glEnd();
    glPointSize(1.0f);

    // Top decorative line
    glColor3f(0.65f, 0.35f, 0.78f);
    glBegin(GL_LINES);
    glVertex2f(-0.52f, 0.67f);
    glVertex2f(-0.18f, 0.67f);
    glVertex2f(0.18f, 0.67f);
    glVertex2f(0.52f, 0.67f);
    glEnd();

    glColor3f(0.88f, 0.58f, 0.16f);
    glBegin(GL_QUADS);
    glVertex2f(-0.055f, 0.67f);
    glVertex2f(0.0f, 0.72f);
    glVertex2f(0.055f, 0.67f);
    glVertex2f(0.0f, 0.62f);
    glEnd();

    // Main title
    glColor3f(0.95f, 0.90f, 0.72f);
    drawText(-0.38f, 0.52f, "LEVEL COMPLETE", GLUT_BITMAP_TIMES_ROMAN_24);

    // Subtitle
    glColor3f(0.67f, 0.35f, 0.82f);
    drawText(-0.27f, 0.43f, "DUNGEON CLEARED");

    // Level information panel
    glColor4f(0.015f, 0.008f, 0.025f, 0.90f);
    drawRectangle(-0.34f, -0.40f, 0.68f, 0.20f);

    glColor3f(0.38f, 0.18f, 0.46f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.34f, -0.40f);
    glVertex2f(0.34f, -0.40f);
    glVertex2f(0.34f, -0.20f);
    glVertex2f(-0.34f, -0.20f);
    glEnd();

    char text[100];
    sprintf(text, "LEVEL %d  CLEARED", level);

    glColor3f(0.95f, 0.92f, 0.85f);
    drawText(-0.20f, -0.28f, text);

    // Small score display
    sprintf(text, "SCORE  %d", score);
    glColor3f(0.85f, 0.60f, 0.18f);
    drawText(-0.15f, -0.36f, text);

    // Next-level button
    float buttonY = -0.57f;
    glColor4f(0.0f, 0.0f, 0.0f, 0.65f);
    drawRectangle(-0.34f, buttonY - 0.012f, 0.68f, 0.095f);

    glColor3f(0.25f, 0.08f, 0.38f);
    drawRectangle(-0.34f, buttonY, 0.68f, 0.095f);

    glColor3f(0.75f, 0.30f, 0.95f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.34f, buttonY);
    glVertex2f(0.34f, buttonY);
    glVertex2f(0.32f, buttonY + 0.095f);
    glVertex2f(-0.32f, buttonY + 0.095f);
    glEnd();

    glColor3f(1.0f, 0.88f, 0.65f);
    drawText(-0.28f, buttonY + 0.030f,
             "ENTER  -  NEXT LEVEL", GLUT_BITMAP_HELVETICA_18);

    // Bottom hint
    glColor3f(0.45f, 0.30f, 0.52f);
    drawText(-0.36f, -0.78f, "THE PATH DEEPENS... PREPARE YOURSELF");

    glFlush();
}

void drawGameOver()
{
    glClear(GL_COLOR_BUFFER_BIT);
    // ========================================================   // LEVEL FAILED - DARK / RED DUNGEON STYLE SCREEN

    // Deep red-black background
    glColor3f(0.025f, 0.008f, 0.010f);
    drawRectangle(-1.0f, -1.0f, 2.0f, 2.0f);

    glColor3f(0.075f, 0.018f, 0.020f);
    drawRectangle(-0.92f, -0.86f, 1.84f, 1.72f);

    // Stone wall
    glColor3f(0.13f, 0.035f, 0.035f);
    for (float y = -0.75f; y <= 0.75f; y += 0.14f)
    {
        glBegin(GL_LINES);
        glVertex2f(-0.92f, y);
        glVertex2f(0.92f, y);
        glEnd();
    }

    glColor3f(0.095f, 0.025f, 0.028f);
    for (int row = 0; row < 11; row++)
    {
        float y = -0.68f + row * 0.14f;
        float offset = (row % 2 == 0) ? -0.10f : 0.0f;

        for (float x = -0.86f + offset; x < 0.95f; x += 0.24f)
        {
            glBegin(GL_LINES);
            glVertex2f(x, y);
            glVertex2f(x, y + 0.14f);
            glEnd();
        }
    }

    // Central dark arch
    glColor3f(0.035f, 0.008f, 0.010f);
    glBegin(GL_POLYGON);
    glVertex2f(-0.70f, -0.73f);
    glVertex2f(-0.70f, 0.00f);
    glVertex2f(-0.64f, 0.22f);
    glVertex2f(-0.50f, 0.39f);
    glVertex2f(-0.28f, 0.49f);
    glVertex2f(0.28f, 0.49f);
    glVertex2f(0.50f, 0.39f);
    glVertex2f(0.64f, 0.22f);
    glVertex2f(0.70f, 0.00f);
    glVertex2f(0.70f, -0.73f);
    glEnd();

    // Red arch outline
    glColor3f(0.34f, 0.055f, 0.055f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(-0.65f, -0.70f);
    glVertex2f(-0.65f, 0.00f);
    glVertex2f(-0.59f, 0.19f);
    glVertex2f(-0.45f, 0.34f);
    glVertex2f(-0.25f, 0.44f);
    glVertex2f(0.25f, 0.44f);
    glVertex2f(0.45f, 0.34f);
    glVertex2f(0.59f, 0.19f);
    glVertex2f(0.65f, 0.00f);
    glVertex2f(0.65f, -0.70f);
    glEnd();

    // Pulsing red danger glow
    float pulse = 0.5f + 0.5f * sin(animationTime * 4.0f);

    glColor4f(0.90f, 0.03f, 0.02f, 0.08f + pulse * 0.05f);
    drawCircle(0.43f);

    glColor4f(1.0f, 0.08f, 0.02f, 0.05f + pulse * 0.04f);
    drawCircle(0.30f);

    // Top decorative lines
    glColor3f(0.52f, 0.12f, 0.10f);
    glBegin(GL_LINES);
    glVertex2f(-0.52f, 0.67f);
    glVertex2f(-0.18f, 0.67f);
    glVertex2f(0.18f, 0.67f);
    glVertex2f(0.52f, 0.67f);
    glEnd();

    glColor3f(0.85f, 0.18f, 0.08f);
    glBegin(GL_QUADS);
    glVertex2f(-0.055f, 0.67f);
    glVertex2f(0.0f, 0.72f);
    glVertex2f(0.055f, 0.67f);
    glVertex2f(0.0f, 0.62f);
    glEnd();

    // Skull-like failure emblem
    glColor3f(0.43f, 0.38f, 0.34f);
    glBegin(GL_POLYGON);
    glVertex2f(-0.105f, 0.43f);
    glVertex2f(-0.075f, 0.49f);
    glVertex2f(0.0f, 0.52f);
    glVertex2f(0.075f, 0.49f);
    glVertex2f(0.105f, 0.43f);
    glVertex2f(0.075f, 0.35f);
    glVertex2f(-0.075f, 0.35f);
    glEnd();

    // Skull horns
    glColor3f(0.58f, 0.12f, 0.08f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.08f, 0.46f);
    glVertex2f(-0.19f, 0.53f);
    glVertex2f(-0.10f, 0.39f);

    glVertex2f(0.08f, 0.46f);
    glVertex2f(0.19f, 0.53f);
    glVertex2f(0.10f, 0.39f);
    glEnd();

    // Glowing skull eyes
    glColor3f(1.0f, 0.08f, 0.03f);
    drawRectangle(-0.062f, 0.405f, 0.038f, 0.025f);
    drawRectangle(0.024f, 0.405f, 0.038f, 0.025f);

    // Skull mouth
    glColor3f(0.10f, 0.015f, 0.015f);
    for (float x = -0.045f; x <= 0.025f; x += 0.023f)
        drawRectangle(x, 0.355f, 0.012f, 0.025f);

    // Main title
    glColor3f(1.0f, 0.30f, 0.20f);
    drawText(-0.34f, 0.29f, "LEVEL FAILED", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(0.72f, 0.12f, 0.10f);
    drawText(-0.12f, 0.20f, "DEFEAT");

    // Result panel
    glColor4f(0.018f, 0.005f, 0.008f, 0.94f);
    drawRectangle(-0.35f, -0.32f, 0.70f, 0.40f);

    glColor3f(0.34f, 0.08f, 0.07f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.35f, -0.32f);
    glVertex2f(0.35f, -0.32f);
    glVertex2f(0.35f, 0.08f);
    glVertex2f(-0.35f, 0.08f);
    glEnd();

    // Panel separators
    glColor3f(0.22f, 0.055f, 0.055f);
    glBegin(GL_LINES);
    glVertex2f(-0.29f, -0.01f);
    glVertex2f(0.29f, -0.01f);
    glVertex2f(-0.29f, -0.14f);
    glVertex2f(0.29f, -0.14f);
    glEnd();

    char text[100];

    sprintf(text, "LEVEL %d", level);
    glColor3f(0.90f, 0.82f, 0.75f);
    drawText(-0.24f, 0.015f, text);

    sprintf(text, "%d", score);
    glColor3f(1.0f, 0.66f, 0.18f);
    drawText(0.20f, 0.015f, text);

    glColor3f(0.90f, 0.72f, 0.30f);
    drawText(-0.24f, -0.115f, "SCORE");

    glColor3f(1.0f, 0.16f, 0.12f);
    drawText(0.16f, -0.115f, "0%");

    glColor3f(0.88f, 0.16f, 0.15f);
    drawText(-0.24f, -0.245f, "REMAINING HEALTH");

    // Retry button
    float retryY = -0.49f;

    glColor4f(0.0f, 0.0f, 0.0f, 0.70f);
    drawRectangle(-0.36f, retryY - 0.012f, 0.72f, 0.085f);

    glColor3f(0.34f, 0.045f, 0.045f);
    drawRectangle(-0.36f, retryY, 0.72f, 0.085f);

    glColor3f(0.88f, 0.20f, 0.16f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.36f, retryY);
    glVertex2f(0.36f, retryY);
    glVertex2f(0.34f, retryY + 0.085f);
    glVertex2f(-0.34f, retryY + 0.085f);
    glEnd();

    glColor3f(1.0f, 0.84f, 0.68f);
    drawText(-0.23f, retryY + 0.027f,
             "R - RETRY LEVEL", GLUT_BITMAP_HELVETICA_18);

    // Exit button
    float exitY = -0.62f;

    glColor4f(0.0f, 0.0f, 0.0f, 0.70f);
    drawRectangle(-0.36f, exitY - 0.012f, 0.72f, 0.075f);

    glColor3f(0.12f, 0.025f, 0.025f);
    drawRectangle(-0.36f, exitY, 0.72f, 0.075f);

    glColor3f(0.52f, 0.15f, 0.12f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.36f, exitY);
    glVertex2f(0.36f, exitY);
    glVertex2f(0.34f, exitY + 0.075f);
    glVertex2f(-0.34f, exitY + 0.075f);
    glEnd();

    glColor3f(0.92f, 0.70f, 0.62f);
    drawText(-0.16f, exitY + 0.024f,
             "ESC - EXIT GAME", GLUT_BITMAP_HELVETICA_18);

    // Floating red embers
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    glColor3f(1.0f, 0.22f, 0.08f);

    for (int i = 0; i < 20; i++)
    {
        float a = i * 0.83f;
        float radius = 0.34f + 0.04f * sin(animationTime * 2.0f + i);
        float px = cos(a + animationTime * 0.20f) * radius;
        float py = 0.02f + sin(a + animationTime * 0.20f) * radius * 0.70f;

        if (fabs(px) < 0.25f && py > -0.28f && py < 0.20f)
            continue;

        glVertex2f(px, py);
    }

    glEnd();
    glPointSize(1.0f);

    // Bottom message
    glColor3f(0.66f, 0.25f, 0.23f);
    drawText(-0.37f, -0.80f,
             "EVERY FALL MAKES YOU STRONGER. TRY AGAIN, WARRIOR!");

    glFlush();
}

void drawWin()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.9f, 0.70f, 0.10f);
    drawText(-0.20f, 0.30f, "VICTORY!");

    glColor3f(0.2f, 0.9f, 0.3f);
    drawText(-0.44f, 0.08f, "ALL 20 LEVELS CLEARED");

    char text[100];
    sprintf(text, "FINAL SCORE: %d", score);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(-0.30f, -0.12f, text);
    drawText(-0.40f, -0.35f, "PRESS R TO PLAY AGAIN");

    glFlush();
}

// ============================================================DISPLAY
void display()
{
    if (gameState == INTRO) { drawIntro(); return; }
    if (gameState == SHOP) { drawShopScreen(); return; }
    if (gameState == INSTRUCTIONS) { drawInstructions(); return; }
    if (gameState == LEVEL_COMPLETE) { drawLevelComplete(); return; }
    if (gameState == GAME_OVER) { drawGameOver(); return; }
    if (gameState == WIN) { drawWin(); return; }

    glClear(GL_COLOR_BUFFER_BIT);

    float sx = (shakeTimer > 0) ? randRange(-shakeStrength, shakeStrength) : 0.0f;
    float sy = (shakeTimer > 0) ? randRange(-shakeStrength, shakeStrength) : 0.0f;

    glPushMatrix();
    glTranslatef(sx, sy, 0.0f);

    drawFloor();
    drawWalls();

    drawTorch(-0.80f, 0.65f);
    drawTorch(0.75f, 0.65f);
    drawTorch(-0.80f, -0.65f);
    drawTorch(0.75f, -0.65f);

    drawTraps();
    drawKey();
    drawDoor();

    for (int i = 0; i < enemyCount; i++) drawEnemy(i);

    if (isBossLevel(level)) drawBoss();

    drawPlayer();
    drawBullets();
    drawParticles();

    glPopMatrix();

    drawUI();

    if (paused)
    {
        glColor3f(1.0f, 0.8f, 0.1f);
        drawText(-0.15f, 0.05f, "PAUSED");
    }

    // damage flash overlay, drawn last so it sits over everything
    if (flashTimer > 0.0f)
    {
        glColor4f(0.8f, 0.05f, 0.05f, (flashTimer / 0.35f) * 0.35f);
        drawRectangle(-1.0f, -1.0f, 2.0f, 2.0f);
    }

    glFlush();
}


// ============================================================MOUSE INTERACTION
void mouseToWorld(int x, int y, float& worldX, float& worldY)
{
    int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
    int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

    if (windowWidth <= 0) windowWidth = 900;
    if (windowHeight <= 0) windowHeight = 700;

    worldX = -1.0f + (2.0f * x) / (float)windowWidth;
    worldY = 1.0f - (2.0f * y) / (float)windowHeight;
}

bool insideRect(float px, float py, float left, float bottom,
                float width, float height)
{
    return px >= left && px <= left + width &&
           py >= bottom && py <= bottom + height;
}

void mouseSelectMenu(float mx, float my)
{
    if (mx < -0.27f || mx > 0.27f)
        return;

    if (insideRect(mx, my, -0.27f, 0.10f, 0.54f, 0.085f))
        menuSelection = 0;
    else if (insideRect(mx, my, -0.27f, -0.01f, 0.54f, 0.085f))
        menuSelection = 1;
    else if (insideRect(mx, my, -0.27f, -0.12f, 0.54f, 0.085f))
        menuSelection = 2;
    else if (insideRect(mx, my, -0.27f, -0.23f, 0.54f, 0.085f))
        menuSelection = 3;
}

void activateMenuSelection()
{
    if (menuSelection == 0)
    {
        srand((unsigned)time(0));
        createLevel(1);
        gameState = PLAYING;
    }
    else if (menuSelection == 1)
    {
        shopMode = 0;
        shopTab = 0;
        gameState = SHOP;
    }
    else if (menuSelection == 2)
    {
        gameState = INSTRUCTIONS;
    }
    else if (menuSelection == 3)
    {
        exit(0);
    }

    glutPostRedisplay();
}

void buyOrEquipSelectedShopItem()
{
    if (shopTab == 0)
    {
        int i = selectedAttack;

        if (attackOwned[i])
            equippedAttack = i;
        else if (score >= attackPrice[i])
        {
            score -= attackPrice[i];
            attackOwned[i] = true;
            equippedAttack = i;
        }
    }
    else
    {
        int i = selectedDefense;

        if (defenseOwned[i])
        {
            equippedDefense = i;
            shieldCapacity = 1.0f;
        }
        else if (score >= defensePrice[i])
        {
            score -= defensePrice[i];
            defenseOwned[i] = true;
            equippedDefense = i;
            shieldCapacity = 1.0f;
        }
    }
}

void mouse(int button, int state, int x, int y)
{
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
        return;

    float mx, my;
    mouseToWorld(x, y, mx, my);

    if (gameState == INTRO)
    {
        mouseSelectMenu(mx, my);
        activateMenuSelection();
        return;
    }

    if (gameState == SHOP)
    {
        if (shopMode == 0)
        {
            if (insideRect(mx, my, -0.55f, 0.16f, 1.10f, 0.22f))
            {
                shopTab = 0;
                shopMode = 1;
                selectedAttack = 0;
            }
            else if (insideRect(mx, my, -0.55f, -0.15f, 1.10f, 0.22f))
            {
                shopTab = 1;
                shopMode = 1;
                selectedDefense = 0;
            }

            glutPostRedisplay();
            return;
        }

        if (insideRect(mx, my, -0.92f, 0.51f, 0.25f, 0.13f))
        {
            shopTab = 0;
            glutPostRedisplay();
            return;
        }

        if (insideRect(mx, my, -0.92f, 0.35f, 0.25f, 0.13f))
        {
            shopTab = 1;
            glutPostRedisplay();
            return;
        }

        float cardX = -0.61f;
        float cardY = -0.08f;
        float cardW = 0.255f;
        float cardH = 0.56f;
        float gap = 0.012f;

        for (int i = 0; i < 5; i++)
        {
            float cardLeft = cardX + i * (cardW + gap);

            if (insideRect(mx, my, cardLeft, cardY, cardW, cardH))
            {
                if (shopTab == 0)
                    selectedAttack = i;
                else
                    selectedDefense = i;

                buyOrEquipSelectedShopItem();
                glutPostRedisplay();
                return;
            }
        }

        // Click the BUY button in the right-side detail panel.
        if (insideRect(mx, my, 0.765f, -0.44f, 0.16f, 0.075f))
        {
            buyOrEquipSelectedShopItem();
            glutPostRedisplay();
            return;
        }

        return;
    }

    if (gameState == PLAYING && !paused)
    {
        float dx = mx - playerX;
        float dy = my - playerY;
        float distance = sqrt(dx * dx + dy * dy);

        if (distance > 0.001f)
        {
            facingX = dx / distance;
            facingY = dy / distance;
        }

        if (fireCooldownTicks <= 0)
        {
            fireBullet();
            fireCooldownTicks = 2;
        }

        glutPostRedisplay();
        return;
    }
}

void mouseMotion(int x, int y)
{
    float mx, my;
    mouseToWorld(x, y, mx, my);

    if (gameState == INTRO)
    {
        int oldSelection = menuSelection;
        mouseSelectMenu(mx, my);

        if (oldSelection != menuSelection)
            glutPostRedisplay();
    }
    else if (gameState == SHOP)
    {
        if (shopMode == 0)
        {
            int oldTab = shopTab;

            if (insideRect(mx, my, -0.55f, 0.16f, 1.10f, 0.22f))
                shopTab = 0;
            else if (insideRect(mx, my, -0.55f, -0.15f, 1.10f, 0.22f))
                shopTab = 1;

            if (oldTab != shopTab)
                glutPostRedisplay();
        }
        else
        {
            int oldAttack = selectedAttack;
            int oldDefense = selectedDefense;

            float cardX = -0.61f;
            float cardY = -0.08f;
            float cardW = 0.255f;
            float cardH = 0.56f;
            float gap = 0.012f;

            for (int i = 0; i < 5; i++)
            {
                float cardLeft = cardX + i * (cardW + gap);

                if (insideRect(mx, my, cardLeft, cardY, cardW, cardH))
                {
                    if (shopTab == 0)
                        selectedAttack = i;
                    else
                        selectedDefense = i;
                    break;
                }
            }

            if (oldAttack != selectedAttack || oldDefense != selectedDefense)
                glutPostRedisplay();
        }
    }
}

// ============================================================
// KEYBOARD
// ============================================================

void specialKeyboard(int key, int x, int y)
{
    // ========================================================MAIN MENU,UP / DOWN = select menu option,ENTER is handled by normal keyboard()
    if (gameState == INTRO)
    {
        if (key == GLUT_KEY_UP)
        {
            menuSelection--;

            if (menuSelection < 0)
                menuSelection = 3;

            glutPostRedisplay();
            return;
        }

        if (key == GLUT_KEY_DOWN)
        {
            menuSelection++;

            if (menuSelection > 3)
                menuSelection = 0;

            glutPostRedisplay();
            return;
        }

        return;
    }

    // ========================================================SHOP
    // Category screen:UP   = ATTACK,DOWN = DEFENSE,ENTER = OPEN
    // Item screen:LEFT/RIGHT = select weapon/shield,UP/DOWN = switch category,ENTER = buy/equip
    if (gameState == SHOP)
    {
        if (shopMode == 0)
        {
            if (key == GLUT_KEY_UP)
            {
                shopTab = 0;
                glutPostRedisplay();
                return;
            }

            if (key == GLUT_KEY_DOWN)
            {
                shopTab = 1;
                glutPostRedisplay();
                return;
            }

            return;
        }

        if (key == GLUT_KEY_LEFT)
        {
            if (shopTab == 0)
            {
                selectedAttack--;
                if (selectedAttack < 0)
                    selectedAttack = 4;
            }
            else
            {
                selectedDefense--;
                if (selectedDefense < 0)
                    selectedDefense = 4;
            }

            glutPostRedisplay();
            return;
        }

        if (key == GLUT_KEY_RIGHT)
        {
            if (shopTab == 0)
            {
                selectedAttack++;
                if (selectedAttack > 4)
                    selectedAttack = 0;
            }
            else
            {
                selectedDefense++;
                if (selectedDefense > 4)
                    selectedDefense = 0;
            }

            glutPostRedisplay();
            return;
        }

        // Allow direct category switching while viewing items.
        if (key == GLUT_KEY_UP)
        {
            shopTab = 0;
            glutPostRedisplay();
            return;
        }

        if (key == GLUT_KEY_DOWN)
        {
            shopTab = 1;
            glutPostRedisplay();
            return;
        }

        return;
    }

    // ========================================================GAME
    if (gameState == PLAYING)
    {
        float newX = playerX;
        float newY = playerY;

        if (key == GLUT_KEY_UP)
        {
            newY += playerSpeed;
            facingX = 0.0f;
            facingY = 1.0f;
        }
        else if (key == GLUT_KEY_DOWN)
        {
            newY -= playerSpeed;
            facingX = 0.0f;
            facingY = -1.0f;
        }
        else if (key == GLUT_KEY_LEFT)
        {
            newX -= playerSpeed;
            facingX = -1.0f;
            facingY = 0.0f;
        }
        else if (key == GLUT_KEY_RIGHT)
        {
            newX += playerSpeed;
            facingX = 1.0f;
            facingY = 0.0f;
        }
        else
        {
            return;
        }

        // Keep player inside the playable area.
        if (newX > 0.78f) newX = 0.78f;
        if (newX < -0.78f) newX = -0.78f;
        if (newY > 0.73f) newY = 0.73f;
        if (newY < -0.73f) newY = -0.73f;

        // Do not allow the player to walk through dungeon walls/obstacles.
        if (!playerHitsWall(newX, newY))
        {
            playerX = newX;
            playerY = newY;
        }

        glutPostRedisplay();
        return;
    }
}



void keyboard(unsigned char key, int x, int y)
{
    if (key == 27) exit(0);

    if (gameState == INTRO)
    {
        if (key == 13)
        {
            activateMenuSelection();
        }
        return;
    }







    if (gameState == SHOP)
    {
        // B: item screen -> category screen -> main menu
        if (key == 'b' || key == 'B')
        {
            if (shopMode == 1)
                shopMode = 0;
            else
            {
                gameState = INTRO;
                menuSelection = 1;
            }

            glutPostRedisplay();
            return;
        }

        // ENTER: category -> item screen, or buy/equip selected item
        if (key == 13)
        {
            if (shopMode == 0)
            {
                shopMode = 1;

                if (shopTab == 0)
                    selectedAttack = 0;
                else
                    selectedDefense = 0;

                glutPostRedisplay();
                return;
            }

            buyOrEquipSelectedShopItem();

            glutPostRedisplay();
            return;
        }

        return;
    }

    if (gameState == INSTRUCTIONS)
    {
        if (key == 13) { srand((unsigned)time(0)); createLevel(1); gameState = PLAYING; }
        return;
    }

    if (gameState == LEVEL_COMPLETE)
    {
        if (key == 13) { createLevel(level + 1); gameState = PLAYING; }
        return;
    }

    if (gameState == GAME_OVER)
    {
        if (key == 'r' || key == 'R')
        {
            health = 100;
            shieldCapacity = 1.0f;
            createLevel(level);
            gameState = PLAYING;
        }
        return;
    }

    if (gameState == WIN)
    {
        if (key == 'r' || key == 'R')
        {
            health = 100;
            score = 0;
            createLevel(1);
            gameState = PLAYING;
        }
        return;
    }

    if (key == 'p' || key == 'P') { paused = !paused; glutPostRedisplay(); return; }
    if (paused) return;

    if (key == ' ')
    {
        if (fireCooldownTicks <= 0)
        {
            fireBullet();
            fireCooldownTicks = 2;   // ~0.2s lockout - fast enough to feel like a gun, not spammable every frame
        }
        glutPostRedisplay();
        return;
    }

    float newX = playerX, newY = playerY;

    // WASD remains supported.
    if (key == 'w' || key == 'W') { newY += playerSpeed; facingX = 0.0f; facingY = 1.0f; }
    if (key == 'x' || key == 'X') { newY -= playerSpeed; facingX = 0.0f; facingY = -1.0f; }
    if (key == 'a' || key == 'A') { newX -= playerSpeed; facingX = -1.0f; facingY = 0.0f; }
    if (key == 'd' || key == 'D') { newX += playerSpeed; facingX = 1.0f; facingY = 0.0f; }

    if (newX > 0.78f) newX = 0.78f;
    if (newX < -0.78f) newX = -0.78f;
    if (newY > 0.73f) newY = 0.73f;
    if (newY < -0.73f) newY = -0.73f;

    if (!playerHitsWall(newX, newY)) { playerX = newX; playerY = newY; }

    if (!keyCollected && collision(playerX, playerY, playerSize, keyX, keyY, 0.05f))
    {
        keyCollected = true;
        doorOpen = true;
        score += 150;
        spawnParticles(keyX, keyY, 1.0f, 0.85f, 0.2f, 12);
    }

    if (doorOpen && collision(playerX, playerY, playerSize, doorX, doorY, 0.08f))
    {
        if (!isBossLevel(level) || !bossAlive)
        {
            if (level < MAX_LEVEL)
                gameState = LEVEL_COMPLETE;
            else
                gameState = WIN;
        }
    }

    for (int i = 0; i < trapCount; i++)
    {
        if (!traps[i].active) continue;

        float centerX = traps[i].x + traps[i].width / 2;
        float centerY = traps[i].y + traps[i].height / 2;

        if (collision(playerX, playerY, playerSize, centerX, centerY, traps[i].width / 2))
        {
            int dmg = 15 + level / 4;
            health -= dmg;
            traps[i].active = false;
            triggerShake(0.02f);
            triggerFlash();
            spawnParticles(centerX, centerY, 0.9f, 0.1f, 0.1f, 10);

            if (health <= 0) { health = 0; gameState = GAME_OVER; }
        }
    }

    glutPostRedisplay();
}

// ============================================================TIMER
float getPlayerBulletDamage()
{
    // Starter pistol = 100% damage.
    return 1.0f + attackBonus[equippedAttack];
}

float getShieldAbsorb()
{
    return defenseAbsorb[equippedDefense];
}

float applyDamageToPlayer(float incomingDamage)
{
    if (defenseOwned[equippedDefense] && shieldCapacity > 0.0f)
    {
        shieldCapacity = 0.0f;
        return 0.0f;
    }

    // Once the shield is broken, future damage reaches health normally.
    return incomingDamage;
}

void timer(int value)
{
    if (fireCooldownTicks > 0) fireCooldownTicks--;
    if (shakeTimer > 0) shakeTimer -= 0.1f;
    if (flashTimer > 0) flashTimer -= 0.1f;
    if (muzzleFlashTimer > 0) muzzleFlashTimer -= 0.1f;

    // Keep menu flames/crystal animated even before the game starts.
    if (gameState == INTRO || gameState == SHOP ||
        gameState == LEVEL_COMPLETE || gameState == GAME_OVER)
        animationTime += 0.1f;

    if (gameState == PLAYING && !paused)
    {
        animationTime += 0.1f;
        updateParticles();
        updateBullets();

        // occasional torch embers, purely decorative
        if (rand() % 5 == 0)
        {
            float tx[4] = { -0.80f, 0.75f, -0.80f, 0.75f };
            float ty[4] = { 0.65f, 0.65f, -0.65f, -0.65f };
            int t = rand() % 4;
            spawnParticles(tx[t], ty[t] + 0.08f, 1.0f, 0.5f, 0.1f, 1);
        }

        for (int i = 0; i < enemyCount; i++)
        {
            if (!enemies[i].alive) continue;

            float dx = playerX - enemies[i].x;
            float dy = playerY - enemies[i].y;
            float distance = sqrt(dx * dx + dy * dy);

            // Constant speed within the current level. & No close-range boost and no dasher burst.
            float currentSpeed = enemies[i].speed;

            if (distance > 0.001f)
            {
                float newX = enemies[i].x + (dx / distance) * currentSpeed;
                float newY = enemies[i].y + (dy / distance) * currentSpeed;

                bool blocked = false;
                for (int w = 0; w < wallCount; w++)
                {
                    float left = walls[w].x - enemies[i].size;
                    float right = walls[w].x + walls[w].width + enemies[i].size;
                    float bottom = walls[w].y - enemies[i].size;
                    float top = walls[w].y + walls[w].height + enemies[i].size;

                    if (newX > left && newX < right && newY > bottom && newY < top)
                    { blocked = true; break; }
                }

                if (!blocked)
                {
                    enemies[i].x = newX;
                    enemies[i].y = newY;
                }
                else
                {
                    // wall-sliding: the direct path is blocked, but the enemy can still creep along whichever single axis is clear instead of freezing in place entirely
                    if (!circleHitsAnyWall(newX, enemies[i].y, enemies[i].size))
                        enemies[i].x = newX;

                    if (!circleHitsAnyWall(enemies[i].x, newY, enemies[i].size))
                        enemies[i].y = newY;
                }
            }

            if (collision(playerX, playerY, playerSize, enemies[i].x, enemies[i].y, enemies[i].size))
            {
                int dmg = 7 + level / 4;
                health -= dmg;
                triggerShake(0.015f);
                triggerFlash();

                float dx2 = enemies[i].x - playerX, dy2 = enemies[i].y - playerY;
                float d2 = sqrt(dx2 * dx2 + dy2 * dy2);
                if (d2 > 0.001f) { enemies[i].x += (dx2 / d2) * 0.08f; enemies[i].y += (dy2 / d2) * 0.08f; }

                if (health <= 0) { health = 0; gameState = GAME_OVER; }
            }
        }

        if (isBossLevel(level) && bossAlive)
        {
            float dx = playerX - bossX, dy = playerY - bossY;
            float distance = sqrt(dx * dx + dy * dy);

            if (distance > 0.001f)
            {
                bool enraged = bossHealth <= bossMaxHealth / 3;
                float bossSpeed = enraged ? 0.034f : 0.020f;
                bossX += (dx / distance) * bossSpeed;
                bossY += (dy / distance) * bossSpeed;
            }

            if (collision(playerX, playerY, playerSize, bossX, bossY, bossSize))
            {
                health -= (bossHealth <= bossMaxHealth / 3) ? 12 : 8;
                triggerShake(0.03f);
                triggerFlash();
                if (health <= 0) { health = 0; gameState = GAME_OVER; }
            }
        }

        static int frameCounter = 0;
        frameCounter++;
        if (frameCounter >= 10) { timeLeft--; frameCounter = 0; }
        if (timeLeft <= 0) { timeLeft = 0; gameState = GAME_OVER; }
    }

    firing = false;
    glutPostRedisplay();
    glutTimerFunc(100, timer, 0);
}

// ============================================================ INIT
void init()
{
    glClearColor(0.05f, 0.04f, 0.07f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.0f, 1.0f, -1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    srand((unsigned)time(0));
}

// ============================================================MAIN
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(900, 700);
    glutCreateWindow("Dungeon Crawler - The Dark Realm (20 Levels)");

    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(mouseMotion);
    glutTimerFunc(100, timer, 0);

    glutMainLoop();
    return 0;
}
