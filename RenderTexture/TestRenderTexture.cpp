#include "RenderTexture.h"

#include <GL/glut.h>
#include <cg/cgGL.h>
#include <assert.h>
#include <stdio.h>

CGprogram   fragProgram;
CGprogram   testProgram;
CGparameter textureParam;
CGcontext   cgContext;

RenderTexture *rt = NULL;

float angle = 0;


//------------------------------------------------------------------------------
// Function     	  : cgErrorCallback
// Description	    : 
//------------------------------------------------------------------------------
void cgErrorCallback()
{
  CGerror lastError = cgGetError();
  
  if(lastError)
  {
    printf("%s\n\n", cgGetErrorString(lastError));
    printf("%s\n", cgGetLastListing(cgContext));
    printf("Cg error, exiting...\n");
    
    exit(0);
  }
} 


//------------------------------------------------------------------------------
// Function     	  : PrintGLerror
// Description	    : 
//------------------------------------------------------------------------------
void PrintGLerror( char *msg )
{
 GLenum errCode;
 const GLubyte *errStr;

 if ((errCode = glGetError()) != GL_NO_ERROR) 
 {
    errStr = gluErrorString(errCode);
    fprintf(stderr,"OpenGL ERROR: %s: %s\n", errStr, msg);
 }
}


//------------------------------------------------------------------------------
// Function     	  : Idle
// Description	    : 
//------------------------------------------------------------------------------
void Idle()
{
  angle += 1;
  glutPostRedisplay();
}

void Reshape(int w, int h)
{
  if (h == 0) h = 1;
  
  glViewport(0, 0, w, h);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  
  gluPerspective(60.0, (GLfloat)w/(GLfloat)h, 0.1, 1000.0);
}


