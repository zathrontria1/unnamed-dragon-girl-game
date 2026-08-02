"""
SNES Parallel & Incremental Asset Builder (tools/build_assets.py)
===================================================================

This script handles converting raw artwork, maps, and sound data into SNES-compatible
binary formats using `superfamiconv`, `lz4`, and custom python tools (`dedupe.py`, `map_conv.py`).

===================================================================
HOW TO ADD NEW ASSETS (DEVELOPER GUIDE):
===================================================================
1. ADDING A NEW PALETTE:
   Add a tuple to `PALETTE_DEFINITIONS` in Phase 1:
   (['bg/my_bg.png'], 'palette/palette_my_bg.bin', ['superfamiconv', 'palette', ...])

2. ADDING NEW TILES:
   Add a tuple to `TILE_DEFINITIONS` in Phase 2:
   (['bg/my_bg.png', 'palette/palette_my_bg.bin'], 'bg/my_bg.bin', ['superfamiconv', 'tiles', ...])

3. ADDING METASPRITE DEDUPLICATION:
   Add a task dict to `p3_tasks` in Phase 3:
   {
       'name': 'dedupe my_sprite',
       'inputs': ['sprites/my_sprite.bin'],
       'outputs': ['sprites/my_sprite.bin.dd'],
       'cmd': [sys.executable, 'tools/dedupe.py', 'sprites/my_sprite.bin', '-o', ...],
       'force': force
   }

4. LZ4 COMPRESSION:
   Files located in `sprites/`, `bg/`, `ui/`, `splash/`, `error/`, and `cutscene/intro/`
   ending in `.bin` are automatically discovered and compressed in Phase 4.

===================================================================
BUILD PHASES OVERVIEW:
===================================================================
- Phase 1: Convert PNG source images to palette binary files (.bin) and map JSONs to C headers (.h).
- Phase 2: Convert PNG source images using Phase 1 palettes into SNES tile data (.bin).
- Phase 3: Deduplicate sprite frame tiles (dedupe.py) and generate tilemaps (splash/error/cutscenes).
- Phase 4: LZ4 compress all binary assets (.bin -> .bin.lz4).
"""

import os
import sys
import glob
import json
import hashlib
import subprocess
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed

# Path to the build cache file storing content hashes
CACHE_FILE = os.path.join('.build', 'asset_cache.json')


def load_cache():
    """Load the recorded asset hash cache from .build/asset_cache.json."""
    if os.path.exists(CACHE_FILE):
        try:
            with open(CACHE_FILE, 'r') as f:
                return json.load(f)
        except Exception:
            return {}
    return {}


def save_cache(cache):
    """Save the updated asset hash cache to .build/asset_cache.json."""
    os.makedirs('.build', exist_ok=True)
    try:
        with open(CACHE_FILE, 'w') as f:
            json.dump(cache, f, indent=2)
    except Exception:
        pass


def compute_files_hash(file_paths, extra_key=""):
    """
    Compute an MD5 hash of one or more input files plus an optional extra key
    (e.g., LZ4 compression level or task name).
    This provides FAT32/Google Drive immune incremental build checks.
    """
    hasher = hashlib.md5()
    hasher.update(extra_key.encode('utf-8'))
    if isinstance(file_paths, str):
        file_paths = [file_paths]
    for fp in sorted(file_paths):
        if os.path.exists(fp):
            with open(fp, 'rb') as f:
                hasher.update(f.read())
        else:
            hasher.update(fp.encode('utf-8'))
    return hasher.hexdigest()


def task_is_up_to_date(task, cache, lz4_level, force=False):
    """
    Check if a task's output files are up-to-date.
    1. If force=True, always re-run.
    2. Check if all target output files exist on disk.
    3. Check if input content hashes match the cached hash from the previous build.
    4. Fall back to a 2.1-second FAT32 timestamp check if hash is missing.
    """
    if force:
        return False
    outputs = task['outputs']
    inputs = task['inputs']
    name = task.get('name', '')

    # All output files must exist
    if not all(os.path.exists(out) for out in outputs):
        return False

    curr_hash = compute_files_hash(inputs, extra_key=f"{name}:{lz4_level}")
    cached_entry = cache.get(name)

    if cached_entry and cached_entry.get('hash') == curr_hash:
        return True

    # Fallback timestamp check (2.1s threshold for FAT32 / Google Drive timestamp granularity)
    for out in outputs:
        out_mtime = os.path.getmtime(out)
        for inp in inputs:
            if os.path.exists(inp):
                inp_mtime = os.path.getmtime(inp)
                if inp_mtime - out_mtime > 2.1:
                    return False
    return True


