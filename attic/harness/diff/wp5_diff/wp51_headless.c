/* wp51_headless.c -- headless SDL2 driver for the REAL WP 5.1 mint.
 * Factory C, initech-7aou. Used only when mint_drive.sh cannot use X11.
 * Interpose SDL_PollEvent to deliver the same .steps keys to DOSBox-X;
 * save its actual 720x400 surface, never synthesize a WP screen.
 * Build/run: fixtures/iword-listfiles-ops/mint.sh. SDL's dummy video/audio
 * drivers need no display server. Nothing is installed into the corpus. */
#define _GNU_SOURCE
#include <SDL.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct action {
    unsigned at;
    SDL_Keycode key;
    SDL_Keymod mod;
    int down;
    char shot[128];
} action_t;
static action_t actions[8192];
static int count, pos, initialized;
static unsigned start;
static SDL_Window *window;

static void fail(const char *msg) { fprintf(stderr,"wp51-headless: %s\n",msg); exit(2); }
static action_t *add(unsigned at)
{
    if (count == (int)(sizeof actions / sizeof actions[0])) fail("too many actions");
    action_t *a = &actions[count++]; a->at = at; return a;
}
static void key(unsigned *at, SDL_Keycode k, SDL_Keymod mod)
{
    if (k == SDLK_UNKNOWN) fail("unknown keysym");
    SDL_Keycode modifier = (mod & KMOD_CTRL) ? SDLK_LCTRL : SDLK_LSHIFT;
    if (mod) { action_t *a=add(*at); a->key=modifier; a->mod=mod; a->down=1; *at+=20; }
    action_t *a=add(*at); a->key=k; a->mod=mod; a->down=1;
    *at+=40; a=add(*at); a->key=k; a->mod=mod;
    if (mod) { *at+=20; a=add(*at); a->key=modifier; }
    *at+=180;
}
static void load(void)
{
    const char *path=getenv("WP_MINT_STEPS");
    if (!path || !getenv("WP_MINT_SHOTS")) fail("WP_MINT_STEPS and WP_MINT_SHOTS required");
    FILE *f=fopen(path,"r"); if (!f) fail("cannot open steps");
    char line[512]; unsigned at=1000;
    while (fgets(line,sizeof line,f)) {
        line[strcspn(line,"\r\n")]=0;
        if (!strncmp(line,"sleep ",6)) at+=(unsigned)(1000*atof(line+6));
        else if (!strncmp(line,"shot ",5)) {
            if (strlen(line+5)>=sizeof actions[0].shot || strchr(line+5,'/')) fail("invalid shot name");
            strcpy(add(at)->shot,line+5); at+=100;
        } else if (!strncmp(line,"type ",5)) {
            for (char *p=line+5; *p; p++) {
                SDL_Keymod mod=KMOD_NONE; char c=*p;
                if (c>='A' && c<='Z') { c+=32; mod=KMOD_LSHIFT; }
                if (c==':') { c=';'; mod=KMOD_LSHIFT; }
                key(&at,(SDL_Keycode)c,mod);
            }
        } else if (!strncmp(line,"key ",4)) {
            const char *k=line+4; SDL_Keymod mod=KMOD_NONE;
            if (!strncmp(k,"ctrl+",5)) { mod=KMOD_LCTRL; k+=5; }
            key(&at,SDL_GetKeyFromName(k),mod);
        } else if (!strcmp(line,"end")) strcpy(add(at+1000)->shot,"QUIT");
        else if (line[0] && line[0]!='#') fail("unrecognized step");
    }
    fclose(f); start=SDL_GetTicks(); initialized=1;
}
SDL_Window *SDL_CreateWindow(const char *t,int x,int y,int w,int h,Uint32 flags)
{
    SDL_Window *(*real)(const char *,int,int,int,int,Uint32)=dlsym(RTLD_NEXT,"SDL_CreateWindow");
    if (!real) fail("missing SDL_CreateWindow");
    window=real(t,x,y,w,h,flags); return window;
}
int SDL_PollEvent(SDL_Event *event)
{
    int (*real)(SDL_Event *)=dlsym(RTLD_NEXT,"SDL_PollEvent");
    if (!real) fail("missing SDL_PollEvent");
    if (!initialized) load();
    if (pos<count && SDL_GetTicks()-start>=actions[pos].at) {
        const action_t *a=&actions[pos++];
        if (a->shot[0]) {
            if (!strcmp(a->shot,"QUIT")) { memset(event,0,sizeof *event); event->type=SDL_QUIT; return 1; }
            char path[1024];
            int n=snprintf(path,sizeof path,"%s/%s.bmp",getenv("WP_MINT_SHOTS"),a->shot);
            if (n<0 || n>=(int)sizeof path || !window) fail("invalid capture path/window");
            SDL_Surface *surface=SDL_GetWindowSurface(window);
            if (!surface || SDL_SaveBMP(surface,path)) fail("surface capture failed");
            fprintf(stderr,"MINT shot %s %dx%d\n",path,surface->w,surface->h);
        } else {
            memset(event,0,sizeof *event); event->type=a->down?SDL_KEYDOWN:SDL_KEYUP;
            event->key.state=a->down?SDL_PRESSED:SDL_RELEASED;
            event->key.keysym.sym=a->key; event->key.keysym.scancode=SDL_GetScancodeFromKey(a->key);
            event->key.keysym.mod=a->mod; SDL_SetModState(a->mod); return 1;
        }
    }
    return real(event);
}
