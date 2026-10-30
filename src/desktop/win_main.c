#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "tg_network.h"

#define APP_TITLE "Golfgame"
#define PI_F 3.14159265358979323846f
#define COURSE_START_X 1.0f
#define COURSE_HOLE_X 21.2f
#define COURSE_HALF_WIDTH 4.65f
#define BALL_RADIUS 0.23f

typedef enum screen_state {
    SCREEN_MENU,
    SCREEN_LOBBY,
    SCREEN_COURSE
} screen_state;

typedef struct point2 { float x, y; } point2;

static HWND g_window;
static HDC g_dc;
static HGLRC g_gl;
static GLuint g_font_base;
static GLUquadric *g_quadric;
static int g_width = 1280, g_height = 720;
static screen_state g_screen = SCREEN_MENU;
static tg_net_player g_local;
static tg_net_info g_net_info;
static int g_wins;
static int g_profile_loaded;
static int g_show_scorecard;
static int g_mouse_down, g_shot_release;
static int g_mouse_x, g_mouse_y, g_drag_x, g_drag_y;
static int g_click, g_click_x, g_click_y;
static int g_input_focus;
static char g_player_name[TG_NET_PLAYER_NAME] = "Player";
static char g_room_name[TG_NET_ROOM_NAME] = "Friday Match";
static char g_join_address[48] = "127.0.0.1";
static int g_rooms_refreshing;
static int g_colour_index;
static int g_hat_index;
static int g_trail_index;
static float g_ball_vx, g_ball_vz;
static float g_elapsed;
static float g_ui_flash;
static int g_local_finished;
static int g_win_awarded;
static int g_course_id;
static float g_moving_obstacle_angle;
static int g_running = 1;

static const uint32_t ball_colours[] = {
    0xF04A4AFFu, 0x38B7F0FFu, 0x64D96BFFu, 0xE6C23CFFu,
    0xB77CF2FFu, 0xFF79B8FFu, 0xF28A36FFu, 0xE9E9E9FFu
};
static const char *ball_colour_names[] = {
    "Cherry", "Ocean", "Lime", "Gold", "Violet", "Bubblegum", "Tangerine", "Pearl"
};

static float clampf_local(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
static float length2(float x, float y) { return sqrtf(x*x + y*y); }

static void profile_load(void) {
    if (g_profile_loaded) return;
    g_wins = (int)GetPrivateProfileIntA("profile", "wins", 0, "profile.ini");
    g_colour_index = (int)GetPrivateProfileIntA("cosmetics", "colour", 0, "profile.ini");
    g_hat_index = (int)GetPrivateProfileIntA("cosmetics", "hat", 0, "profile.ini");
    g_trail_index = (int)GetPrivateProfileIntA("cosmetics", "trail", 0, "profile.ini");
    if (g_colour_index < 0 || g_colour_index >= (int)(sizeof(ball_colours)/sizeof(ball_colours[0]))) g_colour_index = 0;
    if (g_hat_index < 0 || g_hat_index > 3) g_hat_index = 0;
    if (g_trail_index < 0 || g_trail_index > 2) g_trail_index = 0;
    g_local.ball_colour_rgba = ball_colours[g_colour_index];
    g_local.hat_id = (uint16_t)g_hat_index;
    g_local.trail_id = (uint16_t)g_trail_index;
    g_profile_loaded = 1;
}
static void profile_save(void) {
    char n[24], c[24], h[24], t[24];
    snprintf(n, sizeof(n), "%d", g_wins);
    snprintf(c, sizeof(c), "%d", g_colour_index);
    snprintf(h, sizeof(h), "%d", g_hat_index);
    snprintf(t, sizeof(t), "%d", g_trail_index);
    WritePrivateProfileStringA("profile", "wins", n, "profile.ini");
    WritePrivateProfileStringA("cosmetics", "colour", c, "profile.ini");
    WritePrivateProfileStringA("cosmetics", "hat", h, "profile.ini");
    WritePrivateProfileStringA("cosmetics", "trail", t, "profile.ini");
}
static void update_cosmetics(void) {
    if (g_colour_index < 0) g_colour_index = 0;
    g_colour_index %= (int)(sizeof(ball_colours)/sizeof(ball_colours[0]));
    if (g_hat_index > 0 && g_wins < (g_hat_index == 1 ? 1 : (g_hat_index == 2 ? 3 : 10))) g_hat_index = 0;
    if (g_trail_index > 0 && g_wins < (g_trail_index == 1 ? 5 : 15)) g_trail_index = 0;
    g_local.ball_colour_rgba = ball_colours[g_colour_index];
    g_local.hat_id = (uint16_t)g_hat_index;
    g_local.trail_id = (uint16_t)g_trail_index;
    profile_save();
}

static void text2d(float x, float y, const char *s, float r, float g, float b) {
    if (!s || !*s || !g_font_base) return;
    glColor3f(r, g, b);
    glRasterPos2f(x, (float)g_height - y);
    glListBase(g_font_base);
    glCallLists((GLsizei)strlen(s), GL_UNSIGNED_BYTE, s);
}
static void textf(float x, float y, float r, float g, float b, const char *fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    text2d(x, y, buffer, r, g, b);
}
static void rect2d(float x, float y, float w, float h, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y); glVertex2f(x+w, y);
    glVertex2f(x+w, y+h); glVertex2f(x, y+h);
    glEnd();
}
static void outline2d(float x, float y, float w, float h, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y); glVertex2f(x+w, y);
    glVertex2f(x+w, y+h); glVertex2f(x, y+h);
    glEnd();
}
static int hit_rect(float x, float y, float w, float h) {
    return g_click && g_click_x >= x && g_click_x <= x+w && g_click_y >= y && g_click_y <= y+h;
}
static int button2d(float x, float y, float w, float h, const char *label, int primary) {
    int hover = g_mouse_x >= x && g_mouse_x <= x+w && g_mouse_y >= y && g_mouse_y <= y+h;
    float shade = hover ? 0.18f : 0.12f;
    rect2d(x, y, w, h, primary ? 0.10f+shade : 0.10f+shade*0.4f,
           primary ? 0.52f+shade : 0.16f+shade*0.4f,
           primary ? 0.55f+shade : 0.20f+shade*0.4f, 1);
    outline2d(x, y, w, h, primary ? 0.50f : 0.34f, primary ? 0.86f : 0.40f, primary ? 0.88f : 0.45f, 0.9f);
    text2d(x+14, y+h*0.63f, label, 0.96f, 0.98f, 1.0f);
    return hit_rect(x,y,w,h);
}
static void panel2d(float x, float y, float w, float h) {
    rect2d(x, y, w, h, 0.035f, 0.075f, 0.105f, 0.94f);
    outline2d(x, y, w, h, 0.20f, 0.62f, 0.68f, 0.65f);
}
static void input2d(float x, float y, float w, const char *label, const char *value, int focus) {
    text2d(x, y-8, label, 0.63f, 0.77f, 0.81f);
    rect2d(x, y, w, 34, 0.07f, 0.12f, 0.15f, 1);
    outline2d(x, y, w, 34, focus ? 0.30f : 0.18f, focus ? 0.82f : 0.36f, focus ? 0.86f : 0.42f, 1);
    text2d(x+10, y+23, value, 0.94f, 0.97f, 0.97f);
}

