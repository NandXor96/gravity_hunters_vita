#include "scene_campaign.h"
#include <stdlib.h>
#include "../services/services.h"
#include "../services/renderer.h"
#include "../app/app.h"
void scene_campaign_enter(Scene* s){ SceneCampaignState* st=calloc(1,sizeof(SceneCampaignState)); s->state=st; st->svc=app_services(); }
void scene_campaign_leave(Scene* s){ free(s->state); }
void scene_campaign_handle_input(Scene* s,const struct InputState* in){ (void)s;(void)in; }
void scene_campaign_update(Scene* s,float dt){ (void)s;(void)dt; }
void scene_campaign_render(Scene* s, struct Renderer* r){ renderer_draw_text(r,"CAMPAIGN",40,40,(TextStyle){0}); }
