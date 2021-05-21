#include "world.h"
#include "my_math.h"

#include "graphics.h"

//static const v3 WORLD_UP_VECTOR = {0.0f, 1.0f, 0.0f};


static Model hat_handle;
static Model teapot_handle;
static Model ground_handle;

static float camera_latitude = 90.0f;
static float camera_longitude = -90.0f;

bool cursor_locked = true;

// In degrees
static v3 spherical_to_cartesian(float radius, float latitude, float longitude)
{
  float x = radius * sin(deg_to_rad(longitude)) * cos(deg_to_rad(latitude));
  float z = radius * sin(deg_to_rad(longitude)) * sin(deg_to_rad(latitude));
  float y = radius * cos(deg_to_rad(longitude));
  return v3(x, y, z);
}


void init_world()
{
  hat_handle = create_model("assets/wizard_hat.obj", v3(0.0f, 1.55f, 0.0f), v3(1.0f, 1.0f, 1.0f), v3(0.0f, -45.0f, 0.0f));
  set_model_color(hat_handle, Color(0.4f, 0.0f, 0.5f));

  teapot_handle = create_model("assets/teapot.obj");
  set_model_color(teapot_handle, Color(0.0f, 0.0f, 0.5f));

  ground_handle = create_model("assets/terrain.obj", v3(), v3(1000.0f, 1.0f, 1000.0f));
  set_model_color(ground_handle, Color(0.1f, 0.2f, 0.0f));
}

void update_world()
{
  /*
  static bool pressed = true;
  if(key_state(VK_SHIFT) == true && !pressed)
  {
    cursor_locked = !cursor_locked;
  }
  pressed = key_state(VK_SHIFT);
  */


  if(cursor_locked)
  {
    v3 player_position = get_model_position(teapot_handle);
    float player_rotation = get_model_rotation(teapot_handle).y;

    v3 camera_position = get_camera_position();
    v3 camera_looking = get_camera_looking_direction();

    v2 delta = v2();//mouse_delta_position();
    float mouse_sensitivity = 0.1f;
    camera_latitude += delta.x * mouse_sensitivity;
    camera_longitude += delta.y * mouse_sensitivity;
    camera_longitude = clamp(camera_longitude, -179.9f, -0.1f);

    v3 camera_offset = spherical_to_cartesian(8.0f, camera_latitude, camera_longitude);
    camera_position = player_position + camera_offset;


    camera_looking = ((player_position + v3(0.0f, 1.0f, 0.0f) * 1.75f) - camera_position);
    camera_looking = unit(camera_looking);


    {
      v3 forward = v3(camera_looking.x, 0.0f, camera_looking.z);
      forward = unit(forward);

      v3 right_axis = cross(forward, v3(0.0f, 1.0f, 0.0f));
      if(length(right_axis) == 0.0f) right_axis = v3(1.0f, 0.0f, 0.0f);
      right_axis = unit(right_axis);

      v3 up_axis = cross(right_axis, forward);
      up_axis = unit(up_axis);

      v3 direction = v3();
      /*
      if(key_state('W')) direction += forward;
      if(key_state('S')) direction -= forward;
      if(key_state('A')) direction -= right_axis;
      if(key_state('D')) direction += right_axis;
      if(key_state(' ')) direction += up_axis;
      if(key_state(VK_CONTROL)) direction -= up_axis;
      */

      static v3 velocity;

      float acceleration_speed = 0.0003f;
      v3 acceleration = direction * acceleration_speed;
      velocity += acceleration;


      float deceleration_speed = 0.005f;
      v3 deceleration = -velocity * deceleration_speed;
      velocity += deceleration;


      float max_speed = 0.05f;
      velocity = clamp_length(velocity, max_speed);

      player_position += velocity;


      if(length(velocity) != 0.0f)
      {
        player_rotation = -rad_to_deg(atan2f(velocity.z, velocity.x));
        set_model_rotation(hat_handle, v3(0.0f, -rad_to_deg(atan2f(velocity.z, velocity.x)) + 90.0f, 0.0f));
      }
      else
      {
        player_rotation = -rad_to_deg(atan2f(camera_looking.z, camera_looking.x));
        set_model_rotation(hat_handle, v3(0.0f, -camera_latitude + 90.0f, 0.0f));
      }


      set_model_position(hat_handle, player_position + v3(0.0f, 0.6f, 0.0f));
    }

    set_camera_position(camera_position);
    set_camera_looking_direction(camera_looking);
  }
}

