import sys
import os
import struct
import math
import wave
import subprocess
import argparse

def parse_it_file(it_path):
    with open(it_path, "rb") as f:
        data = f.read()

    name = data[4:30].decode("latin1", errors="replace").rstrip("\x00")
    ordnum, insnum, smpnum, patnum = struct.unpack_from("<HHHH", data, 32)
    flags, special = struct.unpack_from("<HH", data, 36)
    gvl, mv, ispd, itm, sep = struct.unpack_from("<BBBBB", data, 48)

    chn_pan = list(data[64:128])
    chn_vol = list(data[128:192])

    order_list = list(data[192 : 192 + ordnum])
    offset = 192 + ordnum + insnum * 4
    smp_offsets = struct.unpack_from(f"<{smpnum}I", data, offset)
    offset += smpnum * 4
    pat_offsets = struct.unpack_from(f"<{patnum}I", data, offset)

    playable_orders = [o for o in order_list if o < patnum]
    unique_patterns = sorted(set(playable_orders))

    # Parse all pattern events
    pattern_data = {} # pidx -> {ch: [(row, note, ins, vol, eff_cmd, eff_val)]}
    pattern_num_rows = {}
    sample_max_notes = {} # s_idx -> max note

    for pidx in range(patnum):
        poff = pat_offsets[pidx]
        if poff == 0:
            pattern_data[pidx] = {ch: [] for ch in range(32)}
            pattern_num_rows[pidx] = 64
            continue

        pat_len, num_rows = struct.unpack_from("<HH", data, poff)
        pdata = data[poff + 8 : poff + 8 + pat_len]
        pattern_num_rows[pidx] = num_rows

        ptr = 0; row = 0; mask_vars = [0] * 64
        last_note = [None] * 64; last_ins = [None] * 64; last_vol = [None] * 64; last_eff = [None] * 64
        ch_events = {ch: [] for ch in range(32)}

        while row < num_rows and ptr < len(pdata):
            b = pdata[ptr]; ptr += 1
            if b == 0: row += 1; continue
            ch = (b - 1) & 63
            if b & 128: mask = pdata[ptr]; ptr += 1; mask_vars[ch] = mask
            else: mask = mask_vars[ch]

            note, ins, vol, eff_cmd, eff_val = None, None, None, None, None
            if mask & 1: note = pdata[ptr]; ptr += 1; last_note[ch] = note
            if mask & 2: ins = pdata[ptr]; ptr += 1; last_ins[ch] = ins
            if mask & 4: vol = pdata[ptr]; ptr += 1; last_vol[ch] = vol
            if mask & 8:
                eff_cmd = pdata[ptr]; eff_val = pdata[ptr+1]; ptr += 2
                last_eff[ch] = (eff_cmd, eff_val)

            if mask & 16: note = last_note[ch]
            if mask & 32: ins = last_ins[ch]
            if mask & 64: vol = last_vol[ch]
            if mask & 128 and last_eff[ch]: eff_cmd, eff_val = last_eff[ch]

            if any(x is not None for x in (note, ins, vol, eff_cmd)):
                ch_events[ch].append((row, note, ins, vol, eff_cmd, eff_val))

            # Track max note per sample
            if note is not None and note < 120:
                s_ref = ins if ins is not None else last_ins[ch]
                if s_ref is not None:
                    sample_max_notes[s_ref] = max(sample_max_notes.get(s_ref, note), note)
            elif ins is not None:
                if ins not in sample_max_notes:
                    sample_max_notes[ins] = 60

        pattern_data[pidx] = ch_events

    return {
        "name": name,
        "speed": ispd,
        "tempo": itm,
        "chn_pan": chn_pan,
        "chn_vol": chn_vol,
        "raw_data": data,
        "smpnum": smpnum,
        "smp_offsets": smp_offsets,
        "patnum": patnum,
        "pat_offsets": pat_offsets,
        "playable_orders": playable_orders,
        "unique_patterns": unique_patterns,
        "pattern_data": pattern_data,
        "pattern_num_rows": pattern_num_rows,
        "sample_max_notes": sample_max_notes
    }

