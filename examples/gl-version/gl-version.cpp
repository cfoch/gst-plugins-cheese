#include <GL/glew.h>
#include <GL/glut.h>
#include <cstdio>


int main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutCreateWindow("GLUT");

  glewInit();
  printf("OpenGL version supported by this platform (%s): \n",
      glGetString(GL_VERSION));
}
