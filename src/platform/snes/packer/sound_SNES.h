// OpenLara - SNES / Super FX target
// The level's sound effects, for the SPC700.
//
// TR1 ships every level with 50-70 seconds of 22 kHz 8-bit PCM, and the
// sound CPU has 64 KB of RAM for the driver, the sample directory and every
// sample it can play. The GBA reads its samples out of ROM as it mixes; the
// SPC700 cannot read the cartridge at all, so what it plays has to be chosen
// here and uploaded once, at boot, while the 65816 still owns the ROM bus.
//
// The choice: only sounds this level can actually fire -- the SOUND commands
// of its own animations and the handful the game calls by number -- ranked
// the ones the game calls by name first (her footsteps, landing, weapons,
// hit and scream), then by how many animations name each. Each is decimated
// by an integer factor (3 for the named set, 4 for the rest, worse if the
// budget forces it), cut at a second, and encoded to BRR: nine bytes per
// sixteen samples, the S-DSP's native format, decoded in hardware.
//
// Output, all in the level's data directory:
//
//   SOUND.BIN   the APU image uploaded to $0400: the source directory (128
//               entries of start/loop address), a pitch word per source (the
//               S-DSP plays 32 kHz at pitch $1000; a sample decimated to
//               11025 Hz wants $0583), a silent one-block source every unused
//               entry points at, and the BRR streams. main.asm uploads it
//               through the IPL protocol after src/spc.asm's driver.
//   SNDTAB.BIN  the host's tables, a fixed 640 bytes copied into WRAM at
//               boot: soundMap[256] (info slot or 255) and 96 four-byte
//               infos -- first source, volume >> 7, chance >> 7, and a flags
//               byte of mode | count << 2 | pitch << 6 | gain << 7. What
//               upstream's soundPlay reads, in the width a 16-bit CPU wants.
//   SOUND.json  the plan, for the runner's oracle and for the curious.
//
// OL_DUMP_SOUNDS=1 prints every decision and the encoder's own SNR.

#ifndef H_SOUND_SNES
#define H_SOUND_SNES

#include <vector>
#include <string>
#include <algorithm>

struct SoundSNES
{
    static const int32 APU_BASE     = 0x0400;   // where SOUND.BIN lands
    static const int32 DIR_ENTRIES  = 128;
    static const int32 PITCH_BASE   = 0x0600;
    static const int32 DATA_BASE    = 0x0700;
    static const int32 APU_END      = 0xFF00;   // the echo's four bytes sit here
    static const int32 INFO_MAX     = 96;
    static const int32 TABLE_BYTES  = 256 + INFO_MAX * 4;
    static const int32 SOURCE_RATE  = 22050;
    static const int32 DSP_RATE     = 32000;
    static const int32 CAP_MS       = 1000;     // the longest any sample keeps
    static const int32 COUNT_MAX    = 2;        // variants kept once the budget bites
    static const int32 FADE         = 64;       // samples of fade at a cut

    enum Priority { PRI_LARA = 0, PRI_LARA_ANIM, PRI_CREATURE, PRI_OTHER };

    struct Sample
    {
        int32 id, info, variant, source;    // TR sound id, info slot, variant, TR sample
        int32 priority;
        int32 div;                          // decimation: 2, 3, 4 or 6
        int32 frames;                       // after decimation and the cap
        std::vector<int16> pcm;             // 22050 Hz, 16-bit, the whole TR sample
        std::vector<uint8> brr;
        int32 addr;
        double snr;
    };

    struct Info
    {
        int32 id;           // TR sound id
        int32 slot;         // host info index, or -1 when dropped
        int32 first;        // first source number
        int32 priority;
        int32 refs;         // SOUND commands naming it: how often it is heard
        int32 div;
        int32 count;        // variants kept
        TR1_PC::SoundInfo tr;
        std::vector<int32> samples;         // indices into samples[]
        bool dropped;
    };

    std::vector<Sample> samples;
    std::vector<Info> infos;
    int32 imageBytes, retained, droppedCount;

    // FX3's guns: she holds one at a time, so their sounds share one region
    // of APU RAM the host refills when she changes gun (GUNSND.BIN, main.asm
    // SndGunUpload) -- the shot in the region's first source, the shotgun's
    // reload in the second. Upstream's ids keep their own infos (volume,
    // pitch, mode); only the samples under the two sources change. Weapon
    // order: pistols, magnums, uzis, shotgun.
    bool gunRegion = false;
    static const int32 GUN_RELOAD = 9;          // SND_SHOTGUN_RELOAD
    static const int32 GUN_DIV = 4;             // what sizes the region: the shotgun's two
    struct Gun { std::vector<uint8> shot, reload; double snr = 0; int32 div = GUN_DIV; };
    Gun guns[4];
    std::vector<Info> gunInfos;
    int32 gunShotSrc = -1, gunReloadSrc = -1, gunAddr = 0, gunSize = 0;

