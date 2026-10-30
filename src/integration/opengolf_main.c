/*
 * Project-owned application entry point based on Open-Golf src/golf/main.c.
 * Original upstream code: Copyright (c) 2020 Michael Gerdes, MIT License.
 * See vendor/Open-Golf/LICENSE and docs/THIRD_PARTY_NOTICES.md.
 *
 * The renderer, game, UI, physics, level loader and asset system remain
 * upstream. This entry point adds the Golfgame multiplayer lifecycle.
 */
#include <stdbool.h>
#include <stdio.h>

#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_imgui.h"
#include "sokol/sokol_time.h"
#include "common/audio.h"
#include "common/common.h"
#include "common/data.h"
#include "common/debug_console.h"
#include "common/graphics.h"
#include "common/inputs.h"
#include "common/log.h"
#include "common/storage.h"
#include "golf/draw.h"
#include "golf/game.h"
#include "golf/golf.h"
#include "golf/ui.h"
#include "integration/golf_multiplayer.h"

static void startup_log(const char *message) {
    fprintf(stderr, "[golfgame] %s\n", message ? message : "(no message)");
    fflush(stderr);
}

static void app_fail(const char *message) {
    fprintf(stderr, "[golfgame] Sokol startup failure: %s\n", message ? message : "(no details)");
    fflush(stderr);
}

static void init(void) {
    startup_log("Sokol graphics context created; entering application init.");
    stm_setup();
    sg_setup(&(sg_desc){
        .buffer_pool_size = 2048,
        .image_pool_size = 2048,
        .context = sapp_sgcontext(),
    });
    simgui_setup(&(simgui_desc_t){
        .dpi_scale = sapp_dpi_scale()
    });
    startup_log("Sokol, graphics and ImGui initialisation complete.");
}

static void cleanup(void) {
    startup_log("Shutting down Golfgame multiplayer and graphics.");
    golf_mp_shutdown();
    sg_shutdown();
}

static void frame(void) {
    static bool storage_inited = false;
    static bool inited = false;
    static uint64_t last_time = 0;

    float dt = (float)stm_sec(stm_laptime(&last_time));
    if (!inited) {
        startup_log("Initialising Open-Golf game systems.");
        golf_data_init();
        golf_data_load("data/static_data.static_data", false);

        golf_storage_init();
        golf_audio_init();
        golf_debug_console_init();
        golf_inputs_init();
        golf_graphics_init();
        golf_draw_init();
        golf_init();
        startup_log("Initialising Golfgame LAN transport.");
        golf_mp_init();
        inited = true;
        startup_log("Game systems initialised; first frame will render.");
    }

    if (!storage_inited && !golf_storage_finish_init()) {
        return;
    }
    storage_inited = true;

    golf_data_update(dt);

    golf_graphics_begin_frame(dt);
    golf_inputs_begin_frame();

    golf_update(dt);
    golf_mp_update(dt);

    golf_graphics_set_viewport(V2(0, 0), V2((float)sapp_width(), (float)sapp_height()));
    golf_graphics_update_proj_view_mat();
    golf_draw();

    golf_inputs_end_frame();
    golf_graphics_end_frame();
}

static void event(const sapp_event *event) {
    simgui_handle_event(event);
    golf_inputs_handle_event(event);
}

sapp_desc sokol_main(int argc, char *argv[]) {
    GOLF_UNUSED(argc);
    GOLF_UNUSED(argv);

    startup_log("Sokol application entry point reached.");
    golf_alloc_init();
    golf_log_init();
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .fail_cb = app_fail,
        .width = 1280,
        .height = 720,
        .window_title = "Golfgame",
        .enable_clipboard = true,
        .clipboard_size = 1024,
        .fullscreen = false,
        .high_dpi = false,
        .html5_canvas_resize = false,
        .win32_console_utf8 = true,
        .win32_console_create = true,
        .swap_interval = 1,
    };
}
