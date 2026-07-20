#ifndef DEFS_STRUCTS_H
#define DEFS_STRUCTS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct cutscene_data
{
    void * frame;
    void * palette;
    void * tilemap;
    uint16_t time; // amount of 60fps frames (16.67ms) to wait before auto-advance, including data decompression time
} cutscene_data_t; // Last entry should be all null and invalid values so the cutscene engine knows when to return control.

typedef struct level_palette_list
{
    void * subpal[16];
} level_palette_list_t;

typedef struct sound_stream_data
{
    void * ptr;
    uint16_t len;
    bool loop;
    uint8_t padding;
} sound_stream_data_t;

typedef struct enemy_data
{
    int32_t hp;
    
    uint16_t attack;
    uint16_t defense;

    uint16_t money_min;
    uint16_t money_max;

    uint16_t width;
    uint16_t height;
} enemy_data_t;

typedef struct level_data
{
    uint16_t player_start_x;
    uint16_t player_start_y;

    void * tileset_tiles_lz4;
    void * tileset_palette;

    void * map_cells;
    void * map_lut;
    void * map_lut_col;

    void * spawner_ptr;
    void * interactable_ptr;

    void * map_overview_tiles_lz4;
    void * map_overview_palette;

    void * level_name;
} level_data_t;

typedef struct seq_command
{
    uint8_t opcode;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
} seq_command_t;

typedef struct sample_list_entry
{
    uint8_t id;
    void * data_ptr;
    uint16_t len;
    uint16_t sample_rate;
    uint16_t adsr;
    uint8_t ticks;
    // ticks are implied by length and sample rate if set to 0
} sample_list_entry_t;

typedef struct sample_list_entry_ins
{
    uint8_t id;
    void * data_ptr;
    uint16_t len;
    uint16_t sample_rate;
    uint16_t adsr;
    uint8_t ticks;
    // ticks are implied by length and sample rate if set to 0
    uint8_t tune;
} sample_list_entry_ins_t;

typedef struct spr_metaspr_definition
{
    uint16_t tileattrib;
    int16_t offset_x;
    int16_t offset_y;
    uint16_t size;
} spr_metaspr_definition_t;

typedef struct hdma_indirect_table_entry
{
    uint8_t count;
    uint16_t addr;
} hdma_indirect_table_entry_t;

typedef struct obj_list_entry_spawns
{
    uint16_t id;
    int16_t x;
    int16_t y;
    uint16_t random_spread;
} obj_list_entry_spawns_t;

typedef struct obj_list_entry_interactable
{
    uint16_t id;
    int16_t x;
    int16_t y;
    void * flag;
} obj_list_entry_interactable_t;

typedef struct obj_list_entry_spawners
{
    uint16_t id; // should always hold the spawner IDs
    int16_t x; // top left
    int16_t y;
    uint16_t w; // room size width
    uint16_t h; // room size height
    int16_t screen_x; // top left of screen bounding box
    int16_t screen_y; 
    uint16_t screen_w; // screen size
    uint16_t screen_h;
    void * spawn_list; // list of spawns

    uint8_t padding[10]; // array access padding
} obj_list_entry_spawners_t;

typedef struct tile_xy
{
    int16_t x;
    int16_t y;
} tile_xy_t;

typedef struct dma_entry 
{
    uint16_t vmain; // VMain type
    // Splits are handled on function, divide it into 2^split. For copying a square section.
    uint8_t * src; // A bus src
    uint16_t dest; // VRAM word dest
    uint16_t length;

    uint8_t padding[6]; 
} dma_entry_t;

typedef struct spr_queue_entry 
{
    int16_t x; 
    int16_t y;
    uint16_t tileattrib;
    uint16_t signsize;
    uint16_t depth;
    uint8_t padding[6];
} spr_queue_entry_t;

typedef struct pos_lh32
{
    int16_t l;
    int16_t h;
} pos_lh32_t;

typedef union pos_32
{
    int32_t a;
    struct pos_lh32 lh;
} pos_32_t;

typedef struct pos_lh16
{
    int8_t l;
    int8_t h;
} pos_lh16_t;

typedef union pos_16
{
    int16_t a;
    struct pos_lh16 lh;
} pos_16_t;

