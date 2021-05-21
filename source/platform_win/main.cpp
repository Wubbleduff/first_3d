#include "../my_math.h"
#include "renderer.h"

#include "../world.h"


#include <windows.h>



static HWND window_handle;
static bool running;

#define MAX_BUTTONS 165
static bool key_states[MAX_BUTTONS] = {};
static bool mouse_states[8] = {};

static v2 mouse_position;
static v2 delta_mouse_position;


bool key_state(unsigned button)
{
  if(button < 0 || button > MAX_BUTTONS) return false;

  return key_states[button];
}

bool mouse_state(unsigned button)
{
  if(button < 0 || button > 8) return false;

  return mouse_states[button];
}

v2 mouse_window_position()
{
  return mouse_position;
}

v2 mouse_delta_position()
{
  return delta_mouse_position;
}



LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;

  switch (message)
  {
    
    case WM_SIZE:
    {
    }
    break;
    
    case WM_DESTROY:
    {
      running = false;
      PostQuitMessage(0);
      return 0;
    }
    break;

    case WM_CLOSE: 
    {
      running = false;
      DestroyWindow(window);
      return 0;
    }  
    break;
    
    case WM_PAINT:
    {
      ValidateRect(window_handle, 0);
    }
    break;

    case WM_KEYDOWN: 
    {
      key_states[wParam] = true;
    }
    break;

    case WM_KEYUP:
    {
      key_states[wParam] = false;
    }
    break;

    case WM_LBUTTONDOWN:
    {
      mouse_states[0] = true;
    }
    break;

    case WM_LBUTTONUP:
    {
      mouse_states[0] = false;
    }
    break;
    
    case WM_KILLFOCUS:
    {
      extern bool cursor_locked;
      cursor_locked = false;
    }
    default:
    {
      result = DefWindowProc(window, message, wParam, lParam);
    }
    break;
  }
  
  return result;
}


int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  // Create the window class
  WNDCLASS window_class = {};

  window_class.style = CS_HREDRAW|CS_VREDRAW;
  window_class.lpfnWndProc = WindowProc;
  window_class.hInstance = hInstance;
  window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
  window_class.lpszClassName = "Windows Program Class";

  if(!RegisterClass(&window_class))
  {
    return 1;
  }

  
  unsigned monitor_width = GetSystemMetrics(SM_CXSCREEN);
  unsigned monitor_height = GetSystemMetrics(SM_CYSCREEN);

#define TRANSPARENT_WINDOW

  // Create the window
#ifdef TRANSPARENT_WINDOW
  window_handle = CreateWindowEx(0,    // Extended style
                                window_class.lpszClassName,        // Class name
                                "First",                           // Window name
                                WS_POPUP | WS_VISIBLE,             // Style of the window
                                0,                                 // Initial X position
                                0,                                 // Initial Y position
                                monitor_width,                     // Initial width
                                monitor_height,                    // Initial height 
                                0,                                 // Handle to the window parent
                                0,                                 // Handle to a menu
                                hInstance,                         // Handle to an instance
                                0);                                // Pointer to a CLIENTCTREATESTRUCT
#else
  window_handle = CreateWindowEx(0,                                // Extended style
                                window_class.lpszClassName,        // Class name
                                "First",                           // Window name
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,  // Style of the window
                                0,                                 // Initial X position
                                0,                                 // Initial Y position
                                monitor_width,                     // Initial width
                                monitor_height,                    // Initial height 
                                0,                                 // Handle to the window parent
                                0,                                 // Handle to a menu
                                hInstance,                         // Handle to an instance
                                0);                                // Pointer to a CLIENTCTREATESTRUCT
#endif
  if(!window_handle)
  {
    return 1;
  }

  RECT cursor_region = {0, 0, monitor_width, monitor_height};
  ClipCursor(&cursor_region);

  // Initialize
  init_renderer(window_handle, monitor_width, monitor_height, false, false);
  init_world();

  // Main loop
  running = true;
  while(running)
  {
    MSG message;
    while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
    {
      if(message.message == WM_QUIT)
      {
        running = false;
      }
      TranslateMessage(&message);
      DispatchMessage(&message);
    }

    if(key_states[VK_ESCAPE])
    {
      PostQuitMessage(0);
      running = false;
    }

    POINT p;
    if(GetCursorPos(&p))
    {
      ScreenToClient(window_handle, &p);
    }
    mouse_position = v2((float)p.x, (float)p.y);
    delta_mouse_position = mouse_position - v2(monitor_width / 2.0f, monitor_height / 2.0f);

    extern bool cursor_locked;

    static bool last_check_cursor_locked = false;
    if(cursor_locked != last_check_cursor_locked)
    {
      if(cursor_locked == true)
      {
        ShowCursor(0);
        ClipCursor(&cursor_region);
        SetCursorPos(monitor_width / 2, monitor_height / 2);
      }
      else
      {
        ShowCursor(1);
        ClipCursor(0);
      }
    }
    if(cursor_locked == true)
    {
      SetCursorPos(monitor_width / 2, monitor_height / 2);
    }

    last_check_cursor_locked = cursor_locked;

    // Updating
    update_world();


    
    // Rendering
    render();
  }

  shutdown_renderer();

  return 0;
}

