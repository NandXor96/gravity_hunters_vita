#pragma once
#include <stdbool.h>

struct Renderer;
struct Services;

/* Draws standard confirmation prompts using Vita button icons. */
void ui_draw_ok_back_prompts(struct Renderer *r, struct Services *svc, bool include_back);