def calc_stereo_vol(raw_vol, pan):
    if raw_vol <= 0:
        return (0, 0)
    if pan > 64 and pan != 100:
        pan = 32
    if pan == 100 or pan > 64:
        vl = max(1, (raw_vol * 31 + 32) // 64)
        return (vl, vl)
    if pan <= 32:
        vl = max(1, (raw_vol * 31 + 32) // 64)
        vr = (vl * pan) // 32
    else:
        vr = max(1, (raw_vol * 31 + 32) // 64)
        vl = (vr * (64 - pan)) // 32
    return (vl, vr)

def convert_it_song(it_path, max_channels=8, prefix="", merge_drums=True):
    parsed = parse_it_file(it_path)
    data = parsed["raw_data"]
    smp_offsets = parsed["smp_offsets"]
    smpnum = parsed["smpnum"]
    playable_orders = parsed["playable_orders"]
    unique_patterns = parsed["unique_patterns"]
    pat_data = parsed["pattern_data"]
    pat_rows = parsed["pattern_num_rows"]
    sample_max_notes = parsed["sample_max_notes"]
    chn_pan = parsed["chn_pan"]

    song_slug = prefix if prefix else os.path.splitext(os.path.basename(it_path))[0].lower().replace(".", "_").replace(" ", "_")
    if song_slug.startswith("module1"):
        song_slug = "module1"

    print(f"\n=======================================================")
    print(f"CONVERTING: {it_path} -> song '{song_slug}'")
    print(f"Speed: {parsed['speed']}, Tempo: {parsed['tempo']}, Orders: {len(playable_orders)}, Patterns: {len(unique_patterns)}")
    print(f"=======================================================")

    os.makedirs("sound/ins/itconv", exist_ok=True)
    os.makedirs("sound/seq", exist_ok=True)

    # 1. Process and convert used samples
    sample_info = {}
    used_samples = sorted(set(sample_max_notes.keys()))

    for s_idx in used_samples:
        if s_idx > smpnum: continue
        soff = smp_offsets[s_idx - 1]
        if soff == 0: continue
        gvl, flags_s, vol, sname = struct.unpack_from("<BBB26s", data, soff + 16)
        length, loopbeg, loopend, c5spd, susloopbeg, susloopend, smpptr = struct.unpack_from("<IIIIIII", data, soff + 48)
        if length == 0: continue

        convert = data[soff + 45]
        is_signed = bool(convert & 1)
        is_looped = bool(flags_s & 16) or (loopbeg > 0 or loopend > 0)

        raw_smp = data[smpptr : smpptr + length]
        s_name = f"ins_{song_slug}_smp{s_idx}"
        max_n = sample_max_notes.get(s_idx, 60)

        # Calculate peak DSP pitch at max note
        semitones = max_n - 60
        pitch_mult = 2.0 ** (semitones / 12.0)
        peak_dsp_pitch = ((c5spd * pitch_mult) * 4096.0) / 32000.0

        # Downsample if peak pitch exceeds 0x1000 (4096) to prevent DSP sample-skipping aliasing
        target_pitch_limit = 4096.0 # 0x1000
        downsample_factor = 1
        if peak_dsp_pitch > target_pitch_limit:
            downsample_factor = math.ceil(peak_dsp_pitch / target_pitch_limit)
            new_peak = peak_dsp_pitch / downsample_factor
            print(f"Sample {s_idx} ({s_name}): Peak pitch 0x{int(peak_dsp_pitch):04X} at note {max_n} -> Downsampling by {downsample_factor}x to keep peak pitch <= 0x1000 (New: 0x{int(new_peak):04X}) to eliminate aliasing!")

        if is_looped and (loopend > loopbeg):
            cycle = raw_smp[loopbeg:loopend]
            if is_signed:
                s_vals = [struct.unpack("b", bytes([b]))[0] * 256 for b in cycle]
            else:
                s_vals = [(b - 128) * 256 for b in cycle]

            L = len(s_vals)
            if downsample_factor > 1:
                target_M = round(L / downsample_factor)
                c5spd = round(c5spd * target_M / L)
                resampled_vals = []
                for i in range(target_M):
                    src_pos = (i * L) / target_M
                    idx0 = int(src_pos)
                    idx1 = (idx0 + 1) % L
                    frac = src_pos - idx0
                    val = (1.0 - frac) * s_vals[idx0] + frac * s_vals[idx1]
                    resampled_vals.append(int(val))
                s_vals = resampled_vals

            cycle_len = len(s_vals)
            # Ensure multiple of 32 samples (even number of 9-byte BRR blocks) so BRR audio bytes is strictly EVEN for 2-byte upload
            m = 32 // math.gcd(cycle_len, 32)
            pcm16 = bytearray()
            for _ in range(m):
                for val in s_vals:
                    pcm16 += struct.pack("<h", val)
        else:
            if is_signed:
                s_vals = [struct.unpack("b", bytes([b]))[0] * 256 for b in raw_smp]
            else:
                s_vals = [(b - 128) * 256 for b in raw_smp]

            L = len(s_vals)
            if downsample_factor > 1:
                target_M = round(L / downsample_factor)
                c5spd = round(c5spd * target_M / L)
                resampled_vals = []
                for i in range(target_M):
                    src_pos = (i * L) / target_M
                    idx0 = int(src_pos)
                    idx1 = min(idx0 + 1, L - 1)
                    frac = src_pos - idx0
                    val = (1.0 - frac) * s_vals[idx0] + frac * s_vals[idx1]
                    resampled_vals.append(int(val))
                s_vals = resampled_vals

            pcm16 = bytearray()
            for val in s_vals:
                pcm16 += struct.pack("<h", val)

            n_samples = len(pcm16) // 2
            # Pad to multiple of 32 samples (even number of 9-byte BRR blocks)
            target_samples = ((n_samples + 31) // 32) * 32
            pad = target_samples - n_samples
            pcm16 += b"\x00\x00" * pad

        wav_path = f"sound/ins/itconv/{s_name}.wav"
        brr_path = f"sound/ins/itconv/{s_name}.brr"

        with wave.open(wav_path, "wb") as wf:
            wf.setnchannels(1)
            wf.setsampwidth(2)
            wf.setframerate(c5spd)
            wf.writeframes(pcm16)

        if is_looped:
            cmd = ["snesbrr", "-e", "-h", "-l", "0", wav_path, brr_path]
        else:
            cmd = ["wav2brr", "-o", brr_path, wav_path]
        
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
            cmd_snes = ["snesbrr", "-e", "-h", wav_path, brr_path]
            subprocess.run(cmd_snes, check=True)

        brr_size = os.path.getsize(brr_path)
        sample_info[s_idx] = {
            "name": s_name,
            "brr_file": brr_path,
            "brr_size": brr_size,
            "c5spd": c5spd,
            "vol": vol,
            "is_looped": is_looped,
            "loop_start": 0
        }
        print(f"Sample {s_idx} ({s_name}): C5spd={c5spd}Hz, BRR Size={brr_size} bytes (with 2-byte header)")

    # 2. Build local instrument mapping: s_idx -> local_id (0..N-1)
    sample_to_local_ins = {}
    for local_id, s_idx in enumerate(sorted(sample_info.keys())):
        sample_to_local_ins[s_idx] = local_id

    # 3. Optimize channels: Detect 100% identical channel pairs and merge percussion
    merged_into = {}
    for i in range(max_channels):
        if i in merged_into: continue
        for j in range(i+1, max_channels):
            if j in merged_into: continue
            is_ident = True
            for pidx in unique_patterns:
                evA = pat_data[pidx][i]
                evB = pat_data[pidx][j]
                if len(evA) != len(evB):
                    is_ident = False
                    break
                for eA, eB in zip(evA, evB):
                    if eA[0] != eB[0] or eA[1] != eB[1]:
                        is_ident = False
                        break
                if not is_ident: break
            if is_ident:
                merged_into[j] = i
                chn_pan[i] = 32
                print(f"Detected identical channel pair: Ch {j+1} merged into Ch {i+1} (Center panned)")

    # Percussion channel consolidation (e.g. Ch 7 & 8 if unlooped one-shots)
    if merge_drums and 6 < max_channels and 7 < max_channels and 6 not in merged_into and 7 not in merged_into:
        has_7 = any(pat_data[pidx][6] for pidx in unique_patterns)
        has_8 = any(pat_data[pidx][7] for pidx in unique_patterns)
        if has_7 and has_8:
            print("Merging percussion channels 7 and 8 into single drum track")
            for pidx in unique_patterns:
                ev7 = {e[0]: e for e in pat_data[pidx][6]}
                ev8 = {e[0]: e for e in pat_data[pidx][7]}
                all_rows = sorted(set(ev7.keys()) | set(ev8.keys()))
                combined = []
                for r in all_rows:
                    e7 = ev7.get(r)
                    e8 = ev8.get(r)
                    if e7 and e8: combined.append(e7) # Kick takes priority
                    elif e7: combined.append(e7)
                    elif e8: combined.append(e8)
                pat_data[pidx][6] = combined
            merged_into[7] = 6
            chn_pan[6] = 32

    active_channels = [ch for ch in range(max_channels) if ch not in merged_into and any(pat_data[pidx][ch] for pidx in unique_patterns)]
    print(f"Active Channels ({len(active_channels)}): {[c+1 for c in active_channels]}")

    # 4. Generate pattern-based bytecode for each active channel
    track_code = {}

    for ch in active_channels:
        # Determine default instrument for channel if any note played
        first_smp = None
        for pidx in unique_patterns:
            for ev in pat_data[pidx][ch]:
                if ev[2] is not None and ev[2] in sample_to_local_ins:
                    first_smp = ev[2]
                    break
            if first_smp is not None: break
        
        default_local_id = sample_to_local_ins.get(first_smp, 0)
        pan = chn_pan[ch] if ch < len(chn_pan) else 32

        pat_tokens = {}
        for pidx in unique_patterns:
            events = sorted(pat_data[pidx][ch], key=lambda x: x[0])
            toks = []
            curr_row = 0
            curr_ins = None
            curr_raw_vol = 64
            curr_vl, curr_vr = calc_stereo_vol(64, pan)
            num_rows = pat_rows[pidx]
            last_d = 0
            speed = parsed["speed"]

            for ev in events:
                r, note, ins, vol, eff_cmd, eff_val = ev
                if r < curr_row: continue
                if r > curr_row:
                    wait_ticks = r - curr_row - 1
                    if wait_ticks == 0: toks.append("    SEQ_WAIT_0,")
                    elif 1 <= wait_ticks <= 15: toks.append(f"    SEQ_WAIT_{wait_ticks},")
                    else: toks.append(f"    SEQ_WAIT({wait_ticks}),")
                    curr_row = r

                if ins is not None and ins in sample_to_local_ins:
                    lid = sample_to_local_ins[ins]
                    if lid != curr_ins:
                        toks.append(f"    SEQ_SET_INS({lid}),")
                        curr_ins = lid

                # Tracker volume handling:
                # 1. New note without volume resets to default note volume (64)
                # 2. Instrument-only row without volume resets to instrument default volume (64) (trance gating)
                # 3. Explicit volume column sets the volume
                if note is not None and note < 120:
                    if vol is not None and 0 <= vol <= 64:
                        curr_raw_vol = vol
                    else:
                        curr_raw_vol = 64
                elif ins is not None and ins in sample_to_local_ins:
                    if vol is not None and 0 <= vol <= 64:
                        curr_raw_vol = vol
                    else:
                        curr_raw_vol = 64
                elif vol is not None and 0 <= vol <= 64:
                    curr_raw_vol = vol

                # Handle IT Effect D (Volume Slide)
                if eff_cmd == 4:
                    if eff_val != 0:
                        last_d = eff_val
                    else:
                        eff_val = last_d
                    if eff_val:
                        hi = (eff_val >> 4) & 0x0F
                        lo = eff_val & 0x0F
                        if lo == 0x0F and hi > 0:
                            curr_raw_vol = min(64, curr_raw_vol + hi) # Fine slide up
                        elif hi == 0x0F and lo > 0:
                            curr_raw_vol = max(0, curr_raw_vol - lo)  # Fine slide down
                        elif lo == 0 and hi > 0:
                            curr_raw_vol = min(64, curr_raw_vol + hi * max(1, speed - 1)) # Slide up
                        elif hi == 0 and lo > 0:
                            curr_raw_vol = max(0, curr_raw_vol - lo * max(1, speed - 1)) # Slide down

                vl, vr = calc_stereo_vol(curr_raw_vol, pan)
                if (vl, vr) != (curr_vl, curr_vr):
                    toks.append(f"    SEQ_SET_VOL({vl}, {vr}),")
                    curr_vl, curr_vr = vl, vr

                if note is not None:
                    if note < 120:
                        toks.append(f"    {note},")
                        curr_row = r + 1
                    elif note == 254:
                        toks.append("    SEQ_NOTE_CUT,")

            if curr_row < num_rows:
                wait_ticks = num_rows - curr_row - 1
                if wait_ticks == 0: toks.append("    SEQ_WAIT_0,")
                elif 1 <= wait_ticks <= 15: toks.append(f"    SEQ_WAIT_{wait_ticks},")
                else: toks.append(f"    SEQ_WAIT({wait_ticks}),")

            toks.append("    SEQ_RET,")
            pat_tokens[pidx] = toks

        def get_tok_len(tok_list):
            l = 0
            for t in tok_list:
                s = t.strip()
                if s.startswith("SEQ_WAIT(") or s.startswith("SEQ_SET_INS(") or s.startswith("SEQ_SET_DURATION(") or s.startswith("SEQ_PLAY_DRUM(") or s.startswith("SEQ_SET_LOOP(") or s.startswith("SEQ_SET_SPEED(") or s.startswith("SEQ_SET_TEMPO("):
                    l += 2
                elif s.startswith("SEQ_SET_VOL(") or s.startswith("SEQ_SET_ADSR(") or s.startswith("SEQ_CALL_SUB("):
                    l += 3
                else:
                    l += 1
            return l

        init_vl, init_vr = calc_stereo_vol(64, pan)
        init_tokens = [
            f"    SEQ_SET_INS({default_local_id}),",
            f"    SEQ_SET_VOL({init_vl}, {init_vr}),"
        ]
        init_len = get_tok_len(init_tokens)
        order_call_len = len(playable_orders) * 3 + 1
        order_script_len = init_len + order_call_len

        pat_offsets = {}
        curr_off = order_script_len
        for pidx in unique_patterns:
            pat_offsets[pidx] = curr_off
            curr_off += get_tok_len(pat_tokens[pidx])

        track_toks = []
        track_toks.append("    // --- Order Call Script ---")
        track_toks.extend(init_tokens)
        for pidx in playable_orders:
            track_toks.append(f"    SEQ_CALL_SUB({pat_offsets[pidx]}), // Pattern {pidx}")
        track_toks.append("    SEQ_RESTART,")
        track_toks.append("")

        for pidx in unique_patterns:
            track_toks.append(f"    // --- Pattern {pidx} Subroutine (Offset: {pat_offsets[pidx]}) ---")
            track_toks.extend(pat_tokens[pidx])
            track_toks.append("")

        track_code[ch] = track_toks

    # 5. Generate C Header
    header_guard = f"SEQ_{song_slug.upper()}_H"
    header_content = f"""#ifndef {header_guard}
#define {header_guard}

#include <stdint.h>
#include "consts.h"
#include "consts_snd.h"
#include "defs_structs.h"
#include "vars_extern_snd.h"

// {song_slug.capitalize()} Song Instrument Bank (Slots 0..{len(sample_info)-1})
const struct sample_list_entry_ins data_snd_instruments_{song_slug}[] = 
{{
"""
    for s_idx, local_id in sorted(sample_to_local_ins.items(), key=lambda x: x[1]):
        info = sample_info[s_idx]
        brr_len = info["brr_size"] - 2
        c5spd = info["c5spd"]
        is_loop = info["is_looped"]
        adsr = "0x0000"
        ticks = "0xffff" if is_loop else "0"
        header_content += f"    {{{local_id}, (void *)&data_snd_smp_{song_slug}_smp{s_idx}, {brr_len}, (({c5spd}l * 4096l) / 32000l), {adsr}, {ticks}, 60}},\n"

    header_content += """    {0, 0, 0, 0x1000, 0x0000, 0, 0},
};

"""
    for idx, ch in enumerate(active_channels):
        header_content += f"HUGE const uint8_t data_seq_{song_slug}_t{idx+1}[] = {{\n"
        header_content += "\n".join(track_code[ch]) + "\n"
        header_content += "};\n\n"

    header_content += f"#endif // {header_guard}\n"

    out_header_path = f"sound/seq/seq_{song_slug}.h"
    with open(out_header_path, "w") as f:
        f.write(header_content)

    print(f"Generated pattern-based {out_header_path} successfully ({len(active_channels)} tracks)!")
    return song_slug, sample_info, active_channels

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generic IT to SNES BRR/Sequence Converter")
    parser.add_argument("it_file", nargs="?", help="Path to .it file")
    parser.add_argument("--all", action="store_true", help="Convert all IT files in sound/seq/")
    parser.add_argument("--max-channels", type=int, default=8, help="Max active channels to emit")
    parser.add_argument("--no-merge-drums", action="store_true", help="Disable automatic drum channel merging")
    args = parser.parse_args()

    if args.all or not args.it_file:
        convert_it_song("sound/seq/Module1.compat.it", max_channels=5, prefix="module1", merge_drums=not args.no_merge_drums)
        convert_it_song("sound/seq/aryx.it", max_channels=8, prefix="aryx", merge_drums=not args.no_merge_drums)
    else:
        convert_it_song(args.it_file, max_channels=args.max_channels, merge_drums=not args.no_merge_drums)