    // FX3's ring: upstream's inventory sounds (SND_INV_SPIN .. SND_INV_PAGE)
    // are heard only while the ring is open, when nothing in the level can
    // sound, so they take no room in the level's image: they ride in
    // GUNSND.BIN after the guns' records, and src/sound_ring.asm uploads
    // them over the last of the level's samples, just under the gun region
    // -- [ringAddr, gunAddr) -- as the ring opens, and puts SOUND.BIN's own
    // bytes back as it closes; the gun in her hands keeps its region. Their
    // own sources and infos, past the guns'. SND_HEALTH, the medipack, is
    // the game's (Lara::useItem), in the level's image with the named set.
    static const int32 RING_FIRST = 108, RING_LAST = 115;   // SND_INV_SPIN, SND_INV_PAGE
    static const int32 SND_HEALTH = 116;
    static const int32 RING_MAX = 8192;          // bytes: the IPL moves ~48 KB/s
    struct Ring { int32 id, div; std::vector<uint8> brr; double snr; TR1_PC::SoundInfo tr; int32 slot; };
    std::vector<Ring> ring;
    int32 ringFirst = -1, ringAddr = 0, ringBytes = 0;

    static int32 gunShotId(int32 gun)
    {
        static const int32 ids[4] = { 8, 44, 43, 45 };   // SND_PISTOLS/MAGNUMS/UZIS/SHOTGUN_SHOT
        return ids[gun];
    }
    bool isGunId(int32 id) const
    {
        if (!gunRegion) return false;
        if (id == GUN_RELOAD) return true;
        for (int32 g = 0; g < 4; g++) if (id == gunShotId(g)) return true;
        return false;
    }

    // One TR sound's first variant, at a gun's rate, as the plan encodes.
    bool gunSample(TR1_PC* level, int32 id, int32 div, std::vector<uint8>& brr, double* snr)
    {
        if (level->soundMap[id] < 0) return false;
        const TR1_PC::SoundInfo& tr = level->soundInfo[level->soundMap[id]];
        int32 src = tr.index;
        if (src < 0 || src >= level->soundOffsetsCount) return false;
        int32 offset = level->soundOffsets[src];
        int32 size = (src + 1 < level->soundOffsetsCount ? (int32)level->soundOffsets[src + 1]
                                                         : level->soundDataSize) - offset;
        Sample s;
        s.div = div;
        if (!readWav(level->soundData + offset, size, s.pcm)) return false;
        std::vector<int16> pcm;
        prepare(s, pcm);
        brrEncode(pcm, brr, snr);
        return true;
    }

    // --- TR's WAV blob to 16-bit PCM ----------------------------------------
    static bool readWav(const uint8* data, int32 size, std::vector<int16>& out)
    {
        if (size < 44 || memcmp(data, "RIFF", 4) != 0 || memcmp(data + 8, "WAVE", 4) != 0)
            return false;
        int32 channels = 1, rate = SOURCE_RATE, bits = 8;
        int32 pos = 12;
        while (pos + 8 <= size)
        {
            uint32 chunk;
            memcpy(&chunk, data + pos + 4, 4);
            if (memcmp(data + pos, "fmt ", 4) == 0 && chunk >= 16)
            {
                uint16 ch, bps; uint32 sr;
                memcpy(&ch, data + pos + 10, 2);
                memcpy(&sr, data + pos + 12, 4);
                memcpy(&bps, data + pos + 22, 2);
                channels = ch; rate = (int32)sr; bits = bps;
            }
            else if (memcmp(data + pos, "data", 4) == 0)
            {
                if (channels != 1 || rate != SOURCE_RATE || bits != 8) {
                    printf("sound: unsupported WAV %d ch %d Hz %d bit\n", channels, rate, bits);
                    return false;
                }
                int32 n = (int32)chunk;
                if (pos + 8 + n > size) n = size - pos - 8;
                out.resize(n);
                for (int32 i = 0; i < n; i++)
                    out[i] = (int16)(((int32)data[pos + 8 + i] - 128) << 7);
                return true;
            }
            pos += 8 + (int32)chunk + ((int32)chunk & 1);
        }
        return false;
    }

    // --- BRR -----------------------------------------------------------------
    // The S-DSP's own decoder, integer for integer, so the encoder's error
    // is measured against what the hardware will actually play back.
    static int32 brrPredict(int32 filter, int32 p1, int32 p2)
    {
        switch (filter) {
            case 1: return p1 + ((-p1) >> 4);
            case 2: return (p1 << 1) + ((-(p1 * 3)) >> 5) - p2 + (p2 >> 4);
            case 3: return (p1 << 1) + ((-(p1 * 13)) >> 6) - p2 + ((p2 * 3) >> 4);
        }
        return 0;
    }

    static int32 brrDecode(int32 nibble, int32 shift, int32 filter, int32 p1, int32 p2)
    {
        int32 s = shift > 12 ? (nibble < 0 ? -2048 : 0) : ((nibble << shift) >> 1);
        s += brrPredict(filter, p1, p2);
        if (s > 32767) s = 32767;
        if (s < -32768) s = -32768;
        return (int16)((int16)(s << 1) >> 1);
    }

