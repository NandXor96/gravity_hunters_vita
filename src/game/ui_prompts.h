#pragma once
#include <stdbool.h>

struct Renderer;
struct Services;

/* Draw generic confirmation prompts using Vita button icons. */
void ui_draw_prompts(struct Renderer *r, struct Services *svc, bool show_ok, bool show_back);