typedef struct pos_bgscroll_lh
{
    int16_t sub;
    union pos_16 high;
} pos_bgscroll_lh_t;

typedef union pos_bgscroll
{
    int32_t a;
    struct pos_bgscroll_lh full;
} pos_bgscroll_t;

typedef struct coords
{
    union pos_32 x;
    union pos_32 y;
    union pos_32 z;
} coords_t;

typedef struct ani_data
{   
    uint16_t frame; // timer of current frame
    uint16_t display; // currently displayed frame
    void * last_address; // Address of previous rendered frame
    uint16_t last_dmafailed; // Did the previous DMA fail?
} ani_data_t;

typedef struct game_data_npc
{
    // Case one: Player, NPCs, and particles
    // Animation system
    uint16_t tilenum; // tile number in sprite VRAM page
    uint16_t vram_addr; // vram address in sprite VRAM, offset from 0x6000
    struct ani_data ani;

    // Game data
    uint16_t ttl; // time remaining for auto-despawn
    int32_t hp; // current HP
    int32_t hp_max; // max HP
    uint16_t status; // current abnormal status
    uint16_t status_time; // abnormal status remaining time
    uint16_t invuln_time; // invuln time
    uint32_t hp_cache; // cached HP value to reduce load
    uint16_t hp_display_time; // time to display mini health bar
    uint16_t hp_tile_offset; // cached tile offset to use if no recalc

    int16_t attack;
    int16_t defense;

    uint32_t money; // held money

    // AI data
    uint16_t ai_state;
    uint16_t ai_timer;
    uint16_t ai_makeattack;
} game_data_npc_t;

typedef struct game_data_interactable
{
    // Case two: interactables and spawners (i.e. non-NPCs)
    uint16_t event_flag; // the local event flag it's tied to
    uint16_t delay_time; // timer

    uint16_t spawn_area_x;
    uint16_t spawn_area_y;
    uint16_t spawn_area_w;
    uint16_t spawn_area_h;

    int16_t screen_x; // top left of screen bounding box
    int16_t screen_y; 
    uint16_t screen_w; // screen size
    uint16_t screen_h;

    uint16_t opened; // for treasure chests, if it's opened
    uint16_t ttl; // time remaining for auto-despawn TODO: put this in shared area
    uint32_t money; // held money TODO: put this in shared area
} game_data_interactable_t;

typedef union game_data
{
    uint8_t size[68];
    struct game_data_npc npc_data;
    struct game_data_interactable interactable_data; 
} game_data_t;

typedef struct game_object
{
    // Object type
    uint16_t id;
    // Object and parent information
    uint16_t uid; // UID generated on instantiation
    uint16_t array_index;
    
    // Positions
    struct coords pos;
    struct coords delta;
    uint16_t state;
    uint16_t facing;
    uint16_t angle;
    struct tile_xy tile; // Used for blockers to simplify calcs. Also used for cached tile tests
    uint16_t w;
    uint16_t h;
    int16_t r; // right edge
    int16_t b; // bottom edge - used to save time in calcs

    uint16_t hit_type; // hitbox type

    void * data_ptr; // generic data pointer
    void * func_ptr; // generic function pointer
    uint16_t next_free; // next free object index

    // Everything after this can go to the same area
    union game_data struct_data;
} game_object_t;

typedef struct oam_entry_low
{
    // Match snes format
    uint8_t x;
    uint8_t y;
    uint16_t tileattrib;
} oam_entry_low_t;

typedef struct oam_entry_high
{
    // Expanded; must be packed before DMA
    uint8_t signsize;
} oam_entry_high_t;

typedef struct oam_combine 
{
    struct oam_entry_low shadow_oam_low[128];
    struct oam_entry_high shadow_oam_high[128];
} oam_combine_t;

typedef union oam_buffer{
    uint8_t bytes[640];
    struct oam_combine entries;
} oam_buffer_t;

typedef struct cgram_group {
    uint16_t bg[128];
    uint16_t spr[128];
} cgram_group_t;

typedef union cgram_full {
    uint16_t entry[256];
    struct cgram_group grouped;
} cgram_full_t;

#endif /* DEFS_STRUCTS_H */
