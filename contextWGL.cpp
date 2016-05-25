#include "gpu/contextWGL.h"

#include "log/log.h"

#include <boost/foreach.hpp>
#include <GL/glcorearb.h>
#include <GL/glu.h>
#include <stdio.h>

#define HIDDEN_NAME "__hidden__"

namespace gpu
{

struct ContextWGL::WindowWGL
{
    HWND m_hWnd;
    uint m_width;
    uint m_height;
    WindowWGL(HWND hWnd, uint width, uint height):
      m_hWnd(hWnd), m_width(width), m_height(height) { }
};

//------------------------------------------------------------------------------
void DrawOpenGLScene( )
{
	PFNGLCLEARPROC c_glClear = 0;
	c_glClear = (PFNGLCLEARPROC)wglGetProcAddress("glClear");
	if (!c_glClear || c_glClear == (void *)0x1 || c_glClear == (void *)0x2 || c_glClear == (void *)0x3 || c_glClear == (void *)-1) {
		printf("trying other thing\n");
		HMODULE module = LoadLibraryA("opengl32.dll");
		c_glClear = (PFNGLCLEARPROC)GetProcAddress(module, "glClear");
	}
	if (!c_glClear) {
		printf("everything failed\n");
	}
	glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
	c_glClear(GL_COLOR_BUFFER_BIT);

    //glEnable( GL_DEPTH_TEST );

    //
    // Define the modelview transformation.
    //
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    // move the viewpoint out to where we can see everything
    glTranslatef( 0.0f, 0.0f, -5.0f );

    glBegin(GL_TRIANGLES);
        glColor3f(1.0,0.0,0.0);//red color for the triangle
        glVertex3f(0.0,0.0,0);
        glVertex3f(0.0,1.0,0);
        glVertex3f(1.0,0.0,0);

        glColor3f(0.0,1.0,0.0);//Green color for the triangle
        glVertex3f(0.0,0.0,0);
        glVertex3f(0.0,1.0,0);
        glVertex3f(-1.0,0.0,0);

        glColor3f(0.0,0.0,1.0);//Blue color for the triangle
        glVertex3f(1.0,0.0,0);
        glVertex3f(0.0,-1.0,0);
        glVertex3f(-1.0,0.0,0);
    glEnd();

    glFlush();
}

void ContextWGL::SetUpOpenGL( HDC hDC)
{
    static PIXELFORMATDESCRIPTOR pfd = {
        sizeof (PIXELFORMATDESCRIPTOR), // strcut size
        1,                              // Version number
        PFD_DRAW_TO_WINDOW |    // Flags, draw to a window,
            PFD_SUPPORT_OPENGL, // use OpenGL
        PFD_TYPE_RGBA,          // RGBA pixel values
        24,                     // 24-bit color
        0, 0, 0,                // RGB bits & shift sizes.
        0, 0, 0,                // Don't care about them
        0, 0,                   // No alpha buffer info
        0, 0, 0, 0, 0,          // No accumulation buffer
        32,                     // 32-bit depth buffer
        0,                      // No stencil buffer
        0,                      // No auxiliary buffers
        PFD_MAIN_PLANE,         // Layer type
        0,                      // Reserved (must be 0)
        0,                      // No layer mask
        0,                      // No visible mask
        0                       // No damage mask
    };

    int nMyPixelFormatID;
    nMyPixelFormatID = ChoosePixelFormat( hDC, &pfd );

    SetPixelFormat( hDC, nMyPixelFormatID, &pfd );

    m_hRC = wglCreateContext( hDC );
}

LONG WINAPI WndProc( HWND hWnd, UINT msg,
                     WPARAM wParam, LPARAM lParam )
{
    HDC hDC;
    PAINTSTRUCT ps;
	  static HGLRC hRC; // Note this is STATIC!
    GLdouble gldAspect;
    GLsizei glnWidth, glnHeight;

    switch(msg)
    {
    case WM_SIZE:
         hDC = GetDC(hWnd);
         wglMakeCurrent(hDC,hRC);

         // get the new size of the client window
            // note that we size according to the height,
            // not the smaller of the height or width.
            glnWidth = (GLsizei) LOWORD (lParam);
            glnHeight = (GLsizei) HIWORD (lParam);
            gldAspect =
                 (GLdouble)glnWidth/(GLdouble)glnHeight;

            // set up a projection matrix to fill the
            //  client window
            glMatrixMode( GL_PROJECTION );
            glLoadIdentity();
            // a perspective-view matrix...
            gluPerspective(
                30.0,   // Field-of-view angle
                gldAspect, // Aspect ratio of view volume
                1.0,   // Distance to near clipping plane
                10.0 ); // Distance to far clipping plane

            glViewport( 0, 0, glnWidth, glnHeight );
            wglMakeCurrent( NULL, NULL );
            ReleaseDC( hWnd, hDC );

            return 0;

    case WM_PAINT:

        // Draw the scene.

            // Get a DC, then make the RC current and
            // associate with this DC
            hDC = BeginPaint( hWnd, &ps );
            wglMakeCurrent( hDC, hRC );

            DrawOpenGLScene();

            // we're done with the RC, so
            // deselect it
            // (note: This technique is not recommended!)
            wglMakeCurrent( NULL, NULL );

            EndPaint( hWnd, &ps );
            return 0;

    case WM_DESTROY:
            // Clean up and terminate.
            wglDeleteContext( hRC );
            PostQuitMessage( 0 );
            return 0;

    }

    // This function handles any messages that we didn't.
    // (Which is most messages) It belongs to the OS.
    return DefWindowProc( hWnd, msg, wParam, lParam );

}

void
ContextWGL::_destroyWindow(WindowWGL * win) const
{
    wglDeleteContext( m_hRC );
    PostQuitMessage( 0 );
    delete win;
};


ContextWGL::WindowWGL *
ContextWGL::_getWindow(const std::string & name) const
{
    WindowWGLMap::const_iterator it = m_windows.find(name);
    return (it == m_windows.end()) ? 0 : it->second;
}

//------------------------------------------------------------------------------

ContextWGL::ContextWGL():
    m_hRC(0),
    m_hInstance(0)
{ }

ContextWGL::~ContextWGL()
{

}

bool
ContextWGL::createWindow(const std::string & name, uint left, uint top, uint width, uint height)
{
    if (m_windows.count(name)) {
        rError("window %s already exists", name.c_str());
        return false;
    }

    static char szAppName[] = "WGL";
    HWND hWnd;
    hWnd = CreateWindowA(
                szAppName, // app name
                name.c_str(),   // Text for window title bar
                WS_OVERLAPPEDWINDOW// Window style
                  // NEED THESE for OpenGL calls to work!
                 | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                left, top, width, height,
                NULL,     // no parent window
                NULL,     // Use the window class menu.
                m_hInstance,// This instance owns this window
                NULL      // We don't use any extra data
        );

    // If window could not be created, return zero
    if ( !hWnd )
        {
        return( 0 );
        }

    // Make the window visible & update its client area

    if(name == HIDDEN_NAME) {
        HDC hDC = GetDC(hWnd);
        SetUpOpenGL(hDC);
        ReleaseDC(hWnd, hDC);
    }
    else {
        ShowWindow( hWnd, 1 );// Show the window
    }

    m_windows[name] = new WindowWGL(hWnd, width, height);
    return true;
}

void
ContextWGL::destroyWindow(const std::string & name)
{
    WindowWGLMap::iterator it = m_windows.find(name);
    if (it == m_windows.end()) {
        rError("window %s does not exist - can't destroy", name.c_str());
        return;
    }
    _destroyWindow( it->second );
    m_windows.erase(it);
}

PFNPROC
ContextWGL::getProcAddress(const char * procName) const
{
  PFNPROC c_glClear = 0;
  c_glClear = (PFNPROC)wglGetProcAddress(procName);
  if (!c_glClear || c_glClear == (void *)0x1 || c_glClear == (void *)0x2 || c_glClear == (void *)0x3 || c_glClear == (void *)-1) {
    printf("contextWGL:getProcAddress:trying other thing\n");
    HMODULE module = LoadLibraryA("opengl32.dll");
    c_glClear = (PFNPROC)GetProcAddress(module, procName);
  }
  if (!c_glClear) {
    printf("everything failed\n");
  }
  return c_glClear;
}

void
ContextWGL::getWindowSize(const std::string & name, uint & width, uint & height) const
{
    WindowWGL* win = _getWindow(name);
    width = win->m_width;
    height = win->m_height;
}

bool
ContextWGL::hasWindow(const std::string & name) const
{
    return makeBool(_getWindow(name));
}

bool
ContextWGL::init()
{
  m_hInstance = GetModuleHandle(0);
  WNDCLASSA wc;   // windows class sruct
  static char szAppName[] = "WGL";

  wc.style         =
  CS_HREDRAW | CS_VREDRAW;// Class style(s).
  wc.lpfnWndProc   =
      (WNDPROC)WndProc;      // Window Procedure
  wc.cbClsExtra    = 0;     // No per-class extra data.
  wc.cbWndExtra    = 0;     // No per-window extra data.
  wc.hInstance     =
      m_hInstance;            // Owner of this class
  wc.hIcon         = NULL;  // Icon name
  wc.hCursor       =
      LoadCursor(NULL, IDC_ARROW);// Cursor
  wc.hbrBackground =
      (HBRUSH)(COLOR_WINDOW+1);// Default color
  wc.lpszMenuName  = NULL;  // Menu from .RC
  wc.lpszClassName = szAppName;            // Name to register as

  // Register the window class
  RegisterClassA( &wc );

  if(!createWindow(HIDDEN_NAME, 0 , 0 , 1, 1)) {
      return false;
  }
  // makes the context active
  setWindow();
	return true;
}

void
ContextWGL::setWindow()
{
    setWindow(HIDDEN_NAME);
}

void
ContextWGL::setWindow(const std::string & name)
{
    WindowWGL * win = _getWindow(name);
    if (win == m_lastWindow) return;  // do nothing if no change
    HDC hDC = GetDC(win->m_hWnd);
    wglMakeCurrent(hDC,m_hRC);
    ReleaseDC(win->m_hWnd, hDC);
    m_lastWindow = win;
}

void
ContextWGL::swapBuffers(const std::string & name) const
{
  WindowWGL * win = _getWindow(name);
  rTimerStart("[wglSwapBuffers]");
  HDC hDC = GetDC(win->m_hWnd);
  SwapBuffers(hDC);
  ReleaseDC(win->m_hWnd, hDC);
  rTimerStop("[wglSwapBuffers]");
}

}