    static void brrEncode(const std::vector<int16>& pcm, std::vector<uint8>& out, double* snr)
    {
        int32 blocks = ((int32)pcm.size() + 15) / 16;
        if (blocks == 0) blocks = 1;
        out.resize(blocks * 9);
        int32 p1 = 0, p2 = 0;
        double errSum = 0, sigSum = 0;

        for (int32 b = 0; b < blocks; b++)
        {
            int32 src[16];
            for (int32 i = 0; i < 16; i++) {
                int32 k = b * 16 + i;
                src[i] = k < (int32)pcm.size() ? pcm[k] : 0;
            }

            int64_t bestErr = -1;
            int32 bestShift = 0, bestFilter = 0, bestP1 = p1, bestP2 = p2;
            uint8 bestNib[16] = {0};

            for (int32 filter = 0; filter < (b == 0 ? 1 : 4); filter++)
            {
                for (int32 shift = 0; shift <= 12; shift++)
                {
                    int32 q1 = p1, q2 = p2;
                    int64_t err = 0;
                    uint8 nib[16];
                    for (int32 i = 0; i < 16; i++)
                    {
                        int32 pred = brrPredict(filter, q1, q2);
                        int32 diff = src[i] - pred;
                        // the DSP scales a nibble by 2^(shift-1)
                        int32 n;
                        if (shift == 0) n = diff << 1;   // decoded as n >> 1
                        else {
                            int32 half = 1 << (shift - 1);
                            n = diff >= 0 ? (diff + (half >> 1)) >> (shift - 1)
                                          : -((-diff + (half >> 1)) >> (shift - 1));
                        }
                        if (n > 7) n = 7;
                        if (n < -8) n = -8;
                        int32 d = brrDecode(n, shift, filter, q1, q2);
                        int64_t e = d - src[i];
                        err += e * e;
                        nib[i] = (uint8)(n & 15);
                        q2 = q1; q1 = d;
                        if (bestErr >= 0 && err > bestErr) break;
                    }
                    if (bestErr < 0 || err < bestErr) {
                        bestErr = err; bestShift = shift; bestFilter = filter;
                        bestP1 = q1; bestP2 = q2;
                        memcpy(bestNib, nib, 16);
                    }
                }
            }

            uint8* blk = &out[b * 9];
            blk[0] = (uint8)((bestShift << 4) | (bestFilter << 2) | (b == blocks - 1 ? 1 : 0));
            for (int32 i = 0; i < 8; i++)
                blk[1 + i] = (uint8)((bestNib[i * 2] << 4) | bestNib[i * 2 + 1]);
            p1 = bestP1; p2 = bestP2;
            errSum += (double)bestErr;
            for (int32 i = 0; i < 16; i++) sigSum += (double)src[i] * src[i];
        }
        if (snr) *snr = errSum > 0 ? 10.0 * log10(sigSum / errSum) : 99.0;
    }

    // --- decimate, cap, fade -------------------------------------------------
    static void prepare(const Sample& s, std::vector<int16>& out)
    {
        int32 rate = SOURCE_RATE / s.div;
        int32 n = (int32)s.pcm.size() / s.div;
        int32 cap = (int32)((int64_t)CAP_MS * rate / 1000);
        bool cut = n > cap;
        if (cut) n = cap;
        out.resize(n);
        for (int32 i = 0; i < n; i++) {
            int32 acc = 0;
            for (int32 k = 0; k < s.div; k++) acc += s.pcm[i * s.div + k];
            out[i] = (int16)(acc / s.div);
        }
        // Caution 8 of the manual: a sample has to end at zero or it clicks.
        int32 fade = FADE < n ? FADE : n;
        for (int32 i = 0; i < fade; i++)
            out[n - 1 - i] = (int16)((int32)out[n - 1 - i] * i / fade);
        (void)cut;
    }

    static int32 pitchFor(int32 div)
    {
        // 32 kHz plays at $1000; a decimated sample at 22050 / div.
        return (int32)(((int64_t)SOURCE_RATE * 4096 + (int64_t)DSP_RATE * div / 2)
                       / ((int64_t)DSP_RATE * div));
    }

    static int32 encodedBytes(int32 frames)
    {
        return ((frames + 15) / 16) * 9;
    }

    // --- the plan -----------------------------------------------------------
    bool excluded(int32 id) const
    {
        // The secret chime is four seconds of budget, the underwater loop
        // three -- the GBA drops those two as well (TR1_PC::cutData). The
        // inventory's are the ring's (FX3) or nobody's (GSU-2 has none).
        if (id == 60 || id == 173) return true;
        if (id == SND_HEALTH) return !gunRegion;
        if (id >= RING_FIRST && id <= SND_HEALTH) return true;
        return false;
    }