def run_cmd(cmd_args):
    """Execute a subprocess command and capture errors if it fails."""
    cmd_str = " ".join(cmd_args) if isinstance(cmd_args, list) else cmd_args
    res = subprocess.run(cmd_args, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"\n[ERROR] Command failed: {cmd_str}\n{res.stderr}")
        return False
    return True


def process_task(task, cache, lz4_level):
    """
    Worker function executed in parallel threads.
    Returns: (success: bool, task_name: str, skipped: bool, new_hash: str or None)
    """
    inputs = task['inputs']
    name = task.get('name', '')
    cmd = task['cmd']
    force = task.get('force', False)

    if task_is_up_to_date(task, cache, lz4_level, force=force):
        return True, name, True, None  # Task skipped (already up to date)

    success = run_cmd(cmd)
    if success:
        new_hash = compute_files_hash(inputs, extra_key=f"{name}:{lz4_level}")
        return True, name, False, new_hash
    return False, name, False, None


def execute_phase(phase_name, tasks, cache, lz4_level, num_threads):
    """Execute a list of tasks in parallel using ThreadPoolExecutor."""
    skipped_count = 0
    executed_count = 0

    with ThreadPoolExecutor(max_workers=num_threads) as executor:
        futures = [executor.submit(process_task, t, cache, lz4_level) for t in tasks]
        for f in as_completed(futures):
            ok, name, skipped, new_hash = f.result()
            if not ok:
                print(f"[FATAL] Asset build failed during {phase_name} on task '{name}'.")
                sys.exit(1)
            if skipped:
                skipped_count += 1
            else:
                executed_count += 1
                if new_hash:
                    cache[name] = {'hash': new_hash}

    return executed_count, skipped_count


