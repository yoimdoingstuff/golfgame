#include "tg_network.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET tg_socket;
typedef int tg_socklen;
#define TG_BAD_SOCKET INVALID_SOCKET
#define tg_close closesocket
#define tg_set_nonblocking(s) do { u_long m = 1; ioctlsocket((s), FIONBIO, &m); } while (0)
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int tg_socket;
typedef socklen_t tg_socklen;
#define TG_BAD_SOCKET (-1)
#define tg_close close
#define tg_set_nonblocking(s) do { int f = fcntl((s), F_GETFL, 0); if (f >= 0) fcntl((s), F_SETFL, f | O_NONBLOCK); } while (0)
#endif

#define TG_MAGIC 0x54474F4Cu
#define TG_VERSION 3u
#define TG_TIMEOUT 8.0f
#define TG_DISCOVERY_TIME 3.0f
#define TG_SNAPSHOT_TIME 0.10f
#define TG_STATE_TIME 0.10f

enum { PK_DISCOVER=1, PK_OFFER, PK_JOIN, PK_WELCOME, PK_STATE, PK_SNAPSHOT, PK_LEAVE, PK_START };

#pragma pack(push, 1)
typedef struct packet_header {
    uint32_t magic;
    uint8_t version, type;
    uint16_t size;
    uint32_t room_id;
} packet_header;
typedef struct offer_packet {
    uint8_t players, max_players;
    char room[TG_NET_ROOM_NAME];
    char host[TG_NET_PLAYER_NAME];
} offer_packet;
typedef struct join_packet { char name[TG_NET_PLAYER_NAME]; } join_packet;
typedef struct welcome_packet {
    uint32_t id, token;
    uint8_t max_players;
    char room[TG_NET_ROOM_NAME];
} welcome_packet;
typedef struct player_packet {
    uint32_t id;
    uint16_t strokes, hat_id, trail_id;
    uint8_t flags;
    uint32_t ball_colour_rgba;
    char name[TG_NET_PLAYER_NAME];
    float x, y, z;
    float qx, qy, qz, qw;
} player_packet;
typedef struct state_packet { uint32_t token; player_packet player; } state_packet;
typedef struct snapshot_packet {
    uint8_t count;
    uint8_t match_started;
    player_packet players[TG_NET_MAX_PLAYERS];
} snapshot_packet;
#pragma pack(pop)

typedef struct peer {
    int active;
    uint32_t id, token;
    float silence;
    struct sockaddr_in address;
    tg_net_player player;
} peer;

typedef struct context {
    int ready;
    tg_socket socket;
    tg_net_info info;
    tg_net_player players[TG_NET_MAX_PLAYERS], local;
    tg_net_room rooms[TG_NET_MAX_ROOMS];
    size_t room_count;
    peer peers[TG_NET_MAX_PLAYERS];
    struct sockaddr_in server;
    uint32_t token, next_id;
    float discovery_left, discovery_tick, state_tick, snapshot_tick;
    float connect_time, server_silence;
} context;

static context g;

