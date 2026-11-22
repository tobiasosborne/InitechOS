#include "volume_db.h"

static int on_drive(finder_dir_id_t dir,unsigned drive,unsigned legacy)
{
    return finder_dir_drive(dir,legacy)==drive;
}

int finder_volume_db_has_state(const finder_shell_t *sh,unsigned drive)
{
    if(!sh)return 0;
    if(drive==sh->volume_drive)return 1;
    for(unsigned i=0;i<sh->desk.n;i++)
        if(sh->desk.icons[i].kind==FINDER_ICON_VOLUME &&
           on_drive(sh->desk.icons[i].dir_start,drive,sh->volume_drive))return 1;
    for(unsigned i=0;i<sh->n_views;i++)
        if(on_drive(sh->views[i].dir_start,drive,sh->volume_drive))return 1;
    for(unsigned i=0;i<sh->n_origins;i++)
        if(on_drive(sh->origins[i].dir_start,drive,sh->volume_drive))return 1;
    return 0;
}

uint32_t finder_volume_db_encode(const finder_shell_t *sh,unsigned drive,
                                 uint8_t *out,uint32_t cap)
{
    finder_desk_t desk;
    finder_desk_icon_t icons[FINDER_DESK_MAX_ICONS];
    finder_view_rec_t views[FINDER_DB_MAX_VIEWS];
    finder_origin_rec_t origins[FINDER_DB_MAX_ORIGINS];
    unsigned nv=0,no=0;
    if(!sh || drive>=26u)return 0;
    desk=sh->desk;desk.icons=icons;desk.n=0;desk.cap=FINDER_DESK_MAX_ICONS;
    for(unsigned i=0;i<sh->desk.n;i++){
        const finder_desk_icon_t *ic=&sh->desk.icons[i];
        if(ic->kind==FINDER_ICON_VOLUME && !on_drive(ic->dir_start,drive,sh->volume_drive))continue;
        icons[desk.n++]=*ic;
    }
    for(unsigned i=0;i<sh->n_views;i++){
        if(!on_drive(sh->views[i].dir_start,drive,sh->volume_drive))continue;
        if(nv==FINDER_DB_MAX_VIEWS)return 0;
        views[nv]=sh->views[i];views[nv++].dir_start=finder_dir_cluster(sh->views[i].dir_start);
    }
    for(unsigned i=0;i<sh->n_origins;i++){
        if(!on_drive(sh->origins[i].dir_start,drive,sh->volume_drive))continue;
        if(no==FINDER_DB_MAX_ORIGINS)return 0;
        origins[no]=sh->origins[i];origins[no++].dir_start=finder_dir_cluster(sh->origins[i].dir_start);
    }
    return finder_desk_db_encode_full(&desk,views,(uint16_t)nv,origins,(uint16_t)no,out,cap);
}

void finder_volume_db_positions(finder_shell_t *sh,unsigned drive,
                                const uint8_t *data,uint32_t len)
{
    finder_desk_t desk;
    finder_desk_icon_t icons[FINDER_DESK_MAX_ICONS];
    unsigned index[FINDER_DESK_MAX_ICONS];
    if(!sh || drive>=26u)return;
    desk=sh->desk;desk.icons=icons;desk.n=0;desk.cap=FINDER_DESK_MAX_ICONS;
    for(unsigned i=0;i<sh->desk.n;i++){
        const finder_desk_icon_t *ic=&sh->desk.icons[i];
        if(ic->kind==FINDER_ICON_VOLUME){
            if(!on_drive(ic->dir_start,drive,sh->volume_drive))continue;
        }else if(drive!=sh->volume_drive)continue; /* one shared desktop Trash */
        index[desk.n]=i;icons[desk.n++]=*ic;
    }
    if(finder_desk_db_apply(&desk,data,len)<0)return;
    for(unsigned i=0;i<desk.n;i++){
        sh->desk.icons[index[i]].x=icons[i].x;
        sh->desk.icons[index[i]].y=icons[i].y;
    }
}

unsigned finder_volume_db_load(finder_shell_t *sh,unsigned drive,
                               const uint8_t *data,uint32_t len)
{
    finder_origin_rec_t origins[FINDER_DB_MAX_ORIGINS];
    unsigned kept=0,took=0;
    if(!sh || drive>=26u || finder_desk_db_validate(data,len)!=FINDER_DB_OK)return 0;
    for(unsigned i=0;i<sh->n_views;i++)
        if(!on_drive(sh->views[i].dir_start,drive,sh->volume_drive))sh->views[kept++]=sh->views[i];
    sh->n_views=(uint16_t)kept;kept=0;
    for(unsigned i=0;i<sh->n_origins;i++)
        if(!on_drive(sh->origins[i].dir_start,drive,sh->volume_drive))sh->origins[kept++]=sh->origins[i];
    sh->n_origins=(uint16_t)kept;
    unsigned n=(unsigned)data[6]|((unsigned)data[7]<<8);
    for(unsigned i=0;i<n;i++){
        unsigned off=FINDER_DB_HEADER_SIZE+i*FINDER_DB_RECORD_SIZE;
        if(data[off]!=FINDER_DB_KIND_VIEW)continue;
        uint16_t cluster=(uint16_t)((unsigned)data[off+2]|((unsigned)data[off+3]<<8));
        finder_view_rec_t v;
        if(finder_desk_db_find_view(data,len,cluster,&v)!=1)continue;
        finder_dir_id_t id=finder_dir_make(drive,cluster);
        if(finder_shell_find_view(sh,id))continue; /* first record wins */
        took+=(unsigned)finder_shell_note_view(sh,id,v.name83,v.x,v.y,v.view_bits);
    }
    n=finder_desk_db_get_origins(data,len,origins,FINDER_DB_MAX_ORIGINS);
    for(unsigned i=0;i<n && sh->n_origins<FINDER_ALL_ORIGINS;i++){
        origins[i].dir_start=finder_dir_make(drive,finder_dir_cluster(origins[i].dir_start));
        sh->origins[sh->n_origins++]=origins[i];took++;
    }
    finder_volume_db_positions(sh,drive,data,len);
    return took;
}
