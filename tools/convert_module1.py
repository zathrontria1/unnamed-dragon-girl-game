import struct
import wave
import subprocess
import os

def extract_samples_and_generate_module1():
    it_path = "sound/seq/Module1.compat.it"
    with open(it_path, "rb") as f:
        data = f.read()

    ordnum, insnum, smpnum, patnum = struct.unpack_from("<HHHH", data, 32)
    offset = 64 + 64 + 64 + ordnum + insnum * 4
    smp_offsets = struct.unpack_from(f"<{smpnum}I", data, offset)
    offset += smpnum * 4
    pat_offsets = struct.unpack_from(f"<{patnum}I", data, offset)

    os.makedirs("sound/ins/itconv", exist_ok=True)

    # Samples: 1 (kick), 2 (hihat), 4 (snare), 5 (bass), 6 (lead)
    sample_defs = {
        1: ("ins_m1_kick", False, 0),
        2: ("ins_m1_hihat", False, 0),
        4: ("ins_m1_snare", False, 0),
        5: ("ins_m1_bass", True, 7),
        6: ("ins_m1_lead", True, 7),
    }

    sample_info = {}

    for s_idx, (s_name, is_looped, loop_start) in sample_defs.items():
        soff = smp_offsets[s_idx - 1]
        sdos = data[soff+4:soff+16].decode("latin1", errors="replace").rstrip("\x00")
        gvl, flags_s, vol, sname = struct.unpack_from("<BBB26s", data, soff + 16)
        length, loopbeg, loopend, c5spd, susloopbeg, susloopend, smpptr = struct.unpack_from("<IIIIIII", data, soff + 48)
        
        convert = data[soff + 45]
        is_signed = bool(convert & 1)

        raw_smp = data[smpptr : smpptr + length]
        if is_signed:
            pcm16 = bytearray()
            for b in raw_smp:
                val = struct.unpack("b", bytes([b]))[0]
                val16 = val * 256
                pcm16 += struct.pack("<h", val16)
        else:
            pcm16 = bytearray()
            for b in raw_smp:
                val16 = (b - 128) * 256
                pcm16 += struct.pack("<h", val16)

        # Pad to multiple of 16 samples
        n_samples = len(pcm16) // 2
        target_samples = ((n_samples + 15) // 16) * 16
        pad = target_samples - n_samples
        pcm16 += b"\x00\x00" * pad

        wav_path = f"sound/ins/itconv/{s_name}.wav"
        brr_path = f"sound/ins/itconv/{s_name}.brr"

        with wave.open(wav_path, "wb") as wf:
            wf.setnchannels(1)
            wf.setsampwidth(2)
            wf.setframerate(c5spd)
            wf.writeframes(pcm16)

        # Encode to BRR
        if is_looped:
            cmd = ["snesbrr", "-e", "-h", "-l", str(loop_start), wav_path, brr_path]
        else:
            cmd = ["wav2brr", "-o", brr_path, wav_path]
        
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
            print(f"Converter error: {res.stderr}")
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
            "loop_start": loop_start
        }
        print(f"Sample {s_idx} ({s_name}): C5Spd={c5spd}Hz, BRR Size={brr_size} bytes (with 2-byte header)")

    # 2. Local instrument mapping (0..4)
    # IT Sample 1 (Kick)  -> ID 0
    # IT Sample 2 (Hihat) -> ID 1
    # IT Sample 4 (Snare) -> ID 2
    # IT Sample 5 (Bass)  -> ID 3
    # IT Sample 6 (Lead)  -> ID 4
    sample_to_local_ins = {
        1: 0, # Kick
        2: 1, # Hihat
        4: 2, # Snare
        5: 3, # Bass
        6: 4  # Lead
    }

    # 3. Parse patterns according to order: [0, 0, 1, 1] for first 5 channels
    order_patterns = [0, 0, 1, 1]
    channel_rows = {ch: [] for ch in range(5)}

    current_global_row = 0
    for pidx in order_patterns:
        poff = pat_offsets[pidx]
        pat_len, num_rows = struct.unpack_from("<HH", data, poff)
        pdata = data[poff + 8 : poff + 8 + pat_len]
        
        ptr = 0
        row = 0
        mask_vars = [0] * 64
        last_note = [None] * 64
        last_ins = [None] * 64
        last_vol = [None] * 64

        pat_row_events = {r: {} for r in range(num_rows)}

        while row < num_rows and ptr < len(pdata):
            b = pdata[ptr]
            ptr += 1
            if b == 0:
                row += 1
                continue
            ch = (b - 1) & 63
            if b & 128:
                mask = pdata[ptr]; ptr += 1; mask_vars[ch] = mask
            else:
                mask = mask_vars[ch]

            note, ins, vol = None, None, None
            if mask & 1: note = pdata[ptr]; ptr += 1; last_note[ch] = note
            if mask & 2: ins = pdata[ptr]; ptr += 1; last_ins[ch] = ins
            if mask & 4: vol = pdata[ptr]; ptr += 1; last_vol[ch] = vol
            if mask & 8: ptr += 2

            if mask & 16: note = last_note[ch]
            if mask & 32: ins = last_ins[ch]
            if mask & 64: vol = last_vol[ch]

            if ch < 5:
                pat_row_events[row][ch] = (note, ins, vol)

        for r in range(num_rows):
            for ch in range(5):
                if ch in pat_row_events[r]:
                    n, i, v = pat_row_events[r][ch]
                    if n is not None or i is not None or v is not None:
                        channel_rows[ch].append((current_global_row + r, n, i, v))

        current_global_row += num_rows

    total_song_rows = current_global_row # 256
    print(f"Total Song Rows across order [0,0,1,1]: {total_song_rows}")

    # Generate sequence bytecode for each track
    track_code = {}

    for ch in range(5):
        events = channel_rows[ch]
        events = sorted(events, key=lambda x: x[0])
        
        tokens = []
        
        # Default instrument for each channel
        default_smp = None
        if ch == 0: default_smp = 1
        elif ch == 1: default_smp = 4
        elif ch == 2: default_smp = 2
        elif ch == 3: default_smp = 5
        elif ch == 4: default_smp = 6

        default_ins_id = sample_to_local_ins[default_smp]

        tokens.append(f"    SEQ_SET_INS({default_ins_id}),")
        tokens.append("    SEQ_SET_VOL(31, 31),")

        curr_row = 0
        curr_ins = default_ins_id
        curr_vol = 31

        for ev in events:
            r, note, ins, vol = ev
            if r < curr_row:
                continue

            # Need to wait until row r
            if r > curr_row:
                wait_ticks = r - curr_row - 1
                if wait_ticks == 0:
                    tokens.append("    SEQ_WAIT_0,")
                elif 1 <= wait_ticks <= 15:
                    tokens.append(f"    SEQ_WAIT_{wait_ticks},")
                else:
                    tokens.append(f"    SEQ_WAIT({wait_ticks}),")
                curr_row = r

            # At row r:
            # Change instrument if needed
            if ins is not None and ins in sample_to_local_ins:
                local_id = sample_to_local_ins[ins]
                if local_id != curr_ins:
                    tokens.append(f"    SEQ_SET_INS({local_id}),")
                    curr_ins = local_id

            # Change volume if needed
            if vol is not None:
                mapped_vol = (vol * 31) // 64
                if mapped_vol != curr_vol:
                    tokens.append(f"    SEQ_SET_VOL({mapped_vol}, {mapped_vol}),")
                    curr_vol = mapped_vol

            # Play note or note cut
            if note is not None:
                if note < 120:
                    tokens.append(f"    {note},")
                    curr_row = r + 1 # Note consumed row r
                elif note == 254:
                    tokens.append("    SEQ_NOTE_CUT,")

        # End of song: wait remaining rows until total_song_rows
        if curr_row < total_song_rows:
            wait_ticks = total_song_rows - curr_row - 1
            if wait_ticks == 0:
                tokens.append("    SEQ_WAIT_0,")
            elif 1 <= wait_ticks <= 15:
                tokens.append(f"    SEQ_WAIT_{wait_ticks},")
            else:
                tokens.append(f"    SEQ_WAIT({wait_ticks}),")

        tokens.append("    SEQ_RESTART,")
        track_code[ch] = tokens

    # Generate seq_module1.h
    header_content = """#ifndef SEQ_MODULE1_H
#define SEQ_MODULE1_H

#include <stdint.h>
#include "consts.h"
#include "consts_snd.h"
#include "defs_structs.h"
#include "vars_extern_snd.h"

// Module 1 Song Instrument Bank (Slots 0..4)
const struct sample_list_entry_ins data_snd_instruments_module1[] = 
{
    {0, (void *)&data_snd_smp_m1_kick, 540, ((9363l * 4096l) / 32000l), 0x0000, 0, 60},
    {1, (void *)&data_snd_smp_m1_hihat, 531, ((4555l * 4096l) / 32000l), 0x0000, 0, 60},
    {2, (void *)&data_snd_smp_m1_snare, 540, ((9363l * 4096l) / 32000l), 0x0000, 0, 60},
    {3, (void *)&data_snd_smp_m1_bass, 1530, ((44125l * 4096l) / 32000l), (0x0f | (0x03 << 4) | (0x01 << 7) | (0x13 << 8) | (0x07 << 13)), 0, 60},
    {4, (void *)&data_snd_smp_m1_lead, 1242, ((44125l * 4096l) / 32000l), (0x0f | (0x03 << 4) | (0x01 << 7) | (0x13 << 8) | (0x07 << 13)), 0, 60},
    {0, 0, 0, 0x1000, 0x0000, 0, 0},
};

"""
    for ch in range(5):
        header_content += f"HUGE const uint8_t data_seq_module1_t{ch+1}[] = {{\n"
        header_content += "\n".join(track_code[ch]) + "\n"
        header_content += "};\n\n"

    header_content += "#endif // SEQ_MODULE1_H\n"

    with open("sound/seq/seq_module1.h", "w") as f:
        f.write(header_content)

    print("Generated sound/seq/seq_module1.h with local instrument bank successfully!")

if __name__ == "__main__":
    extract_samples_and_generate_module1()
