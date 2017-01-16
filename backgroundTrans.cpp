#include <windows.h>
#include <GL/glcorearb.h>
#include <GL/glu.h>
#include <stdio.h>

LONG WINAPI WndProc(HWND,UINT,WPARAM,LPARAM);//Window procedure
HGLRC SetUpOpenGL(HWND hwnd);//Initialize OpenGL
void DrawOpenGLScene(void);//Actual Drawing code

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
                                      LPSTR lpCmdLine,int nCmdShow)
{
    static char szAppName[] = "OpenGL";
    static char szTitle[]="Getting Started With OpenGL";
    WNDCLASS wc;   // windows class sruct
    MSG      msg;  // message struct 
    HWND     hWnd; // Main window handle.

    // Fill in window class structure with parameters that
    //  describe the main window.

    wc.style         =
        CS_HREDRAW | CS_VREDRAW;// Class style(s).
    wc.lpfnWndProc   = 
        (WNDPROC)WndProc;      // Window Procedure
    wc.cbClsExtra    = 0;     // No per-class extra data.
    wc.cbWndExtra    = 0;     // No per-window extra data.
    wc.hInstance     =
        hInstance;            // Owner of this class
    wc.hIcon         = NULL;  // Icon name 
    wc.hCursor       =
        LoadCursor(NULL, IDC_ARROW);// Cursor
    wc.hbrBackground = 
        (HBRUSH)(COLOR_WINDOW+1);// Default color
    wc.lpszMenuName  = NULL;  // Menu from .RC
    wc.lpszClassName =
        szAppName;            // Name to register as

    // Register the window class
    RegisterClass( &wc );
  
    // Create a main window for this application instance.

    hWnd = CreateWindow(
                szAppName, // app name
                szTitle,   // Text for window title bar
                WS_OVERLAPPEDWINDOW// Window style 
                  // NEED THESE for OpenGL calls to work!
                 | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
                NULL,     // no parent window
                NULL,     // Use the window class menu.
                hInstance,// This instance owns this window
                NULL      // We don't use any extra data
        );

    // If window could not be created, return zero
    if ( !hWnd )
        {
        return( 0 );
        }

    // Make the window visible & update its client area
    ShowWindow( hWnd, 1 );// Show the window
    UpdateWindow( hWnd );        // Sends WM_PAINT message

    // Enter the Windows message loop
    // Get and dispatch messages until WM_QUIT 
    while (GetMessage(&msg, // message structure
               NULL,       // handle of window receiving
                           // the message
               0,          // lowest message id to examine
               0))         // highest message id to examine
        {
        TranslateMessage( &msg ); // Translates messages
        DispatchMessage( &msg );  // then dispatches
        }

    return( msg.wParam );

}

LONG WINAPI WndProc( HWND hWnd, UINT msg,
                     WPARAM wParam, LPARAM lParam )
{
    HDC hDC;
    static HGLRC hRC; // Note this is STATIC!
    PAINTSTRUCT ps;
    GLdouble gldAspect;
    GLsizei glnWidth, glnHeight;

    switch(msg)
    {
    case WM_CREATE:
        // Select a pixel format and then
        // create a rendering context from it.
        hRC = SetUpOpenGL(hWnd);
        return 0;

    case WM_SIZE:
         // Redefine the viewing volume and viewport
         // when the window size changes.

         // Make the RC current since we're going to
         // make an OpenGL call here...
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

HGLRC SetUpOpenGL( HWND hWnd )
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
    HDC hDC;
    HGLRC hRC;

    hDC = GetDC( hWnd );
    nMyPixelFormatID = ChoosePixelFormat( hDC, &pfd );

    // catch errors here.
    // If nMyPixelFormat is zero, then there's
    // something wrong... most likely the window's
    // style bits are incorrect (in CreateWindow() )
    // or OpenGl isn't installed on this machine

    SetPixelFormat( hDC, nMyPixelFormatID, &pfd );

    hRC = wglCreateContext( hDC );
    ReleaseDC( hWnd, hDC );

    return hRC;
}

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