static void copy_text(char *dst, size_t cap, const char *src) {
    if (cap) snprintf(dst, cap, "%s", src ? src : "");
}
static uint32_t random_id(void) {
    static uint32_t x = 0x61C88647u;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return x ? x : 1u;
}
static void error_text(const char *s) {
    copy_text(g.info.last_error, sizeof(g.info.last_error), s);
}
static tg_socket open_socket(uint16_t port) {
    tg_socket s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in a;
    int yes = 1;
    if (s == TG_BAD_SOCKET) {
        error_text("Could not create UDP socket");
        return TG_BAD_SOCKET;
    }
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons(port);
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) != 0) {
        error_text(port ? "UDP port 27890 is unavailable" : "Could not bind UDP socket");
        tg_close(s);
        return TG_BAD_SOCKET;
    }
    tg_set_nonblocking(s);
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, (const char *)&yes, sizeof(yes));
    return s;
}
static void close_socket(void) {
    if (g.socket != TG_BAD_SOCKET) tg_close(g.socket);
    g.socket = TG_BAD_SOCKET;
}
static size_t make_packet(unsigned char *out, uint8_t type, uint32_t room,
                          const void *payload, size_t payload_size) {
    packet_header h;
    size_t total = sizeof(h) + payload_size;
    if (total > 1200) return 0;
    h.magic = TG_MAGIC; h.version = TG_VERSION; h.type = type;
    h.size = (uint16_t)total; h.room_id = room;
    memcpy(out, &h, sizeof(h));
    if (payload_size) memcpy(out + sizeof(h), payload, payload_size);
    return total;
}
static void send_to(const struct sockaddr_in *to, const void *data, size_t size) {
    if (g.socket != TG_BAD_SOCKET && to && size)
        (void)sendto(g.socket, (const char *)data, (int)size, 0,
                     (const struct sockaddr *)to, (int)sizeof(*to));
}
static int same_address(const struct sockaddr_in *a, const struct sockaddr_in *b) {
    return a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port;
}
static peer *find_peer_address(const struct sockaddr_in *a) {
    size_t i;
    for (i=0; i<TG_NET_MAX_PLAYERS; i++)
        if (g.peers[i].active && same_address(&g.peers[i].address,a)) return &g.peers[i];
    return NULL;
}
static peer *find_peer_token(uint32_t token) {
    size_t i;
    for (i=0; i<TG_NET_MAX_PLAYERS; i++)
        if (g.peers[i].active && g.peers[i].token==token) return &g.peers[i];
    return NULL;
}
static player_packet to_wire(const tg_net_player *p) {
    player_packet w;
    memset(&w,0,sizeof(w));
    w.id=p->id; w.strokes=p->strokes; w.hat_id=p->hat_id; w.trail_id=p->trail_id;
    w.flags=p->flags; w.ball_colour_rgba=p->ball_colour_rgba;
    copy_text(w.name,sizeof(w.name),p->name);
    w.x=p->ball_x; w.y=p->ball_y; w.z=p->ball_z;
    w.qx=p->ball_qx; w.qy=p->ball_qy; w.qz=p->ball_qz; w.qw=p->ball_qw;
    return w;
}
static void from_wire(tg_net_player *p, const player_packet *w) {
    memset(p,0,sizeof(*p));
    p->id=w->id; p->strokes=w->strokes; p->hat_id=w->hat_id;
    p->trail_id=w->trail_id; p->flags=w->flags;
    p->ball_colour_rgba=w->ball_colour_rgba;
    copy_text(p->name,sizeof(p->name),w->name);
    p->ball_x=w->x; p->ball_y=w->y; p->ball_z=w->z;
    p->ball_qx=w->qx; p->ball_qy=w->qy; p->ball_qz=w->qz; p->ball_qw=w->qw;
}
static void refresh_host_players(void) {
    size_t i,n=0;
    memset(g.players,0,sizeof(g.players));
    g.players[n++]=g.local;
    for(i=0; i<TG_NET_MAX_PLAYERS && n<TG_NET_MAX_PLAYERS; i++)
        if(g.peers[i].active) g.players[n++]=g.peers[i].player;
    g.info.player_count=(uint8_t)n;
}
static void send_offer(const struct sockaddr_in *to) {
    unsigned char buf[1200]; offer_packet p; size_t n;
    memset(&p,0,sizeof(p));
    p.players=g.info.player_count; p.max_players=g.info.max_players;
    copy_text(p.room,sizeof(p.room),g.info.room_name);
    copy_text(p.host,sizeof(p.host),g.info.local_name);
    n=make_packet(buf,PK_OFFER,g.info.room_id,&p,sizeof(p));
    send_to(to,buf,n);
}
static void send_welcome(const peer *p) {
    unsigned char buf[1200]; welcome_packet w; size_t n;
    memset(&w,0,sizeof(w));
    w.id=p->id; w.token=p->token; w.max_players=g.info.max_players;
    copy_text(w.room,sizeof(w.room),g.info.room_name);
    n=make_packet(buf,PK_WELCOME,g.info.room_id,&w,sizeof(w));
    send_to(&p->address,buf,n);
}
static void send_snapshot(const struct sockaddr_in *to) {
    unsigned char buf[1200]; snapshot_packet snap; size_t i,n;
    memset(&snap,0,sizeof(snap));
    snap.match_started=g.info.match_started;
    snap.players[snap.count++]=to_wire(&g.local);
    for(i=0; i<TG_NET_MAX_PLAYERS && snap.count<TG_NET_MAX_PLAYERS; i++)
        if(g.peers[i].active) snap.players[snap.count++]=to_wire(&g.peers[i].player);
    n=make_packet(buf,PK_SNAPSHOT,g.info.room_id,&snap,
        offsetof(snapshot_packet,players)+(size_t)snap.count*sizeof(player_packet));
    if(n) send_to(to,buf,n);
}
static void broadcast_snapshot(void) {
    size_t i;
    refresh_host_players();
    for(i=0; i<TG_NET_MAX_PLAYERS; i++)
        if(g.peers[i].active) send_snapshot(&g.peers[i].address);
}
static void add_room(const struct sockaddr_in *from, const packet_header *h, const offer_packet *o) {
    size_t i; char ip[48];
    if(!inet_ntop(AF_INET,&from->sin_addr,ip,sizeof(ip))) return;
    for(i=0; i<g.room_count; i++)
        if(g.rooms[i].room_id==h->room_id && strcmp(g.rooms[i].address,ip)==0) break;
    if(i==g.room_count) {
        if(i>=TG_NET_MAX_ROOMS) return;
        g.room_count++;
    }
    copy_text(g.rooms[i].name,sizeof(g.rooms[i].name),o->room);
    copy_text(g.rooms[i].host_name,sizeof(g.rooms[i].host_name),o->host);
    copy_text(g.rooms[i].address,sizeof(g.rooms[i].address),ip);
    g.rooms[i].port=TG_NET_DEFAULT_PORT;
    g.rooms[i].player_count=o->players; g.rooms[i].max_players=o->max_players;
    g.rooms[i].room_id=h->room_id;
}
static void handle_packet(const unsigned char *buf, size_t bytes, const struct sockaddr_in *from) {
    packet_header h; const unsigned char *p; size_t payload;
    if(bytes<sizeof(h)) return;
    memcpy(&h,buf,sizeof(h));
    if(h.magic!=TG_MAGIC || h.version!=TG_VERSION || h.size!=bytes) return;
    p=buf+sizeof(h); payload=bytes-sizeof(h);
    if(h.type==PK_DISCOVER && payload==0 && g.info.role==TG_NET_ROLE_HOST) {
        send_offer(from); return;
    }
    if(h.type==PK_OFFER && payload==sizeof(offer_packet) && g.info.status==TG_NET_STATUS_SEARCHING) {
        offer_packet o; memcpy(&o,p,sizeof(o)); add_room(from,&h,&o); return;
    }
    if(g.info.role==TG_NET_ROLE_HOST) {
        if(h.room_id!=g.info.room_id && !(h.type==PK_JOIN && h.room_id==0)) return;
        if(h.type==PK_JOIN && payload==sizeof(join_packet)) {
            join_packet j; peer *q; size_t i;
            memcpy(&j,p,sizeof(j)); j.name[sizeof(j.name)-1]='\0';
            q=find_peer_address(from);
            if(q) { q->silence=0; send_welcome(q); send_snapshot(from); return; }
            if(g.info.player_count>=g.info.max_players) return;
            for(i=0; i<TG_NET_MAX_PLAYERS && g.peers[i].active; i++) { }
            if(i==TG_NET_MAX_PLAYERS) return;
            q=&g.peers[i]; memset(q,0,sizeof(*q)); q->active=1;
            q->id=g.next_id++; q->token=random_id(); q->address=*from; q->player.id=q->id;
            q->player.ball_colour_rgba=TG_NET_DEFAULT_BALL_COLOUR_RGBA;
            copy_text(q->player.name,sizeof(q->player.name),j.name);
            send_welcome(q); broadcast_snapshot(); return;
        }
        if(h.type==PK_STATE && payload==sizeof(state_packet)) {
            state_packet s; peer *q;
            memcpy(&s,p,sizeof(s)); q=find_peer_token(s.token);
            if(q && same_address(&q->address,from)) {
                q->silence=0; from_wire(&q->player,&s.player); q->player.id=q->id;
            }
            return;
        }
        if(h.type==PK_LEAVE && payload==0) {
            peer *q=find_peer_address(from);
            if(q) memset(q,0,sizeof(*q));
            broadcast_snapshot(); return;
        }
        return;
    }
    if(g.info.role==TG_NET_ROLE_CLIENT && same_address(from,&g.server)) {
        g.server_silence=0;
        if(h.type==PK_START && payload==0 && h.room_id==g.info.room_id) {
            g.info.match_started=1; return;
        }
        if(h.type==PK_WELCOME && payload==sizeof(welcome_packet)) {
            welcome_packet w; memcpy(&w,p,sizeof(w));
            g.info.local_player_id=w.id; g.info.max_players=w.max_players;
            g.info.room_id=h.room_id; g.token=w.token; g.info.match_started=0;
            copy_text(g.info.room_name,sizeof(g.info.room_name),w.room);
            g.local.id=w.id; g.info.status=TG_NET_STATUS_CONNECTED; return;
        }
        if(h.type==PK_SNAPSHOT && payload>=offsetof(snapshot_packet,players)) {
            snapshot_packet s; size_t count,i,need;
            memset(&s,0,sizeof(s));
            memcpy(&s,p,payload<sizeof(s)?payload:sizeof(s));
            count=s.count; need=offsetof(snapshot_packet,players)+count*sizeof(player_packet);
            if(count>TG_NET_MAX_PLAYERS || payload!=need) return;
            memset(g.players,0,sizeof(g.players));
            g.info.match_started=s.match_started;
            for(i=0; i<count; i++) from_wire(&g.players[i],&s.players[i]);
            g.info.player_count=(uint8_t)count;
        }
    }
}
static void send_discover(void) {
    unsigned char buf[1200]; struct sockaddr_in to;
    size_t n=make_packet(buf,PK_DISCOVER,0,NULL,0);
    memset(&to,0,sizeof(to)); to.sin_family=AF_INET;
    to.sin_port=htons(TG_NET_DEFAULT_PORT); to.sin_addr.s_addr=htonl(INADDR_BROADCAST);
    send_to(&to,buf,n);
}
static void send_join(void) {
    unsigned char buf[1200]; join_packet j; size_t n;
    memset(&j,0,sizeof(j)); copy_text(j.name,sizeof(j.name),g.info.local_name);
    n=make_packet(buf,PK_JOIN,0,&j,sizeof(j)); send_to(&g.server,buf,n);
}
static void send_state(void) {
    unsigned char buf[1200]; state_packet s; size_t n;
    memset(&s,0,sizeof(s)); s.token=g.token; s.player=to_wire(&g.local);
    n=make_packet(buf,PK_STATE,g.info.room_id,&s,sizeof(s)); send_to(&g.server,buf,n);
}

