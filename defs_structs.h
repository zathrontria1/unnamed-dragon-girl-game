struct level_data
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
};

struct seq_command
{
    uint8_t opcode;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
};

struct sample_list_entry
{
    uint8_t id;
    void * data_ptr;
    uint16_t len;
    uint16_t sample_rate;
    uint16_t adsr;
    uint8_t ticks;
    // ticks are implied by length and sample rate if set to 0
};

struct sample_list_entry_ins
{
    uint8_t id;
    void * data_ptr;
    uint16_t len;
    uint16_t sample_rate;
    uint16_t adsr;
    uint8_t ticks;
    // ticks are implied by length and sample rate if set to 0
    uint8_t tune;
};

struct spr_metaspr_definition
{
    uint16_t tileattrib;
    int16_t offset_x;
    int16_t offset_y;
    uint16_t size;
};

struct hdma_indirect_table_entry
{
    uint8_t count;
    uint16_t addr;
};

struct obj_list_entry_spawns
{
    uint16_t id;
    int16_t x;
    int16_t y;
    uint16_t random_spread;
};

struct obj_list_entry_interactable
{
    uint16_t id;
    int16_t x;
    int16_t y;
    void * flag;
};

struct obj_list_entry_spawners
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
};

struct tile_xy
{
    int16_t x;
    int16_t y;
};

struct dma_entry 
{
    uint16_t vmain; // VMain type
    // Splits are handled on function, divide it into 2^split. For copying a square section.
    uint8_t * src; // A bus src
    uint16_t dest; // VRAM word dest
    uint16_t length;

    uint8_t padding[6]; 
};

struct spr_queue_entry 
{
    int16_t x; 
    int16_t y;
    uint16_t tileattrib;
    uint16_t signsize;
    uint16_t depth;
    uint8_t padding[6];
};

struct pos_lh32
{
    int16_t l;
    int16_t h;
};

union pos_32
{
    int32_t a;
    struct pos_lh32 lh;
};

struct pos_lh16
{
    int8_t l;
    int8_t h;
};

union pos_16
{
    int16_t a;
    struct pos_lh16 lh;
};

struct pos_bgscroll_lh
{
    int16_t sub;
    union pos_16 high;
};

union pos_bgscroll
{
    int32_t a;
    struct pos_bgscroll_lh full;
};

struct coords
{
    union pos_32 x;
    union pos_32 y;
    union pos_32 z;
};

struct ani_data
{   
    uint16_t frame; // timer of current frame
    uint16_t display; // currently displayed frame
    uint8_t * last_address; // Address of previous rendered frame
    uint16_t last_dmafailed; // Did the previous DMA fail?
};

struct game_object
{
    // Object type
    uint16_t id;
    // Object and parent information
    uint16_t uid; // UID generated on instantiation
    uint32_t * parent_map; // The map the object belonged to

    uint16_t array_index;
    
    // Positions
    struct coords pos;
    struct coords delta;
    uint16_t state;
    uint16_t facing;
    uint8_t angle;
    struct tile_xy tile; // Used for blockers to simplify calcs. Also used for cached tile tests
    uint16_t w;
    uint16_t h;

    // Animation system
    //uint16_t oam_index; // the index number of the OAM slot it had
    uint16_t tilenum; // tile number in sprite VRAM page
    uint16_t vram_addr; // vram address in sprite VRAM, offset from 0x6000
    struct ani_data ani;

    // AI data
    uint16_t ai_state;
    uint16_t ai_timer;

    uint16_t ai_makeattack;

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

    uint16_t event_flag; // the local event flag it's tied to

    uint32_t money; // held money

    uint8_t * string_ptr;

    uint16_t spawn_area_x;
    uint16_t spawn_area_y;
    uint16_t spawn_area_w;
    uint16_t spawn_area_h;

    int16_t screen_x; // top left of screen bounding box
    int16_t screen_y; 
    uint16_t screen_w; // screen size
    uint16_t screen_h;

    uint16_t hit_type; // hitbox type

    uint16_t next_free; // next free object index
    
    uint8_t padding[3]; 
};

struct oam_entry_low
{
    // Match snes format
    uint8_t x;
    uint8_t y;
    uint16_t tileattrib;
};
struct oam_entry_high
{
    // Expanded; must be packed before DMA
    uint8_t signsize;
};

struct oam_combine 
{
    struct oam_entry_low shadow_oam_low[128];
    struct oam_entry_high shadow_oam_high[128];
};

union oam_buffer{
    uint8_t bytes[640];
    struct oam_combine entries;
};

struct cgram_group {
    uint16_t bg[128];
    uint16_t spr[128];
};

union cgram_full {
    uint16_t entry[256];
    struct cgram_group grouped;
};