    void plan(TR1_PC* level, bool dump)
    {
        // Which model owns which animation: the model list is unordered, so a
        // model's animations run up to the next larger animIndex.
        std::vector<int32> animOwner(level->animsCount, -1);
        for (int32 m = 0; m < level->modelsCount; m++)
        {
            const TR1_PC::Model& model = level->models[m];
            if (model.animIndex == 0xFFFF) continue;
            int32 end = level->animsCount;
            for (int32 k = 0; k < level->modelsCount; k++) {
                int32 a = level->models[k].animIndex;
                if (a != 0xFFFF && a > model.animIndex && a < end) end = a;
            }
            for (int32 a = model.animIndex; a < end; a++) animOwner[a] = m;
        }

        // Every sound id the level can fire, and how much it matters.
        std::vector<int32> priorityOf(256, -1), refsOf(256, 0);
        auto want = [&](int32 id, int32 pri) {
            if (id < 0 || id >= 256 || excluded(id) || level->soundMap[id] < 0) return;
            if (isGunId(id)) return;                // the guns' region instead
            if (priorityOf[id] < 0 || pri < priorityOf[id]) priorityOf[id] = pri;
            refsOf[id]++;
        };
        // The game calls these by number: Hole::collide's refusal, the pistol
        // draw, holster and shot, a creature flinching, Lara hit, the scream;
        // her footsteps and landing are named here too, so that the sounds
        // heard every second of play rank with those.
        const int32 explicitIds[] = { 0, 2, 4, 6, 7, 8, 16, 20, 27, 30 };
        for (int32 id : explicitIds) want(id, PRI_LARA);
        if (gunRegion) {
            for (int32 i = 0; i < level->itemsCount; i++) {
                if (level->items[i].type == 12 || level->items[i].type == 13) {
                    want(85, PRI_LARA);         // Lion::hit (puma stays silent)
                    break;
                }
            }
        }
        want(SND_HEALTH, PRI_LARA);             // FX3: a medipack used (excluded otherwise)
        // TrapDartEmitter's shot, by number, where darts can fly: the dart
        // pool (out_SNES.h) is the sign, FX3 only.
        for (int32 i = 0; i < level->itemsCount; i++)
            if (level->items[i].type == 39) { want(151, PRI_LARA); break; }

        for (int32 a = 0; a < level->animsCount; a++)
        {
            const TR1_PC::Animation& anim = level->anims[a];
            int32 ci = anim.commandsStart;
            for (int32 n = 0; n < anim.commandsCount && ci < level->commandsCount; n++)
            {
                int32 op = level->commands[ci++];
                int32 args = (op == 1) ? 3 : (op == 2 || op == 5 || op == 6) ? 2 : 0;
                if (ci + args > level->commandsCount) break;
                if (op == 5)
                {
                    int32 id = level->commands[ci + 1] & 0x3FFF;
                    int32 owner = animOwner[a];
                    int32 type = owner < 0 ? -1 : (int32)level->models[owner].type;
                    int32 pri = PRI_OTHER;
                    if (type == ITEM_LARA || (type >= 1 && type <= 5)) pri = PRI_LARA_ANIM;
                    else if (type >= 7 && type <= 34) pri = PRI_CREATURE;
                    want(id, pri);
                }
                ci += args;
            }
        }

        // One info per id, its variants consecutive, as SI_COUNT addresses them.
        infos.clear();
        samples.clear();
        for (int32 id = 0; id < 256; id++)
        {
            if (priorityOf[id] < 0) continue;
            Info info;
            info.id = id;
            info.slot = -1;
            info.first = -1;
            info.priority = priorityOf[id];
            info.refs = refsOf[id];
            // the named set at /3, but the medipack's chime at /4: it is
            // heard once a use, and a second of it at /3 pushed out more
            info.div = info.priority == PRI_LARA && id != SND_HEALTH ? 3 : 4;
            info.tr = level->soundInfo[level->soundMap[id]];
            info.dropped = false;
            int32 count = info.tr.flags.count ? info.tr.flags.count : 1;
            info.count = count;
            for (int32 v = 0; v < count; v++)
            {
                int32 src = info.tr.index + v;
                if (src < 0 || src >= level->soundOffsetsCount) { info.dropped = true; break; }
                int32 offset = level->soundOffsets[src];
                int32 size = (src + 1 < level->soundOffsetsCount ? (int32)level->soundOffsets[src + 1]
                                                                 : level->soundDataSize) - offset;
                Sample s;
                s.id = id; s.info = (int32)infos.size(); s.variant = v; s.source = src;
                s.priority = info.priority;
                s.div = info.div;
                s.frames = 0; s.addr = 0; s.snr = 0;
                if (!readWav(level->soundData + offset, size, s.pcm)) { info.dropped = true; break; }
                info.samples.push_back((int32)samples.size());
                samples.push_back(s);
            }
            if (info.dropped) {
                printf("sound: id %d has an unreadable sample, dropped\n", id);
                for (int32 k : info.samples) samples[k].pcm.clear();
                info.samples.clear();
            }
            infos.push_back(info);
        }

        // The guns' region, set aside first: every gun at GUN_DIV sizes it,
        // then each gun takes the finest rate that still fits (the pistols'
        // and uzis' shots stay at the named set's /3).
        gunInfos.clear();
        gunSize = 0;
        if (gunRegion) {
            for (int32 g = 0; g < 4; g++) {
                guns[g] = Gun();
                gunSample(level, gunShotId(g), GUN_DIV, guns[g].shot, &guns[g].snr);
                if (g == 3) gunSample(level, GUN_RELOAD, GUN_DIV, guns[g].reload, NULL);
                gunSize = std::max(gunSize, (int32)(guns[g].shot.size() + guns[g].reload.size()));
            }
            for (int32 g = 0; g < 4; g++) {
                Gun fine;
                fine.div = 3;
                gunSample(level, gunShotId(g), fine.div, fine.shot, &fine.snr);
                if (g == 3) gunSample(level, GUN_RELOAD, fine.div, fine.reload, NULL);
                if ((int32)(fine.shot.size() + fine.reload.size()) <= gunSize) guns[g] = fine;
            }
            const int32 ids[5] = { 8, 44, 43, 45, GUN_RELOAD };
            for (int32 id : ids) {
                if (level->soundMap[id] < 0) continue;
                Info info;
                info.id = id; info.slot = -1; info.first = -1;
                info.priority = PRI_LARA; info.refs = 1; info.count = 1;
                info.div = guns[id == GUN_RELOAD ? 3 : id == 8 ? 0 : id == 44 ? 1 : id == 43 ? 2 : 3].div;
                info.tr = level->soundInfo[level->soundMap[id]];
                info.dropped = false;
                gunInfos.push_back(info);
            }
        }

        // Fit the budget. The order of sacrifice: variants beyond two of
        // every sound; then, least important first -- the sounds the game
        // calls by name last, otherwise by how few animations name it, then
        // by size -- a coarser rate; and only a sound already at the
        // coarsest rate is dropped.
        const int32 budget = APU_END - DATA_BASE - 9 - gunSize;
        auto bytesOf = [&](const Info& info) {
            int32 total = 0;
            for (int32 k : info.samples) {
                const Sample& s = samples[k];
                if (s.variant >= info.count) continue;
                int32 rate = SOURCE_RATE / info.div;
                int32 n = (int32)s.pcm.size() / info.div;
                int32 cap = (int32)((int64_t)CAP_MS * rate / 1000);
                if (n > cap) n = cap;
                total += encodedBytes(n);
            }
            return total;
        };
        auto totalBytes = [&]() {
            int32 total = 0;
            for (const Info& info : infos) if (!info.dropped) total += bytesOf(info);
            return total;
        };
        if (totalBytes() > budget) {
            for (Info& info : infos) if (info.count > COUNT_MAX) info.count = COUNT_MAX;
        }
        static const int32 divSteps[] = { 2, 3, 4, 6 };
        while (totalBytes() > budget)
        {
            int32 pick = -1, pickBytes = 0;
            for (int32 i = 0; i < (int32)infos.size(); i++)
            {
                const Info& info = infos[i];
                if (info.dropped) continue;
                int32 b = bytesOf(info);
                // The sounds the game calls by name are the last to suffer;
                // among the rest, what more animations name is heard more.
                int32 tier = info.priority == PRI_LARA ? 0 : 1;
                int32 pickTier = pick < 0 ? 0 : (infos[pick].priority == PRI_LARA ? 0 : 1);
                bool later = pick < 0 || tier > pickTier ||
                    (tier == pickTier &&
                     (info.refs < infos[pick].refs ||
                      (info.refs == infos[pick].refs && b > pickBytes)));
                if (later) { pick = i; pickBytes = b; }
            }
            if (pick < 0) break;
            Info& info = infos[pick];
            int32 step = 0;
            while (divSteps[step] < info.div) step++;
            if (step + 1 < 4) {
                info.div = divSteps[step + 1];
            } else {
                info.dropped = true;
            }
        }

        // Assign sources, encode, place.
        int32 source = 0, addr = DATA_BASE + 9;
        retained = droppedCount = 0;
        for (Info& info : infos)
        {
            if (info.dropped) { droppedCount++; continue; }
            if (source + info.count > DIR_ENTRIES) {
                info.dropped = true; droppedCount++;
                printf("sound: id %d exceeds the %d source entries, dropped\n", info.id, DIR_ENTRIES);
                continue;
            }
            info.first = source;
            for (int32 k : info.samples)
            {
                Sample& s = samples[k];
                if (s.variant >= info.count) continue;
                s.div = info.div;
                std::vector<int16> pcm;
                prepare(s, pcm);
                s.frames = (int32)pcm.size();
                brrEncode(pcm, s.brr, &s.snr);
                s.addr = addr;
                addr += (int32)s.brr.size();
                source++;
            }
            retained++;
        }
        if (gunRegion) {
            if (source + 2 > DIR_ENTRIES) { printf("sound: no sources left for the guns\n"); exit(1); }
            gunShotSrc = source++;
            gunReloadSrc = source++;
            gunAddr = addr;
            addr += gunSize;
            for (Info& info : gunInfos) info.first = info.id == GUN_RELOAD ? gunReloadSrc : gunShotSrc;
        }
        imageBytes = addr - APU_BASE;
        if (addr > APU_END) { printf("sound: image overflows APU RAM\n"); exit(1); }

        // The ring's: one source each, /4, the longest down to /6 until the
        // set fits RING_MAX; Lara's Home only where the level has its model.
        ring.clear();
        ringBytes = 0;
        if (gunRegion) {
            bool home = false;
            for (int32 m = 0; m < level->modelsCount; m++)
                if (level->models[m].type == ITEM_INV_HOME) home = true;
            for (int32 id = RING_FIRST; id <= RING_LAST; id++) {
                if (level->soundMap[id] < 0 || (id == RING_FIRST + 1 && !home)) continue;
                Ring r;
                r.id = id; r.div = 4; r.snr = 0; r.slot = -1;
                r.tr = level->soundInfo[level->soundMap[id]];
                if (!gunSample(level, id, r.div, r.brr, &r.snr)) continue;
                ring.push_back(r);
            }
            auto total = [&]() { int32 t = 0; for (const Ring& r : ring) t += (int32)r.brr.size(); return t; };
            while (total() > RING_MAX) {
                Ring* big = NULL;
                for (Ring& r : ring) if (r.div == 4 && (!big || r.brr.size() > big->brr.size())) big = &r;
                if (!big) { printf("sound: the ring's sounds exceed %d bytes at /6\n", RING_MAX); exit(1); }
                big->div = 6;
                gunSample(level, big->id, big->div, big->brr, &big->snr);
            }
            ringBytes = total();
            if (source + (int32)ring.size() > DIR_ENTRIES) { printf("sound: no sources left for the ring\n"); exit(1); }
            ringFirst = source;
            source += (int32)ring.size();
            ringAddr = gunAddr - ringBytes;
            if (ringAddr < DATA_BASE) { printf("sound: the ring does not fit APU RAM\n"); exit(1); }
        }

        int32 slot = 0;
        for (Info& info : infos) {
            if (info.dropped) continue;
            if (slot >= INFO_MAX) { printf("sound: more than %d infos\n", INFO_MAX); exit(1); }
            info.slot = slot++;
        }
        for (Info& info : gunInfos) {
            if (slot >= INFO_MAX) { printf("sound: more than %d infos\n", INFO_MAX); exit(1); }
            info.slot = slot++;
        }
        for (Ring& r : ring) {
            if (slot >= INFO_MAX) { printf("sound: more than %d infos\n", INFO_MAX); exit(1); }
            r.slot = slot++;
        }

        if (dump)
        {
            static const char* priNames[] = { "lara", "lara-anim", "creature", "other" };
            printf("sound plan: %d of %d ids kept, %d bytes of APU image, budget %d\n",
                   retained, (int32)infos.size(), imageBytes, budget);
            for (const Info& info : infos)
            {
                printf("  id %3d %-9s x%-2d vol %5d chance %5d mode %d count %d pitch %d gain %d: ",
                       info.id, priNames[info.priority], info.refs, info.tr.volume, info.tr.chance,
                       info.tr.flags.mode, info.tr.flags.count, info.tr.flags.pitch, info.tr.flags.gain);
                if (info.dropped) { printf("dropped\n"); continue; }
                printf("slot %d src %d /%d (%d Hz)", info.slot, info.first, info.div, SOURCE_RATE / info.div);
                for (int32 k : info.samples) {
                    const Sample& s = samples[k];
                    if (s.variant >= info.count) continue;
                    printf(" [%d: %.2fs of %.2fs, %d B at $%04X, snr %.1f dB]", s.source,
                           (double)s.frames * s.div / SOURCE_RATE,
                           (double)s.pcm.size() / SOURCE_RATE, (int32)s.brr.size(), s.addr, s.snr);
                }
                printf("\n");
            }
            if (gunRegion) {
                printf("  guns: sources %d/%d, %d B at $%04X:", gunShotSrc, gunReloadSrc, gunSize, gunAddr);
                for (int32 g = 0; g < 4; g++)
                    printf(" [/%d %d+%d B, snr %.1f dB]", guns[g].div, (int32)guns[g].shot.size(),
                           (int32)guns[g].reload.size(), guns[g].snr);
                printf("\n");
            }
            if (!ring.empty()) {
                printf("  ring: sources %d..%d, %d B at $%04X:", ringFirst, ringFirst + (int32)ring.size() - 1,
                       ringBytes, ringAddr);
                for (const Ring& r : ring)
                    printf(" [%d /%d %d B, snr %.1f dB]", r.id, r.div, (int32)r.brr.size(), r.snr);
                printf("\n");
            }
        }
    }