static void box3d(float x, float y, float z, float sx, float sy, float sz,
                  float r, float g, float b) {
    float x0=x-sx*0.5f, x1=x+sx*0.5f;
    float y0=y-sy*0.5f, y1=y+sy*0.5f;
    float z0=z-sz*0.5f, z1=z+sz*0.5f;
    glBegin(GL_QUADS);
    glColor3f(r*0.82f,g*0.82f,b*0.82f);
    glVertex3f(x0,y0,z0); glVertex3f(x0,y0,z1); glVertex3f(x0,y1,z1); glVertex3f(x0,y1,z0);
    glVertex3f(x1,y0,z1); glVertex3f(x1,y0,z0); glVertex3f(x1,y1,z0); glVertex3f(x1,y1,z1);
    glColor3f(r*0.92f,g*0.92f,b*0.92f);
    glVertex3f(x0,y0,z1); glVertex3f(x1,y0,z1); glVertex3f(x1,y1,z1); glVertex3f(x0,y1,z1);
    glVertex3f(x1,y0,z0); glVertex3f(x0,y0,z0); glVertex3f(x0,y1,z0); glVertex3f(x1,y1,z0);
    glColor3f(r,g,b);
    glVertex3f(x0,y1,z0); glVertex3f(x0,y1,z1); glVertex3f(x1,y1,z1); glVertex3f(x1,y1,z0);
    glColor3f(r*0.72f,g*0.72f,b*0.72f);
    glVertex3f(x0,y0,z1); glVertex3f(x0,y0,z0); glVertex3f(x1,y0,z0); glVertex3f(x1,y0,z1);
    glEnd();
}
static void disk_xz(float x, float y, float z, float radius, float r, float g, float b) {
    int i;
    glColor3f(r,g,b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(x,y,z);
    for(i=0;i<=24;i++) {
        float a=(float)i*(2.0f*PI_F/24.0f);
        glVertex3f(x+cosf(a)*radius,y,z+sinf(a)*radius);
    }
    glEnd();
}
static void ramp3d(void) {
    float x0=4.7f, x1=8.4f, z0=-2.6f, z1=2.6f, h=1.05f;
    glBegin(GL_TRIANGLES);
    glColor3f(0.28f,0.61f,0.31f);
    glVertex3f(x0,0.0f,z0); glVertex3f(x1,h,z0); glVertex3f(x1,h,z1);
    glVertex3f(x0,0.0f,z0); glVertex3f(x1,h,z1); glVertex3f(x0,0.0f,z1);
    glColor3f(0.20f,0.45f,0.24f);
    glVertex3f(x0,0.0f,z0); glVertex3f(x0,0.0f,z1); glVertex3f(x1,h,z1);
    glVertex3f(x0,0.0f,z0); glVertex3f(x1,h,z0); glVertex3f(x1,h,z1);
    glEnd();
    box3d(x0,0.0f,0,0.16f,0.18f,5.2f,0.78f,0.72f,0.57f);
}
static float course_height(float x) {
    if (x >= 4.7f && x < 6.55f) return (x-4.7f)*(1.05f/1.85f);
    if (x >= 6.55f && x < 8.4f) return (8.4f-x)*(1.05f/1.85f);
    return 0.0f;
}
static void cylinder_y(float x, float y, float z, float radius, float height,
                       float r, float g, float b) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(-90.0f,1,0,0);
    glColor3f(r,g,b);
    if(g_quadric) {
        gluCylinder(g_quadric,radius,radius,height,16,1);
        glRotatef(180,1,0,0);
        gluDisk(g_quadric,0,radius,16,1);
        glRotatef(180,1,0,0);
        glTranslatef(0,0,height);
        gluDisk(g_quadric,0,radius,16,1);
    }
    glPopMatrix();
}
static void draw_course(float t) {
    int i;
    /* Darker base and clean, raised green play surface. */
    box3d(11.0f,-0.42f,0,26.8f,0.55f,11.8f,0.54f,0.39f,0.24f);
    box3d(11.0f,-0.09f,0,25.8f,0.18f,10.5f,0.23f,0.64f,0.30f);
    /* Pale course edging and dark outer lip. */
    box3d(11.0f,0.13f,-5.23f,26.0f,0.34f,0.38f,0.92f,0.88f,0.72f);
    box3d(11.0f,0.13f, 5.23f,26.0f,0.34f,0.38f,0.92f,0.88f,0.72f);
    box3d(-1.85f,0.13f,0,0.38f,0.34f,10.5f,0.92f,0.88f,0.72f);
    box3d(23.85f,0.13f,0,0.38f,0.34f,10.5f,0.92f,0.88f,0.72f);
    /* Fairway stripes make the green readable without noisy texture. */
    for(i=0;i<9;i++) {
        float x=0.15f+(float)i*2.55f;
        box3d(x,0.008f,0,1.22f,0.018f,9.85f,0.24f+(i%2)*0.015f,0.67f,0.31f);
    }
    /* Little tee pad. */
    box3d(1.05f,0.04f,0,1.4f,0.09f,2.0f,0.32f,0.73f,0.36f);
    /* Ramp and raised plateau. */
    ramp3d();
    box3d(7.7f,0.55f,0,1.7f,0.10f,5.1f,0.34f,0.69f,0.36f);
    /* Soft bumpers, bright caps. */
    cylinder_y(10.0f,0.04f,-1.8f,0.46f,0.62f,0.95f,0.47f,0.25f);
    cylinder_y(10.0f,0.04f, 1.8f,0.46f,0.62f,0.95f,0.47f,0.25f);
    disk_xz(10.0f,0.68f,-1.8f,0.43f,1.0f,0.78f,0.36f);
    disk_xz(10.0f,0.68f, 1.8f,0.43f,1.0f,0.78f,0.36f);
    /* A slowly rotating padded arm across the fairway. */
    glPushMatrix();
    glTranslatef(14.1f,0.5f,0);
    glRotatef(t*38.0f,0,1,0);
    box3d(0,0,0,0.34f,0.85f,5.1f,0.94f,0.49f,0.20f);
    box3d(0,0.48f,0,0.62f,0.13f,0.62f,0.96f,0.82f,0.30f);
    glPopMatrix();
    /* Cup, rim and flag. */
    disk_xz(COURSE_HOLE_X,0.018f,0,0.49f,0.92f,0.91f,0.80f);
    disk_xz(COURSE_HOLE_X,0.025f,0,0.34f,0.045f,0.065f,0.075f);
    box3d(COURSE_HOLE_X,1.08f,0,0.055f,2.1f,0.055f,0.83f,0.84f,0.80f);
    glBegin(GL_TRIANGLES);
    glColor3f(0.93f,0.22f,0.22f);
    glVertex3f(COURSE_HOLE_X,2.05f,0);
    glVertex3f(COURSE_HOLE_X+0.85f,1.78f,0);
    glVertex3f(COURSE_HOLE_X,1.52f,0);
    glEnd();
    /* Small decorative markers near the cup. */
    for(i=0;i<3;i++) {
        float x=17.2f+(float)i*0.72f;
        box3d(x,0.18f,-3.7f,0.35f,0.36f,0.35f,0.82f,0.79f,0.66f);
    }
}
static void draw_hat(uint16_t hat_id) {
    if(!hat_id || !g_quadric) return;
    glPushMatrix();
    glTranslatef(0,BALL_RADIUS*0.82f,0);
    glRotatef(-90.0f,1,0,0);
    if(hat_id==1) { /* party cone */
        glColor3f(0.98f,0.45f,0.60f);
        gluCylinder(g_quadric,0.18f,0.015f,0.33f,9,1);
    } else if(hat_id==2) { /* top hat */
        glColor3f(0.12f,0.15f,0.18f);
        gluDisk(g_quadric,0,0.26f,16,1);
        glTranslatef(0,0.03f,0);
        gluCylinder(g_quadric,0.16f,0.16f,0.31f,16,1);
        glTranslatef(0,0,0.31f);
        gluDisk(g_quadric,0,0.16f,16,1);
    } else if(hat_id==3) { /* little crown */
        int i;
        glColor3f(0.96f,0.73f,0.20f);
        gluCylinder(g_quadric,0.19f,0.17f,0.15f,12,1);
        for(i=0;i<5;i++) {
            float a=(float)i*(2.0f*PI_F/5.0f);
            glPushMatrix();
            glTranslatef(cosf(a)*0.17f,0.16f,sinf(a)*0.17f);
            glColor3f(1.0f,0.86f,0.29f);
            gluSphere(g_quadric,0.055f,6,4);
            glPopMatrix();
        }
    }
    glPopMatrix();
}
static void draw_ball(const tg_net_player *p, int local, float alpha) {
    float r=(float)((p->ball_colour_rgba>>24)&255)/255.0f;
    float g=(float)((p->ball_colour_rgba>>16)&255)/255.0f;
    float b=(float)((p->ball_colour_rgba>>8)&255)/255.0f;
    float y=p->ball_y;
    glPushMatrix();
    glTranslatef(p->ball_x,y,p->ball_z);
    if(p->ball_qx!=0 || p->ball_qy!=0 || p->ball_qz!=0) {
        float angle=2.0f*acosf(clampf_local(p->ball_qw,-1,1))*180.0f/PI_F;
        glRotatef(angle,p->ball_qx,p->ball_qy,p->ball_qz);
    }
    glColor4f(r,g,b,alpha);
    if(g_quadric) gluSphere(g_quadric,BALL_RADIUS,14,9);
    if(p->hat_id) draw_hat(p->hat_id);
    if(p->trail_id && (p->flags&1)) {
        int i;
        for(i=1;i<=5;i++) {
            float k=(float)i/6.0f;
            float tr=0.92f, tg=0.95f, tb=1.0f;
            if(p->trail_id==1) { tr=1.0f; tg=0.83f; tb=0.32f; }
            else if(p->trail_id==2) { tr=0.40f+0.10f*i; tg=0.85f; tb=1.0f; }
            glPushMatrix();
            glTranslatef(-0.17f*i,0.01f+(float)(i%2)*0.025f,0);
            glColor4f(tr,tg,tb,alpha*(1.0f-k)*0.62f);
            if(g_quadric) gluSphere(g_quadric,BALL_RADIUS*(1.0f-k)*0.62f,8,5);
            glPopMatrix();
        }
    }
    glPopMatrix();
    if(local) {
        /* A thin player ring, not a character/avatar. */
        glDisable(GL_LIGHTING);
        glColor4f(1,1,1,0.72f);
        glBegin(GL_LINE_LOOP);
        {
            int i;
            for(i=0;i<32;i++) {
                float a=(float)i*(2.0f*PI_F/32.0f);
                glVertex3f(p->ball_x+cosf(a)*0.33f,0.035f,p->ball_z+sinf(a)*0.33f);
            }
        }
        glEnd();
    }
}
static void draw_aim_line(void) {
    if(!g_mouse_down || g_screen!=SCREEN_COURSE || g_local_finished) return;
    {
        float dx=(float)(g_mouse_x-g_drag_x);
        float dy=(float)(g_mouse_y-g_drag_y);
        float d=length2(dx,dy);
        float power=clampf_local(d/170.0f,0,1);
        float dirx=dx/(d>0?d:1);
        float dirz=-dy/(d>0?d:1);
        glDisable(GL_LIGHTING);
        glLineWidth(5.0f);
        glColor4f(0.98f,0.28f,0.22f,0.90f);
        glBegin(GL_LINES);
        glVertex3f(g_local.ball_x,course_height(g_local.ball_x)+0.12f,g_local.ball_z);
        glVertex3f(g_local.ball_x+dirx*(1.1f+power*4.6f),
                   course_height(g_local.ball_x)+0.12f,
                   g_local.ball_z+dirz*(1.1f+power*4.6f));
        glEnd();
    }
}
static void render_world(void) {
    tg_net_player players[TG_NET_MAX_PLAYERS];
    size_t count=0,i;
    const tg_net_player *net_players=tg_net_get_players(&count);
    if(count>TG_NET_MAX_PLAYERS) count=TG_NET_MAX_PLAYERS;
    if(net_players && count) memcpy(players,net_players,count*sizeof(tg_net_player));
    else { players[0]=g_local; count=1; }
    glViewport(0,0,g_width,g_height);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(0.46f,0.73f,0.88f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(52.0,(double)g_width/(double)(g_height>0?g_height:1),0.1,100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if(g_screen==SCREEN_COURSE) {
        gluLookAt(g_local.ball_x-6.6f, 7.4f, g_local.ball_z+8.2f,
                  g_local.ball_x+3.5f, 0.15f, g_local.ball_z,
                  0,1,0);
    } else {
        gluLookAt(10.5, 12.0, 17.0, 11.0,0.0,0.0, 0,1,0);
    }
    draw_course(g_elapsed);
    for(i=0;i<count;i++) {
        int local=(players[i].id==g_net_info.local_player_id && g_net_info.local_player_id!=0);
        draw_ball(&players[i],local,1.0f);
    }
    if(count==0) draw_ball(&g_local,1,1.0f);
    glDisable(GL_LIGHTING);
    draw_aim_line();
    glDisable(GL_DEPTH_TEST);
}
static void begin_2d(void) {
    glViewport(0,0,g_width,g_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,g_width,g_height,0,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}
static void draw_scorecard(int force) {
    size_t count=0,i;
    const tg_net_player *players=tg_net_get_players(&count);
    if(!force && !g_show_scorecard) return;
    if(count>TG_NET_MAX_PLAYERS) count=TG_NET_MAX_PLAYERS;
    panel2d(170,18,(float)g_width-340.0f,48.0f+26.0f*(float)(count>5?5:count));
    text2d(190,43,"SCORECARD",0.72f,0.91f,0.93f);
    text2d((float)g_width-310.0f,43,"HOLE 1",0.97f,0.89f,0.61f);
    for(i=0;i<count && i<5;i++) {
        const tg_net_player *p=&players[i];
        float x=190.0f+(float)i*175.0f;
        textf(x,70,0.94f,0.96f,0.96f,"%s",p->name);
        textf(x+110,70,0.97f,0.78f,0.36f,"%u",(unsigned)p->strokes);
    }
    if(count>5) textf((float)g_width-250.0f,70,0.7f,0.85f,0.87f,"+%u more",(unsigned)(count-5));
}
static void draw_hud(void) {
    float power=0;
    if(g_mouse_down) power=clampf_local(length2((float)(g_mouse_x-g_drag_x),(float)(g_mouse_y-g_drag_y))/170.0f,0,1);
    panel2d(22,(float)g_height-124,330,100);
    text2d(42,(float)g_height-96,"POWER",0.91f,0.94f,0.94f);
    rect2d(42,(float)g_height-82,286,14,0.035f,0.055f,0.07f,1);
    rect2d(44,(float)g_height-80,282*power,10,0.92f*(1-power)+0.19f*power,
           0.38f+0.38f*power,0.30f,1);
    rect2d(42,(float)g_height-57,92,25,0.11f,0.65f,0.70f,1);
    rect2d(137,(float)g_height-57,92,25,0.17f,0.37f,0.48f,1);
    rect2d(232,(float)g_height-57,96,25,0.85f,0.65f,0.22f,1);
    text2d(49,(float)g_height-40,"HOLE  1",0.02f,0.08f,0.10f);
    text2d(150,(float)g_height-40,"PAR  3",0.97f,0.99f,1);
    textf(244,(float)g_height-40,0.07f,0.08f,0.09f,"STROKE  %u",(unsigned)g_local.strokes);
    textf((float)g_width-250.0f,(float)g_height-40,0.92f,0.95f,0.95f,"%u PLAYERS",(unsigned)g_net_info.player_count);
    text2d(22,26,"DRAG TO AIM  |  RELEASE TO PUTT",0.89f,0.94f,0.94f);
    text2d((float)g_width-330.0f,(float)g_height-26,"TAB SCORECARD   ESC MENU",0.78f,0.87f,0.88f);
}
static void draw_result_overlay(void) {
    panel2d((float)g_width*0.5f-220.0f,(float)g_height*0.5f-105.0f,440,210);
    text2d((float)g_width*0.5f-142.0f,(float)g_height*0.5f-48.0f,"HOLE COMPLETE!",0.97f,0.90f,0.62f);
    textf((float)g_width*0.5f-140.0f,(float)g_height*0.5f-6.0f,0.92f,0.96f,0.96f,"Your ball finished in %u strokes.",(unsigned)g_local.strokes);
    text2d((float)g_width*0.5f-140.0f,(float)g_height*0.5f+34.0f,"Other players can keep putting.",0.68f,0.83f,0.85f);
    if(button2d((float)g_width*0.5f-78.0f,(float)g_height*0.5f+62.0f,156,38,"BACK TO MENU",1)) {
        tg_net_leave();
        g_screen=SCREEN_MENU;
        g_local_finished=0;
        g_win_awarded=0;
    }
}
static void draw_menu(void) {
    tg_net_room rooms[TG_NET_MAX_ROOMS];
    size_t room_count=tg_net_get_rooms(rooms,TG_NET_MAX_ROOMS),i;
    float x=32,y=32;
    panel2d(x,y,430,(float)g_height-64);
    text2d(x+28,y+54,"GOLFGAME",0.97f,0.97f,0.93f);
    text2d(x+28,y+82,"MULTIPLAYER MINIGOLF",0.36f,0.80f,0.82f);
    text2d(x+28,y+124,"A tiny room. A ridiculous course. 16 golf balls.",0.77f,0.87f,0.88f);

    input2d(x+28,y+166,350,"PLAYER NAME",g_player_name,g_input_focus==1);
    if(hit_rect(x+28,y+166,350,34)) g_input_focus=1;
    input2d(x+28,y+226,350,"ROOM NAME",g_room_name,g_input_focus==2);
    if(hit_rect(x+28,y+226,350,34)) g_input_focus=2;
    if(button2d(x+28,y+285,168,43,"HOST MATCH",1)) {
        if(tg_net_host(g_player_name,g_room_name,TG_NET_MAX_PLAYERS)) {
            g_screen=SCREEN_LOBBY;
            g_local.id=g_net_info.local_player_id;
        }
    }
    if(button2d(x+210,y+285,168,43,"FIND LAN",0)) {
        tg_net_discover();
        g_rooms_refreshing=1;
    }
    input2d(x+28,y+365,350,"DIRECT IP / HOSTNAME",g_join_address,g_input_focus==3);
    if(hit_rect(x+28,y+365,350,34)) g_input_focus=3;
    if(button2d(x+28,y+414,350,42,"JOIN ADDRESS",0)) {
        if(tg_net_join(g_join_address,g_player_name)) g_screen=SCREEN_LOBBY;
    }
    textf(x+28,y+485,0.68f,0.83f,0.85f,"LOCAL WINS: %d",g_wins);
    text2d(x+28,y+516,"Cosmetics are visual only. Wins unlock extra flair.",0.61f,0.76f,0.78f);
    if(room_count>0) {
        text2d(x+28,y+556,"FOUND ROOMS",0.91f,0.89f,0.65f);
        for(i=0;i<room_count && i<3;i++) {
            char label[96];
            snprintf(label,sizeof(label),"%s  (%u/%u)",rooms[i].name,
                (unsigned)rooms[i].player_count,(unsigned)rooms[i].max_players);
            if(button2d(x+28,y+568+(float)i*42.0f,350,36,label,0))
                tg_net_join(rooms[i].address,g_player_name);
        }
    } else if(g_rooms_refreshing) {
        text2d(x+28,y+556,"Searching this local network...",0.72f,0.86f,0.86f);
    }

    panel2d(500,32,(float)g_width-532,(float)g_height-64);
    text2d(535,85,"BALL CUSTOMISER",0.96f,0.96f,0.92f);
    text2d(535,115,"You're the ball. No tiny golfer required.",0.68f,0.82f,0.84f);
    textf(535,167,0.94f,0.95f,0.95f,"COLOUR: %s",ball_colour_names[g_colour_index]);
    if(button2d(535,183,170,38,"NEXT COLOUR",0)) {
        g_colour_index=(g_colour_index+1)%(int)(sizeof(ball_colours)/sizeof(ball_colours[0]));
        update_cosmetics();
    }
    textf(535,259,0.94f,0.95f,0.95f,"HAT: %s",g_hat_index==0?"None":(g_hat_index==1?"Party hat":(g_hat_index==2?"Top hat":"Crown")));
    if(button2d(535,275,170,38,"NEXT HAT",0)) {
        int next=(g_hat_index+1)%4;
        int required=next==1?1:(next==2?3:(next==3?10:0));
        if(g_wins>=required) { g_hat_index=next; update_cosmetics(); }
    }
    textf(535,351,0.94f,0.95f,0.95f,"TRAIL: %s",g_trail_index==0?"None":(g_trail_index==1?"Sparkles":"Rainbow"));
    if(button2d(535,367,170,38,"NEXT TRAIL",0)) {
        int next=(g_trail_index+1)%3;
        int required=next==1?5:(next==2?15:0);
        if(g_wins>=required) { g_trail_index=next; update_cosmetics(); }
    }
    text2d(535,436,"UNLOCKS",0.91f,0.87f,0.60f);
    text2d(535,464,"1 win: party hat   3 wins: top hat",0.68f,0.81f,0.82f);
    text2d(535,488,"5 wins: sparkle trail   10 wins: crown",0.68f,0.81f,0.82f);
    text2d(535,512,"15 wins: rainbow trail",0.68f,0.81f,0.82f);
    text2d(535,572,"Low-poly courses  |  2-16 players  |  Simultaneous putting",0.71f,0.82f,0.83f);
    text2d(535,(float)g_height-64,"ESC: back    C/H/T: customise in menu",0.55f,0.70f,0.72f);

    /* Preview ball in the scene, with current cosmetic equipment. */
    if(room_count==0 && g_net_info.role==TG_NET_ROLE_OFFLINE) {
        tg_net_player preview=g_local;
        preview.ball_x=18.5f; preview.ball_y=course_height(preview.ball_x)+BALL_RADIUS;
        preview.ball_z=0.0f;
        glEnable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        gluPerspective(36.0,(double)g_width/(double)(g_height>0?g_height:1),0.1,100.0);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        gluLookAt(16.0,5.5,8.0,18.5,0.3,0.0,0,1,0);
        draw_ball(&preview,0,1);
        glDisable(GL_DEPTH_TEST);
    }
    if(g_net_info.last_error[0])
        textf(535,(float)g_height-96,1.0f,0.55f,0.48f,"NETWORK: %s",g_net_info.last_error);
}
static void draw_lobby(void) {
    const tg_net_player *players;
    size_t count=0,i;
    panel2d(80,55,(float)g_width-160,(float)g_height-110);
    text2d(118,102,"MATCH ROOM",0.96f,0.95f,0.90f);
    textf(118,137,0.53f,0.82f,0.84f,"%s",g_net_info.room_name[0]?g_net_info.room_name:"Connecting...");
    textf(118,177,0.90f,0.93f,0.93f,"PLAYERS  %u / %u",(unsigned)g_net_info.player_count,(unsigned)g_net_info.max_players);
    players=tg_net_get_players(&count);
    for(i=0;i<count && i<TG_NET_MAX_PLAYERS;i++) {
        float yy=222.0f+(float)(i/2)*48.0f;
        float xx=118.0f+(float)(i%2)*390.0f;
        uint32_t col=players[i].ball_colour_rgba;
        rect2d(xx,yy-19,24,24,(float)((col>>24)&255)/255.0f,(float)((col>>16)&255)/255.0f,(float)((col>>8)&255)/255.0f,1);
        textf(xx+38,yy,0.94f,0.96f,0.96f,"%s",players[i].name);
        if(players[i].id==g_net_info.local_player_id)
            text2d(xx+280,yy,"YOU",0.40f,0.82f,0.82f);
    }
    if(g_net_info.role==TG_NET_ROLE_HOST) {
        if(button2d((float)g_width-310,(float)g_height-150,190,48,"START MATCH",1))
            tg_net_start_match();
        text2d((float)g_width-520,(float)g_height-120,"Everyone putts at the same time.",0.70f,0.83f,0.84f);
    } else {
        text2d(118,(float)g_height-130,"Waiting for the host to start the match...",0.75f,0.85f,0.86f);
    }
    if(button2d(118,(float)g_height-150,145,42,"LEAVE ROOM",0)) {
        tg_net_leave(); g_screen=SCREEN_MENU; g_rooms_refreshing=0;
    }
    if(g_net_info.status==TG_NET_STATUS_CONNECTING)
        text2d(118,205,"Connecting to host...",0.95f,0.83f,0.52f);
}
static int all_players_finished(void) {
    const tg_net_player *players;
    size_t count=0,i;
    if(g_net_info.player_count==0) return g_local_finished;
    players=tg_net_get_players(&count);
    if(!players || count==0) return 0;
    for(i=0;i<count;i++) if(!(players[i].flags&2)) return 0;
    return 1;
}
static void maybe_award_win(void) {
    const tg_net_player *players;
    size_t count=0,i;
    uint16_t best=65535;
    if(g_win_awarded || !all_players_finished()) return;
    players=tg_net_get_players(&count);
    if(count==0) { best=g_local.strokes; }
    else for(i=0;i<count;i++) if(players[i].strokes<best) best=players[i].strokes;
    if(g_local.strokes==best) { g_wins++; profile_save(); }
    g_win_awarded=1;
}
static void draw_game_ui(void) {
    draw_hud();
    draw_scorecard(0);
    if(g_local_finished) draw_result_overlay();
    if(all_players_finished()) {
        panel2d((float)g_width*0.5f-210,(float)g_height-160,420,54);
        text2d((float)g_width*0.5f-82,(float)g_height-126,"ROUND FINISHED",0.98f,0.89f,0.62f);
    }
}
static void render_frame(void) {
    render_world();
    begin_2d();
    if(g_screen==SCREEN_MENU) draw_menu();
    else if(g_screen==SCREEN_LOBBY) draw_lobby();
    else draw_game_ui();
    SwapBuffers(g_dc);
}
static void update_local_player(void) {
    g_local.id=g_net_info.local_player_id;
    strncpy(g_local.name,g_player_name,sizeof(g_local.name)-1);
    g_local.name[sizeof(g_local.name)-1]='\0';
    g_local.ball_colour_rgba=ball_colours[g_colour_index];
    g_local.hat_id=(uint16_t)g_hat_index;
    g_local.trail_id=(uint16_t)g_trail_index;
    g_local.ball_y=course_height(g_local.ball_x)+BALL_RADIUS;
    g_local.ball_qx=0; g_local.ball_qy=0; g_local.ball_qz=0; g_local.ball_qw=1;
    if(g_ball_vx*g_ball_vx+g_ball_vz*g_ball_vz>0.035f) g_local.flags|=1;
    else g_local.flags&=(uint8_t)~1u;
    if(g_local_finished) g_local.flags|=2;
    else g_local.flags&=(uint8_t)~2u;
    tg_net_set_local_player(&g_local);
}
static void reset_ball(void) {
    g_local.ball_x=COURSE_START_X;
    g_local.ball_z=0;
    g_local.ball_y=BALL_RADIUS;
    g_local.strokes=0;
    g_local.flags=0;
    g_ball_vx=0;
    g_ball_vz=0;
    g_local_finished=0;
    g_win_awarded=0;
}
static void shoot_ball(float dx, float dy) {
    float distance=length2(dx,dy);
    float scale;
    if(distance<8 || g_local_finished) return;
    scale=clampf_local(distance/170.0f,0,1);
    if(scale<0.04f) return;
    g_ball_vx=(dx/(distance>0?distance:1))*(1.8f+scale*7.2f);
    g_ball_vz=(-dy/(distance>0?distance:1))*(1.8f+scale*7.2f);
    g_local.strokes++;
    g_local.flags|=1;
}
static void update_physics(float dt) {
    float speed,friction,slope,oldx;
    float bx,bz;
    if(g_screen!=SCREEN_COURSE || g_local_finished) return;
    if(dt>0.05f) dt=0.05f;
    oldx=g_local.ball_x;
    slope=(course_height(g_local.ball_x+0.02f)-course_height(g_local.ball_x-0.02f))/0.04f;
    g_ball_vx-=9.81f*slope*dt*0.30f;
    speed=length2(g_ball_vx,g_ball_vz);
    if(speed>0.001f) {
        friction=0.72f*dt;
        speed=(speed>friction)?speed-friction:0;
        if(speed>0) { g_ball_vx*=speed/length2(g_ball_vx,g_ball_vz); g_ball_vz*=speed/length2(g_ball_vx,g_ball_vz); }
        else { g_ball_vx=0; g_ball_vz=0; }
    }
    g_local.ball_x+=g_ball_vx*dt;
    g_local.ball_z+=g_ball_vz*dt;
    /* Raised course edges and bumpers. */
    if(g_local.ball_z<-4.45f) { g_local.ball_z=-4.45f; g_ball_vz=fabsf(g_ball_vz)*0.72f; }
    if(g_local.ball_z> 4.45f) { g_local.ball_z= 4.45f; g_ball_vz=-fabsf(g_ball_vz)*0.72f; }
    bx=g_local.ball_x-10.0f; bz=g_local.ball_z+1.8f;
    if(bx*bx+bz*bz<0.73f*0.73f) {
        float d=sqrtf(bx*bx+bz*bz); if(d<0.001f)d=0.001f;
        bx/=d; bz/=d; g_local.ball_x=10.0f+bx*0.74f; g_local.ball_z=-1.8f+bz*0.74f;
        { float dot=g_ball_vx*bx+g_ball_vz*bz; if(dot<0) { g_ball_vx-=1.55f*dot*bx; g_ball_vz-=1.55f*dot*bz; } }
    }
    bx=g_local.ball_x-10.0f; bz=g_local.ball_z-1.8f;
    if(bx*bx+bz*bz<0.73f*0.73f) {
        float d=sqrtf(bx*bx+bz*bz); if(d<0.001f)d=0.001f;
        bx/=d; bz/=d; g_local.ball_x=10.0f+bx*0.74f; g_local.ball_z=1.8f+bz*0.74f;
        { float dot=g_ball_vx*bx+g_ball_vz*bz; if(dot<0) { g_ball_vx-=1.55f*dot*bx; g_ball_vz-=1.55f*dot*bz; } }
    }
    speed=length2(g_ball_vx,g_ball_vz);
    if(speed<0.12f) { g_ball_vx=0; g_ball_vz=0; }
    if(g_local.ball_x<-0.4f || g_local.ball_x>24.0f) {
        g_local.ball_x=COURSE_START_X; g_local.ball_z=0;
        g_ball_vx=0; g_ball_vz=0; g_local.strokes++;
    }
    g_local.ball_y=course_height(g_local.ball_x)+BALL_RADIUS;
    if(fabsf(g_local.ball_x-COURSE_HOLE_X)<0.46f &&
       fabsf(g_local.ball_z)<0.46f && speed<1.3f) {
        g_local.ball_x=COURSE_HOLE_X; g_local.ball_z=0;
        g_local.ball_y=BALL_RADIUS; g_ball_vx=0; g_ball_vz=0;
        g_local_finished=1;
        g_ui_flash=2.0f;
    }
    (void)oldx;
}
static void update_frame(float dt) {
    tg_net_update(dt);
    tg_net_get_info(&g_net_info);
    if(g_rooms_refreshing && g_net_info.status!=TG_NET_STATUS_SEARCHING) g_rooms_refreshing=0;
    update_local_player();
    if(((g_net_info.role==TG_NET_ROLE_HOST && g_net_info.status==TG_NET_STATUS_HOSTING) ||
        (g_net_info.role==TG_NET_ROLE_CLIENT && g_net_info.status==TG_NET_STATUS_CONNECTED)) &&
       g_net_info.match_started && g_screen!=SCREEN_COURSE) {
        g_screen=SCREEN_COURSE;
        g_elapsed=0;
        reset_ball();
    }
    if(g_screen==SCREEN_COURSE) {
        update_physics(dt);
        g_elapsed+=dt;
        g_moving_obstacle_angle+=dt*38.0f;
        if(g_ui_flash>0) g_ui_flash-=dt;
        update_local_player();
        maybe_award_win();
    }
}
static void process_click(void) {
    if(!g_click) return;
    if(g_screen==SCREEN_COURSE) {
        /* Result button is handled by draw_result_overlay. */
        return;
    }
    if(g_screen==SCREEN_MENU) {
        /* Menu widgets consume the click during rendering. */
    } else if(g_screen==SCREEN_LOBBY) {
        /* Lobby widgets consume the click during rendering. */
    }
}
static void create_gl(void) {
    PIXELFORMATDESCRIPTOR pfd;
    int format;
    memset(&pfd,0,sizeof(pfd));
    pfd.nSize=sizeof(pfd); pfd.nVersion=1;
    pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    pfd.iPixelType=PFD_TYPE_RGBA; pfd.cColorBits=32; pfd.cDepthBits=24;
    pfd.cStencilBits=8; pfd.iLayerType=PFD_MAIN_PLANE;
    format=ChoosePixelFormat(g_dc,&pfd);
    SetPixelFormat(g_dc,format,&pfd);
    g_gl=wglCreateContext(g_dc);
    wglMakeCurrent(g_dc,g_gl);
    g_quadric=gluNewQuadric();
    g_font_base=glGenLists(256);
    {
        HFONT font=CreateFontA(-18,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,
            ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,
            DEFAULT_PITCH|FF_DONTCARE,"Arial");
        HGDIOBJ old=SelectObject(g_dc,font);
        wglUseFontBitmapsA(g_dc,0,256,g_font_base);
        SelectObject(g_dc,old);
        DeleteObject(font);
    }
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glShadeModel(GL_SMOOTH);
}
static void destroy_gl(void) {
    if(g_gl) {
        wglMakeCurrent(NULL,NULL);
        if(g_font_base) glDeleteLists(g_font_base,256);
        if(g_quadric) gluDeleteQuadric(g_quadric);
        wglDeleteContext(g_gl);
        g_gl=NULL;
    }
}
static void set_text_char(char *buffer,size_t cap,char ch) {
    size_t n=strlen(buffer);
    if(n+1<cap && ch>=32 && ch<127) { buffer[n]=ch; buffer[n+1]='\0'; }
}
static LRESULT CALLBACK window_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
        case WM_SIZE:
            g_width=LOWORD(lp); g_height=HIWORD(lp);
            if(g_width<1)g_width=1; if(g_height<1)g_height=1;
            return 0;
        case WM_MOUSEMOVE:
            g_mouse_x=GET_X_LPARAM(lp); g_mouse_y=GET_Y_LPARAM(lp);
            return 0;
        case WM_LBUTTONDOWN:
            SetCapture(hwnd);
            g_mouse_down=1;
            g_drag_x=GET_X_LPARAM(lp); g_drag_y=GET_Y_LPARAM(lp);
            g_mouse_x=g_drag_x; g_mouse_y=g_drag_y;
            return 0;
        case WM_LBUTTONUP:
            ReleaseCapture();
            g_mouse_down=0;
            g_mouse_x=GET_X_LPARAM(lp); g_mouse_y=GET_Y_LPARAM(lp);
            if(g_screen==SCREEN_COURSE && !g_local_finished) {
                shoot_ball((float)(g_mouse_x-g_drag_x),(float)(g_mouse_y-g_drag_y));
            } else {
                g_click=1; g_click_x=g_mouse_x; g_click_y=g_mouse_y;
            }
            return 0;
        case WM_CHAR:
            if(g_input_focus==1) {
                if(wp==8) { size_t n=strlen(g_player_name); if(n)g_player_name[n-1]='\0'; }
                else set_text_char(g_player_name,sizeof(g_player_name),(char)wp);
            } else if(g_input_focus==2) {
                if(wp==8) { size_t n=strlen(g_room_name); if(n)g_room_name[n-1]='\0'; }
                else set_text_char(g_room_name,sizeof(g_room_name),(char)wp);
            } else if(g_input_focus==3) {
                if(wp==8) { size_t n=strlen(g_join_address); if(n)g_join_address[n-1]='\0'; }
                else set_text_char(g_join_address,sizeof(g_join_address),(char)wp);
            }
            return 0;
        case WM_KEYDOWN:
            if(wp==VK_ESCAPE) {
                if(g_screen==SCREEN_COURSE) { g_screen=SCREEN_MENU; tg_net_leave(); reset_ball(); }
                else if(g_screen==SCREEN_LOBBY) { tg_net_leave(); g_screen=SCREEN_MENU; }
                else { g_input_focus=0; }
            } else if(wp==VK_TAB && g_screen==SCREEN_COURSE) {
                g_show_scorecard=1;
            } else if(wp==VK_F5 && g_screen==SCREEN_MENU) {
                tg_net_discover(); g_rooms_refreshing=1;
            } else if(wp=='C' && g_screen==SCREEN_MENU) {
                g_colour_index=(g_colour_index+1)%8; update_cosmetics();
            } else if(wp=='H' && g_screen==SCREEN_MENU && g_wins>=1) {
                g_hat_index=(g_hat_index+1)%4; update_cosmetics();
            } else if(wp=='T' && g_screen==SCREEN_MENU && g_wins>=5) {
                g_trail_index=(g_trail_index+1)%3; update_cosmetics();
            }
            return 0;
        case WM_KEYUP:
            if(wp==VK_TAB) g_show_scorecard=0;
            return 0;
        case WM_ERASEBKGND: return 1;
        case WM_CLOSE: g_running=0; DestroyWindow(hwnd); return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcA(hwnd,msg,wp,lp);
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE prev,LPSTR cmd,int show) {
    WNDCLASSA wc;
    MSG message;
    LARGE_INTEGER freq,last,now;
    (void)prev; (void)cmd;
    profile_load();
    ZeroMemory(&wc,sizeof(wc));
    wc.style=CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc=window_proc;
    wc.hInstance=instance;
    wc.hCursor=LoadCursor(NULL,IDC_ARROW);
    wc.lpszClassName="GolfgameWindow";
    RegisterClassA(&wc);
    g_window=CreateWindowExA(0,wc.lpszClassName,APP_TITLE,
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,1280,760,
        NULL,NULL,instance,NULL);
    if(!g_window) return 1;
    g_dc=GetDC(g_window);
    create_gl();
    ShowWindow(g_window,show);
    UpdateWindow(g_window);

    if(!tg_net_init()) {
        MessageBoxA(g_window,"Could not initialise the UDP networking layer.","Golfgame",MB_ICONWARNING);
    }
    g_local.ball_colour_rgba=ball_colours[g_colour_index];
    g_local.ball_x=COURSE_START_X;
    g_local.ball_y=BALL_RADIUS;
    g_local.ball_qw=1;
    strncpy(g_local.name,g_player_name,sizeof(g_local.name)-1);

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&last);
    while(g_running) {
        while(PeekMessageA(&message,NULL,0,0,PM_REMOVE)) {
            if(message.message==WM_QUIT) g_running=0;
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        QueryPerformanceCounter(&now);
        {
            float dt=(float)((double)(now.QuadPart-last.QuadPart)/(double)freq.QuadPart);
            last=now;
            if(dt>0.1f)dt=0.1f;
            update_frame(dt);
        }
        process_click();
        render_frame();
        g_click=0;
        Sleep(1);
    }

    tg_net_shutdown();
    destroy_gl();
    ReleaseDC(g_window,g_dc);
    return 0;
}