int tg_net_init(void) {
    if(g.ready) return 1;
    memset(&g,0,sizeof(g)); g.socket=TG_BAD_SOCKET;
#ifdef _WIN32
    { WSADATA w; if(WSAStartup(MAKEWORD(2,2),&w)!=0) { error_text("Winsock startup failed"); return 0; } }
#endif
    g.socket=open_socket(0);
    if(g.socket==TG_BAD_SOCKET) {
#ifdef _WIN32
        WSACleanup();
#endif
        return 0;
    }
    g.ready=1; g.info.role=TG_NET_ROLE_OFFLINE; g.info.status=TG_NET_STATUS_OFFLINE;
    return 1;
}
void tg_net_shutdown(void) {
    if(!g.ready) return;
    tg_net_leave(); close_socket();
#ifdef _WIN32
    WSACleanup();
#endif
    memset(&g,0,sizeof(g)); g.socket=TG_BAD_SOCKET;
}
int tg_net_host(const char *name,const char *room,uint8_t max_players) {
    if(!g.ready && !tg_net_init()) return 0;
    if(max_players<2) max_players=2;
    if(max_players>TG_NET_MAX_PLAYERS) max_players=TG_NET_MAX_PLAYERS;
    close_socket(); g.socket=open_socket(TG_NET_DEFAULT_PORT);
    if(g.socket==TG_BAD_SOCKET) return 0;
    memset(&g.info,0,sizeof(g.info)); memset(g.peers,0,sizeof(g.peers)); memset(&g.local,0,sizeof(g.local));
    g.info.role=TG_NET_ROLE_HOST; g.info.status=TG_NET_STATUS_HOSTING;
    g.info.max_players=max_players; g.info.player_count=1;
    g.info.local_player_id=1; g.info.room_id=random_id(); g.next_id=2;
    copy_text(g.info.local_name,sizeof(g.info.local_name),name);
    copy_text(g.info.room_name,sizeof(g.info.room_name),room);
    g.local.id=1; g.local.ball_colour_rgba=TG_NET_DEFAULT_BALL_COLOUR_RGBA;
    copy_text(g.local.name,sizeof(g.local.name),name);
    refresh_host_players(); return 1;
}
int tg_net_discover(void) {
    if(!g.ready && !tg_net_init()) return 0;
    if(g.info.role==TG_NET_ROLE_HOST) {
        error_text("Leave the hosted room before searching"); return 0;
    }
    g.room_count=0; memset(g.rooms,0,sizeof(g.rooms));
    g.discovery_left=TG_DISCOVERY_TIME; g.discovery_tick=0;
    g.info.status=TG_NET_STATUS_SEARCHING; g.info.role=TG_NET_ROLE_OFFLINE;
    send_discover(); return 1;
}
size_t tg_net_get_rooms(tg_net_room *out,size_t cap) {
    size_t n=g.room_count<cap?g.room_count:cap;
    if(out && n) memcpy(out,g.rooms,n*sizeof(*out));
    return g.room_count;
}
int tg_net_join(const char *host,const char *name) {
    struct addrinfo hints,*res=NULL; int rc;
    if(!g.ready && !tg_net_init()) return 0;
    if(!host || !*host) { error_text("Host address is empty"); return 0; }
    memset(&hints,0,sizeof(hints)); hints.ai_family=AF_INET; hints.ai_socktype=SOCK_DGRAM;
    rc=getaddrinfo(host,NULL,&hints,&res);
    if(rc || !res) { error_text("Could not resolve IPv4 host"); return 0; }
    memset(&g.server,0,sizeof(g.server)); memcpy(&g.server,res->ai_addr,sizeof(g.server));
    g.server.sin_port=htons(TG_NET_DEFAULT_PORT); freeaddrinfo(res);
    if(g.info.role==TG_NET_ROLE_HOST) { error_text("Leave the hosted room first"); return 0; }
    copy_text(g.info.local_name,sizeof(g.info.local_name),name);
    copy_text(g.local.name,sizeof(g.local.name),name);
    if(!g.local.ball_colour_rgba) g.local.ball_colour_rgba=TG_NET_DEFAULT_BALL_COLOUR_RGBA;
    g.info.role=TG_NET_ROLE_CLIENT; g.info.status=TG_NET_STATUS_CONNECTING;
    g.info.room_id=0; g.info.local_player_id=0; g.info.match_started=0; g.token=0;
    g.connect_time=0; g.server_silence=0; g.state_tick=0;
    send_join(); return 1;
}
int tg_net_start_match(void) {
    unsigned char buf[64];
    size_t n, i;
    if (!g.ready || g.info.role != TG_NET_ROLE_HOST || g.info.status != TG_NET_STATUS_HOSTING) {
        error_text("Only the room host can start the match");
        return 0;
    }
    g.info.match_started = 1;
    n = make_packet(buf, PK_START, g.info.room_id, NULL, 0);
    for (i = 0; i < TG_NET_MAX_PLAYERS; i++) {
        if (g.peers[i].active) send_to(&g.peers[i].address, buf, n);
    }
    return 1;
}