def build_assets(release_mode=False, force=False, num_threads=None):
    """Primary build runner for all SNES assets across 4 parallel execution phases."""
    if num_threads is None:
        num_threads = max(1, os.cpu_count() or 4)

    lz4_level = "-12" # LZ4 compression is so fast that we can always use maximum compression for release builds
    print(f"Building assets (LZ4 Level: {lz4_level}, Parallel Threads: {num_threads})...")

    cache = load_cache()
    total_executed = 0
    total_skipped = 0

    # =========================================================================
    # PHASE 1: MAP HEADERS & PALETTES
    # Inputs: .png files, .tmj maps
    # Outputs: palette .bin files, map header .h files
    # =========================================================================
    phase1_tasks = []

    # Map Lookup table & headers generator
    phase1_tasks.append({
        'name': 'map_make_lookup',
        'inputs': glob.glob('maps/*.tmj'),
        'outputs': ['maps/map_lut_dungeon.h'],
        'cmd': [sys.executable, 'maps/map_make_lookup.py', 'maps/map_lut_dungeon.h'],
        'force': force
    })

    # Individual Tiled map conversions (.tmj -> .h)
    for tmj in glob.glob('maps/*.tmj'):
        header_out = os.path.splitext(tmj)[0] + '.h'
        phase1_tasks.append({
            'name': f'map_conv {os.path.basename(tmj)}',
            'inputs': [tmj],
            'outputs': [header_out],
            'cmd': [sys.executable, 'maps/map_conv.py', tmj],
            'force': force
        })

    # Palette definitions table
    # Format: (input_png_list, output_palette_bin, command_args)
    PALETTE_DEFINITIONS = [
        (['bg/bg_blank.png'], 'palette/palette_bg_blank.bin', ['superfamiconv', 'palette', '-i', 'bg/bg_blank.png', '-d', 'palette/palette_bg_blank.bin', '-R', '-C', '16', '--color-zero', '#000000']),
        (['bg/bg_dungeon.png'], 'palette/palette_bg_dungeon_0.bin', ['superfamiconv', 'palette', '-i', 'bg/bg_dungeon.png', '-d', 'palette/palette_bg_dungeon_0.bin', '-R', '-C', '16', '--color-zero', '#000000']),
        (['bg/bg_dungeon_wall_brick.png'], 'palette/palette_bg_dungeon_1.bin', ['superfamiconv', 'palette', '-i', 'bg/bg_dungeon_wall_brick.png', '-d', 'palette/palette_bg_dungeon_1.bin', '-R', '-C', '16', '--color-zero', '#000000']),
        (['bg/bg_dungeon_anim_water.png'], 'palette/palette_bg_dungeon_2.bin', ['superfamiconv', 'palette', '-i', 'bg/bg_dungeon_anim_water.png', '-d', 'palette/palette_bg_dungeon_2.bin', '-R', '-C', '16', '--color-zero', '#000000']),
        (['maps/map_debug0_map.png'], 'palette/palette_bg_map_dungeon_0_8bpp.bin', ['superfamiconv', 'palette', '-i', 'maps/map_debug0_map.png', '-d', 'palette/palette_bg_map_dungeon_0_8bpp.bin', '-R', '-C', '256', '--color-zero', '#000000']),
        (['maps/map_debug1_map.png'], 'palette/palette_bg_map_dungeon_1_8bpp.bin', ['superfamiconv', 'palette', '-i', 'maps/map_debug1_map.png', '-d', 'palette/palette_bg_map_dungeon_1_8bpp.bin', '-R', '-C', '256', '--color-zero', '#000000']),
        (['ui/ui_fixed_2bpp.png'], 'palette/palette_ui_fixed_2bpp.bin', ['superfamiconv', 'palette', '-i', 'ui/ui_fixed_2bpp.png', '-d', 'palette/palette_ui_fixed_2bpp.bin', '-R', '-C', '4', '--color-zero', '#000000']),
        (['ui/ui_fixed_4bpp.png'], 'palette/palette_ui_fixed_4bpp.bin', ['superfamiconv', 'palette', '-i', 'ui/ui_fixed_4bpp.png', '-d', 'palette/palette_ui_fixed_4bpp.bin', '-R', '-C', '16', '--color-zero', '#000000']),
        (['sprites/spr_player.png'], 'palette/palette_spr_player.bin', ['superfamiconv', 'palette', '-i', 'sprites/spr_player.png', '-d', 'palette/palette_spr_player.bin', '-R', '-C', '16', '--color-zero', '#000000']),
        (['sprites/spr_player_portrait_indexed.png'], 'palette/palette_spr_player_portrait.bin', ['superfamiconv', 'palette', '-i', 'sprites/spr_player_portrait_indexed.png', '-d', 'palette/palette_spr_player_portrait.bin', '-R', '-C', '16', '--color-zero', '#000000']),
        (['sprites/spr_lizardman.png'], 'palette/palette_spr_common1.bin', ['superfamiconv', 'palette', '-i', 'sprites/spr_lizardman.png', '-d', 'palette/palette_spr_common1.bin', '-R', '-C', '16', '--color-zero', '#000000']),
        (['sprites/spr_slime.png'], 'palette/palette_spr_common0.bin', ['superfamiconv', 'palette', '-i', 'sprites/spr_slime.png', '-d', 'palette/palette_spr_common0.bin', '-R', '-C', '16', '--color-zero', '#000000']),
        (['sprites/boss/spr_boss_placeholder.png'], 'palette/palette_spr_boss_placeholder.bin', ['superfamiconv', 'palette', '-i', 'sprites/boss/spr_boss_placeholder.png', '-d', 'palette/palette_spr_boss_placeholder.bin', '-R', '-C', '16', '--color-zero', '#000000']),
        (['sprites/spr_cycle_fire.png'], 'palette/palette_cycle_fire.bin', ['superfamiconv', 'palette', '-i', 'sprites/spr_cycle_fire.png', '-d', 'palette/palette_cycle_fire.bin', '-R', '-C', '16', '--color-zero', '#000000']),
        (['splash/loading_splash_textonly.png'], 'splash/palette_splash.bin', ['superfamiconv', 'palette', '-i', 'splash/loading_splash_textonly.png', '-d', 'splash/palette_splash.bin', '-C', '16', '--color-zero', '#000000', '-R']),
        (['error/error_background_quantized.png'], 'palette/palette_error_background.bin', ['superfamiconv', 'palette', '-i', 'error/error_background_quantized.png', '-d', 'palette/palette_error_background.bin', '-P', '7', '-C', '16', '--color-zero', '#000000']),
        (['title/title_options.png'], 'palette/palette_title_options.bin', ['superfamiconv', 'palette', '-i', 'title/title_options.png', '-d', 'palette/palette_title_options.bin', '-P', '7', '-C', '16', '--color-zero', '#000000', '-R']),
    ]

    for inputs, out, cmd in PALETTE_DEFINITIONS:
        phase1_tasks.append({
            'name': f'palette {os.path.basename(out)}',
            'inputs': inputs,
            'outputs': [out],
            'cmd': cmd,
            'force': force
        })

    # Cutscene Palettes (dynamically discovered in cutscene/intro/)
    cs_pngs = glob.glob('cutscene/intro/*.png')
    for png in cs_pngs:
        base = os.path.splitext(png)[0]
        pal_out = f"{base}_p.bin"
        phase1_tasks.append({
            'name': f'cs palette {os.path.basename(png)}',
            'inputs': [png],
            'outputs': [pal_out],
            'cmd': ['superfamiconv', 'palette', '-i', png, '-d', pal_out, '-C', '16', '--color-zero', '#000000'],
            'force': force
        })

    exec1, skip1 = execute_phase("Phase 1 (Palettes & Headers)", phase1_tasks, cache, lz4_level, num_threads)
    total_executed += exec1
    total_skipped += skip1

    # =========================================================================
    # PHASE 2: TILE CONVERSIONS
    # Inputs: .png source images + Phase 1 .bin palettes
    # Outputs: tile binary data (.bin)
    # =========================================================================
    TILE_DEFINITIONS = [
        (['bg/bg_dungeon.png', 'palette/palette_bg_dungeon_0.bin'], 'bg/bg_dungeon.bin', ['superfamiconv', 'tiles', '-i', 'bg/bg_dungeon.png', '-p', 'palette/palette_bg_dungeon_0.bin', '-d', 'bg/bg_dungeon.bin', '-B', '4', '-D', '-F', '-R']),
        (['bg/bg_dungeon_anim_water.png', 'palette/palette_bg_dungeon_2.bin'], 'bg/bg_dungeon_anim_water.bin', ['superfamiconv', 'tiles', '-i', 'bg/bg_dungeon_anim_water.png', '-p', 'palette/palette_bg_dungeon_2.bin', '-d', 'bg/bg_dungeon_anim_water.bin', '-B', '4', '-D', '-F', '-R']),
        (['bg/bg_dungeon_anim_torch.png', 'palette/palette_bg_dungeon_0.bin'], 'bg/bg_dungeon_anim_torch.bin', ['superfamiconv', 'tiles', '-i', 'bg/bg_dungeon_anim_torch.png', '-p', 'palette/palette_bg_dungeon_0.bin', '-d', 'bg/bg_dungeon_anim_torch.bin', '-B', '4', '-D', '-F', '-R']),
        (['maps/map_debug0_map.png', 'palette/palette_bg_map_dungeon_0_8bpp.bin'], 'bg/bg_map_dungeon_0_8bpp.bin', ['superfamiconv', 'tiles', '-i', 'maps/map_debug0_map.png', '-p', 'palette/palette_bg_map_dungeon_0_8bpp.bin', '-d', 'bg/bg_map_dungeon_0_8bpp.bin', '-B', '8', '-D', '-F', '-R']),
        (['maps/map_debug1_map.png', 'palette/palette_bg_map_dungeon_1_8bpp.bin'], 'bg/bg_map_dungeon_1_8bpp.bin', ['superfamiconv', 'tiles', '-i', 'maps/map_debug1_map.png', '-p', 'palette/palette_bg_map_dungeon_1_8bpp.bin', '-d', 'bg/bg_map_dungeon_1_8bpp.bin', '-B', '8', '-D', '-F', '-R']),
        (['ui/ui_fixed_2bpp.png', 'palette/palette_ui_fixed_2bpp.bin'], 'ui/ui_fixed_2bpp.bin', ['superfamiconv', 'tiles', '-i', 'ui/ui_fixed_2bpp.png', '-p', 'palette/palette_ui_fixed_2bpp.bin', '-d', 'ui/ui_fixed_2bpp.bin', '-B', '2', '-D', '-F', '-R']),
        (['ui/ui_dynamic_textadvance.png', 'palette/palette_ui_fixed_2bpp.bin'], 'ui/ui_dynamic_textadvance.bin', ['superfamiconv', 'tiles', '-i', 'ui/ui_dynamic_textadvance.png', '-p', 'palette/palette_ui_fixed_2bpp.bin', '-d', 'ui/ui_dynamic_textadvance.bin', '-B', '2', '-D', '-F', '-R']),
        (['ui/ui_dynamic_selectcursor.png', 'palette/palette_ui_fixed_2bpp.bin'], 'ui/ui_dynamic_selectcursor.bin', ['superfamiconv', 'tiles', '-i', 'ui/ui_dynamic_selectcursor.png', '-p', 'palette/palette_ui_fixed_2bpp.bin', '-d', 'ui/ui_dynamic_selectcursor.bin', '-B', '2', '-D', '-F', '-R']),
        (['ui/ui_vwf.png', 'palette/palette_ui_fixed_2bpp.bin'], 'ui/ui_vwf.bin', ['superfamiconv', 'tiles', '-i', 'ui/ui_vwf.png', '-p', 'palette/palette_ui_fixed_2bpp.bin', '-d', 'ui/ui_vwf.bin', '-B', '2', '-D', '-F', '-R']),
        (['ui/ui_fixed_4bpp.png', 'palette/palette_ui_fixed_4bpp.bin'], 'ui/ui_fixed_4bpp.bin', ['superfamiconv', 'tiles', '-i', 'ui/ui_fixed_4bpp.png', '-p', 'palette/palette_ui_fixed_4bpp.bin', '-d', 'ui/ui_fixed_4bpp.bin', '-B', '4', '-D', '-F', '-R']),
        (['ui/ui_dynamic_hp.png', 'palette/palette_ui_fixed_4bpp.bin'], 'ui/ui_dynamic_hp.bin', ['superfamiconv', 'tiles', '-i', 'ui/ui_dynamic_hp.png', '-p', 'palette/palette_ui_fixed_4bpp.bin', '-d', 'ui/ui_dynamic_hp.bin', '-B', '4', '-D', '-F', '-R']),
        (['sprites/spr_player.png', 'palette/palette_spr_player.bin'], 'sprites/spr_player.bin', ['superfamiconv', 'tiles', '-i', 'sprites/spr_player.png', '-p', 'palette/palette_spr_player.bin', '-d', 'sprites/spr_player.bin', '-B', '4', '-D', '-F', '-R']),
        (['sprites/spr_player_portrait_indexed.png', 'palette/palette_spr_player_portrait.bin'], 'sprites/spr_player_portrait.bin', ['superfamiconv', 'tiles', '-i', 'sprites/spr_player_portrait_indexed.png', '-p', 'palette/palette_spr_player_portrait.bin', '-d', 'sprites/spr_player_portrait.bin', '-B', '4', '-D', '-F', '-R']),
        (['sprites/spr_lizardman.png', 'palette/palette_spr_common1.bin'], 'sprites/spr_lizardman.bin', ['superfamiconv', 'tiles', '-i', 'sprites/spr_lizardman.png', '-p', 'palette/palette_spr_common1.bin', '-d', 'sprites/spr_lizardman.bin', '-B', '4', '-D', '-F', '-R']),
        (['sprites/spr_slime.png', 'palette/palette_spr_common0.bin'], 'sprites/spr_slime.bin', ['superfamiconv', 'tiles', '-i', 'sprites/spr_slime.png', '-p', 'palette/palette_spr_common0.bin', '-d', 'sprites/spr_slime.bin', '-B', '4', '-D', '-F', '-R']),
        (['sprites/spr_spawn_placeholder.png', 'palette/palette_spr_common0.bin'], 'sprites/spr_spawn_placeholder.bin', ['superfamiconv', 'tiles', '-i', 'sprites/spr_spawn_placeholder.png', '-p', 'palette/palette_spr_common0.bin', '-d', 'sprites/spr_spawn_placeholder.bin', '-B', '4', '-D', '-F', '-R']),
        (['sprites/spr_fixed.png', 'palette/palette_spr_player.bin'], 'sprites/spr_fixed.bin', ['superfamiconv', 'tiles', '-i', 'sprites/spr_fixed.png', '-p', 'palette/palette_spr_player.bin', '-d', 'sprites/spr_fixed.bin', '-B', '4', '-D', '-F', '-R']),
        (['sprites/spr_drop_coin.png', 'palette/palette_spr_player.bin'], 'sprites/spr_drop_coin.bin', ['superfamiconv', 'tiles', '-i', 'sprites/spr_drop_coin.png', '-p', 'palette/palette_spr_player.bin', '-d', 'sprites/spr_drop_coin.bin', '-B', '4', '-D', '-F', '-R']),
        (['sprites/boss/spr_boss_placeholder.png', 'palette/palette_spr_boss_placeholder.bin'], 'sprites/boss/spr_boss_placeholder.bin', ['superfamiconv', 'tiles', '-i', 'sprites/boss/spr_boss_placeholder.png', '-p', 'palette/palette_spr_boss_placeholder.bin', '-d', 'sprites/boss/spr_boss_placeholder.bin', '-B', '4', '-D', '-F', '-R', '-T', '2560']),
        (['sprites/boss/spr_boss_addon_attack1.png', 'palette/palette_spr_boss_placeholder.bin'], 'sprites/boss/spr_boss_addon_attack1.bin', ['superfamiconv', 'tiles', '-i', 'sprites/boss/spr_boss_addon_attack1.png', '-p', 'palette/palette_spr_boss_placeholder.bin', '-d', 'sprites/boss/spr_boss_addon_attack1.bin', '-B', '4', '-D', '-F', '-R']),
        (['sprites/boss/spr_boss_hands.png', 'palette/palette_spr_boss_placeholder.bin'], 'sprites/boss/spr_boss_hands.bin', ['superfamiconv', 'tiles', '-i', 'sprites/boss/spr_boss_hands.png', '-p', 'palette/palette_spr_boss_placeholder.bin', '-d', 'sprites/boss/spr_boss_hands.bin', '-B', '4', '-D', '-F', '-R']),
        (['splash/loading_splash_textonly.png', 'splash/palette_splash.bin'], 'splash/loading_splash.bin', ['superfamiconv', 'tiles', '-i', 'splash/loading_splash_textonly.png', '-p', 'splash/palette_splash.bin', '-d', 'splash/loading_splash.bin', '-B', '4']),
        (['error/error_background_quantized.png', 'palette/palette_error_background.bin'], 'error/error_background.bin', ['superfamiconv', 'tiles', '-i', 'error/error_background_quantized.png', '-p', 'palette/palette_error_background.bin', '-d', 'error/error_background.bin', '-B', '4']),
        (['title/title_options.png', 'palette/palette_title_options.bin'], 'title/title_options.bin', ['superfamiconv', 'tiles', '-i', 'title/title_options.png', '-p', 'palette/palette_title_options.bin', '-d', 'title/title_options.bin', '-B', '4', '-D', '-F', '-R']),
    ]

    phase2_tasks = []
    for inputs, out, cmd in TILE_DEFINITIONS:
        phase2_tasks.append({
            'name': f'tiles {os.path.basename(out)}',
            'inputs': inputs,
            'outputs': [out],
            'cmd': cmd,
            'force': force
        })

    # Cutscene Tiles
    for png in cs_pngs:
        base = os.path.splitext(png)[0]
        pal_in = f"{base}_p.bin"
        tile_out = f"{base}.bin"
        phase2_tasks.append({
            'name': f'cs tiles {os.path.basename(png)}',
            'inputs': [png, pal_in],
            'outputs': [tile_out],
            'cmd': ['superfamiconv', 'tiles', '-i', png, '-p', pal_in, '-d', tile_out, '-B', '4'],
            'force': force
        })

    exec2, skip2 = execute_phase("Phase 2 (Tiles)", phase2_tasks, cache, lz4_level, num_threads)
    total_executed += exec2
    total_skipped += skip2

    # =========================================================================
    # PHASE 3: SPRITE DEDUPLICATION & TILEMAP GENERATION
    # Inputs: Phase 2 .bin tiles
    # Outputs: .bin.dd deduplicated tiles, tilemap .bin files
    # =========================================================================
    p3_tasks = [
        {
            'name': 'dedupe spr_player',
            'inputs': ['sprites/spr_player.bin'],
            'outputs': ['sprites/spr_player.bin.dd'],
            'cmd': [sys.executable, 'tools/dedupe.py', 'sprites/spr_player.bin', '-o', 'sprites/spr_player.bin.dd', '--bpp4', '-w', '--framewidth', '16', '--frameheight', '16'],
            'force': force
        },
        {
            'name': 'dedupe spr_boss_placeholder',
            'inputs': ['sprites/boss/spr_boss_placeholder.bin'],
            'outputs': ['sprites/boss/spr_boss_placeholder.bin.dd'],
            'cmd': [sys.executable, 'tools/dedupe.py', 'sprites/boss/spr_boss_placeholder.bin', '-o', 'sprites/boss/spr_boss_placeholder.bin.dd', '--bpp4', '-w', '--framewidth', '64', '--frameheight', '96'],
            'force': force
        },
        {
            'name': 'map splash',
            'inputs': ['splash/loading_splash_textonly.png', 'splash/palette_splash.bin', 'splash/loading_splash.bin'],
            'outputs': ['splash/loading_splash_tilemap.bin'],
            'cmd': ['superfamiconv', 'map', '-i', 'splash/loading_splash_textonly.png', '-p', 'splash/palette_splash.bin', '-t', 'splash/loading_splash.bin', '-d', 'splash/loading_splash_tilemap.bin', '-B', '4'],
            'force': force
        },
        {
            'name': 'map error',
            'inputs': ['error/error_background_quantized.png', 'palette/palette_error_background.bin', 'error/error_background.bin'],
            'outputs': ['error/error_background_tilemap.bin'],
            'cmd': ['superfamiconv', 'map', '-i', 'error/error_background_quantized.png', '-p', 'palette/palette_error_background.bin', '-t', 'error/error_background.bin', '-d', 'error/error_background_tilemap.bin', '-B', '4', '-P', '1'],
            'force': force
        }
    ]

    # Cutscene Tilemaps
    for png in cs_pngs:
        base = os.path.splitext(png)[0]
        pal_in = f"{base}_p.bin"
        tile_in = f"{base}.bin"
        map_out = f"{base}_t.bin"
        p3_tasks.append({
            'name': f'cs map {os.path.basename(png)}',
            'inputs': [png, pal_in, tile_in],
            'outputs': [map_out],
            'cmd': ['superfamiconv', 'map', '-i', png, '-p', pal_in, '-t', tile_in, '-d', map_out, '-B', '4'],
            'force': force
        })

    exec3, skip3 = execute_phase("Phase 3 (Dedupe & Maps)", p3_tasks, cache, lz4_level, num_threads)
    total_executed += exec3
    total_skipped += skip3

    # =========================================================================
    # PHASE 4: LZ4 COMPRESSION
    # Inputs: All generated .bin files in target directories
    # Outputs: compressed .bin.lz4 files
    # =========================================================================
    lz4_folders = ['sprites', 'bg', 'ui', 'splash', 'error', 'cutscene/intro', 'title']
    bin_files = []
    for fld in lz4_folders:
        bin_files.extend(glob.glob(f"{fld}/*.bin"))

    lz4_tasks = []
    for bf in bin_files:
        out_lz4 = bf + ".lz4"
        lz4_tasks.append({
            'name': f'lz4 {os.path.basename(bf)}',
            'inputs': [bf],
            'outputs': [out_lz4],
            'cmd': ['lz4', lz4_level, '-f', '-m', '--content-size', bf],
            'force': force
        })

    exec4, skip4 = execute_phase("Phase 4 (LZ4 Compression)", lz4_tasks, cache, lz4_level, num_threads)
    total_executed += exec4
    total_skipped += skip4

    # Save the updated content hash cache to disk
    save_cache(cache)

    print(f"Asset build completed: {total_executed} tasks executed, {total_skipped} tasks skipped (up-to-date).")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Parallel & Incremental Asset Builder")
    parser.add_argument('--release', action='store_true', help="Use maximum LZ4 compression (-12)")
    parser.add_argument('--fast', action='store_true', help="Use fast LZ4 compression (-1)")
    parser.add_argument('--force', action='store_true', help="Force rebuild all assets regardless of timestamps")
    parser.add_argument('--threads', type=int, default=None, help="Number of parallel threads")
    args = parser.parse_args()

    build_assets(release_mode=args.release, force=args.force, num_threads=args.threads)
