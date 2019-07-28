#include "world.h"
#include "my_math.h"

#include "renderer.h"

bool key_state(u32 button);
v2 mouse_window_position();
v2 mouse_delta_position();
void set_cursor_locked(bool locked);

static bool cursor_locked = false;
static const v3 WORLD_UP_VECTOR = {0.0f, 1.0f, 0.0f};


static ModelHandle hat_handle;
static ModelHandle teapot_handle;
static ModelHandle ground_handle;

// In degrees
v3 get_camera_to_target(float radius, float latitude, float longitude)
{
  float x = radius * sin(deg_to_rad(longitude)) * cos(deg_to_rad(latitude));
  float z = radius * sin(deg_to_rad(longitude)) * sin(deg_to_rad(latitude));
  float y = radius * cos(deg_to_rad(longitude));
  return v3(x, y, z);
}


void init_world()
{
  hat_handle = create_model("assets/wizard_hat.obj");
  Model *hat = get_temp_model_pointer(hat_handle);
  hat->position = v3(0.0f, 1.55f, 0.0f);

  teapot_handle = create_model("assets/teapot.obj");
  Model *teapot = get_temp_model_pointer(teapot_handle);
  teapot->position = v3(0.0f, 1.0f, 0.0f);

  ground_handle = create_model(PRIMITIVE_QUAD, "assets/wizard_hat.png");

  Model *ground = get_temp_model_pointer(ground_handle);
  ground->scale = v3(10.0f, 1.0f, 10.0f);
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


  if(cursor_locked)
  {
    v3 *cam_position = camera_position();
    float *latitude = camera_latitude();
    float *longitude = camera_longitude();

    v2 delta = mouse_delta_position();
    *latitude += delta.x * 0.05f;
    *longitude -= delta.y * 0.05f;

    *longitude = clamp(*longitude, -179.99f, 0.0f);


    v3 looking = get_camera_to_target(1.0f, *latitude, *longitude);

    

    Model *teapot = get_temp_model_pointer(teapot_handle);
    v3 *position = &teapot->position;

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


    *cam_position = *position + v3(0.0f, 1.5f, 3.0f);

    Model *hat = get_temp_model_pointer(hat_handle);
    hat->position = *position + v3(0.0f, 0.55f, 0.0f);
  }

  
}