void tg_net_leave(void) {
    if(g.info.role==TG_NET_ROLE_CLIENT && g.info.status==TG_NET_STATUS_CONNECTED) {
        unsigned char buf[64];
        size_t n=make_packet(buf,PK_LEAVE,g.info.room_id,NULL,0);
        send_to(&g.server,buf,n);
    }
    if(g.info.role==TG_NET_ROLE_HOST) { close_socket(); g.socket=open_socket(0); }
    memset(g.peers,0,sizeof(g.peers)); memset(g.players,0,sizeof(g.players));
    memset(&g.local,0,sizeof(g.local)); memset(&g.info,0,sizeof(g.info));
    g.info.role=TG_NET_ROLE_OFFLINE; g.info.status=TG_NET_STATUS_OFFLINE;
    g.room_count=0; g.token=0;
}
void tg_net_set_local_player(const tg_net_player *p) {
    if(!p) return;
    g.local=*p; g.local.id=g.info.local_player_id;
    copy_text(g.local.name,sizeof(g.local.name),g.info.local_name);
    if(g.info.role==TG_NET_ROLE_HOST) refresh_host_players();
}
void tg_net_update(float dt) {
    unsigned char buf[1200]; struct sockaddr_in from;
    tg_socklen from_len; int n; size_t i;
    if(!g.ready || g.socket==TG_BAD_SOCKET) return;
    if(dt<0) dt=0;
    if(dt>0.25f) dt=0.25f;
    for(;;) {
        from_len=(tg_socklen)sizeof(from);
        n=(int)recvfrom(g.socket,(char *)buf,sizeof(buf),0,(struct sockaddr *)&from,&from_len);
        if(n<=0) break;
        handle_packet(buf,(size_t)n,&from);
    }
    if(g.info.status==TG_NET_STATUS_SEARCHING) {
        g.discovery_left-=dt; g.discovery_tick-=dt;
        if(g.discovery_left>0 && g.discovery_tick<=0) { send_discover(); g.discovery_tick=0.5f; }
        if(g.discovery_left<=0) g.info.status=TG_NET_STATUS_OFFLINE;
    }
    if(g.info.role==TG_NET_ROLE_CLIENT && g.info.status==TG_NET_STATUS_CONNECTING) {
        g.connect_time+=dt;
        if(g.connect_time>10) {
            g.info.role=TG_NET_ROLE_OFFLINE; g.info.status=TG_NET_STATUS_OFFLINE;
            error_text("Connection timed out");
        } else {
            g.state_tick-=dt;
            if(g.state_tick<=0) { send_join(); g.state_tick=0.8f; }
        }
    } else if(g.info.role==TG_NET_ROLE_CLIENT && g.info.status==TG_NET_STATUS_CONNECTED) {
        g.server_silence+=dt;
        if(g.server_silence>TG_TIMEOUT) {
            g.info.role=TG_NET_ROLE_OFFLINE; g.info.status=TG_NET_STATUS_OFFLINE;
            g.info.player_count=0; memset(g.players,0,sizeof(g.players));
            error_text("Host disconnected");
        } else {
            g.state_tick-=dt;
            if(g.state_tick<=0) { send_state(); g.state_tick=TG_STATE_TIME; }
        }
    }
    if(g.info.role==TG_NET_ROLE_HOST) {
        for(i=0;i<TG_NET_MAX_PLAYERS;i++) if(g.peers[i].active) {
            g.peers[i].silence+=dt;
            if(g.peers[i].silence>TG_TIMEOUT) memset(&g.peers[i],0,sizeof(g.peers[i]));
        }
        g.snapshot_tick-=dt;
        if(g.snapshot_tick<=0) { broadcast_snapshot(); g.snapshot_tick=TG_SNAPSHOT_TIME; }
    }
}
void tg_net_get_info(tg_net_info *out) { if(out) *out=g.info; }
const tg_net_player *tg_net_get_players(size_t *count) {
    if(count) *count=g.info.player_count;
    return g.players;
}
