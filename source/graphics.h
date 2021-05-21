
#pragma once

#include "my_math.h" // v3, Color

typedef int Model;

Model create_model(const char *model_name, v3 position = v3(), v3 scale = v3(1.0f, 1.0f, 1.0f), v3 rotation = v3());

void set_model_position(Model model, v3 pos);
v3   get_model_position(Model model);
void change_model_position(Model model, v3 offset);

void set_model_scale(Model model, v3 scale);
v3   get_model_scale(Model model);
void change_model_scale(Model model, v3 scale);

void set_model_rotation(Model model, v3 rotation);
v3   get_model_rotation(Model model);
void change_model_rotation(Model model, v3 rotation);

void set_model_color(Model model, Color color);



void set_camera_position(v3 position);
v3 get_camera_position();

void set_camera_looking_direction(v3 direction);
v3 get_camera_looking_direction();

