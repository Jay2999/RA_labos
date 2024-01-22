#include "Shader.h"
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <GL/glut.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <chrono>

std::random_device rd;
std::mt19937 mt(rd());
std::chrono::steady_clock::time_point timePoint = std::chrono::high_resolution_clock::now();
float ms = 0;
int maxFps = 150;

GLuint width = 600, height = 600;

typedef struct _Ociste {
    GLdouble	x;
    GLdouble	y;
    GLdouble	z;
} Ociste;

Ociste	ociste = { 0.0f, 0.0f, 2.8f };
glm::vec3 particleSystemPosition(-2, 2.5, 0);

void myDisplay();
void myReshape(int width, int height);
void myRenderScene();
void idle();
void myKeyboard(unsigned char theKey, int mouseX, int mouseY);

struct Particle
{
    glm::vec3 pos;
    glm::vec3 vel;
    glm::vec4 color;
    float scale;
    float life;
    float maxLife;

    Particle() : pos(0), vel(0), color(1), scale(0.2), life(1), maxLife(1) { }
};

unsigned int n_particles = 100000;
std::vector<Particle> particles(n_particles, Particle());
Shader* particleShader;
GLuint VAO;
GLuint texture;

void initVertexArrays()
{
    glGenVertexArrays(1, &VAO);

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    float quad[] = {
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void initTextue()
{
    int width, height, nrChannels;
    unsigned char* data = stbi_load("snow.png", &width, &height, &nrChannels, 0);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("Primjer animacije");
    glutDisplayFunc(myDisplay);
    glutReshapeFunc(myReshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(myKeyboard);
    printf("Tipka: a/d - pomicanje ocista po x os +/-\n");
    printf("Tipka: w/s - pomicanje ocista po y os +/-\n");
    printf("Tipka: r - pocetno stanje\n");
    printf("esc: izlaz iz programa\n");

    gladLoadGL();
    particleShader = new Shader("./particle.vert", "./particle.frag");
    particleShader->use();
    initVertexArrays();
    initTextue();

    glutMainLoop();
    delete particleShader;
    glDeleteVertexArrays(1, &VAO);
    glDeleteTextures(1, &texture);
    return 0;
}

void myDisplay()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    myRenderScene();
    glutSwapBuffers();
}

void myReshape(int w, int h)
{
    width = w; height = h;
}

void drawParticles()
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(VAO);

    glm::mat4 model = glm::translate(glm::mat4(1), particleSystemPosition);
    particleShader->setUniform("model", model);

    glm::mat4 camera = glm::rotate(glm::mat4(1), (float)atan2(ociste.x, ociste.z) / 3.14f * 180, glm::vec3(0, 1, 0));
    camera = glm::rotate(camera, (float)atan2(ociste.y, ociste.z) / 3.14f * 180, glm::vec3(-1, 0, 0));
    glm::mat4 camOrientation = camera;
    particleShader->setUniform("camOrientation", camOrientation);
    camera = glm::translate(camera, glm::vec3(ociste.x, ociste.y, ociste.z));

    glm::mat4 view = glm::inverse(camera);
    particleShader->setUniform("view", view);

    glm::mat4 fr = glm::frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 50.0f);
    particleShader->setUniform("projection", fr);

    for (const auto& particle : particles)
    {
        particleShader->setUniform("offset", particle.pos);
        particleShader->setUniform("color", particle.color);
        particleShader->setUniform("scale", particle.scale);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glBindVertexArray(0);
}

float random(float min = 0.0f, float max = 1.0f)
{
    std::uniform_real_distribution<> ud(min, max);
    return ud(mt);
}

glm::vec3 randomVec3()
{
    return glm::vec3(random(-1, 1), random(-1, 1), random(-1, 1));
}

glm::vec3 wind(-1, 0, 0);
float clamp(float x, float min, float max)
{
    if (x < min)
        return min;
    if (x > max)
        return max;
    return x;
}
void updateWind()
{
    float lim = 1.2;
    wind += randomVec3() * 0.1f;
    wind.x = clamp(wind.x, -1 - lim, -1 + lim);
    wind.y = clamp(wind.y, -lim, lim);
    wind.z = clamp(wind.z, -lim * 0.1, lim * 0.1);
}

void updateParticles()
{
    float tc = 0.0001;

    for (auto& particle : particles)
    {
        particle.pos += particle.vel * ms;

        //gravitacija
        if (particle.vel.y > -tc * 20)
            particle.vel.y -= 0.001 * tc * ms;

        //vjetar
        if (particle.vel.x > -tc * 200)
            particle.vel -= 0.002f * tc * ms * wind;

        float coef = (particle.maxLife - particle.life) / particle.maxLife;
        float coef2 = coef * coef;
        //boja
        float yd = clamp(-particle.vel.y * 10000, 0, 1) / 10 + 1;
        particle.color = glm::vec4(coef2, std::max(coef, 0.1f), std::max(sqrt(coef), 0.1f) * yd, coef2);
        //velicina
        particle.scale = 0.2f * coef;


        particle.life += tc * ms;
        if (particle.life >= particle.maxLife)
        {
            particle.pos = randomVec3() * 0.5f;
            particle.pos.z = 0;
            particle.vel = randomVec3() * 0.001f;
            particle.maxLife = random(0, 1.5);
            particle.life = 0;
        }
    }
}

void myRenderScene()
{
    std::chrono::steady_clock::time_point timePoint2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dt = timePoint2 - timePoint;
    auto mic = std::chrono::duration_cast<std::chrono::microseconds>(dt);
    ms = mic.count() / 1000.0f;
    
    float targetMS = 1000.0f / maxFps;
    Sleep(std::max(0.0f, targetMS - ms));
    timePoint = std::chrono::high_resolution_clock::now();
    ms = std::max(targetMS, ms);

    drawParticles();
    updateParticles();
    updateWind();
}

void myKeyboard(unsigned char theKey, int mouseX, int mouseY)
{
    switch (theKey)
    {
    case 'a': ociste.x = ociste.x - 0.2f;
        break;
    case 'd': ociste.x = ociste.x + 0.2f;
        break;
    case 'w': ociste.y = ociste.y + 0.2f;
        break;
    case 's': ociste.y = ociste.y - 0.2f;
        break;
    case 'r': ociste.x = 0.0; ociste.y = 0.0;
        break;
    case 27:  exit(0);
        break;
    }

    myReshape(width, height);
    glutPostRedisplay();
}

void idle() {
    glutPostRedisplay();
}