    void write(const char* dir, TR1_PC* level)
    {
        plan(level, getenv("OL_DUMP_SOUNDS") != NULL);

        // --- SOUND.BIN: the APU image from $0400 ---------------------------
        std::vector<uint8> image(imageBytes, 0);
        auto put16 = [&](int32 apuAddr, int32 v) {
            image[apuAddr - APU_BASE] = (uint8)v;
            image[apuAddr - APU_BASE + 1] = (uint8)(v >> 8);
        };
        for (int32 i = 0; i < DIR_ENTRIES; i++) {
            put16(APU_BASE + i * 4, DATA_BASE);         // the silent block
            put16(APU_BASE + i * 4 + 2, DATA_BASE);
            put16(PITCH_BASE + i * 2, 0x0400);
        }
        image[DATA_BASE - APU_BASE] = 0x01;             // END, filter 0, shift 0
        if (gunRegion) {
            // the pistols' shot, which she starts the level holding
            put16(APU_BASE + gunShotSrc * 4, gunAddr);
            put16(APU_BASE + gunShotSrc * 4 + 2, gunAddr);
            put16(PITCH_BASE + gunShotSrc * 2, pitchFor(guns[0].div));
            put16(PITCH_BASE + gunReloadSrc * 2, pitchFor(guns[0].div));
            if (!guns[0].shot.empty())
                memcpy(&image[gunAddr - APU_BASE], guns[0].shot.data(), guns[0].shot.size());
        }
        for (const Info& info : infos)
        {
            if (info.dropped) continue;
            for (int32 k : info.samples)
            {
                const Sample& s = samples[k];
                if (s.variant >= info.count) continue;
                int32 n = info.first + s.variant;
                put16(APU_BASE + n * 4, s.addr);
                put16(APU_BASE + n * 4 + 2, s.addr);
                put16(PITCH_BASE + n * 2, pitchFor(s.div));
                memcpy(&image[s.addr - APU_BASE], s.brr.data(), s.brr.size());
            }
        }

        char path[512];
        sprintf(path, "%s/SOUND.BIN", dir);
        FILE* f = fopen(path, "wb");
        if (!f) { printf("can't write %s\n", path); exit(1); }
        fwrite(image.data(), 1, image.size(), f);
        fclose(f);

        // --- SNDTAB.BIN: the host's soundMap and infos -----------------------
        std::vector<uint8> table(TABLE_BYTES, 0xFF);
        for (int32 i = 256; i < TABLE_BYTES; i++) table[i] = 0;
        for (const Info& info : infos)
        {
            if (info.dropped) continue;
            table[info.id] = (uint8)info.slot;
            uint8* rec = &table[256 + info.slot * 4];
            int32 count = info.count;
            rec[0] = (uint8)info.first;
            rec[1] = (uint8)(info.tr.volume >> 7);
            rec[2] = (uint8)(info.tr.chance >> 7);
            rec[3] = (uint8)(info.tr.flags.mode | (count << 2) |
                             (info.tr.flags.pitch << 6) | (info.tr.flags.gain << 7));
        }
        for (const Info& info : gunInfos)
        {
            table[info.id] = (uint8)info.slot;
            uint8* rec = &table[256 + info.slot * 4];
            rec[0] = (uint8)info.first;
            rec[1] = (uint8)(info.tr.volume >> 7);
            rec[2] = (uint8)(info.tr.chance >> 7);
            rec[3] = (uint8)(info.tr.flags.mode | (1 << 2) |
                             (info.tr.flags.pitch << 6) | (info.tr.flags.gain << 7));
        }

        for (const Ring& r : ring)
        {
            table[r.id] = (uint8)r.slot;
            uint8* rec = &table[256 + r.slot * 4];
            rec[0] = (uint8)(ringFirst + (&r - ring.data()));
            rec[1] = (uint8)(r.tr.volume >> 7);
            rec[2] = (uint8)(r.tr.chance >> 7);
            rec[3] = (uint8)(r.tr.flags.mode | (1 << 2) |
                             (r.tr.flags.pitch << 6) | (r.tr.flags.gain << 7));
        }

        // --- GUNSND.BIN (FX3): each gun's directory entries and samples ------
        //   u16 shot source, reload source, region address, region size;
        //   per gun (pistols, magnums, uzis, shotgun) 16 bytes: the two
        //   sources' directory entries (start, loop; start, loop) as the
        //   region holds that gun's sounds, their two pitch words, u16
        //   bytes, u16 file offset; then the ring's 12 bytes at 72 (u16 first
        //   source, count, APU address -- SOUND.BIN's own bytes from there
        //   are what it covers -- bytes, and the file offsets of its
        //   directory entries, then its pitch words, and of its samples),
        //   those, and then the samples, each gun's shot and reload
        //   together.
        if (gunRegion)
        {
            std::vector<uint8> gun(8 + 4 * 16 + 12, 0);
            auto set16 = [&](int32 at, int32 v) { gun[at] = (uint8)v; gun[at + 1] = (uint8)(v >> 8); };
            set16(0, gunShotSrc); set16(2, gunReloadSrc); set16(4, gunAddr); set16(6, gunSize);
            {
                const int32 n = (int32)ring.size();
                set16(72, ringFirst); set16(74, n); set16(76, ringAddr); set16(78, ringBytes);
                set16(80, (int32)gun.size());
                int32 at = ringAddr;
                for (const Ring& r : ring) {
                    const uint8 e[4] = { (uint8)at, (uint8)(at >> 8), (uint8)at, (uint8)(at >> 8) };
                    gun.insert(gun.end(), e, e + 4);
                    at += (int32)r.brr.size();
                }
                for (const Ring& r : ring) {
                    const int32 p = pitchFor(r.div);
                    gun.push_back((uint8)p); gun.push_back((uint8)(p >> 8));
                }
                set16(82, (int32)gun.size());
                for (const Ring& r : ring) gun.insert(gun.end(), r.brr.begin(), r.brr.end());
            }
            for (int32 g = 0; g < 4; g++)
            {
                const Gun& G = guns[g];
                const int32 rec = 8 + g * 16;
                const int32 shot = G.shot.empty() ? DATA_BASE : gunAddr;
                const int32 reload = G.reload.empty() ? DATA_BASE : gunAddr + (int32)G.shot.size();
                set16(rec + 0, shot); set16(rec + 2, shot);
                set16(rec + 4, reload); set16(rec + 6, reload);
                set16(rec + 8, pitchFor(G.div)); set16(rec + 10, pitchFor(G.div));
                set16(rec + 12, (int32)(G.shot.size() + G.reload.size()));
                set16(rec + 14, (int32)gun.size());
                gun.insert(gun.end(), G.shot.begin(), G.shot.end());
                gun.insert(gun.end(), G.reload.begin(), G.reload.end());
            }
            sprintf(path, "%s/GUNSND.BIN", dir);
            FILE* gf = fopen(path, "wb");
            if (!gf) { printf("can't write %s\n", path); exit(1); }
            fwrite(gun.data(), 1, gun.size(), gf);
            fclose(gf);
        }
        sprintf(path, "%s/SNDTAB.BIN", dir);
        f = fopen(path, "wb");
        if (!f) { printf("can't write %s\n", path); exit(1); }
        fwrite(table.data(), 1, table.size(), f);
        fclose(f);

        // --- SOUND.json ------------------------------------------------------
        sprintf(path, "%s/SOUND.json", dir);
        f = fopen(path, "w");
        if (!f) { printf("can't write %s\n", path); exit(1); }
        fprintf(f, "{\"apuBase\":%d,\"dataBase\":%d,\"imageBytes\":%d,\"tableBytes\":%d,"
                   "\"retained\":%d,\"dropped\":%d,", APU_BASE, DATA_BASE,
                imageBytes, TABLE_BYTES, retained, droppedCount);
        if (gunRegion)
            fprintf(f, "\n \"gun\":{\"shotSource\":%d,\"reloadSource\":%d,\"addr\":%d,\"size\":%d},",
                    gunShotSrc, gunReloadSrc, gunAddr, gunSize);
        if (!ring.empty()) {
            fprintf(f, "\n \"ring\":{\"first\":%d,\"addr\":%d,\"bytes\":%d,\"ids\":{",
                    ringFirst, ringAddr, ringBytes);
            for (const Ring& r : ring)
                fprintf(f, "%s\"%d\":{\"slot\":%d,\"source\":%d,\"div\":%d,\"bytes\":%d}",
                        &r == ring.data() ? "" : ",", r.id, r.slot, ringFirst + (int32)(&r - ring.data()),
                        r.div, (int32)r.brr.size());
            fprintf(f, "}},");
        }
        fprintf(f, "\n \"ids\":{");
        bool firstId = true;
        std::vector<Info> listed = infos;
        listed.insert(listed.end(), gunInfos.begin(), gunInfos.end());
        for (const Info& info : listed)
        {
            if (info.dropped) continue;
            int32 count = info.count;
            fprintf(f, "%s\n  \"%d\":{\"slot\":%d,\"first\":%d,\"count\":%d,\"div\":%d,"
                       "\"pitch\":%d,\"volume\":%d,\"chance\":%d,\"mode\":%d,"
                       "\"randomPitch\":%d,\"randomGain\":%d}",
                    firstId ? "" : ",", info.id, info.slot, info.first, count, info.div,
                    pitchFor(info.div), info.tr.volume, info.tr.chance, info.tr.flags.mode,
                    info.tr.flags.pitch, info.tr.flags.gain);
            firstId = false;
        }
        fprintf(f, "},\n \"sources\":[");
        bool firstSrc = true;
        for (const Info& info : infos)
        {
            if (info.dropped) continue;
            for (int32 k : info.samples)
            {
                const Sample& s = samples[k];
                if (s.variant >= info.count) continue;
                fprintf(f, "%s\n  {\"source\":%d,\"id\":%d,\"variant\":%d,\"addr\":%d,"
                           "\"bytes\":%d,\"frames\":%d,\"div\":%d,\"snr\":%.1f}",
                        firstSrc ? "" : ",", info.first + s.variant, s.id, s.variant,
                        s.addr, (int32)s.brr.size(), s.frames, s.div, s.snr);
                firstSrc = false;
            }
        }
        fprintf(f, "]}\n");
        fclose(f);

        printf("  SOUND.BIN: %d of %d sounds, %d KB of APU RAM; SNDTAB.BIN %d bytes%s\n",
               retained, (int32)infos.size(), (imageBytes + 1023) / 1024, TABLE_BYTES,
               gunRegion ? "; GUNSND.BIN" : "");
    }
};

#endif
