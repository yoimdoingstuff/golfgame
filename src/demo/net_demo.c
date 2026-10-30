#include "tg_network.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
static void sleep_ms(unsigned int ms) { Sleep(ms); }
#else
#include <time.h>
static void sleep_ms(unsigned int ms) {
    struct timespec ts;
    ts.tv_sec=(time_t)(ms/1000U);
    ts.tv_nsec=(long)(ms%1000U)*1000000L;
    nanosleep(&ts,NULL);
}
#endif

static void print_rooms(void) {
    tg_net_room rooms[TG_NET_MAX_ROOMS];
    size_t i,count=tg_net_get_rooms(rooms,TG_NET_MAX_ROOMS);
    printf("\nRooms found: %zu\n",count);
    for(i=0;i<count && i<TG_NET_MAX_ROOMS;i++)
        printf("  %zu) %-24s host=%-18s players=%u/%u address=%s:%u\n",
            i+1,rooms[i].name,rooms[i].host_name,
            (unsigned)rooms[i].player_count,(unsigned)rooms[i].max_players,
            rooms[i].address,(unsigned)rooms[i].port);
}
static void print_players(void) {
    tg_net_info info; const tg_net_player *players; size_t i,count=0;
    tg_net_get_info(&info); players=tg_net_get_players(&count);
    printf("\r[%s] %s | players %u/%u | ",
        info.role==TG_NET_ROLE_HOST?"HOST":"CLIENT",info.room_name,
        (unsigned)info.player_count,(unsigned)info.max_players);
    for(i=0;i<count && i<TG_NET_MAX_PLAYERS;i++)
        printf("%s%s",i?", ":"",players[i].name);
    printf("                    "); fflush(stdout);
}
static void usage(const char *exe) {
    printf("Golfgame network prototype\n\n");
    printf("Usage:\n  %s host <player-name> [room-name]\n",exe);
    printf("  %s scan\n  %s join <ip-or-hostname> <player-name>\n\n",exe,exe);
    printf("Host/join use UDP port %d. Ctrl+C exits.\n",TG_NET_DEFAULT_PORT);
}
static double monotonic_seconds(void) {
    struct timespec ts; timespec_get(&ts,TIME_UTC);
    return (double)ts.tv_sec+(double)ts.tv_nsec/1000000000.0;
}
int main(int argc,char **argv) {
    double last; float print_timer=0; tg_net_info info;
    if(argc<2) { usage(argv[0]); return 0; }
    if(!tg_net_init()) {
        tg_net_get_info(&info);
        fprintf(stderr,"Network init failed: %s\n",info.last_error);
        return 1;
    }
    if(strcmp(argv[1],"host")==0) {
        const char *name=argc>=3?argv[2]:"Host";
        const char *room=argc>=4?argv[3]:"Minigolf Match";
        if(!tg_net_host(name,room,TG_NET_MAX_PLAYERS)) {
            tg_net_get_info(&info);
            fprintf(stderr,"Host failed: %s\n",info.last_error);
            tg_net_shutdown(); return 1;
        }
        printf("Hosting '%s' as %s on UDP %d. Up to 15 other players.\n",
            room,name,TG_NET_DEFAULT_PORT);
    } else if(strcmp(argv[1],"scan")==0) {
        int ticks;
        if(!tg_net_discover()) {
            tg_net_get_info(&info);
            fprintf(stderr,"Discovery failed: %s\n",info.last_error);
            tg_net_shutdown(); return 1;
        }
        printf("Searching local network for 3 seconds...\n");
        for(ticks=0;ticks<35;ticks++) { tg_net_update(0.1f); sleep_ms(100); }
        print_rooms(); tg_net_shutdown(); return 0;
    } else if(strcmp(argv[1],"join")==0 && argc>=4) {
        if(!tg_net_join(argv[2],argv[3])) {
            tg_net_get_info(&info);
            fprintf(stderr,"Join failed: %s\n",info.last_error);
            tg_net_shutdown(); return 1;
        }
        printf("Joining %s as %s...\n",argv[2],argv[3]);
    } else {
        usage(argv[0]); tg_net_shutdown(); return 0;
    }
    last=monotonic_seconds();
    for(;;) {
        double now=monotonic_seconds(); float dt=(float)(now-last); last=now;
        tg_net_update(dt); print_timer+=dt;
        if(print_timer>=1.0f) {
            tg_net_get_info(&info);
            if(info.role==TG_NET_ROLE_CLIENT && info.status==TG_NET_STATUS_CONNECTING)
                { printf("\rConnecting...                                  "); fflush(stdout); }
            else print_players();
            print_timer=0;
        }
        sleep_ms(16);
    }
    tg_net_shutdown(); return 0;
}
