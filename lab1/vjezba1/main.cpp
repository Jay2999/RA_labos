#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <GL/glut.h>
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include <assimp/scene.h>
#include <glm.hpp>

GLuint width = 600, height = 600;
float kut = 0;
float dir = 0.03;

typedef struct _Ociste {
    GLdouble	x;
    GLdouble	y;
    GLdouble	z;
} Ociste;

Ociste	ociste = { 3.2f, 0.0f, 2.8f };

void myDisplay();
void myReshape(int width, int height);
void myRenderScene();
void idle();
void myKeyboard(unsigned char theKey, int mouseX, int mouseY);

struct Triangle
{
    int vx;
    int vy;
    int vz;
};

std::vector<float> vertices;
std::vector<Triangle> triangles;
std::vector<glm::vec3> splineVertices;
std::vector<float> splineControlVertices;

glm::vec4 getSplinePoint(float t, int segment)
{
    glm::mat4 b = glm::mat4(
        -1, 3, -3, 1,
        3, -6, 3, 0,
        -3, 0, 3, 0,
        1, 4, 1, 0
    );
    b = b / 6.0f;

    int i = segment;
    glm::vec4 v0 = glm::vec4(splineControlVertices[(i - 1) * 3], splineControlVertices[(i - 1) * 3 + 1], splineControlVertices[(i - 1) * 3 + 2], 1);
    glm::vec4 v1 = glm::vec4(splineControlVertices[i * 3], splineControlVertices[i * 3 + 1], splineControlVertices[i * 3 + 2], 1);
    glm::vec4 v2 = glm::vec4(splineControlVertices[(i + 1) * 3], splineControlVertices[(i + 1) * 3 + 1], splineControlVertices[(i + 1) * 3 + 2], 1);
    glm::vec4 v3 = glm::vec4(splineControlVertices[(i + 2) * 3], splineControlVertices[(i + 2) * 3 + 1], splineControlVertices[(i + 2) * 3 + 2], 1);

    glm::mat4 r = glm::mat4(v0, v1, v2, v3);
    glm::vec4 t3 = glm::vec4(pow(t, 3), pow(t, 2), t, 1);

    glm::vec4 p = r * b * t3;
    return p / p.w;
}

void generateSplineVertices(float detail = 8)
{
    float tStep = 1 / detail;
    splineVertices.clear();

    for (int i = 1; i < splineControlVertices.size() / 3 - 2; i++)
    {
        for (float t = 0; t < 1.0; t += tStep)
        {
            glm::vec4 p = getSplinePoint(t, i);
            splineVertices.push_back(glm::vec3(p));
        }
    }
}

int main(int argc, char** argv)
{
    Assimp::Importer Importer;

    const aiScene* pScene = Importer.ReadFile("f16.obj", aiProcess_Triangulate |
                                                    aiProcess_GenSmoothNormals |
                                                    aiProcess_FlipUVs |
                                                    aiProcess_JoinIdenticalVertices);

    const aiMesh* mesh = pScene->mMeshes[0];
    for (int i = 0; i < mesh->mNumVertices; i++)
    {
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);
    }
    for (int i = 0; i < mesh->mNumFaces; i++)
    {
        Triangle t = Triangle();
        t.vx = mesh->mFaces[i].mIndices[0] * 3;
        t.vy = mesh->mFaces[i].mIndices[1] * 3;
        t.vz = mesh->mFaces[i].mIndices[2] * 3;
        triangles.push_back(t);
    }

    pScene = Importer.ReadFile("spline.obj", aiProcess_JoinIdenticalVertices);
    mesh = pScene->mMeshes[0];
    
    for (int i = 0; i < mesh->mNumVertices; i++)
    {
        splineControlVertices.push_back(mesh->mVertices[i].x);
        splineControlVertices.push_back(mesh->mVertices[i].y);
        splineControlVertices.push_back(mesh->mVertices[i].z);
    }
    generateSplineVertices();

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

    glutMainLoop();
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
    //glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //glOrtho(-1, 1, -1, 1, 1, 50);
    glFrustum(-1, 1, -1, 1, 1, 50);
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(ociste.x, ociste.y, ociste.z, 0.0, 0.0, 2.8, 0.0, 1.0, 0.0);	// ociste x,y,z; glediste x,y,z; up vektor x,y,z
}

void drawObject()
{
    for (int i = 0; i < triangles.size(); i++)
    {
        glBegin(GL_TRIANGLES);
        glVertex3fv(&vertices[triangles[i].vx]);
        glVertex3fv(&vertices[triangles[i].vy]);
        glVertex3fv(&vertices[triangles[i].vz]);
        glEnd();
    }
}

void drawSpline()
{
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < splineVertices.size(); i++)
        glVertex3fv(&splineVertices[i].x);
    glEnd();
}

void drawLine()
{
    glBegin(GL_LINES);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, 1);
    glEnd();
}

