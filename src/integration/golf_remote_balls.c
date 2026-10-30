#include "integration/golf_multiplayer.h"

#include "common/data.h"
#include "common/graphics.h"
#include "common/maths.h"
#include "golf/game.h"
#include "golf/golf.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

void golf_mp_draw_remote_balls(void) {
    size_t count = 0;
    size_t i;
    const tg_net_player *players;
    golf_t *golf = golf_get();
    golf_game_t *game = golf_game_get();

    if (golf->state != GOLF_STATE_IN_GAME ||
        game->state == GOLF_GAME_STATE_MAIN_MENU) {
        return;
    }

    players = golf_mp_get_remote_players(&count);
    if (!players || count == 0) return;

    {
        golf_shader_t *shader = golf_data_get_shader("data/shaders/ball.glsl");
        golf_shader_pipeline_t *pipeline;
        golf_model_t *model = golf_data_get_model("data/models/golf_ball.obj");
        golf_texture_t *texture = golf_data_get_texture("data/textures/golf_ball_normal_map.jpg");
        golf_graphics_t *graphics = golf_graphics_get();

        if (!shader || !model || !texture) return;
        pipeline = golf_shader_get_pipeline(shader, "ball");
        if (!pipeline) return;
        sg_apply_pipeline(pipeline->sg_pipeline);

        for (i = 0; i < count; ++i) {
            const tg_net_player *player = &players[i];
            vec3 ball_pos = V3(player->ball_x, player->ball_y, player->ball_z);
            vec3 ball_scale = V3(game->ball.radius, game->ball.radius, game->ball.radius);
            quat orientation = QUAT(player->ball_qx, player->ball_qy, player->ball_qz, player->ball_qw);
            mat4 model_mat;
            sg_bindings bindings;
            golf_shader_uniform_t *vs_uniform;
            golf_shader_uniform_t *fs_uniform;
            uint32_t packed = player->ball_colour_rgba ? player->ball_colour_rgba : TG_NET_DEFAULT_BALL_COLOUR_RGBA;
            vec4 color;

            if (fabsf(ball_pos.x) > 10000.0f || fabsf(ball_pos.y) > 10000.0f ||
                fabsf(ball_pos.z) > 10000.0f) {
                continue;
            }

            color = V4(
                ((packed >> 24) & 0xFFu) / 255.0f,
                ((packed >> 16) & 0xFFu) / 255.0f,
                ((packed >> 8) & 0xFFu) / 255.0f,
                (packed & 0xFFu) / 255.0f);
            model_mat = mat4_multiply_n(3,
                mat4_translation(ball_pos),
                mat4_scale(ball_scale),
                mat4_from_quat(orientation));

            memset(&bindings, 0, sizeof(bindings));
            bindings.vertex_buffers[0] = model->sg_positions_buf;
            bindings.vertex_buffers[1] = model->sg_normals_buf;
            bindings.vertex_buffers[2] = model->sg_texcoords_buf;
            bindings.fs_images[0] = texture->sg_image;
            sg_apply_bindings(&bindings);

            vs_uniform = golf_shader_vs_uniform_setup(shader, "ball_vs_params", 2,
                UNIFORM_MAT4("proj_view_mat", mat4_transpose(graphics->proj_view_mat)),
                UNIFORM_MAT4("model_mat", mat4_transpose(model_mat)));
            sg_apply_uniforms(SG_SHADERSTAGE_VS, 0,
                &(sg_range){vs_uniform->data, vs_uniform->size});

            fs_uniform = golf_shader_fs_uniform_setup(shader, "ball_fs_params", 1,
                UNIFORM_VEC4("color", color));
            sg_apply_uniforms(SG_SHADERSTAGE_FS, 0,
                &(sg_range){fs_uniform->data, fs_uniform->size});

            sg_draw(0, model->positions.length, 1);
        }
    }
}