//------------------------------------------------------------------------------
// Function     	  : Display
// Description	    : 
//------------------------------------------------------------------------------
void Display()
{
  rt->BeginCapture();
  { 
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    {
      glRotatef(angle, 1, 0, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glColor3f(1,1,0);
      
      if (rt->IsFloatTexture() && rt->IsRectangleTexture())
      {
        cgGLBindProgram(testProgram);
        cgGLEnableProfile(GL_ARB_fragment_program ? CG_PROFILE_ARBFP1 : CG_PROFILE_FP30);
      }
      glutWireTorus(0.25, 1, 32, 64);
      
      if (rt->IsFloatTexture() && rt->IsRectangleTexture())
        cgGLDisableProfile(GL_ARB_fragment_program ? CG_PROFILE_ARBFP1 : CG_PROFILE_FP30);
    }
    glPopMatrix();
    PrintGLerror("RT Update");
  }    
  rt->EndCapture();
  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glColor3f(1, 1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glRotatef(angle / 10, 0, 1, 0);

  if (rt->IsFloatTexture())
  {
    cgGLBindProgram(fragProgram);
    cgGLEnableProfile(GL_ARB_fragment_program ? CG_PROFILE_ARBFP1 : CG_PROFILE_FP30);
    cgGLSetTextureParameter(textureParam, rt->GetTextureID());
  
    cgGLEnableTextureParameter(textureParam);
  }
  else 
  {
    rt->Bind();
    rt->EnableTextureTarget();
  }
  
  if (rt->IsRectangleTexture())
  {
    glBegin(GL_QUADS);
    glTexCoord2f(0,                            0); glVertex3f(-1, -1, -0.5f);
    glTexCoord2f(rt->GetWidth(),               0); glVertex3f( 1, -1, -0.5f);
    glTexCoord2f(rt->GetWidth(), rt->GetHeight()); glVertex3f( 1,  1, -0.5f);
    glTexCoord2f(0,              rt->GetHeight()); glVertex3f(-1,  1, -0.5f);
    glEnd();
  }
  else
  {
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-1, -1, -0.5f);
    glTexCoord2f(1, 0); glVertex3f( 1, -1, -0.5f);
    glTexCoord2f(1, 1); glVertex3f( 1,  1, -0.5f);
    glTexCoord2f(0, 1); glVertex3f(-1,  1, -0.5f);
    glEnd();
  }  
    
  if (rt->IsFloatTexture())
  {
    cgGLDisableTextureParameter(textureParam);
    cgGLDisableProfile(GL_ARB_fragment_program ? CG_PROFILE_ARBFP1 : CG_PROFILE_FP30);
  }
  else 
  {
    rt->DisableTextureTarget();
  }

  glPopMatrix();
  
  PrintGLerror("display");
  glutSwapBuffers();
}


//------------------------------------------------------------------------------
// Function     	  : main
// Description	    : 
//------------------------------------------------------------------------------
void main()
{
  glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
  glutInitWindowPosition(50, 50);
  glutInitWindowSize(512, 512);
  glutCreateWindow("TestRenderTexture");  

  int err = glewInit();
  if (GLEW_OK != err)
  {
    // problem: glewInit failed, something is seriously wrong
    fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
    exit(-1);
  }  

  glutDisplayFunc(Display);
  glutIdleFunc(Idle);
  glutReshapeFunc(Reshape);
  
  Reshape(512, 512);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0, 0, 2, 0, 0, 0, 0, 1, 0);
  glDisable(GL_LIGHTING);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_DEPTH_TEST); 
  glClearColor(0.4, 0.6, 0.8, 1);
  
  // A square, mipmapped, anisotropically filtered 8-bit RGBA texture with
  // depth and stencil.
  rt = new RenderTexture(256, 256);
  rt->Initialize(true, true, true, true, true, 8, 8, 8, 8);

  // The same as above, but updates are performed via copies rather than RTT.
  //rt = new RenderTexture(256, 256);
  //rt->Initialize(true, true, true, true, true, 8, 8, 8, 8, RenderTexture::RT_COPY_TO_TEXTURE);
  
  // Try these for other types of render textures.

  // A rectangle 8-bit RGB texture with depth and stencil.
  //rt = new RenderTexture(200, 231);
  //rt->Initialize(true, true, true, false, false, 8, 8, 8, 0);
  
  // A square float texture.with depth and stencil.
  //rt = new RenderTexture(256, 256);
  //rt->Initialize(true, true, true, false, false, 32, 32, 32, 32);
  
  // A square single channel float texture.without depth or stencil.
  //rt = new RenderTexture(256, 256);
  //rt->Initialize(true, false, false, false, false, 32, 0, 0, 0);

  // setup the rendering context for the RenderTexture
  rt->BeginCapture();
  Reshape(256, 256);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0, 0, 3, 0, 0, 0, 0, 1, 0);
  glDisable(GL_LIGHTING);
  glEnable(GL_COLOR_MATERIAL);
  glDisable(GL_DEPTH_TEST); 
  glEnable(GL_DEPTH_TEST); 
  glClearColor(0.2, 0.2, 0.2, 1);
  rt->EndCapture();
  
  if (rt->IsFloatTexture())
  {
      // Setup Cg if needed
      cgSetErrorCallback(cgErrorCallback);

    // Create cgContext.
    cgContext = cgCreateContext();
    fragProgram = cgCreateProgramFromFile(cgContext, 
                                           CG_SOURCE,
                                           rt->IsRectangleTexture() ? "textureRECT.cg" : "texture2D.cg",
                                           GL_ARB_fragment_program ? CG_PROFILE_ARBFP1 : CG_PROFILE_FP30, 
                                           NULL,
                                           NULL);

    if(fragProgram != NULL)
    {
      cgGLLoadProgram(fragProgram);
    
      textureParam = cgGetNamedParameter(fragProgram, "texture");
      assert(textureParam != NULL);
    }

    testProgram = cgCreateProgramFromFile(cgContext, 
                                          CG_SOURCE,
                                          "test.cg", 
                                           GL_ARB_fragment_program ? CG_PROFILE_ARBFP1 : CG_PROFILE_FP30,
                                           NULL,
                                           NULL);

    if(testProgram != NULL)
    {
      cgGLLoadProgram(testProgram);
    }
  }
  
  glutMainLoop();
}