glm::vec4 getTangent(float t, int segment)
{
    int i = segment;
    glm::vec4 v0 = glm::vec4(splineControlVertices[(i - 1) * 3], splineControlVertices[(i - 1) * 3 + 1], splineControlVertices[(i - 1) * 3 + 2], 1);
    glm::vec4 v1 = glm::vec4(splineControlVertices[i * 3], splineControlVertices[i * 3 + 1], splineControlVertices[i * 3 + 2], 1);
    glm::vec4 v2 = glm::vec4(splineControlVertices[(i + 1) * 3], splineControlVertices[(i + 1) * 3 + 1], splineControlVertices[(i + 1) * 3 + 2], 1);
    glm::vec4 v3 = glm::vec4(splineControlVertices[(i + 2) * 3], splineControlVertices[(i + 2) * 3 + 1], splineControlVertices[(i + 2) * 3 + 2], 1);

    glm::mat3x4 b = glm::mat3x4(
        -1, 3, -3, 1,
        2, -4, 2, 0,
        -1, 0, 1, 0
    );
    b = b / 2.0f;

    glm::mat4 r = glm::mat4(v0, v1, v2, v3);
    glm::vec3 t2 = glm::vec3(pow(t, 2), t, 1);

    return r * b * t2;
}

glm::vec4 getNormal(glm::vec3 tangent,float t, int segment)
{
    int i = segment;
    glm::vec4 v0 = glm::vec4(splineControlVertices[(i - 1) * 3], splineControlVertices[(i - 1) * 3 + 1], splineControlVertices[(i - 1) * 3 + 2], 1);
    glm::vec4 v1 = glm::vec4(splineControlVertices[i * 3], splineControlVertices[i * 3 + 1], splineControlVertices[i * 3 + 2], 1);
    glm::vec4 v2 = glm::vec4(splineControlVertices[(i + 1) * 3], splineControlVertices[(i + 1) * 3 + 1], splineControlVertices[(i + 1) * 3 + 2], 1);
    glm::vec4 v3 = glm::vec4(splineControlVertices[(i + 2) * 3], splineControlVertices[(i + 2) * 3 + 1], splineControlVertices[(i + 2) * 3 + 2], 1);

    glm::mat3x4 b = glm::mat3x4(
        -1, 3, -3, 1,
        2, -4, 2, 0,
        -1, 0, 1, 0
    );
    b = b / 2.0f;

    glm::mat4 r = glm::mat4(v0, v1, v2, v3);
    glm::vec3 t2 = glm::vec3(2 * t, 1, 0);

    glm::vec3 d2 = r * b * t2;
    if (glm::distance(d2, glm::vec3(0)) < 0.0001)
        d2 = glm::vec3(0, 1, 0);
    return glm::vec4(glm::cross(tangent, d2), 0);
}

void myRenderScene()
{
    //Spline
    glPushMatrix();
    glColor3f(0.0f, 0.0f, 1.0f);
    drawSpline();
    glPopMatrix();

    float degPerSeg = 360.0f / (splineControlVertices.size() / 3 - 3);
    float k1 = kut;
    k1 = k1 >= 360 ? 359 : k1;
    k1 = k1 < 0 ? 0 : k1;
    int segment = (int)floorf(k1 / degPerSeg) + 1;
    float t = kut / degPerSeg - floorf(kut / degPerSeg);

    glm::vec3 tan = getTangent(t, segment);
    glm::vec4 loc = getSplinePoint(t, segment);
    glm::vec3 normal = getNormal(tan, t, segment);

    glm::vec3 rotS = glm::vec3(0, 0, 1);
    glm::vec3 os = glm::cross(rotS, tan);
    float angle = acos(glm::dot(rotS, tan) / glm::distance(rotS, glm::vec3(0)) / glm::distance(tan, glm::vec3(0)));
    angle = angle / 2 / AI_MATH_PI * 360;

    glm::vec3 rotS2 = glm::vec3(0, 1, 0);
    float angle2 = acos(glm::dot(rotS2, normal) / glm::distance(rotS2, glm::vec3(0)) / glm::distance(normal, glm::vec3(0)));
    angle2 = angle2 / 2 / AI_MATH_PI * 360;

    //Plane
    glColor3f(1.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(loc.x, loc.y, loc.z);
    glRotatef(angle, os.x, os.y, os.z);
    glRotatef(angle2, rotS.x, rotS.y, rotS.z);
    if (dir < 0)
        glRotatef(180, 0, 1, 0);
    drawObject();
    glPopMatrix();

    //Tangent
    glPushMatrix();
    glColor3f(0.0f, 1.0f, 0.0f);
    glTranslatef(loc.x, loc.y, loc.z);
    glRotatef(angle, os.x, os.y, os.z);
    if (dir < 0)
        glRotatef(180, 0, 1, 0);
    drawLine();
    glPopMatrix();
}

void myKeyboard(unsigned char theKey, int mouseX, int mouseY)
{
    switch (theKey)
    {
    case 'a': ociste.z = ociste.z + 0.2f;
        break;
    case 'd': ociste.z = ociste.z - 0.2f;
        break;
    case 'w': ociste.y = ociste.y + 0.2f;
        break;
    case 's': ociste.y = ociste.y - 0.2f;
        break;
    case 'r': ociste.z = 0.0; ociste.y = 0.0;
        break;
    case 27:  exit(0);
        break;
    }

    myReshape(width, height);
    glutPostRedisplay();
}

void idle() {
    kut += dir;
    if (kut >= 360 || kut <= 0)
        dir *= -1;
    glutPostRedisplay();
}