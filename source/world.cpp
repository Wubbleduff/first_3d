#include "world.h"
#include "my_math.h"

#include "renderer.h"

bool key_state(u32 button);
v2 mouse_window_position();
v2 mouse_delta_position();
void set_cursor_locked(bool locked);

static bool cursor_locked = false;
static const v3 WORLD_UP_VECTOR = {0.0f, 1.0f, 0.0f};

void init_world()
{
}

void update_world()
{
  static bool pressed = true;
  if(key_state(VK_SHIFT) == true && !pressed)
  {
    cursor_locked = !cursor_locked;
  }
  pressed = key_state(VK_SHIFT);
  set_cursor_locked(cursor_locked);

  v3 *position = camera_position();
  float *latitude = camera_latitude();
  float *longitude = camera_longitude();

  if(cursor_locked)
  {
    v2 delta = mouse_delta_position();
    *latitude += delta.x * 0.05f;
    *longitude -= delta.y * 0.05f;

    *longitude = clamp(*longitude, -179.99f, 0.0f);


    v3 looking = get_camera_to_target(1.0f, *latitude, *longitude);

    
    v3 forward;
#if 0
    forward = unit(looking);
#else
    forward = v3(looking.x, 0.0f, looking.z);
    forward = unit(forward);
#endif

    v3 right_axis = cross(forward, WORLD_UP_VECTOR);
    if(length(right_axis) == 0.0f) right_axis = v3(1.0f, 0.0f, 0.0f);
    right_axis = unit(right_axis);

    v3 up_axis = cross(right_axis, forward);
    up_axis = unit(up_axis);

    float speed = 0.001f;
    if(key_state('W')) *position += forward * speed;
    if(key_state('S')) *position -= forward * speed;
    if(key_state('A')) *position -= right_axis * speed;
    if(key_state('D')) *position += right_axis * speed;
    if(key_state(' ')) *position += up_axis * speed;
    if(key_state(VK_CONTROL)) *position -= up_axis * speed;
  }

  
}

