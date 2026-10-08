// OpenLara - SNES / Super FX target
// Level data conversion.
//
// The SNES frame buffer is 4bpp, so the whole level renders through sixteen
// colours. That is the hard constraint everything here bends around, and it is
// a much bigger cut than the GBA port's 256 with a 32-level lightmap.
//
// Shading is what makes those sixteen hurt. The GBA and 32X ports both index
// a 32 x 256 lightmap per pixel; four bits of frame buffer leaves no room for
// that. What the SNES does have is eight background palettes selected per
// 8x8 character by the tile map, so the shade term lands there: CG-RAM holds
// eight darkened copies of the sixteen colours, and the renderer picks a row
// per character. Per-pixel texel, per-character shade.
//
// The rows come from TR's own lightmap rather than a linear fade, so a
// darkened stone reads the same way it does on the other targets.
//
// Output, one file each so the linker can drop them at the right ROM banks:
//
//   PALETTE.BIN   8 x 16 x uint16, SNES BGR555 -- one row per shade level
//   TEXnn.BIN     256x256 texels: colours 0-15 or $80 (source index zero)
//   LEVEL.BIN     the room table, every room's geometry, and its portals
//
// A page is a whole 64 KB GSU bank rather than half of one. That matters: with
// the page filling the bank, a texel address is exactly (v << 8) | u, which is
// one MERGE in the inner loop. Half-pages would need an offset added per pixel
// and would also split TR tiles down the middle, dropping every face that
// straddled the seam.
//
// Room geometry format (little endian):
//
//   uint16 vertexCount
//   uint16 faceCount
//   struct { int16 x, y, z; uint16 g; } vertices[vertexCount]
//   faces[faceCount]:
//       uint8  page                      page + FACE_AKILL, or FACE_FLAT + colour
//       uint8  vertexCount               3 for triangles, 4 for quads
//       struct { uint16 index; uint8 u, v; } [vertexCount]
//
// Vertices keep TR's own frame: x and z relative to the room origin, y
// relative to the room's ceiling. The room table carries the offset that puts
// them back in the world, and the renderer folds it into the camera
// translation, which costs nothing per vertex. `g` is the shade index the GSU
// interpolates, 0 brightest, 31 darkest -- the same quantity as the GBA and
// 32X room vertex `g`, kept at its full 32 levels even though only the top
// three bits reach CG-RAM, because the interpolation happens before the
// quantisation and 8 levels of edge accumulator would band badly.
//
// The vertex index is sixteen bits because a TR room routinely has more than
// 256 vertices -- room 0 of LEVEL1 has 311 -- and a byte silently wrapped the
// tail of the list onto the head. It is written as the absolute game pak RAM
// address TV_BASE + index * TV_SIZE. Both hot face passes want that address,
// and making each reconstruct it cost an IWT and an ADD per reference.
//
// Faces are wound clockwise as seen from outside, which is what the backface
// test and the rasterizer's prev/next walk both expect.

#ifndef H_OUT_SNES
#include <stdlib.h>
#include "sound_SNES.h"
#include <map>
#include <algorithm>
#include <vector>

#define H_OUT_SNES

#define SNES_COLORS     16
#define SNES_SHADES     8               // background palettes, tile map bits 10-12
#define SNES_PAGE_W     256
#define SNES_PAGE_H     256
#define SNES_PAGE_SIZE  (SNES_PAGE_W * SNES_PAGE_H)

// The GSU interpolates g at 32 levels and CG-RAM holds 8, so the renderer
// takes g >> 2. Interpolation drifts by up to a couple of levels over a long
// edge (truncated reciprocals, and again for the span gradient), and the
// renderer masks rather than clamps -- one instruction instead of six in a
// per-character loop. Clamping the source to [3, 28] is what makes that safe:
// the drift can no longer wrap 0 round to 31, and since 0..3 all quantise to
// row 0 and 28..31 all to row 7, nothing visible is lost.
#define SNES_SHADE_MIN  3
#define SNES_SHADE_MAX  28

// gsu/mesh.gsu reserves this many words each for OT_NEXT and OT_OFF, and
// 12 KB at TV_BASE for one room's transformed vertices, 14 bytes each.
#define SNES_MAX_FACES  576
#define SNES_MAX_VERTS  768
#define SNES_TV_BASE    0xB000
// uint16 room, int16 normal[3], int16 vertices[4][3]
#define SNES_PORTAL_SIZE 32
// int8 floor, int8 ceiling, uint8 roomBelow, uint8 roomAbove, uint16 trigger
//
// The trigger is resolved here rather than shipped as TR's FloorData. That
// data is a bytecode -- portals, slants and triggers interleaved in one
// stream, ended by a bit -- and walking it needs a parser on a processor
// that has to be handed answers rather than questions. What the host needs
// to know is "standing here, which entities does that activate", so that is
// what the sector carries: an index into a table of exactly that, or $FFFF.
#define SNES_SECTOR_SIZE 6
#define SNES_TRIGGER_NONE 0xFFFF

// uint8 type, count, mask, once, timer, pad[3], items[8]
//
// The mask and the one-shot bit are TR's own and are the whole of how a
// trigger latches: an entity activates when every bit of its mask has been
// set, and a one-shot trigger can never set them again afterwards. The timer
// is how long it stays that way, in TR's seconds.
#define SNES_TRIGGER_ITEMS 8
#define SNES_TRIGGER_SIZE  16
#define SNES_TRIGGER_ACTION_END     1   // byte 5 of the record
#define SNES_TRIGGER_ACTION_SECRET  2   // ... with the secret's index in byte 6
#define SNES_TRIGGER_ACTION_CAMERA  4   // FX3: ACTIVATE_CAMERA, in CAMERA.BIN
#define SNES_TRIGGER_ACTION_TARGET  8   // FX3: CAMERA_TARGET, in CAMERA.BIN
#define SNES_TRIGGER_MAX   255   // host sectors reserve $FF for no trigger
#define SNES_SPRITE_MAX    16
// uint16 meshAddr, uint8 meshBank, angle, int16 x, y, z, uint8 g, pad
#define SNES_INST_SIZE   12
#define SNES_INST_NONE   0xFF    // a placement whose mesh had nothing to draw

// ----------------------------------------------------------------------
// Models
//
// Upstream keeps three tables -- models, nodes and frameData -- and walks
// them together: the model says where its nodes and its frame begin, the
// node says push/pop and a translate, the frame says a packed rotation per
// mesh. Three pointers advancing in step is cheap on a CPU with 32-bit
// registers and expensive on one without, so this target flattens them.
//
// A model here is one linear stream of fixed-size records, one per mesh,
// which is exactly what the GSU's ROM buffer wants: R14 walks forward and
// never chases a pointer. Frame 0's pose is baked in, since that is all
// there is until animation arrives.
//
//   uint16 meshAddr     patched after placement
//   uint8  meshBank     $FF = nothing drawable, skip the mesh
//   uint8  flags        bit 0 pop, bit 1 push -- upstream's NODE_FLAG_* --
//                       bit 2 root and bit 3 billboard
//   uint8  radius       conservative node radius / 32; zero disables the
//                       visibility test. It precedes the coordinates so the
//                       GSU reads flags and radius with one ROM word fetch.
//   int16  x, y, z      translate, in the parent's frame
//
// What varies between frames is not in there. A node's translate is static
// in TR -- only the root's comes from the frame -- so the pose is three
// angles per node, and keeping it separate is what makes animation fit:
// baking a whole node stream per frame costs 11 bytes a node where this
// costs 3, and LEVEL1 has enough frames for the difference to be the whole
// question. So a model is a header, then its nodes, then its frames:
//
//   uint8  nodeCount
//   uint8  pad
//   uint16 frameStride  6 + 3 * nodeCount, at most 255
//   uint16 animCount    animations the model owns; the GSU clamps the host's
//                       animation byte against it
//   uint16 animTable    within the bank, patched after placement: one record
//                       per animation saying where its frames start and how
//                       many there are
//
// Which frame to draw is the host's decision, arriving through GS_PHASE. The
// GSU clamps rather than trusts, because a frame index out of range would
// read a mesh address out of unrelated bytes.
//
// and a frame is
//
//   int16  x, y, z      the *root's* translate, which is the only one that
//                       animates
//   uint8  aX, aY, aZ   per node, rotation, 256 to the turn
//
// The angles are narrowed here rather than on the GSU. TR packs three
// 10-bit fields across a 32-bit word, which costs a dozen shifts to pull
// apart, and the sine table has 256 entries anyway -- so the bits below
// that resolution have nowhere to go. The rotation order is upstream's:
// Y, then X, then Z, applied after the translate.
#define SNES_NODE_SIZE   11
#define SNES_MODEL_HDR   8
// uint16 frameBase, uint8 bank, uint8 frameCount
#define SNES_ANIM_SIZE   4
#define SNES_NODE_ROOT   0x04
/* A billboard: the renderer drops the rotation out of the matrix for this
   node, so the quad faces the camera whatever the camera is doing. */
#define SNES_NODE_BILLBOARD 0x08
/* FX3: a sprite sequence. The node's mesh is a run of one-quad sprite meshes
   of SNES_SPRITE_MESH bytes each, and the frame's Y angle byte says which of
   them to draw (mesh.gsu BillboardNode). The effects model is the only user. */
#define SNES_NODE_SEQUENCE  0x10
#define SNES_SPRITE_MESH    54          // 4 header + 4 x 8 vertices + 2 + 4 x 4

/* Bit 7 identifies a flat-coloured polygon (colour in the low nibble).
   Textured polygons use bit 6 for colour-keying and bits 0-5 for the page.
   In texture data, bit 7 marks source index zero, separately from the colour
   nibble: nonzero source colours may also quantize to opaque black. */
#define SNES_FACE_FLAT   0x80
#define SNES_FACE_AKILL  0x40
#define SNES_TEX_HOLE    0x80

static uint8 snesTexturePage(uint16 tile, bool keyed)
{
    uint16 page = tile & 0x7FFF;
    if (page >= 64) {
        printf("SNES texture page %u overlaps face flags\n", page);
        exit(1);
    }
    return (uint8)(page | (keyed ? SNES_FACE_AKILL : 0));
}

// How many faces want the colour key (TR's TEX_ATTR_AKILL: texel 0 is a
// hole). A separate texel marker preserves opaque colours quantized to black.
static int32 g_akillFaces = 0;
static int32 g_facesSeen  = 0;

// uint16 streamAddr, uint8 bank, itemIndex, angle, g, int16 x, y, z
//
// itemIndex numbers the entities of a level from zero, and is what makes the
// host and the GSU agree about which entity is which without either having to
// ask: the host owns a byte per index in game pak RAM, the GSU reads the one
// this placement points at. Entities past SNES_ITEM_MAX get no slot and are
// dropped rather than sharing one.
#define SNES_ITEM_SIZE   12
#define SNES_ITEM_MAX    128

// ----------------------------------------------------------------------
// And the same entities again, in the shape the *host* needs them.
//
// The renderer's copy is per room and points at addresses; this one is a
// flat list of what exists, which is what game logic asks about. It cannot
// be the same table: the host has no way to read game pak ROM at all, so
// everything it will ever know about the level has to arrive through game
// pak RAM, and the GSU stages this blob there verbatim. A straight copy is
// all the co-processor has to do, and the host reads it once.
//
//   uint16 bytes        how much of it there is, so the GSU need not compute
//   uint16 itemCount
//   uint16 modelCount
//   uint16 animCount
//   uint16 transCount
//   uint16 triggerCount
//   uint16 cmdCount
//   uint16 boxCount
//   uint16 overlapCount
//   uint16 roomCount
//   uint16 itemBase     where the entity list starts, so nothing has to add
//                       up the tables in front of it to find it
//   uint16 sectorCount
//   uint16 linkCount
//   uint16 staticCount  placements that collide, see the end
//   per room:   uint8 sectorX, sectorZ, int16 x, z, uint16 sectorOffset,
//               uint8 xSectors, zSectors -- bit 7 of xSectors is TR's
//               water flag (ROOM_FLAG_WATER); a room is under twenty
//               sectors wide, so the count has the low seven bits
//   per sector: int8 floor, ceiling, uint16 link
//   per link:   uint8 roomBelow, roomAbove, portal, trigger,
//               uint16 floorSlants, ceilingSlants -- two signed bytes on FX3,
//               signed five-bit values on GSU-2, X low. Link 0 is "nothing that
//               way, flat".
//   per model:  uint16 firstAnim, uint16 animCount
//               int16 minX, maxX, minY, maxY, minZ, maxZ  (frame 0's box)
//   per anim box: int16 minX, maxX, minY, maxY, minZ, maxZ
//   per anim:   uint8 tickCount, frameRate       30 Hz duration and pose rate
//               uint16 nextAnim, nextFrame      TR's chain, already resolved
//               uint16 transStart, uint8 transCount, uint8 state
//               uint16 cmdStart, uint8 cmdCount, uint8 pad
//   per trans:  uint8 goalState, frameBegin, frameEnd, nextFrame
//               uint16 nextAnim
//   per cmd:    uint8 op, uint8 pad, int16 a0, a1, a2
//   per trigger:uint8 type, count, mask, once, timer, actions, secret, pad, items[8]
//   per box:    uint8 minZ, maxZ, minX, maxX  (sectors), int16 floor,
//               uint16 overlap
//   per overlap:uint16 box, the top bit ending a run
//   uint16 zoneGround[boxCount], uint16 zoneFly[boxCount]
//   per item:   uint8 type, uint8 room, uint8 model, uint8 pad
//               int16 x, y, z
//               uint16 angle, uint16 trigger, uint8 sectorX, uint8 sectorZ
//   per box, overlap and zone: the navigation graph, see below
//   per static: uint8 room, pad, int16 x, y, z,
//               int16 minX, maxX, minY, maxY, minZ, maxZ
//               the placements that collide, in room order, the box already
//               turned to face the way the mesh does, for Room::collideStatic
//   uint16 staticFirst[roomCount + 1]   where each room's run starts
//   aim table: uint16 animCount, uint16 animOffset[animCount] (0 = union box)
//               each nonzero offset names uint16 tickCount, then one uint16
//               pointOffset per tick. Points are int16 x,y,z, shared across
//               animations. All offsets are relative to this table's start.
//   navigation map: byte 'N', literalBits (1..10), 2-bit sector tags
//               (none, previous Z, previous X, literal), uint16 literal
//               ranks per 32 sectors plus a terminal rank, packed box IDs.
//   uint16 navBase  final word: offset of the navigation map in this blob
//
// Positions are world coordinates here rather than room-relative: the
// renderer wants them in the room's frame because that is the frame it
// draws in, and the host wants them in the world because that is where it
// thinks. Converting once, here, is cheaper than either side doing it.
#define SNES_TV_SIZE     14     // the GSU's transformed vertex record; TV_SIZE in gsu/mesh.gsu
#define SNES_HOST_HEADER 28
#define SNES_HOST_STATIC 20
#define SNES_HOST_ITEM   16
#define SNES_HOST_MODEL  16
#define SNES_HOST_ROOM   10
// Everything about a sector that is not its two heights -- the rooms it
// links to, its trigger, its slants -- repeats: LEVEL1's 4,026 sectors carry
// 583 distinct combinations. So a sector is its heights and a 16-bit index
// into a table of those, four bytes, instead of the eight and a half it took
// to spell them out per sector. That is what put the blob 13 KB under the
// bank it has to fit in, with room to add to it.
#define SNES_HOST_SECTOR 4
#define SNES_HOST_LINK   8
// uint16 firstFrame, frameCount, nextAnim, nextFrame,
// uint16 transStart, uint8 transCount, uint8 state,
// uint16 cmdStart, uint8 cmdCount, uint8 pad
#define SNES_HOST_ANIM   14
// uint8 goalState, frameBegin, frameEnd, nextFrame, uint16 nextAnim
#define SNES_TRANS_SIZE  6
// uint8 op, uint8 pad, int16 a0, a1, a2 -- fixed, so walking needs no table
#define SNES_CMD_SIZE    8

struct out_SNES
{
    uint8  palIndex[256];               // TR palette entry -> SNES palette slot
    uint8  palRep[SNES_COLORS];          // and a TR entry back, for the lightmap
    uint16 palette[SNES_SHADES * SNES_COLORS];
    int32  paletteCount;
    uint16 hudPal[2][16];               // HUD.BIN's, CG-RAM 128-159 (writeTitle)
    const char* trDir = NULL;           // the TR1 data directory: TITLEH.PCX

    // ------------------------------------------------------------------
    // Palette
    //
    // TR1's 256-colour palette is quantised down to sixteen by clustering in
    // RGB. libimagequant is in the tree and would do better, but it wants an
    // image rather than a palette; the textures are the image that matters, so
    // colours are weighted by how much texture area actually uses them.
    // ------------------------------------------------------------------

    // HUD.BIN: the health and air bars as sprites (README "HUD").
    //
    //   uint16 palettes[2][16]   BGR555, sprite palettes 0 (health), 1 (air)
    //   uint8  tiles[23][32]     4bpp SNES characters
    //
    // Upstream's renderBar (gba/render.iwram.cpp) with TR palette indices:
    // 102 x 7, a border of 19 (top, left) and 17 (bottom, right), five rows
    // of the bar's colour, 27 where it is empty. On screen it is thirteen
    // 8x8 sprites; one character per cell shape and fill count, the two
    // bars the same characters under different palettes:
    //
    //   0-7    the first cell: border column, then fill columns 0-6, n filled
    //   8-16   a middle cell: eight fill columns, n = 0..8 filled
    //   17-22  the last cell: fill columns 95-99, n = 0..5, border, 2 clear
    //
    // Pixel values: 1-5 the bar's rows, 6 empty, 7 border 19, 8 border 17.
    // The exact TR colours, not the quantised level palette: sprites have
    // CG-RAM 128-255 to themselves (FX3 keeps colour 0 of every row black,
    // which the frame's $80 transparency marker lands on; sprites never
    // show colour 0).
    // Upstream's dart emitter takes a dart from ItemObj's free list at every
    // loop of its FIRE animation. Here every entity exists from the load --
    // the host's list and the renderer's slot table are this packer's -- so
    // the level gains two darts per emitter, hidden (TR's bit 8, "invisible
    // until triggered", which nothing triggers) in the emitter's room. A dart
    // lives up to 32 ticks in LEVEL1's room 6 against a 30-tick FIRE, so two
    // can be in flight from one emitter; below 60 ticks two each is never
    // short where upstream's list would not be. src/trap.c flies them; GSU-2
    // has no room for that code, so its levels get no pool.
    void appendDartPool(TR1_PC* level)
    {
        const int32 TYPE_DART = 39, TYPE_EMITTER = 40;
        int32 emitters = 0;
        const int32 PER_EMITTER = 2;
        for (int32 i = 0; i < level->itemsCount; i++)
            if (level->items[i].type == TYPE_EMITTER) emitters++;
        if (!emitters) return;

        TR1_PC::Item* items = new TR1_PC::Item[level->itemsCount + emitters * PER_EMITTER];
        memcpy(items, level->items, level->itemsCount * sizeof(TR1_PC::Item));
        int32 n = level->itemsCount;
        for (int32 i = 0; i < level->itemsCount; i++)
        {
            const TR1_PC::Item& e = level->items[i];
            if (e.type != TYPE_EMITTER) continue;
            for (int32 k = 0; k < PER_EMITTER; k++) {
                TR1_PC::Item& d = items[n++];
                d = e;
                d.type = TYPE_DART;
                d.intensity = -1;
                d.flags = 0x0100;
            }
        }
        level->items = items;
        level->itemsCount = n;
        printf("  dart pool: %d darts for %d emitters\n", emitters * PER_EMITTER, emitters);
    }

    // Upstream's effects -- blood, ricochet, splash, bubble, smoke -- are
    // items ItemObj::add takes from the free list and SpriteEffect/Bubble
    // update until their sprite sequence ends. As with the darts, every
    // entity here exists from the load, so the level gains FX_POOL hidden
    // ones after everything else, of the blood's type (the host retypes one
    // as it takes it); all five types draw with the one effects model
    // (buildFxModel). Upstream's pool is MAX_ITEMS (256) less the level; a
    // bat's attack bleeds every tick for twelve-tick sprites, so 24 is what
    // two bats and both pistols want at once. src/fx.c flies them; GSU-2
    // has no room for it.
    static int32 fxType(int32 kind)
    {
        static const int32 types[FX_KINDS] = { 153, 155, 158, 160, 164 };
        return types[kind];
    }

    void appendFxPool(TR1_PC* level)
    {
        fxPoolItems = 0;
        bool any = false;
        for (int32 j = 0; j < level->spriteSequencesCount; j++)
            any |= isFxType(level->spriteSequences[j].type);
        int32 lara = -1;
        for (int32 i = 0; i < level->itemsCount && lara < 0; i++)
            if (level->items[i].type == ITEM_LARA) lara = i;
        if (!any || lara < 0) return;

        TR1_PC::Item* items = new TR1_PC::Item[level->itemsCount + FX_POOL];
        memcpy(items, level->items, level->itemsCount * sizeof(TR1_PC::Item));
        for (int32 k = 0; k < FX_POOL; k++) {
            TR1_PC::Item& e = items[level->itemsCount + k];
            e = level->items[lara];             // her room and place, until used
            e.type = 158;
            e.intensity = -1;
            e.flags = 0x0100;                   // hidden, as the darts
        }
        level->items = items;
        level->itemsCount += FX_POOL;
        fxPoolItems = FX_POOL;
        printf("  effects pool: %d entities\n", FX_POOL);
    }

    // The font (FX3): TR's glyph sprites, the GLYPHS sequence (type 190), for
    // the GSU's text (gsu/mesh.gsu TextRender, src/inventory_text.c). The
    // pixels go into LEVEL.BIN, where the GSU reads them -- a byte a pixel,
    // the colour in the low nibble and 0 where the sprite is clear, each
    // glyph trimmed to what it covers -- and GETC plots them under a frozen
    // high nibble, in CG-RAM 160-175 (sprite palette 2), which the 8bpp
    // frame reaches. The colours: every index the glyphs use, fitted to 15
    // by weighted k-means in TR's own RGB.
    //
    // With them, the inventory's background (upstream's copyBackground):
    // palGrayRemap's eight greys -- TR indices 1, 22 .. 17, 33 -- for CG-RAM
    // 176-183, and its remap from each frame pixel value (PALETTE.BIN's
    // colours; anything past them black) to one of them by luminance, 256
    // bytes after the glyphs, which the GSU reads (mesh.gsu BgCapture).
    // FONT.BIN, for the CPU:
    //
    //   uint16 palette[16]   BGR555; entry 0 unused   (CG-RAM 160-175)
    //   uint16 grey[8]       BGR555                   (CG-RAM 176-183)
    //   uint16 lines[4]      TR 14, 10, 15, 0: the options' borders (184-187)
    //   uint8  level         upstream's LevelID: key and puzzle names
    //   uint8  count         glyphs
    //   uint8  bank          the pixels' GSU bank, relative to the level's
    //   uint8  0
    //   uint16 remap         the grey remap's address in that bank
    //   uint16 dark          and renderFill's darkening's (shade 25)
    //   per glyph, 6 bytes:  uint16 address, uint8 w, uint8 h, int8 x, int8 y
    //                        -- the trimmed corner from the pen, as l and t
    //   uint16 bar[4]        BGR555, TR 41-44: renderBar's BAR_OPTION, for
    //                        the settings pages (CG-RAM 188-191)
    enum { FONT_GLYPHS = 64 };
    std::vector<uint8> fontPixels;
    std::vector<uint8> fontInfo;
    std::vector<int32> fontOffset;
    int32 fontRemap, fontDark;

    void buildFont(TR1_PC* level)
    {
        fontPixels.clear(); fontInfo.clear(); fontOffset.clear();
        int32 seq = -1;
        for (int32 j = 0; j < level->spriteSequencesCount; j++)
            if (level->spriteSequences[j].type == 190) seq = j;
        if (seq < 0) return;
        const int32 count = -level->spriteSequences[seq].count;
        const int32 start = level->spriteSequences[seq].start;
        if (count <= 0 || count > 255 || start + count > level->spriteTexturesCount) return;

        double weight[256] = { 0 };
        for (int32 g = 0; g < count; g++) {
            const TR1_PC::SpriteTexture& t = level->spriteTextures[start + g];
            const int32 w = (t.w + 255) >> 8, h = (t.h + 255) >> 8;
            for (int32 y = 0; y < h; y++)
                for (int32 x = 0; x < w; x++) {
                    const uint8 c = level->tiles[t.tile].indices[((t.v + y) & 255) * 256 + ((t.u + x) & 255)];
                    if (c) weight[c] += 1.0;
                }
        }

        // weighted k-means: the heaviest colour first, then each next centre
        // the one farthest (by weight x distance squared) from those chosen
        double cr[15], cg[15], cb[15];
        int32 k = 0;
        const uint8* pal = level->palette.colors;
        #define DIST(i, r, g, b) ((pal[(i)*3]-(r))*(pal[(i)*3]-(r)) + (pal[(i)*3+1]-(g))*(pal[(i)*3+1]-(g)) + (pal[(i)*3+2]-(b))*(pal[(i)*3+2]-(b)))
        while (k < 15) {
            int32 best = -1; double bestScore = 0;
            for (int32 i = 1; i < 256; i++) {
                if (weight[i] <= 0) continue;
                double d = 1e9;
                for (int32 j = 0; j < k; j++) d = std::min(d, (double)DIST(i, cr[j], cg[j], cb[j]));
                const double s = weight[i] * (k ? d : 1.0);
                if (s > bestScore) { bestScore = s; best = i; }
            }
            if (best < 0) break;
            cr[k] = pal[best * 3]; cg[k] = pal[best * 3 + 1]; cb[k] = pal[best * 3 + 2];
            k++;
        }
        uint8 map[256] = { 0 };
        for (int32 iter = 0; iter < 32; iter++) {
            double sr[15] = { 0 }, sg[15] = { 0 }, sb[15] = { 0 }, sw[15] = { 0 };
            for (int32 i = 1; i < 256; i++) {
                if (weight[i] <= 0) continue;
                int32 bj = 0; double bd = 1e18;
                for (int32 j = 0; j < k; j++) {
                    const double d = DIST(i, cr[j], cg[j], cb[j]);
                    if (d < bd) { bd = d; bj = j; }
                }
                map[i] = uint8(bj + 1);
                sr[bj] += weight[i] * pal[i * 3]; sg[bj] += weight[i] * pal[i * 3 + 1];
                sb[bj] += weight[i] * pal[i * 3 + 2]; sw[bj] += weight[i];
            }
            for (int32 j = 0; j < k; j++)
                if (sw[j] > 0) { cr[j] = sr[j] / sw[j]; cg[j] = sg[j] / sw[j]; cb[j] = sb[j] / sw[j]; }
        }
        #undef DIST

        fontInfo.assign(FONT_GLYPHS + count * 6, 0);
        for (int32 j = 0; j < k; j++) {
            const int32 r = int32(cr[j] + 0.5) >> 1, g = int32(cg[j] + 0.5) >> 1, b = int32(cb[j] + 0.5) >> 1;
            const uint16 c = uint16(r | (g << 5) | (b << 10));
            fontInfo[2 + j * 2] = uint8(c); fontInfo[3 + j * 2] = uint8(c >> 8);
        }
        static const uint8 grad[8] = { 1, 22, 21, 20, 19, 18, 17, 33 };
        for (int32 g = 0; g < 8; g++) {
            const uint8* c = level->palette.colors + grad[g] * 3;
            const uint16 v = uint16((c[0] >> 1) | ((c[1] >> 1) << 5) | ((c[2] >> 1) << 10));
            fontInfo[32 + g * 2] = uint8(v); fontInfo[33 + g * 2] = uint8(v >> 8);
        }
        static const uint8 lines[3] = { 14, 10, 15 };
        for (int32 g = 0; g < 3; g++) {
            const uint8* c = level->palette.colors + lines[g] * 3;
            const uint16 v = uint16((c[0] >> 1) | ((c[1] >> 1) << 5) | ((c[2] >> 1) << 10));
            fontInfo[48 + g * 2] = uint8(v); fontInfo[49 + g * 2] = uint8(v >> 8);
        }
        fontInfo[56] = uint8(level->id);
        fontInfo[57] = uint8(count);

        for (int32 g = 0; g < count; g++) {
            const TR1_PC::SpriteTexture& t = level->spriteTextures[start + g];
            const int32 w = (t.w + 255) >> 8, h = (t.h + 255) >> 8;
            const uint8* src = level->tiles[t.tile].indices;
            #define PIX(x, y) src[((t.v + (y)) & 255) * 256 + ((t.u + (x)) & 255)]
            int32 x0 = w, y0 = h, x1 = -1, y1 = -1;
            for (int32 y = 0; y < h; y++)
                for (int32 x = 0; x < w; x++)
                    if (PIX(x, y)) {
                        x0 = std::min(x0, x); x1 = std::max(x1, x);
                        y0 = std::min(y0, y); y1 = std::max(y1, y);
                    }
            uint8* r = &fontInfo[FONT_GLYPHS + g * 6];
            fontOffset.push_back(int32(fontPixels.size()));
            if (x1 < 0) continue;                       // nothing drawn: w = h = 0
            r[2] = uint8(x1 - x0 + 1);
            r[3] = uint8(y1 - y0 + 1);
            r[4] = uint8(int8(t.l + x0));
            r[5] = uint8(int8(t.t + y0));
            for (int32 y = y0; y <= y1; y++)
                for (int32 x = x0; x <= x1; x++)
                    fontPixels.push_back(map[PIX(x, y)]);
            #undef PIX
        }
        // palGrayRemap: lum = (r 77 + g 150 + b 29) >> 10 on five bits
        fontRemap = int32(fontPixels.size());
        for (int32 v = 0; v < 256; v++) {
            int32 lum = 0;
            if (v < SNES_SHADES * SNES_COLORS) {
                const uint16 c = palette[v];
                lum = ((c & 31) * 77 + ((c >> 5) & 31) * 150 + ((c >> 10) & 31) * 29) >> 10;
            }
            fontPixels.push_back(uint8(0xB0 + lum));
        }
        // renderFill's shade 25 over the options: a shaded texel five rows
        // darker, a grey a quarter as bright, anything else black
        fontDark = int32(fontPixels.size());
        for (int32 v = 0; v < 256; v++) {
            uint8 d = 0x80;
            if (v < SNES_SHADES * SNES_COLORS)
                d = uint8((std::min(SNES_SHADES - 1, (v >> 4) + 5) << 4) | (v & 15));
            else if (v >= 0xB0 && v < 0xB8)
                d = uint8(0xB0 + ((v - 0xB0) >> 2));
            fontPixels.push_back(d);
        }
        for (int32 c = 41; c <= 44; c++) {
            const uint8* rgb = level->palette.colors + c * 3;
            const uint16 v = uint16((rgb[0] >> 1) | ((rgb[1] >> 1) << 5) | ((rgb[2] >> 1) << 10));
            fontInfo.push_back(uint8(v));
            fontInfo.push_back(uint8(v >> 8));
        }
        printf("  font: %d glyphs, %d colours, %d bytes of pixels and remap\n", count, k, int32(fontPixels.size()));
    }

    void writeHUD(const char* dir, TR1_PC* level)
    {
        static const uint8 barColors[2][5] = {
            {  8, 11,  8,  6, 24 },     // BAR_HEALTH
            { 32, 41, 32, 19, 21 },     // BAR_OXYGEN
        };

        uint16 pal[2][16];
        memset(pal, 0, sizeof(pal));
        for (int32 p = 0; p < 2; p++)
        {
            uint8 idx[9] = { 0, barColors[p][0], barColors[p][1], barColors[p][2],
                             barColors[p][3], barColors[p][4], 27, 19, 17 };
            for (int32 k = 1; k < 9; k++)
            {
                int32 r = level->palette.colors[idx[k] * 3 + 0] >> 1;
                int32 g = level->palette.colors[idx[k] * 3 + 1] >> 1;
                int32 b = level->palette.colors[idx[k] * 3 + 2] >> 1;
                pal[p][k] = (uint16)(r | (g << 5) | (b << 10));
            }
        }

        uint8 tiles[23][32];
        memset(tiles, 0, sizeof(tiles));
        for (int32 t = 0; t < 23; t++)
        {
            // Each cell's pixel x as a fill column relative to the cell: the
            // first cell's border sits at -1, the last cell's at 5, and the
            // last cell's columns 6-7 are clear.
            int32 shift = (t < 8) ? 1 : 0;
            int32 n = (t < 8) ? t : (t < 17 ? t - 8 : t - 17);

            for (int32 y = 0; y < 8; y++)
            {
                for (int32 x = 0; x < 8; x++)
                {
                    int32 col = x - shift;
                    int32 c;
                    if (y == 7 || (t >= 17 && col > 5)) c = 0;
                    else if (col == -1) c = 7;
                    else if (t >= 17 && col == 5) c = 8;
                    else if (y == 0) c = 7;
                    else if (y == 6) c = 8;
                    else c = (col < n) ? y : 6;

                    // 4bpp character: planes 0/1 interleaved in rows 0-15,
                    // planes 2/3 in rows 16-31.
                    uint8 bit = (uint8)(0x80 >> x);
                    if (c & 1) tiles[t][y * 2 + 0]  |= bit;
                    if (c & 2) tiles[t][y * 2 + 1]  |= bit;
                    if (c & 4) tiles[t][y * 2 + 16] |= bit;
                    if (c & 8) tiles[t][y * 2 + 17] |= bit;
                }
            }
        }

        char path[512];
        sprintf(path, "%s/HUD.BIN", dir);
        FILE* f = fopen(path, "wb");
        if (!f) {
            printf("can't write \"%s\"\n", path);
            return;
        }
        fwrite(pal, 2, 32, f);
        fwrite(tiles, 1, sizeof(tiles), f);
        fclose(f);
        memcpy(hudPal, pal, sizeof(hudPal));
        printf("  HUD.BIN: 2 bar palettes, 23 characters\n");
    }

    // TITLE.BIN (FX3): upstream's title screen -- TITLE.SCR, which the GBA
    // packer's convertScreen makes of a 240 x 160 screens/TITLE.bmp -- from
    // the DOS release's own TITLEH.PCX: its rows 16-447 (the black above the
    // logo and the copyright line go, and what is left is within a sixth of
    // the proportions 216 x 144 pixels of 8:7 show), box-filtered to the FX3
    // frame. A pixel is a CG-RAM index as the title boot leaves CG-RAM: the
    // level's 128 shade colours, the HUD's, the font's, the greys and the
    // borders as they are (1-191; 0 is the backdrop), and 64 colours of the
    // picture's own, fitted to what those miss by k-means, at 192-255,
    // which nothing else uses. main.asm puts them in BG_BUF and CG-RAM at a
    // title boot, and the ring is drawn over them (mesh.gsu IR_Grey).
    //
    //   uint16 palette[64]          BGR555, CG-RAM 192-255
    //   uint8  pixels[216 * 144]    CG-RAM indices, never 0
    enum { TITLE_W = 216, TITLE_H = 144, TITLE_OWN = 64, TITLE_FIRST = 0xC0,
           TITLE_SRC_Y = 16, TITLE_SRC_H = 432 };

    static bool loadPCX(const char* path, int32& w, int32& h, std::vector<uint8>& rgb)
    {
        FILE* f = fopen(path, "rb");
        if (!f) return false;
        fseek(f, 0, SEEK_END);
        const long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<uint8> d(size > 0 ? size : 0);
        const bool read = size > 0 && fread(d.data(), 1, size, f) == size_t(size);
        fclose(f);
        // ZSoft: 8 bits, one plane, run-length; the palette in the last 769
        if (!read || size < 128 + 769 || d[0] != 0x0A || d[2] != 1 || d[3] != 8 ||
            d[65] != 1 || d[size - 769] != 0x0C)
            return false;
        w = (d[8] | d[9] << 8) - (d[4] | d[5] << 8) + 1;
        h = (d[10] | d[11] << 8) - (d[6] | d[7] << 8) + 1;
        const int32 bpl = d[66] | d[67] << 8;
        if (w <= 0 || h <= 0 || bpl < w) return false;
        std::vector<uint8> idx(size_t(bpl) * h);
        size_t o = 0;
        long p = 128;
        while (o < idx.size()) {
            if (p >= size - 769) return false;
            uint8 b = d[p++];
            int32 n = 1;
            if ((b & 0xC0) == 0xC0) {
                n = b & 0x3F;
                if (p >= size - 769) return false;
                b = d[p++];
            }
            while (n-- > 0 && o < idx.size()) idx[o++] = b;
        }
        const uint8* pal = &d[size - 768];
        rgb.resize(size_t(w) * h * 3);
        for (int32 y = 0; y < h; y++)
            for (int32 x = 0; x < w; x++)
                memcpy(&rgb[(size_t(y) * w + x) * 3], pal + idx[size_t(y) * bpl + x] * 3, 3);
        return true;
    }

    static int32 c5to8(int32 c) { return (c << 3) | (c >> 2); }

    void writeTitle(const char* dir)
    {
        char path[512];
        sprintf(path, "%s/TITLE.BIN", dir);
        FILE* out = fopen(path, "wb");          // empty: no title (mkbanks)
        if (!out) { printf("can't write %s\n", path); exit(1); }
        int32 w = 0, h = 0;
        std::vector<uint8> src;
        char pcx[512];
        bool loaded = false;
        if (trDir && !fontInfo.empty()) {
            sprintf(pcx, "%s/TITLEH.PCX", trDir);
            loaded = loadPCX(pcx, w, h, src);
            if (!loaded) {
                sprintf(pcx, "%s/titleh.pcx", trDir);
                loaded = loadPCX(pcx, w, h, src);
            }
        }
        if (!loaded || w != 640 || h != 480) {
            fclose(out);
            printf("  TITLE.BIN: none (%s)\n", fontInfo.empty() ? "no font" : "no 640 x 480 TITLEH.PCX");
            return;
        }

        // the box filter: 640 / 216 columns a pixel, with partial ones at
        // the edges weighted by how much of them it covers; 3 rows
        std::vector<double> img(TITLE_W * TITLE_H * 3);
        for (int32 y = 0; y < TITLE_H; y++)
            for (int32 x = 0; x < TITLE_W; x++) {
                const double x0 = x * 640.0 / TITLE_W, x1 = (x + 1) * 640.0 / TITLE_W;
                double sum[3] = { 0, 0, 0 }, area = 0;
                for (int32 sy = 0; sy < TITLE_SRC_H / TITLE_H; sy++) {
                    const int32 row = TITLE_SRC_Y + y * (TITLE_SRC_H / TITLE_H) + sy;
                    for (int32 sx = int32(x0); sx < 640 && sx < x1; sx++) {
                        const double cover = std::min(x1, sx + 1.0) - std::max(x0, double(sx));
                        if (cover <= 0) continue;
                        for (int32 c = 0; c < 3; c++) sum[c] += cover * src[(size_t(row) * 640 + sx) * 3 + c];
                        area += cover;
                    }
                }
                for (int32 c = 0; c < 3; c++) img[(y * TITLE_W + x) * 3 + c] = sum[c] / area;
            }

        // CG-RAM 1-191 as the title has it: PALETTE.BIN, HUD.BIN's two
        // palettes, FONT.BIN's colours, greys, borders and the options' bar
        // (main.asm UploadPalette, InitHud)
        double cr[256], cg[256], cb[256];
        for (int32 i = 0; i < 256; i++) {
            uint16 c = 0;
            if (i < SNES_SHADES * SNES_COLORS) c = palette[i];
            else if (i < 0xA0) c = hudPal[(i - 0x80) >> 4][i & 15];
            else if (i < 0xBC) c = uint16(fontInfo[(i - 0xA0) * 2] | fontInfo[(i - 0xA0) * 2 + 1] << 8);
            else if (i < TITLE_FIRST) { // the options' bar, after the glyphs
                const size_t at = FONT_GLYPHS + fontInfo[57] * 6 + (i - 0xBC) * 2;
                c = uint16(fontInfo[at] | fontInfo[at + 1] << 8);
            }
            cr[i] = c5to8(c & 31); cg[i] = c5to8((c >> 5) & 31); cb[i] = c5to8((c >> 10) & 31);
        }
        #define TDIST(i, r, g, b) (2.0 * (cr[i] - (r)) * (cr[i] - (r)) + 4.0 * (cg[i] - (g)) * (cg[i] - (g)) + \
                                   3.0 * (cb[i] - (b)) * (cb[i] - (b)))
        const int32 n = TITLE_W * TITLE_H;
        std::vector<double> err(n);
        for (int32 p = 0; p < n; p++) {
            double d = 1e18;
            for (int32 i = 1; i < TITLE_FIRST; i++) d = std::min(d, TDIST(i, img[p * 3], img[p * 3 + 1], img[p * 3 + 2]));
            err[p] = d;
        }
        // seeds: each the pixel worst served by everything chosen so far
        for (int32 k = 0; k < TITLE_OWN; k++) {
            int32 best = 0;
            for (int32 p = 1; p < n; p++) if (err[p] > err[best]) best = p;
            const int32 i = TITLE_FIRST + k;
            cr[i] = img[best * 3]; cg[i] = img[best * 3 + 1]; cb[i] = img[best * 3 + 2];
            for (int32 p = 0; p < n; p++) err[p] = std::min(err[p], TDIST(i, img[p * 3], img[p * 3 + 1], img[p * 3 + 2]));
        }
        std::vector<uint8> pix(n);
        for (int32 iter = 0; iter <= 24; iter++) {
            double sr[TITLE_OWN] = { 0 }, sg[TITLE_OWN] = { 0 }, sb[TITLE_OWN] = { 0 }, sw[TITLE_OWN] = { 0 };
            for (int32 p = 0; p < n; p++) {
                const double r = img[p * 3], g = img[p * 3 + 1], b = img[p * 3 + 2];
                int32 bi = 1;
                double bd = 1e18;
                for (int32 i = 1; i < 256; i++) {
                    const double d = TDIST(i, r, g, b);
                    if (d < bd) { bd = d; bi = i; }
                }
                pix[p] = uint8(bi);
                if (bi >= TITLE_FIRST) {
                    sr[bi - TITLE_FIRST] += r; sg[bi - TITLE_FIRST] += g;
                    sb[bi - TITLE_FIRST] += b; sw[bi - TITLE_FIRST] += 1;
                }
            }
            if (iter == 24) break;
            for (int32 k = 0; k < TITLE_OWN; k++) {
                if (sw[k] <= 0) continue;
                const int32 i = TITLE_FIRST + k;
                // the mean, then where BGR555 can actually put it
                cr[i] = c5to8(int32(sr[k] / sw[k] / 255.0 * 31.0 + 0.5));
                cg[i] = c5to8(int32(sg[k] / sw[k] / 255.0 * 31.0 + 0.5));
                cb[i] = c5to8(int32(sb[k] / sw[k] / 255.0 * 31.0 + 0.5));
            }
        }
        #undef TDIST

        uint16 own[TITLE_OWN];
        for (int32 k = 0; k < TITLE_OWN; k++) {
            const int32 i = TITLE_FIRST + k;
            own[k] = uint16((int32(cr[i]) >> 3) | ((int32(cg[i]) >> 3) << 5) | ((int32(cb[i]) >> 3) << 10));
        }
        double total = 0;
        int32 fixedUsed = 0;
        for (int32 p = 0; p < n; p++) {
            const int32 i = pix[p];
            total += (cr[i] - img[p * 3]) * (cr[i] - img[p * 3]) + (cg[i] - img[p * 3 + 1]) * (cg[i] - img[p * 3 + 1]) +
                     (cb[i] - img[p * 3 + 2]) * (cb[i] - img[p * 3 + 2]);
            if (i < TITLE_FIRST) fixedUsed++;
        }
        fwrite(own, 2, TITLE_OWN, out);
        fwrite(pix.data(), 1, n, out);
        fclose(out);
        printf("  TITLE.BIN: %d x %d from %s, %d own colours, %d%% of pixels on the level's, "
               "rms %.1f\n", TITLE_W, TITLE_H, pcx, TITLE_OWN, fixedUsed * 100 / n, sqrt(total / (3.0 * n)));
    }

    void buildPalette(TR1_PC* level)
    {
        int64 weight[256];
        memset(weight, 0, sizeof(weight));

        for (int32 i = 0; i < level->tilesCount; i++) {
            for (int32 j = 0; j < 256 * 256; j++) {
                weight[level->tiles[i].indices[j]]++;
            }
        }
        weight[0] = 0;                  // index 0 is transparent in TR

        struct Cluster {
            int32 r, g, b;
            int64 w;
        };

        Cluster c[SNES_COLORS];
        memset(c, 0, sizeof(c));

        // Seed: the fifteen heaviest colours, plus black reserved at slot 0.
        bool taken[256];
        memset(taken, 0, sizeof(taken));

        palette[0] = 0;
        c[0].r = c[0].g = c[0].b = 0;
        c[0].w = 1;

        for (int32 k = 1; k < SNES_COLORS; k++)
        {
            int32 best = -1;
            int64 bestW = -1;
            for (int32 i = 1; i < 256; i++) {
                if (!taken[i] && weight[i] > bestW) {
                    bestW = weight[i];
                    best = i;
                }
            }
            if (best < 0) best = k;
            taken[best] = true;
            c[k].r = level->palette.colors[best * 3 + 0];
            c[k].g = level->palette.colors[best * 3 + 1];
            c[k].b = level->palette.colors[best * 3 + 2];
            c[k].w = 1;
        }

        // A few rounds of k-means over the weighted palette entries.
        for (int32 iter = 0; iter < 12; iter++)
        {
            Cluster next[SNES_COLORS];
            memset(next, 0, sizeof(next));

            for (int32 i = 0; i < 256; i++)
            {
                if (!weight[i]) continue;
                int32 r = level->palette.colors[i * 3 + 0];
                int32 g = level->palette.colors[i * 3 + 1];
                int32 b = level->palette.colors[i * 3 + 2];

                int32 best = 0;
                int64 bestD = 1LL << 60;
                for (int32 k = 0; k < SNES_COLORS; k++)
                {
                    int64 dr = r - c[k].r, dg = g - c[k].g, db = b - c[k].b;
                    int64 d = dr * dr + dg * dg + db * db;
                    if (d < bestD) { bestD = d; best = k; }
                }
                next[best].r += (int32)(r * weight[i] / 64);
                next[best].g += (int32)(g * weight[i] / 64);
                next[best].b += (int32)(b * weight[i] / 64);
                next[best].w += weight[i] / 64;
            }

            for (int32 k = 1; k < SNES_COLORS; k++)
            {
                if (next[k].w) {
                    c[k].r = (int32)(next[k].r / next[k].w);
                    c[k].g = (int32)(next[k].g / next[k].w);
                    c[k].b = (int32)(next[k].b / next[k].w);
                }
            }
        }

        // Map every TR palette entry to its nearest SNES slot.
        for (int32 i = 0; i < 256; i++)
        {
            int32 r = level->palette.colors[i * 3 + 0];
            int32 g = level->palette.colors[i * 3 + 1];
            int32 b = level->palette.colors[i * 3 + 2];

            int32 best = 0;
            int64 bestD = 1LL << 60;
            for (int32 k = 0; k < SNES_COLORS; k++)
            {
                int64 dr = r - c[k].r, dg = g - c[k].g, db = b - c[k].b;
                int64 d = dr * dr + dg * dg + db * db;
                if (d < bestD) { bestD = d; best = k; }
            }
            palIndex[i] = (uint8)best;
        }

        // Shading runs the other way round: TR's lightmap is a table over TR
        // palette entries, so each SNES slot needs one TR entry to stand for
        // it. The heaviest contributor is the honest choice -- it is the
        // colour most of the slot's texels actually were.
        int64 repWeight[SNES_COLORS];
        memset(repWeight, 0, sizeof(repWeight));
        memset(palRep, 0, sizeof(palRep));

        for (int32 i = 1; i < 256; i++)
        {
            int32 k = palIndex[i];
            if (weight[i] > repWeight[k]) {
                repWeight[k] = weight[i];
                palRep[k] = (uint8)i;
            }
        }

        // A slot no texel uses still needs a plausible ramp, so fall back to
        // the TR entry nearest its centroid.
        for (int32 k = 1; k < SNES_COLORS; k++)
        {
            if (repWeight[k]) continue;

            int32 best = 1;
            int64 bestD = 1LL << 60;
            for (int32 i = 1; i < 256; i++)
            {
                int64 dr = level->palette.colors[i * 3 + 0] - c[k].r;
                int64 dg = level->palette.colors[i * 3 + 1] - c[k].g;
                int64 db = level->palette.colors[i * 3 + 2] - c[k].b;
                int64 d = dr * dr + dg * dg + db * db;
                if (d < bestD) { bestD = d; best = i; }
            }
            palRep[k] = (uint8)best;
        }

        // One CG-RAM row per shade level. The renderer quantises its 32-level
        // g with g >> 2, so row s stands for g in [4s, 4s+3] and takes its
        // lightmap row from the middle of that bucket.
        //
        // The lightmap is applied as a brightness ratio against its own row 0
        // rather than as a colour lookup. Reading the shaded TR entry directly
        // would throw away the k-means centroid and put the representative's
        // colour on screen instead, which is a worse fit for every slot that
        // stands for more than one TR entry -- and row 0 has to come out as
        // the centroid exactly, since that is what the texture quantiser
        // measured its error against.
        for (int32 s = 0; s < SNES_SHADES; s++)
        {
            int32 lmRow = s * 4 + 2;
            if (lmRow > 31) lmRow = 31;

            for (int32 k = 0; k < SNES_COLORS; k++)
            {
                int32 r = 0, g = 0, b = 0;

                if (k)                  // slot 0 stays black in every row
                {
                    int32 lit  = level->lightmap[lmRow * 256 + palRep[k]];
                    int32 full = level->lightmap[palRep[k]];

                    int32 litSum  = level->palette.colors[lit * 3 + 0] +
                                    level->palette.colors[lit * 3 + 1] +
                                    level->palette.colors[lit * 3 + 2];
                    int32 fullSum = level->palette.colors[full * 3 + 0] +
                                    level->palette.colors[full * 3 + 1] +
                                    level->palette.colors[full * 3 + 2];

                    // 8.8 brightness ratio; an unrepresentable rep colour just
                    // means no falloff information, so leave it lit.
                    int32 scale = fullSum ? (litSum * 256 / fullSum) : 256;
                    if (scale > 256) scale = 256;

                    r = c[k].r * scale >> 8;
                    g = c[k].g * scale >> 8;
                    b = c[k].b * scale >> 8;
                }

                r >>= 1;                // 6-bit TR -> 5-bit SNES
                g >>= 1;
                b >>= 1;
                if (r > 31) r = 31;
                if (g > 31) g = 31;
                if (b > 31) b = 31;
                palette[s * SNES_COLORS + k] = (uint16)(r | (g << 5) | (b << 10));
            }
        }
        paletteCount = SNES_SHADES * SNES_COLORS;
    }

    // ------------------------------------------------------------------
    // Texture pages: sixteen colours plus a source-index-zero marker.
    // ------------------------------------------------------------------

    int32 writeTextures(const char* dir, TR1_PC* level)
    {
        char path[512];

        for (int32 i = 0; i < level->tilesCount; i++)
        {
            uint8* page = new uint8[SNES_PAGE_SIZE];
            const uint8* src = level->tiles[i].indices;
            for (int32 j = 0; j < SNES_PAGE_SIZE; j++) {
                page[j] = src[j] ? palIndex[src[j]] : SNES_TEX_HOLE;
            }

            sprintf(path, "%s/TEX%02d.BIN", dir, i);
            FILE* f = fopen(path, "wb");
            if (!f) {
                printf("can't write \"%s\"\n", path);
                delete[] page;
                exit(1);
            }
            if (fwrite(page, 1, SNES_PAGE_SIZE, f) != SNES_PAGE_SIZE) {
                printf("short write to \"%s\"\n", path);
                exit(1);
            }
            fclose(f);
            delete[] page;
        }
        // The selected level owns this list. A directory scan also includes
        // pages left by a larger level previously built in the same folder.
        sprintf(path, "%s/TEXTURES.json", dir);
        FILE* manifest = fopen(path, "wb");
        if (!manifest) {
            printf("can't write \"%s\"\n", path);
            exit(1);
        }
        fprintf(manifest, "{\"pages\": %d}\n", level->tilesCount);
        if (fclose(manifest)) exit(1);
        return level->tilesCount;
    }

    // ------------------------------------------------------------------
    // Room geometry
    // ------------------------------------------------------------------

    struct OutFace {
        uint8  page;
        uint8  count;
        uint16 v[4];
        uint8  u[4];
        uint8  t[4];
    };

    // ------------------------------------------------------------------
    // Meshes.
    //
    // A TR mesh is a stream: centre, radius, flags, then vertices, then either
    // normals or per-vertex intensities, then textured rectangles, textured
    // triangles, and the two coloured variants. Only the textured faces come
    // across -- the renderer has one rasterizer and it reads a texture page.
    //
    // What is emitted is byte for byte the *room* format: a vertex count, a
    // face count, eight-byte vertices and the same face records. That is the
    // whole trick to getting meshes drawn at all. TransformMesh and DrawMesh
    // do not know or care that they are looking at a mesh rather than a room,
    // so the entire geometry pipeline is reused without a line changed.
    // ------------------------------------------------------------------

    // TR's 16-bit angle down to the sine table's 256 entries, rounded rather
    // than truncated: truncating biases every joint the same way round, which
    // on a chain of them adds up to a visible lean.
    static uint8 angle8(int32 angle)
    {
        return (uint8)(((angle + 128) >> 8) & 0xFF);
    }

    struct MeshBlob {
        uint8* data;
        int32  size;
        int32  vCount, faceCount;
        bool   valid;
    };

    void buildMesh(TR1_PC* level, int32 meshIndex, MeshBlob& out)
    {
        memset(&out, 0, sizeof(out));

        const uint8* ptr = (const uint8*)level->meshData + level->meshOffsets[meshIndex];

        ptr += 3 * sizeof(int16);                       // centre
        ptr += sizeof(int16);                           // radius
        ptr += sizeof(uint16);                          // flags

        int32 vCount = *(int16*)ptr; ptr += 2;
        const int16* vertices = (const int16*)ptr;
        ptr += vCount * 3 * sizeof(int16);

        const uint16* vIntensity = NULL;
        int32 nCount = *(int16*)ptr; ptr += 2;
        if (nCount > 0) {
            ptr += nCount * 3 * sizeof(int16);          // normals, unused here
        } else {
            vIntensity = (const uint16*)ptr;
            ptr += vCount * sizeof(uint16);
        }

        int32 rCount = *(int16*)ptr; ptr += 2;
        const TR1_PC::Quad* rFaces = (const TR1_PC::Quad*)ptr;
        ptr += rCount * sizeof(TR1_PC::Quad);

        int32 tCount = *(int16*)ptr; ptr += 2;
        const TR1_PC::Triangle* tFaces = (const TR1_PC::Triangle*)ptr;
        ptr += tCount * sizeof(TR1_PC::Triangle);

        int32 crCount = *(int16*)ptr; ptr += 2;
        const TR1_PC::Quad* crFaces = (const TR1_PC::Quad*)ptr;
        ptr += crCount * sizeof(TR1_PC::Quad);

        int32 ctCount = *(int16*)ptr; ptr += 2;
        const TR1_PC::Triangle* ctFaces = (const TR1_PC::Triangle*)ptr;

        if (vCount <= 0 || vCount > SNES_MAX_VERTS) {
            if (getenv("SNES_MESH_DEBUG"))
                printf("  mesh %d dropped: vCount %d\n", meshIndex, vCount);
            return;
        }

        OutFace* faces = new OutFace[rCount + tCount + crCount + ctCount + 1];
        int32 faceCount = 0;

        for (int32 i = 0; i < rCount; i++)
        {
            int32 texIndex = rFaces[i].flags & FACE_TEXTURE;
            if (texIndex >= level->objectTexturesCount) continue;
            const TR1_PC::ObjectTexture& t = level->objectTextures[texIndex];

            g_facesSeen++;
            if (t.attribute & TEX_ATTR_AKILL) g_akillFaces++;
            OutFace& o = faces[faceCount];
            o.page = snesTexturePage(t.tile, (t.attribute & TEX_ATTR_AKILL) != 0);
            o.count = 4;
            o.v[0] = rFaces[i].indices[0];  o.u[0] = t.x0;  o.t[0] = t.y0;
            o.v[1] = rFaces[i].indices[1];  o.u[1] = t.x1;  o.t[1] = t.y1;
            o.v[2] = rFaces[i].indices[2];  o.u[2] = t.x2;  o.t[2] = t.y2;
            o.v[3] = rFaces[i].indices[3];  o.u[3] = t.x3;  o.t[3] = t.y3;
            faceCount++;
        }

        for (int32 i = 0; i < tCount; i++)
        {
            int32 texIndex = tFaces[i].flags & FACE_TEXTURE;
            if (texIndex >= level->objectTexturesCount) continue;
            const TR1_PC::ObjectTexture& t = level->objectTextures[texIndex];

            g_facesSeen++;
            if (t.attribute & TEX_ATTR_AKILL) g_akillFaces++;
            OutFace& o = faces[faceCount];
            o.page = snesTexturePage(t.tile, (t.attribute & TEX_ATTR_AKILL) != 0);
            o.count = 3;
            o.v[0] = tFaces[i].indices[0];  o.u[0] = t.x0;  o.t[0] = t.y0;
            o.v[1] = tFaces[i].indices[1];  o.u[1] = t.x1;  o.t[1] = t.y1;
            o.v[2] = tFaces[i].indices[2];  o.u[2] = t.x2;  o.t[2] = t.y2;
            faceCount++;
        }

        /* GBA carries these as FACE_TYPE_F.  Keeping their normal face shape
           and putting the type in the page byte lets DrawFace reuse its
           clipping, culling and edge setup before selecting RasterF. */
        for (int32 i = 0; i < crCount; i++)
        {
            OutFace& o = faces[faceCount];
            o.page = (uint8)(SNES_FACE_FLAT |
                             palIndex[crFaces[i].flags & 0xFF]);
            o.count = 4;
            for (int32 j = 0; j < 4; j++) {
                o.v[j] = crFaces[i].indices[j];
                o.u[j] = o.t[j] = 0;
            }
            faceCount++;
        }

        for (int32 i = 0; i < ctCount; i++)
        {
            OutFace& o = faces[faceCount];
            o.page = (uint8)(SNES_FACE_FLAT |
                             palIndex[ctFaces[i].flags & 0xFF]);
            o.count = 3;
            for (int32 j = 0; j < 3; j++) {
                o.v[j] = ctFaces[i].indices[j];
                o.u[j] = o.t[j] = 0;
            }
            faceCount++;
        }

        if (faceCount == 0) {
            if (getenv("SNES_MESH_DEBUG"))
                printf("  mesh %d dropped: no faces (%d/%d textured, "
                       "%d/%d coloured)\n", meshIndex, rCount, tCount,
                       crCount, ctCount);
            delete[] faces; return;
        }

        int32 faceBytes = 0;
        for (int32 i = 0; i < faceCount; i++)
            faceBytes += 2 + faces[i].count * 4;

        out.data = new uint8[4 + vCount * 8 + faceBytes];
        uint8* p = out.data;

        #define PUT16(v) { uint16 _t = (uint16)(v); memcpy(p, &_t, 2); p += 2; }
        #define PUT8(v)  { *p++ = (uint8)(v); }

        PUT16(vCount);
        PUT16(faceCount);

        for (int32 i = 0; i < vCount; i++)
        {
            // A mesh carries its own shading where TR gave it any; where it
            // gave normals instead, the middle of the ramp is the honest
            // answer until there is dynamic lighting to use them with.
            int32 g = (SNES_SHADE_MIN + SNES_SHADE_MAX) / 2;
            if (vIntensity) {
                g = vIntensity[i] >> 8;
                if (g < SNES_SHADE_MIN) g = SNES_SHADE_MIN;
                if (g > SNES_SHADE_MAX) g = SNES_SHADE_MAX;
            }
            PUT16(vertices[i * 3 + 0]);
            PUT16(vertices[i * 3 + 1]);
            PUT16(vertices[i * 3 + 2]);
            PUT16(g);
        }

        for (int32 i = 0; i < faceCount; i++)
        {
            PUT8(faces[i].page);
            PUT8(faces[i].count);
            for (int32 j = 0; j < faces[i].count; j++)
            {
                PUT16(SNES_TV_BASE + faces[i].v[j] * SNES_TV_SIZE);
                PUT8(faces[i].u[j]);
                PUT8(faces[i].t[j]);
            }
        }

        #undef PUT16
        #undef PUT8

        out.size = (int32)(p - out.data);
        out.vCount = vCount;
        out.faceCount = faceCount;
        out.valid = true;
        delete[] faces;
    }

    // ------------------------------------------------------------------
    // Models
    //
    // One stream of SNES_NODE_SIZE records, one per mesh, in draw order.
    // See the format note at the top of the file for why the three upstream
    // tables are flattened into one here.
    // ------------------------------------------------------------------

    // Whether a mesh has anything to draw, without building it. TR1 leaves
    // Lara's arm meshes empty in her base model on purpose: the game fills
    // them at run time from whichever weapon model she is holding, which is
    // ItemObj::meshSwap. Fill these empty joints once here; weapon.gsu also
    // swaps the nonempty thigh/hand meshes at runtime using WEAPON.json.
    bool meshDrawable(TR1_PC* level, int32 meshIndex)
    {
        if (meshIndex < 0 || meshIndex >= level->meshOffsetsCount) return false;

        const uint8* ptr = (const uint8*)level->meshData + level->meshOffsets[meshIndex];
        ptr += 4 * sizeof(int16) + sizeof(uint16);       // centre, radius, flags

        int32 vCount = *(int16*)ptr; ptr += 2;
        ptr += vCount * 3 * sizeof(int16);

        int32 nCount = *(int16*)ptr; ptr += 2;
        ptr += (nCount > 0) ? nCount * 3 * sizeof(int16)
                            : vCount * (int32)sizeof(uint16);

        int32 rCount = *(int16*)ptr; ptr += 2;
        ptr += rCount * sizeof(TR1_PC::Quad);
        int32 tCount = *(int16*)ptr; ptr += 2;
        ptr += tCount * sizeof(TR1_PC::Triangle);
        int32 crCount = *(int16*)ptr; ptr += 2;
        ptr += crCount * sizeof(TR1_PC::Quad);
        int32 ctCount = *(int16*)ptr;

        return vCount > 0 && vCount <= SNES_MAX_VERTS &&
               (rCount + tCount + crCount + ctCount) > 0;
    }

    // A model is a small blob -- header, nodes, and a table of its
    // animations -- plus one run of frames per animation, placed separately.
    //
    // They have to be separate because Lara's frames come to 101 KB and R14
    // is sixteen bits: a model's frames cannot be one addressable run. Per
    // animation they always fit, since an animation is a few dozen frames,
    // and the table gives each one its own bank. Capping the model instead
    // would have quietly dropped two fifths of her animations.
    struct ModelBlob {
        uint8* data;            // header, nodes, anim table
        int32  size;
        int32  animTableOff;
        int32  nodeCount;
        int32  animCount;
        int32  frameTotal;      // across all its animations, for the report

        uint8* frames;          // every animation's frames, concatenated
        int32* animOff;         // where each animation's start inside `frames`
        int32* animLen;         // and how many bytes
        int32* animFrames;      // and how many frames
        bool   valid;
    };

    // A bounding radius for the renderer, in units of 32: the farthest a
    // box's corner lies from the origin, rounded up, at least one. The GSU
    // tests a sphere of it against the clip rectangle before it builds a
    // model's nodes or places a static mesh (SphereVisible in mesh.gsu).
    static int32 radiusByte(int32 minX, int32 maxX, int32 minY, int32 maxY,
                            int32 minZ, int32 maxZ)
    {
        double x = std::max(std::abs(minX), std::abs(maxX));
        double y = std::max(std::abs(minY), std::abs(maxY));
        double z = std::max(std::abs(minZ), std::abs(maxZ));
        int32 r = (int32)ceil(sqrt(x * x + y * y + z * z) / 32.0);
        if (r < 1) r = 1;
        if (r > 255) r = 255;
        return r;
    }

    // A mesh node rotates around its origin, not around the centre stored in
    // TR's mesh header. Bound the vertices from that origin so the sphere is
    // valid before or after any pose rotation. Zero means "too large to encode"
    // and makes the renderer keep the node; saturating the byte would cull real
    // geometry beyond the shortened radius.
    static int32 meshRadiusByte(TR1_PC* level, int32 meshIndex)
    {
        if (meshIndex < 0 || meshIndex >= level->meshOffsetsCount) return 0;

        const uint8* ptr = (const uint8*)level->meshData
                         + level->meshOffsets[meshIndex];
        ptr += 3 * sizeof(int16);                 // centre, not the node origin
        ptr += sizeof(int16);                     // TR's centre-relative radius
        ptr += sizeof(uint16);                    // flags

        int32 vCount = *(int16*)ptr; ptr += 2;
        if (vCount <= 0 || vCount > SNES_MAX_VERTS) return 0;
        const int16* vertices = (const int16*)ptr;

        double r2 = 0.0;
        for (int32 i = 0; i < vCount; i++) {
            double x = vertices[i * 3 + 0];
            double y = vertices[i * 3 + 1];
            double z = vertices[i * 3 + 2];
            r2 = std::max(r2, x * x + y * y + z * z);
        }

        int32 r = (int32)ceil(sqrt(r2) / 32.0);
        if (r < 1) r = 1;
        return r <= 255 ? r : 0;
    }

    void buildModel(TR1_PC* level, int32 modelIndex, ModelBlob& out)
    {
        memset(&out, 0, sizeof(out));

        const TR1_PC::Model& model = level->models[modelIndex];
        const int32 meshCount = model.count;
        if (meshCount <= 0 || meshCount > 255) return;

        // frameStride and the frame index are the two operands of an 8x8
        // multiply on the GSU, so both have to stay inside a byte.
        const int32 frameStride = 6 + meshCount * 3;
        if (frameStride > 255) return;

        // Every animation the model owns. TR stores them contiguously and a
        // model's run ends where the next model's begins, which is the only
        // thing that says how many it has -- there is no count in the file.
        int32 animStart = (int32)model.animIndex;
        int32 animEnd = level->animsCount;
        for (int32 j = 0; j < level->modelsCount; j++) {
            int32 a = (int32)level->models[j].animIndex;
            if (a > animStart && a < animEnd) animEnd = a;
        }
        if (animStart < 0 || animStart >= level->animsCount) {
            animStart = 0;
            animEnd = 0;
        }

        const int32 fallbackStep = 10 + meshCount * 2;
        const int32 fallbackWord = (int32)(model.frameIndex >> 1);
        const int32 animCount = (animEnd > animStart) ? (animEnd - animStart) : 1;

        out.animOff    = new int32[animCount];
        out.animLen    = new int32[animCount];
        out.animFrames = new int32[animCount];

        // Two passes: how many frames each animation has, then the frames.
        int32 frameTotal = 0;
        for (int32 k = 0; k < animCount; k++)
        {
            int32 count = 1;
            if (animEnd > animStart)
            {
                const TR1_PC::Animation& anim = level->anims[animStart + k];
                int32 step = anim.frameSize > 0 ? anim.frameSize : fallbackStep;
                int32 word = (int32)(anim.frameOffset >> 1);
                count = 1;
                if (anim.frameRate > 0 && anim.frameEnd >= anim.frameBegin)
                    count = (anim.frameEnd - anim.frameBegin) / anim.frameRate + 1;
                while (count > 0 && word + count * step > level->frameDataSize)
                    count--;
                // The frame index within an animation is a byte on the wire.
                if (count > 255) count = 255;
                if (count < 0) count = 0;
            }
            else if (fallbackWord + fallbackStep > level->frameDataSize)
            {
                count = 0;
            }

            /* And the box the animation occupies, which is the union of the
               boxes TR puts at the head of each of its frames. */
            if (animEnd > animStart && count > 0)
            {
                const TR1_PC::Animation& anim = level->anims[animStart + k];
                int32 step = anim.frameSize > 0 ? anim.frameSize : fallbackStep;
                int32 word = (int32)(anim.frameOffset >> 1);
                int16* box = animBox + (animStart + k) * 6;
                int32 f;

                for (f = 0; f < count; f++)
                {
                    const uint16* fr = level->frameData + word + f * step;
                    int32 j;
                    for (j = 0; j < 6; j++) {
                        int16 v = (int16)fr[j];
                        if (f == 0) box[j] = v;
                        else if (j & 1) { if (v > box[j]) box[j] = v; }
                        else            { if (v < box[j]) box[j] = v; }
                    }
                }
            }

            if (animEnd > animStart) {
                animSpeed[(animStart + k) * 2 + 0] = level->anims[animStart + k].speed;
                animSpeed[(animStart + k) * 2 + 1] = level->anims[animStart + k].accel;
            }

            out.animFrames[k] = count;
            out.animOff[k] = frameTotal * frameStride;
            out.animLen[k] = count * frameStride;
            frameTotal += count;

            if (animEnd > animStart) {
                animFirstFrame[animStart + k] = 0;   // frames are per animation
                animFrameCount[animStart + k] = (uint16)count;
            }
        }

        if (frameTotal == 0) return;

        out.frames = new uint8[frameTotal * frameStride];
        uint8* p = out.frames;

        #define PUT16(v) { uint16 _t = (uint16)(v); memcpy(p, &_t, 2); p += 2; }
        #define PUT8(v)  { *p++ = (uint8)(v); }
        int32 maxR = 1;                         // over every frame's box
        double maxRoot = 0;
        #define PUT_FRAME(base)                                                \
        {                                                                      \
            const uint16* frame = level->frameData + (base);                   \
            maxR = std::max(maxR, radiusByte((int16)frame[0], (int16)frame[1], \
                                             (int16)frame[2], (int16)frame[3], \
                                             (int16)frame[4], (int16)frame[5]));\
            double rx = (int16)frame[6], ry = (int16)frame[7], rz = (int16)frame[8];\
            maxRoot = std::max(maxRoot, sqrt(rx * rx + ry * ry + rz * rz));      \
            PUT16((int16)frame[6]);             /* the root's translate */     \
            PUT16((int16)frame[7]);                                            \
            PUT16((int16)frame[8]);                                            \
            for (int32 i = 0; i < meshCount; i++) {                            \
                uint32 a = (uint32)frame[10 + i * 2]                           \
                         | ((uint32)frame[11 + i * 2] << 16);                  \
                PUT8(angle8(((a >> 16) & 0x3FF0) << 2));                       \
                PUT8(angle8((((a >> 16) & 0x000F) << 12) | ((a & 0xFC00) >> 4)));\
                PUT8(angle8((a & 0x03FF) << 6));                               \
            }                                                                  \
        }

        for (int32 k = 0; k < animCount; k++)
        {
            if (animEnd <= animStart) {
                for (int32 f = 0; f < out.animFrames[k]; f++) PUT_FRAME(fallbackWord)
                continue;
            }
            const TR1_PC::Animation& anim = level->anims[animStart + k];
            int32 step = anim.frameSize > 0 ? anim.frameSize : fallbackStep;
            int32 word = (int32)(anim.frameOffset >> 1);
            for (int32 f = 0; f < out.animFrames[k]; f++) {
                PUT_FRAME(word + f * step)
            }
        }
        #undef PUT_FRAME

        const TR1_PC::Node* nodes =
            (const TR1_PC::Node*)level->nodesData + model.nodeIndex / 4;
        if (model.type == ITEM_LARA) {
            // Aimed arms and torso/head turns can leave the body frame's box. A
            // sum of bone lengths bounds every rotation, including a basis
            // override, and includes the drawn/holstered weapon meshes.
            double parents[32], reach = 0;
            int32 depth = 0;
            int32 guns = level->getModelIndex(ITEM_LARA_PISTOLS);
            for (int32 i = 0; i < meshCount; i++) {
                if (i) {
                    const TR1_PC::Node& n = nodes[i - 1];
                    if (n.flags & TR1_PC::NODE_FLAG_POP) {
                        if (!depth) { printf("invalid Lara skeleton pop\n"); exit(1); }
                        reach = parents[--depth];
                    }
                    if (n.flags & TR1_PC::NODE_FLAG_PUSH) {
                        if (depth == 32) { printf("Lara skeleton too deep\n"); exit(1); }
                        parents[depth++] = reach;
                    }
                    double x = n.pos.x, y = n.pos.y, z = n.pos.z;
                    reach += sqrt(x * x + y * y + z * z);
                }
                int32 radius = meshRadiusByte(level, model.start + i);
                if (guns >= 0 && i < level->models[guns].count)
                    radius = std::max(radius, meshRadiusByte(level, level->models[guns].start + i));
                // FX3 swaps in the other three guns' meshes too (writeWeaponKit)
                for (int32 w = 1; packFx3 && w < 4; w++) {
                    const int32 m = level->getModelIndex((ItemType)(ITEM_LARA_PISTOLS + w));
                    if (m >= 0 && i < level->models[m].count && meshDrawable(level, level->models[m].start + i))
                        radius = std::max(radius, meshRadiusByte(level, level->models[m].start + i));
                }
                maxR = std::max(maxR, (int32)ceil((maxRoot + reach) / 32.0) + radius);
            }
            if (maxR > 255) { printf("Lara's weapon bounds exceed the model radius\n"); exit(1); }
        }

        // The blob itself: header, nodes, then the animation table, whose
        // addresses are filled in once the frames have been placed.
        const int32 nodeBytes = meshCount * SNES_NODE_SIZE;
        const int32 animTableOff = SNES_MODEL_HDR + nodeBytes;

        out.data = new uint8[animTableOff + animCount * SNES_ANIM_SIZE];
        p = out.data;

        PUT8(meshCount);
        PUT8(maxR);                             // bounding radius / 32
        PUT16(frameStride);
        PUT16(animCount);
        PUT16(animTableOff);

        for (int32 i = 0; i < meshCount; i++)
        {
            // Mesh 0 is placed by the frame; every mesh after it by the node
            // that precedes it, exactly as drawNodes walks them.
            int32 flags = SNES_NODE_ROOT, x = 0, y = 0, z = 0;
            if (i > 0) {
                const TR1_PC::Node& node = nodes[i - 1];
                flags = node.flags & (TR1_PC::NODE_FLAG_POP | TR1_PC::NODE_FLAG_PUSH);
                x = node.pos.x;
                y = node.pos.y;
                z = node.pos.z;
            }

            int32 meshIndex = model.start + i;
            if (!meshDrawable(level, meshIndex) && model.type == ITEM_LARA)
            {
                int32 alt = level->getModelIndex(ITEM_LARA_PISTOLS);
                if (alt >= 0) {
                    int32 sub = level->models[alt].start + i;
                    if (meshDrawable(level, sub)) meshIndex = sub;
                }
            }

            bool haveMesh = meshDrawable(level, meshIndex);
            if (haveMesh && meshUsed) meshUsed[meshIndex] = 1;

            PUT16(haveMesh ? meshIndex : 0);    // patched to an address later
            PUT8(haveMesh ? 0 : SNES_INST_NONE);
            PUT8(flags);
            PUT8(haveMesh ? meshRadiusByte(level, meshIndex) : 0);
            PUT16(x);
            PUT16(y);
            PUT16(z);
        }

        for (int32 k = 0; k < animCount; k++)
        {
            PUT16(0);                           // frameBase, patched
            PUT8(SNES_INST_NONE);               // bank, patched
            PUT8(out.animFrames[k]);
        }

        #undef PUT16
        #undef PUT8

        out.size = (int32)(p - out.data);
        out.animTableOff = animTableOff;
        out.nodeCount = meshCount;
        out.animCount = animCount;
        out.frameTotal = frameTotal;
        out.valid = true;

        // The bounding box of the model's first frame, which is what TR puts
        // at the head of every frame and this port has until now walked
        // straight past. It is the only thing that gives an entity a size,
        // and without one nothing in the world can be bumped into.
        {
            int32 word = (animEnd > animStart)
                       ? (int32)(level->anims[animStart].frameOffset >> 1)
                       : fallbackWord;
            if (word + 6 <= level->frameDataSize) {
                for (int32 k = 0; k < 6; k++) {
                    modelBox[modelIndex * 6 + k] = (int16)level->frameData[word + k];
                }
            }
        }

        modelFirstAnim[modelIndex] = (uint16)animStart;
        modelAnimCount[modelIndex] = (uint16)(animEnd > animStart ? animCount : 0);
    }

    // ------------------------------------------------------------------
    // Sprites
    //
    // TR draws a pickup as a sprite: a texture rectangle with a world size
    // and no geometry at all. There is no model, no node, no animation and
    // no frame, and every table on the other side is indexed by one of those
    // -- so a medipack that is "just a texture" is a medipack the host cannot
    // hold, the renderer cannot place and the item code cannot animate.
    //
    // Rather than a second path through all of that, the packer builds the
    // thing TR is describing: one quad, one node, one animation, one frame.
    // Everything downstream then treats a pickup as any other entity, and the
    // only difference that survives is a flag on the node saying "drop the
    // rotation out of the matrix", which is what makes a flat quad face the
    // camera. That costs the renderer three stores. See DrawModel.
    // ------------------------------------------------------------------

    void buildSpriteMesh(TR1_PC* level, int32 spriteIndex, MeshBlob& out)
    {
        memset(&out, 0, sizeof(out));
        if (spriteIndex < 0 || spriteIndex >= level->spriteTexturesCount) return;

        const TR1_PC::SpriteTexture& spr = level->spriteTextures[spriteIndex];

        // TR keeps a sprite's size in 256ths of a texel, and its extent in
        // the world as the four numbers a quad wants: l/t is the corner above
        // and left of where it stands, r/b the one below and right. Y is
        // down, as everywhere in TR.
        int32 w = (spr.w + 255) >> 8;
        int32 h = (spr.h + 255) >> 8;
        if (w <= 0 || h <= 0) return;

        int32 u0 = spr.u, v0 = spr.v;
        int32 u1 = u0 + w - 1, v1 = v0 + h - 1;
        if (u1 > 255) u1 = 255;
        if (v1 > 255) v1 = 255;

        out.data = new uint8[4 + 4 * 8 + 2 + 4 * 4];
        uint8* p = out.data;

        #define PUT16(v) { uint16 _t = (uint16)(v); memcpy(p, &_t, 2); p += 2; }
        #define PUT8(v)  { *p++ = (uint8)(v); }

        PUT16(4);                               // four vertices, one face
        PUT16(1);

        // Lit as bright as the ramp goes. Nothing here tracks what a sprite
        // is standing in, and TR draws its pickups unshaded anyway -- a
        // medipack you cannot see in a dark room is a medipack you walk past.
        static const int32 corner[4][2] = { {0,0}, {1,0}, {1,1}, {0,1} };
        for (int32 i = 0; i < 4; i++)
        {
            PUT16(corner[i][0] ? spr.r : spr.l);
            PUT16(corner[i][1] ? spr.b : spr.t);
            PUT16(0);
            PUT16(SNES_SHADE_MAX);
        }

        PUT8(snesTexturePage(spr.tile, true));
        PUT8(4);
        for (int32 i = 0; i < 4; i++)
        {
            PUT16(SNES_TV_BASE + i * SNES_TV_SIZE);
            PUT8(corner[i][0] ? u1 : u0);
            PUT8(corner[i][1] ? v1 : v0);
        }

        #undef PUT16
        #undef PUT8

        out.size = (int32)(p - out.data);
        out.vCount = 4;
        out.faceCount = 1;
        out.valid = true;
    }

    // The model that draws it: one node holding that quad, and one animation
    // of one frame holding nothing. The frame exists because animProcess
    // reads one every tick and a model with none is a model the host will not
    // index. It costs nine bytes.
    void buildSpriteModel(TR1_PC* level, int32 spriteIndex, int32 modelIndex,
                          int32 animIndex, int32 meshIndex, ModelBlob& out)
    {
        memset(&out, 0, sizeof(out));
        if (spriteIndex < 0 || spriteIndex >= level->spriteTexturesCount) return;

        const TR1_PC::SpriteTexture& spr = level->spriteTextures[spriteIndex];
        const int32 frameStride = 6 + 3;        // one node's worth
        const int32 animTableOff = SNES_MODEL_HDR + SNES_NODE_SIZE;

        out.data = new uint8[animTableOff + SNES_ANIM_SIZE];
        uint8* p = out.data;

        #define PUT16(v) { uint16 _t = (uint16)(v); memcpy(p, &_t, 2); p += 2; }
        #define PUT8(v)  { *p++ = (uint8)(v); }

        PUT8(1);                                // nodeCount
        PUT8(radiusByte(spr.l, spr.r, spr.t, spr.b, 0, 0)); // bounding radius / 32
        PUT16(frameStride);
        PUT16(1);                               // one animation
        PUT16(animTableOff);

        PUT16(meshIndex);                       // patched to an address later
        PUT8(0);                                // and a bank
        PUT8(SNES_NODE_ROOT | SNES_NODE_BILLBOARD);
        PUT8(radiusByte(spr.l, spr.r, spr.t, spr.b, 0, 0));
        PUT16(0); PUT16(0); PUT16(0);           // no offset from the item

        PUT16(0);                               // frameBase, patched
        PUT8(SNES_INST_NONE);                   // bank, patched
        PUT8(1);                                // one frame

        #undef PUT16
        #undef PUT8

        out.size = (int32)(p - out.data);
        out.animTableOff = animTableOff;
        out.nodeCount = 1;
        out.animCount = 1;
        out.frameTotal = 1;

        out.animOff    = new int32[1];
        out.animLen    = new int32[1];
        out.animFrames = new int32[1];
        out.animOff[0] = 0;
        out.animLen[0] = frameStride;
        out.animFrames[0] = 1;

        out.frames = new uint8[frameStride];
        memset(out.frames, 0, frameStride);     // no translate, no rotation
        out.valid = true;

        // The box it occupies. A pickup is not something to walk into -- the
        // host decides that, in Item_solid -- but it is something to stand
        // near and to shoot past, and both want a size. Z takes X's extent:
        // the quad has no depth, and a box with none contains nothing.
        int16* box = modelBox + modelIndex * 6;
        box[0] = spr.l; box[1] = spr.r;
        box[2] = spr.t; box[3] = spr.b;
        box[4] = spr.l; box[5] = spr.r;
        for (int32 k = 0; k < 6; k++) animBox[animIndex * 6 + k] = box[k];

        modelFirstAnim[modelIndex] = (uint16)animIndex;
        modelAnimCount[modelIndex] = 1;

        animFirstFrame[animIndex] = 0;
        animFrameCount[animIndex] = 1;
        animNextAnim[animIndex]   = (uint16)animIndex;  // it loops on itself
        animNextFrame[animIndex]  = 0;
        animTransStart[animIndex] = 0;
        animTransCount[animIndex] = 0;
        animState[animIndex]      = 0;
        animCmdStart[animIndex]   = 0;
        animCmdCount[animIndex]   = 0;
    }

    // The effects model (FX3). Upstream draws an effect as the sprite its
    // frameIndex names in the type's sequence; here every kind's frames are
    // one mesh -- a run of buildSpriteMesh's one-quad records, each exactly
    // SNES_SPRITE_MESH bytes, kind after kind -- under a single billboard
    // node marked SNES_NODE_SEQUENCE, and the model has an animation per kind
    // whose frame f carries, in its Y angle byte, which record of the run it
    // is. The GSU adds that times SNES_SPRITE_MESH to the node's mesh address
    // (mesh.gsu BillboardNode). The host plays a kind's animation with
    // frameIndex as upstream's (rate 1); the GSU clamps a frame past the end,
    // as it does any model's, so a bad index draws frame 0, never stray bytes.
    void buildFxModel(TR1_PC* level, MeshBlob& mesh, ModelBlob& out)
    {
        memset(&mesh, 0, sizeof(mesh));
        memset(&out, 0, sizeof(out));
        int32 total = 0;
        for (int32 k = 0; k < FX_KINDS; k++) {
            fxFrames[k] = 0;
            fxStart[k] = total;
            for (int32 j = 0; j < level->spriteSequencesCount; j++)
                if (level->spriteSequences[j].type == fxType(k))
                    fxFrames[k] = -level->spriteSequences[j].count;
            if (fxFrames[k] < 0 || fxFrames[k] > 64) {
                printf("effect type %d has %d frames\n", fxType(k), fxFrames[k]); exit(1);
            }
            total += fxFrames[k];
        }
        if (total == 0 || total > 255) { printf("effects: %d sprite frames\n", total); exit(1); }

        mesh.data = new uint8[total * SNES_SPRITE_MESH];
        memset(mesh.data, 0, total * SNES_SPRITE_MESH);
        int16 all[6] = { 0, 0, 0, 0, 0, 0 };
        int16 kindBox[FX_KINDS][6];
        int32 radius = 1;
        for (int32 k = 0; k < FX_KINDS; k++) {
            int32 start = -1;
            for (int32 j = 0; j < level->spriteSequencesCount; j++)
                if (level->spriteSequences[j].type == fxType(k)) start = level->spriteSequences[j].start;
            memset(kindBox[k], 0, sizeof(kindBox[k]));
            for (int32 f = 0; f < fxFrames[k]; f++) {
                MeshBlob quad;
                buildSpriteMesh(level, start + f, quad);
                uint8* p = mesh.data + (fxStart[k] + f) * SNES_SPRITE_MESH;
                if (quad.valid) {
                    if (quad.size != SNES_SPRITE_MESH) { printf("sprite mesh is %d bytes\n", quad.size); exit(1); }
                    memcpy(p, quad.data, SNES_SPRITE_MESH);
                    delete[] quad.data;
                } else {
                    p[0] = 4;                   // four vertices and no face:
                }                               // a frame with nothing to show
                const TR1_PC::SpriteTexture& spr = level->spriteTextures[start + f];
                const int16 b[6] = { spr.l, spr.r, spr.t, spr.b, spr.l, spr.r };
                for (int32 c = 0; c < 6; c++) {
                    if (f == 0) kindBox[k][c] = b[c];
                    else if (c & 1) kindBox[k][c] = std::max(kindBox[k][c], b[c]);
                    else            kindBox[k][c] = std::min(kindBox[k][c], b[c]);
                    if (c & 1) all[c] = std::max(all[c], b[c]);
                    else       all[c] = std::min(all[c], b[c]);
                }
                radius = std::max(radius, radiusByte(spr.l, spr.r, spr.t, spr.b, 0, 0));
            }
        }
        mesh.size = total * SNES_SPRITE_MESH;
        mesh.vCount = 4;
        mesh.faceCount = 1;
        mesh.valid = true;

        const int32 frameStride = 6 + 3;        // one node's worth
        const int32 animTableOff = SNES_MODEL_HDR + SNES_NODE_SIZE;
        out.data = new uint8[animTableOff + FX_KINDS * SNES_ANIM_SIZE];
        uint8* p = out.data;
        #define PUT16(v) { uint16 _t = (uint16)(v); memcpy(p, &_t, 2); p += 2; }
        #define PUT8(v)  { *p++ = (uint8)(v); }
        PUT8(1);                                // nodeCount
        PUT8(radius);
        PUT16(frameStride);
        PUT16(FX_KINDS);
        PUT16(animTableOff);
        PUT16(fxMesh);                          // patched to an address later
        PUT8(0);
        PUT8(SNES_NODE_ROOT | SNES_NODE_BILLBOARD | SNES_NODE_SEQUENCE);
        PUT8(radius);
        PUT16(0); PUT16(0); PUT16(0);
        for (int32 k = 0; k < FX_KINDS; k++) {
            PUT16(0);                           // frameBase, patched
            PUT8(SNES_INST_NONE);               // bank, patched; NONE: no frames
            PUT8(fxFrames[k]);
        }
        #undef PUT16
        #undef PUT8
        out.size = (int32)(p - out.data);
        out.animTableOff = animTableOff;
        out.nodeCount = 1;
        out.animCount = FX_KINDS;
        out.frameTotal = total;
        out.animOff    = new int32[FX_KINDS];
        out.animLen    = new int32[FX_KINDS];
        out.animFrames = new int32[FX_KINDS];
        out.frames = new uint8[total * frameStride];
        memset(out.frames, 0, total * frameStride);
        for (int32 k = 0; k < FX_KINDS; k++) {
            out.animOff[k] = fxStart[k] * frameStride;
            out.animLen[k] = fxFrames[k] * frameStride;
            out.animFrames[k] = fxFrames[k];
            for (int32 f = 0; f < fxFrames[k]; f++)
                out.frames[(fxStart[k] + f) * frameStride + 7] = uint8(fxStart[k] + f);  // AY

            const int32 a = fxAnim0 + k;
            for (int32 c = 0; c < 6; c++) animBox[a * 6 + c] = kindBox[k][c];
            animFirstFrame[a] = 0;
            animFrameCount[a] = uint16(fxFrames[k] ? fxFrames[k] : 1);
            animTickCount[a]  = uint8(fxFrames[k] ? fxFrames[k] : 1);
            animFrameRate[a]  = 1;
            animNextAnim[a]   = uint16(a);
            animNextFrame[a]  = 0;
            animTransStart[a] = 0;
            animTransCount[a] = 0;
            animState[a]      = 0;
            animCmdStart[a]   = 0;
            animCmdCount[a]   = 0;
        }
        out.valid = true;
        int16* box = modelBox + fxModel * 6;
        for (int32 c = 0; c < 6; c++) box[c] = all[c];
        modelFirstAnim[fxModel] = uint16(fxAnim0);
        modelAnimCount[fxModel] = FX_KINDS;
        printf("     effects: %d sprite frames (splash %d, bubble %d, blood %d, smoke %d, ricochet %d), %d bytes of mesh\n",
               total, fxFrames[0], fxFrames[1], fxFrames[2], fxFrames[3], fxFrames[4], mesh.size);
    }

    // Which model draws an entity of this type: the level's own where it has
    // one, and the synthetic billboard where the type is a sprite. Sprite
    // models are appended after the level's, so an index stays an index.
    int32 modelOfType(TR1_PC* level, int32 type)
    {
        if (fxModel >= 0 && isFxType(type)) return fxModel;
        for (int32 j = 0; j < level->modelsCount; j++) {
            if (level->models[j].type == type) return j;
        }
        for (int32 j = 0; j < spriteCount; j++) {
            if (spriteType[j] == type) return level->modelsCount + j;
        }
        return -1;
    }

    // ------------------------------------------------------------------
    // The level: every room, in one blob the GSU walks by itself.
    //
    // The host cannot help with this. While the GSU holds the ROM bus the
    // 65816 reads dummy data, so room traversal -- which is a walk over
    // portal records in ROM -- has to happen on the co-processor. That is why
    // the room table below is laid out for the GSU's addressing rather than
    // the CPU's: a bank byte the GSU puts in ROMBR, and a 16-bit address it
    // puts in R14.
    //
    //   uint16 roomCount
    //   uint16 pad
    //   RoomEntry rooms[roomCount]          32 bytes each, so index << 5
    //   ... room geometry and portal lists, packed into 64 KB GSU banks
    //
    // Rooms never straddle a bank: R14 is 16 bits and wraps inside ROMBR's
    // bank, so a record split across the boundary would read its own start.
    // ------------------------------------------------------------------

    static const int32 GSU_BANK_SIZE = 0x10000;

    // 32 bytes, so indexing a room is five doublings rather than a multiply.
    // The spare half is not waste worth removing: it is 456 bytes across the
    // level, and the next thing the renderer needs to know about a room will
    // want somewhere to live.
    struct RoomEntry {
        uint16 addr;            // geometry, within the bank
        uint16 portalAddr;      // portal list, within the same bank
        uint8  bank;            // GSU ROM bank, $40 and up
        uint8  portalCount;
        int16  x, y, z;         // world offset added to every vertex
        int16  yBottom;         // floor-most extent, relative to y
        uint16 sectorAddr;      // sector grid, within the same bank
        uint8  xSectors, zSectors;
        uint16 instAddr;        // static mesh placements, within the same bank
        uint8  instCount;
        uint8  itemCount;       // entity placements -- the count first, so the
        uint16 itemAddr;        // address stays two-byte aligned inside 32
        uint8  pad[8];
    };

    // One room's geometry, built in memory so it can be placed in a bank.
    struct RoomBlob {
        uint8* data;
        int32  size;
        int32  portalOffset;    // where the portal list starts inside data
        int32  sectorOffset;    // and the sector grid after it
        int32  instOffset;      // and the static mesh placements after that
        int32  instCount;
        int32  itemOffset;      // and the entity placements after that
        int32  itemCount;
        int32  x, y, z;
        int32  yBottom;
        int32  portalCount;
        int32  xSectors, zSectors;
        int32  flags;           // TR's room flags: bit 0 is water
        int32  vCount, qCount, tCount, faceCount;
    };

    int32 buildRoomFaces(TR1_PC* level, TR1_PC::Room* room, OutFace* faces)
    {
        int32 faceCount = 0;

        for (int32 i = 0; i < room->qCount; i++)
        {
            const TR1_PC::Quad& q = room->quads[i];
            int32 texIndex = q.flags & FACE_TEXTURE;
            if (texIndex >= level->objectTexturesCount) continue;

            const TR1_PC::ObjectTexture& t = level->objectTextures[texIndex];

            g_facesSeen++;
            if (t.attribute & TEX_ATTR_AKILL) g_akillFaces++;
            OutFace& o = faces[faceCount];
            o.page = snesTexturePage(t.tile, (t.attribute & TEX_ATTR_AKILL) != 0);
            o.count = 4;
            o.v[0] = q.indices[0];  o.u[0] = t.x0;  o.t[0] = t.y0;
            o.v[1] = q.indices[1];  o.u[1] = t.x1;  o.t[1] = t.y1;
            o.v[2] = q.indices[2];  o.u[2] = t.x2;  o.t[2] = t.y2;
            o.v[3] = q.indices[3];  o.u[3] = t.x3;  o.t[3] = t.y3;
            faceCount++;
        }

        for (int32 i = 0; i < room->tCount; i++)
        {
            const TR1_PC::Triangle& q = room->triangles[i];
            int32 texIndex = q.flags & FACE_TEXTURE;
            if (texIndex >= level->objectTexturesCount) continue;

            const TR1_PC::ObjectTexture& t = level->objectTextures[texIndex];

            g_facesSeen++;
            if (t.attribute & TEX_ATTR_AKILL) g_akillFaces++;
            OutFace& o = faces[faceCount];
            o.page = snesTexturePage(t.tile, (t.attribute & TEX_ATTR_AKILL) != 0);
            o.count = 3;
            o.v[0] = q.indices[0];  o.u[0] = t.x0;  o.t[0] = t.y0;
            o.v[1] = q.indices[1];  o.u[1] = t.x1;  o.t[1] = t.y1;
            o.v[2] = q.indices[2];  o.u[2] = t.x2;  o.t[2] = t.y2;
            faceCount++;
        }

        return faceCount;
    }

    // Room vertices keep TR's own frame -- x and z relative to the room
    // origin, y absolute -- shifted down by yTop so that y fits int16 with
    // room to spare. The offset that puts it back is in the room entry, and
    // the renderer folds it into the camera translation rather than
    // transforming it, which costs nothing per vertex.
    uint8* meshUsed;

    // Which slot in the host's array each of the level's items owns, or -1
    // for one with no model. Both tables have to agree about this or the
    // renderer reads another entity's state: the renderer walks items room by
    // room and the host walks them in level order, so neither can just count
    // as it goes. Computed once, up front, and read by both.
    int32* itemSlot;
    int32* slotItem;

    /* Entity types that are a sprite rather than a model, and which sprite.
       They become models of one quad each, appended after the level's own. */
    int32  spriteType[SNES_SPRITE_MAX];
    int32  spriteIndex[SNES_SPRITE_MAX];
    int32  spriteCount;

    /* FX3's effects (appendFxPool, buildFxModel, buildFxData): one model for
       every kind, a mesh of all their sprite frames, an animation per kind,
       and the pool of hidden entities ItemObj::add takes them from. */
    enum { FX_KINDS = 5, FX_POOL = 24 };
    int32  fxModel = -1, fxAnim0 = -1, fxMesh = -1;
    int32  fxPoolItems = 0;             // appended to level->items, last
    /* FX3's room sprites (upstream's room->info->sprites, renderSprite): a
       billboard quad mesh per sprite texture any room uses, placed as a
       static instance whose angle byte is ROOM_SPRITE_TURN, which no static
       has -- TR stands them on quarter turns -- and which the GSU's
       InstTurn reads as "face the camera". */
    std::vector<int32> roomSpriteTex;
    int32  roomSpriteMesh0 = -1;
    std::vector<int32> roomSpriteRooms, roomSpriteInst;   // per placement: room, offset in its blob
    std::vector<int32> roomSpriteWorld;                   // and x, y, z in the world
    enum { ROOM_SPRITE_TURN = 0x01 };
    int32  fxFrames[FX_KINDS] = {}, fxStart[FX_KINDS] = {};
    static bool isFxType(int32 type)
    {
        return type == 153 || type == 155 || type == 158 || type == 160 || type == 164;
    }

    // The animation table, in the host's terms rather than TR's. TR numbers
    // frames globally and chains animations by global frame number; the host
    // works in frames local to a model, because that is what the renderer is
    // handed. Converting once, here, saves the other side a divide per
    // transition and a table it would otherwise have to hold.
    uint16* animFirstFrame;     // per global anim: where its frames start
    uint16* animFrameCount;     // in the model's block
    uint8*  animTickCount;      // 30 Hz ticks, not quantised renderer poses
    uint8*  animFrameRate;
    uint16* animNextAnim;
    uint16* animNextFrame;
    uint16* modelFirstAnim;
    uint16* modelAnimCount;
    int16*  modelBox;           // six per model: minX maxX minY maxY minZ maxZ

    // The transitions out of each animation, flattened. TR keeps them as two
    // nested tables -- an animation lists the states it can reach, and each
    // of those lists the frame ranges it can be reached from -- which is two
    // loops on a processor that would rather have one. Here they are one
    // list per animation, in TR's own order so the first match still wins.
    // Triggers, resolved out of TR's FloorData: which entities standing on
    // a sector activates. Indexed by the sector's trigger word.
    uint8*  triggerData;        // SNES_TRIGGER_SIZE bytes each
    // FX3's camera actions, 4 bytes a trigger: camera index, its timer, once
    // | speed << 1, the target's slot ($FF: none) -- CAMERA.BIN's second half
    std::vector<uint8> triggerCam;
    bool    packFx3;
    int32   triggerCount;
    int32*  triggerOfFloor;     // floorIndex -> trigger, so a sector asked
                                // about twice gets one record

    // And the commands an animation carries: what it moves, kills, or makes a
    // noise about. TR packs them as a variable-length stream indexed per
    // animation; here they are fixed records, because the cartridge would
    // otherwise have to know each command's argument count to walk past it.
    /* The bounding box of an animation: the union of its frames'. Upstream's
       getBoundingBox is per *frame* -- TR puts one at the head of each -- and
       3901 of them is 47 KB, which the legacy GSU-2 host blob cannot hold.
       FX3 also ships exact current-frame boxes in its independent ANIM_BOX
       table. Keep this prefix for legacy interaction readers while they
       migrate individually: a larger union can incorrectly trigger states. */
    int16*  animBox;

    // Keep the original sector->box relation for runtime navigation and its
    // independent cartridge oracle. A rectangle scan cannot disambiguate
    // rooms on different floors that share the same X/Z coordinates.
    std::vector<uint8> buildNavData(TR1_PC* level, const char* dir)
    {
        if (level->boxesCount > 1024) {
            printf("navigation has %d boxes; OL_BOX_MAX is 1024\n", level->boxesCount); exit(1);
        }
        std::vector<uint8> tags;
        std::vector<uint16> literals;
        int32 bits = 1;
        while ((1 << bits) < level->boxesCount) bits++;
        char path[512];
        sprintf(path, "%s/NAV.json", dir);
        FILE* audit = fopen(path, "wb");
        if (!audit) { printf("can't write %s\n", path); exit(1); }
        fprintf(audit, "{\"rooms\":[");
        for (int32 r = 0; r < level->roomsCount; r++) {
            const TR1_PC::Room& room = level->rooms[r];
            fprintf(audit, "%s{\"x\":%d,\"z\":%d,\"width\":%d,\"depth\":%d,\"boxes\":[",
                r ? "," : "", room.info.x, room.info.z, room.xSectors, room.zSectors);
            for (int32 s = 0; s < room.xSectors * room.zSectors; s++) {
                int32 box = room.sectors[s].boxIndex;
                if (box != 0xFFFF && box >= level->boxesCount) {
                    printf("room %d sector %d has invalid navigation box %d\n", r, s, box); exit(1);
                }
                // X-major, Z-minor, like the four-byte host sectors. Never
                // predict across a room boundary or from the previous X row
                // as though it were the previous Z cell. NO_BOX is explicit.
                uint8 tag = 0;
                if (box != 0xFFFF) {
                    if (s % room.zSectors && box == room.sectors[s - 1].boxIndex) tag = 1;
                    else if (s >= room.zSectors && box == room.sectors[s - room.zSectors].boxIndex) tag = 2;
                    else { tag = 3; literals.push_back(uint16(box)); }
                }
                tags.push_back(tag);
                fprintf(audit, "%s%d", s ? "," : "", box);
            }
            // Raw source geometry, independent of the packed LINK codec.
            fprintf(audit, "],\"sectors\":[");
            for (int32 s = 0; s < room.xSectors * room.zSectors; s++) {
                const TR1_PC::Room::Sector& sector = room.sectors[s];
                fprintf(audit, "%s[%d,%d,%u,%u,%u]", s ? "," : "",
                    int(sector.floor), int(sector.ceiling), unsigned(sector.roomBelow),
                    unsigned(sector.roomAbove), unsigned(sector.floorIndex));
            }
            fprintf(audit, "],\"portals\":[");
            for (int32 p = 0; p < room.pCount; p++)
                fprintf(audit, "%s%d", p ? "," : "", room.portals[p].roomIndex);
            fprintf(audit, "]}");
        }
        fprintf(audit, "],\"floors\":[");
        for (int32 i = 0; i < level->floorsCount; i++)
            fprintf(audit, "%s%u", i ? "," : "", unsigned(level->floors[i].value));
        fprintf(audit, "],\"boxBounds\":[");
        for (int32 i = 0; i < level->boxesCount; i++) {
            const TR1_PC::Box& b = level->boxes[i];
            // Raw, inclusive PC bounds: independent of the packed byte form.
            fprintf(audit, "%s[%d,%d,%d,%d,%d,%d]", i ? "," : "",
                b.minZ, b.maxZ, b.minX, b.maxX, b.floor, b.overlap);
        }
        fprintf(audit, "],\"zones\":[");
        for (int32 flip = 0; flip < 2; flip++) {
            const uint16* maps[] = {level->zones[flip].ground1,
                level->zones[flip].ground2, level->zones[flip].fly};
            for (int32 kind = 0; kind < 3; kind++) {
                fprintf(audit, "%s[", flip || kind ? "," : "");
                for (int32 i = 0; i < level->boxesCount; i++)
                    fprintf(audit, "%s%u", i ? "," : "", unsigned(maps[kind][i]));
                fprintf(audit, "]");
            }
        }
        fprintf(audit, "]}\n");
        fclose(audit);
        const int32 sectors = int32(tags.size());
        if (sectors > 65535) {
            printf("navigation sector count %d exceeds host index\n", sectors); exit(1);
        }
        const int32 rankBase = 2 + (sectors + 3) / 4;
        const int32 literalBase = rankBase + ((sectors + 31) / 32 + 1) * 2;
        std::vector<uint8> data(literalBase + (literals.size() * bits + 7) / 8, 0);
        data[0] = 0x4E; data[1] = uint8(bits); // format N, literal width
        int32 rank = 0;
        for (int32 s = 0; s < sectors; s++) {
            if (!(s & 31)) {
                const int32 p = rankBase + (s / 32) * 2;
                data[p] = uint8(rank); data[p + 1] = uint8(rank >> 8);
            }
            data[2 + s / 4] |= tags[s] << ((s & 3) * 2);
            rank += tags[s] == 3;
        }
        data[literalBase - 2] = uint8(rank);
        data[literalBase - 1] = uint8(rank >> 8);
        for (size_t i = 0; i < literals.size(); i++) {
            for (int32 b = 0; b < bits; b++) {
                const size_t bit = i * bits + b;
                data[literalBase + bit / 8] |= ((literals[i] >> b) & 1) << (bit & 7);
            }
        }
        printf("  navigation: %d sectors, %d literals, %d-bit IDs, %d bytes (+2 footer)\n",
            sectors, int32(literals.size()), bits, int32(data.size()));
        return data;
    }

    // FX3 ZONE: six relative u16 offsets, then whole zone arrays. Identical
    // arrays share their offset, including normal/alternate maps. This is
    // lossless and lets Nav_init avoid a redundant second scan when equal.
    std::vector<uint8> buildZoneData(TR1_PC* level)
    {
        std::vector<uint8> data(12);
        std::map<std::vector<uint16>, uint16> offsets;
        for (int32 flip = 0; flip < 2; flip++) {
            const uint16* maps[] = {level->zones[flip].ground1,
                level->zones[flip].ground2, level->zones[flip].fly};
            for (int32 kind = 0; kind < 3; kind++) {
                std::vector<uint16> values(maps[kind], maps[kind] + level->boxesCount);
                auto found = offsets.find(values);
                uint16 offset;
                if (found != offsets.end()) offset = found->second;
                else {
                    if (data.size() + values.size() * 2 > 65536) {
                        printf("FX3 zone arrays exceed one CPU table bank\n"); exit(1);
                    }
                    offset = uint16(data.size());
                    offsets[values] = offset;
                    for (uint16 value : values) {
                        data.push_back(uint8(value)); data.push_back(uint8(value >> 8));
                    }
                }
                const int32 p = (flip * 3 + kind) * 2;
                data[p] = uint8(offset); data[p + 1] = uint8(offset >> 8);
            }
        }
        return data;
    }

    // FX3 keeps the legacy union prefix for coarse tests, followed by a
    // directory and tick maps to shared current-frame boxes. The CPU reads
    // these immutable records directly from FastROM, even while FX renders.
    // Offsets are relative to ANIM_BOX, not to the aggregate host image.
    std::vector<uint8> buildFrameBoxes(TR1_PC* level, int32 animsN,
                                       const ModelBlob* models, const char* dir)
    {
        std::vector<uint8> data(animsN * 14, 0);
        std::map<std::vector<int16>, int32> boxes;
        auto word = [&](int32 offset, int32 value) {
            if (value < 0 || value > 65535) {
                printf("frame box offset/count out of range: %d\n", value); exit(1);
            }
            data[offset] = uint8(value); data[offset + 1] = uint8(value >> 8);
        };
        for (int32 a = 0; a < animsN; a++) {
            std::vector<int16> box(animBox + a * 6, animBox + a * 6 + 6);
            boxes[box] = a * 12;
            for (int32 k = 0; k < 6; k++) word(a * 12 + k * 2, uint16(box[k]));
        }
        char path[512];
        sprintf(path, "%s/FRAMES.json", dir);
        FILE* audit = fopen(path, "wb");
        if (!audit) { printf("can't write %s\n", path); exit(1); }
        fprintf(audit, "{\"animations\":[");
        bool first = true;
        int32 ticksTotal = 0;
        for (int32 m = 0; m < level->modelsCount; m++) {
            const TR1_PC::Model& model = level->models[m];
            if (!models[m].valid) continue;
            for (int32 a = model.animIndex; a < model.animIndex + models[m].animCount; a++) {
                const TR1_PC::Animation& anim = level->anims[a];
                const int32 ticks = animTickCount[a];
                const int32 rate = anim.frameBegin == anim.frameEnd ? 1 : anim.frameRate;
                const int32 step = anim.frameSize ? anim.frameSize : 10 + model.count * 2;
                const int32 start = anim.frameOffset >> 1;
                if (rate < 1 || step < 6 || !ticks) {
                    printf("invalid animation %d for frame boxes\n", a); exit(1);
                }
                const int32 record = data.size();
                word(animsN * 12 + a * 2, record);
                data.resize(record + 2 + ticks * 2);
                word(record, ticks);
                int32 last = 0;
                for (int32 tick = 0; tick < ticks; tick++) {
                    int32 frame = tick / rate;
                    // Match fixed/item.h getFrames/getFrame, including its
                    // comparison against the original absolute frameEnd.
                    if (tick % rate > (rate >> 1) && (frame + 1) * rate < anim.frameEnd) frame++;
                    last = std::max(last, frame);
                    const int32 offset = start + frame * step;
                    if (offset < 0 || offset + 6 > level->frameDataSize) {
                        printf("animation %d tick %d has no frame box\n", a, tick); exit(1);
                    }
                    const int16* src = (const int16*)level->frameData + offset;
                    std::vector<int16> box(src, src + 6);
                    auto found = boxes.find(box);
                    int32 p;
                    if (found == boxes.end()) {
                        p = data.size(); boxes[box] = p;
                        for (int16 v : box) {
                            data.push_back(uint8(v)); data.push_back(uint8(uint16(v) >> 8));
                        }
                    } else p = found->second;
                    word(record + 2 + tick * 2, p);
                }
                fprintf(audit, "%s{\"anim\":%d,\"type\":%d,\"begin\":%d,\"end\":%d,\"rate\":%d,\"ticks\":%d,\"boxes\":[",
                    first ? "" : ",", a, model.type, anim.frameBegin, anim.frameEnd, rate, ticks);
                first = false;
                for (int32 f = 0; f <= last; f++) {
                    const int16* b = (const int16*)level->frameData + start + f * step;
                    fprintf(audit, "%s[%d,%d,%d,%d,%d,%d]", f ? "," : "", b[0], b[1], b[2], b[3], b[4], b[5]);
                }
                fprintf(audit, "]}"); ticksTotal += ticks;
            }
        }
        fprintf(audit, "]}\n"); fclose(audit);
        if (data.size() > 65536) {
            printf("FX3 frame boxes exceed one CPU table bank: %d\n", int32(data.size())); exit(1);
        }
        printf(" frame boxes: %d ticks, %d unique boxes, %d bytes in FX3 ROM\n",
            ticksTotal, int32(boxes.size()), int32(data.size()));
        return data;
    }

    // Camera::update/lookAt consume four values from getBoundingBox(true).
    // Bake the GBA's interpolation and wide arithmetic into ROM, so the CPU
    // only indexes a tick. CAMERA.BIN's existing camera/trigger prefix stays
    // intact. The appended directory is u16 animCount, u16 offsets[animCount][4]
    // (absolute within CAMERA.BIN; zero = unavailable), for followY, middleY,
    // middleX, middleZ. Scalar streams have u8 ticks, u8 bits, i16 minimum,
    // then little-endian packed unsigned deltas from that minimum. Constant
    // streams use ticks=bits=0. Equal streams share storage; three final guard
    // bytes keep both word loads for the last 24-bit window inside the table.
    void buildCameraPoses(TR1_PC* level, int32 animsN, const ModelBlob* models,
                          std::vector<uint8>& data)
    {
        const int32 directory = data.size();
        data.resize(directory + 2 + animsN * 8, 0);
        auto word = [&](int32 at, int32 v) {
            data[at] = uint8(v); data[at + 1] = uint8(v >> 8);
        };
        word(directory, animsN);
        std::map<std::vector<int16>, int32> streams;
        int32 ticksTotal = 0;
        for (int32 m = 0; m < level->modelsCount; m++) {
            const TR1_PC::Model& model = level->models[m];
            if (!models[m].valid) continue;
            for (int32 a = model.animIndex; a < model.animIndex + models[m].animCount; a++) {
                const TR1_PC::Animation& anim = level->anims[a];
                const int32 ticks = animTickCount[a];
                const int32 rate = anim.frameBegin == anim.frameEnd ? 1 : anim.frameRate;
                const int32 step = 10 + model.count * 2;
                if (rate < 1 || !ticks || (anim.frameSize && anim.frameSize != step)) {
                    printf("invalid animation %d for camera interpolation\n", a); exit(1);
                }
                std::vector<int16> values;
                for (int32 tick = 0; tick < ticks; tick++) {
                    int32 fa = tick / rate, fb = fa + 1, delta = tick % rate;
                    // Preserve getFrames' absolute frameEnd comparison.
                    if (fb * rate >= anim.frameEnd) fb = fa;
                    if (anim.frameBegin == anim.frameEnd) fa = fb = delta = 0;
                    const int32 start = anim.frameOffset >> 1;
                    if (start + fb * step + 6 > level->frameDataSize) {
                        printf("animation %d tick %d has no camera frame\n", a, tick); exit(1);
                    }
                    const int16* pa = (const int16*)level->frameData + start + fa * step;
                    const int16* pb = (const int16*)level->frameData + start + fb * step;
                    // fixed/common.cpp's GBA divTable[2] is $7FFF, not $8000.
                    const int32 factor = std::min(32767, 65536 / rate) * delta;
                    int32 b[6];
                    for (int32 k = 0; k < 6; k++)
                        b[k] = int16(pa[k] + ((int32(pb[k]) - pa[k]) * factor >> 16));
                    values.push_back(int16(b[3] + ((b[2] - b[3]) * 3 >> 2)));
                    values.push_back(int16((b[2] + b[3]) >> 1));
                    values.push_back(int16((b[0] + b[1]) >> 1));
                    values.push_back(int16((b[4] + b[5]) >> 1));
                }
                for (int32 component = 0; component < 4; component++) {
                    std::vector<int16> column;
                    for (int32 tick = 0; tick < ticks; tick++) column.push_back(values[tick * 4 + component]);
                    const int32 low = *std::min_element(column.begin(), column.end());
                    const int32 range = int32(*std::max_element(column.begin(), column.end())) - low;
                    if (!range) column.resize(1);
                    auto found = streams.find(column);
                    int32 at;
                    if (found != streams.end()) at = found->second;
                    else {
                        int32 bits = 0;
                        while ((uint32(1) << bits) <= uint32(range)) bits++;
                        at = data.size();
                        const int32 bytes = (bits * column.size() + 7) / 8;
                        if (at + 4 + bytes + 3 > 65535) {
                            printf("CAMERA.BIN interpolated poses exceed one bank\n"); exit(1);
                        }
                        streams[column] = at;
                        data.resize(at + 4 + bytes, 0);
                        data[at] = range ? ticks : 0;
                        data[at + 1] = bits;
                        word(at + 2, low);
                        for (int32 tick = 0; tick < int32(column.size()); tick++) {
                            const uint32 delta = int32(column[tick]) - low;
                            for (int32 bit = 0; bit < bits; bit++)
                                if (delta & (uint32(1) << bit)) {
                                    const int32 pos = tick * bits + bit;
                                    data[at + 4 + pos / 8] |= 1 << (pos & 7);
                                }
                        }
                    }
                    word(directory + 2 + a * 8 + component * 2, at);
                }
                ticksTotal += ticks;
            }
        }
        data.push_back(0); data.push_back(0); data.push_back(0);
        printf(" camera poses: %d ticks, %d shared streams, %d bytes\n",
               ticksTotal, int32(streams.size()), int32(data.size()) - directory);
    }

    // weaponGetAimPoint needs only three local coordinates from a creature's
    // current box. Resolve getFrame's nearest-pose choice once per game tick
    // and share identical points; shipping every six-word box would not fit.
    std::vector<uint8> buildAimData(TR1_PC* level, int32 animsN,
                                    const ModelBlob* models, const char* dir)
    {
        std::vector<uint8> data(2 + animsN * 2, 0);
        std::map<std::vector<int16>, int32> points;
        auto word = [&](int32 offset, int32 value) {
            if (value < 0 || value > 65535) {
                printf("aim table offset/count out of range: %d\n", value); exit(1);
            }
            data[offset] = uint8(value); data[offset + 1] = uint8(value >> 8);
        };
        word(0, animsN);
        char path[512];
        sprintf(path, "%s/AIM.json", dir);
        FILE* audit = fopen(path, "wb");
        if (!audit) { printf("can't write %s\n", path); exit(1); }
        fprintf(audit, "{\"animations\":[");
        bool first = true;
        int32 ticksTotal = 0;
        for (int32 m = 0; m < level->modelsCount; m++) {
            const TR1_PC::Model& model = level->models[m];
            // Creature models from wolf through Adam, including those whose
            // AI is still to be ported. Other types keep their animation union.
            if (model.type < ITEM_WOLF || model.type > ITEM_ADAM || !models[m].valid)
                continue;
            for (int32 a = model.animIndex; a < model.animIndex + models[m].animCount; a++) {
                const TR1_PC::Animation& anim = level->anims[a];
                const int32 ticks = animTickCount[a];
                const int32 rate = anim.frameBegin == anim.frameEnd ? 1 : anim.frameRate;
                const int32 step = anim.frameSize ? anim.frameSize : 10 + model.count * 2;
                const int32 start = anim.frameOffset >> 1;
                if (rate < 1 || step < 6) {
                    printf("invalid creature animation %d for aiming\n", a); exit(1);
                }
                const int32 record = data.size();
                word(2 + a * 2, record);
                data.resize(record + 2 + ticks * 2);
                word(record, ticks);
                int32 last = 0;
                for (int32 tick = 0; tick < ticks; tick++) {
                    int32 frame = tick / rate;
                    // ItemObj::getFrames tests indexB against the original
                    // stored frameEnd. Do not replace it with the local end.
                    if (tick % rate > (rate >> 1) && (frame + 1) * rate < anim.frameEnd)
                        frame++;
                    if (frame > last) last = frame;
                    const int32 offset = start + frame * step;
                    if (offset < 0 || offset + 6 > level->frameDataSize) {
                        printf("animation %d tick %d has no aim box\n", a, tick); exit(1);
                    }
                    const int16* box = (const int16*)level->frameData + offset;
                    std::vector<int16> point(3);
                    point[0] = int16((int32(box[0]) + box[1]) >> 1);
                    point[1] = int16(int32(box[2]) + (int32(box[3]) - box[2]) / 3);
                    point[2] = int16((int32(box[4]) + box[5]) >> 1);
                    auto found = points.find(point);
                    int32 p;
                    if (found == points.end()) {
                        p = data.size();
                        points[point] = p;
                        for (int32 k = 0; k < 3; k++) {
                            data.push_back(uint8(point[k]));
                            data.push_back(uint8(uint16(point[k]) >> 8));
                        }
                    } else {
                        p = found->second;
                    }
                    word(record + 2 + tick * 2, p);
                }
                // Raw source boxes and timing, rather than expected points,
                // let the cartridge oracle compute its own frame and aim.
                fprintf(audit, "%s{\"anim\":%d,\"type\":%d,\"begin\":%d,\"end\":%d,\"rate\":%d,\"boxes\":[",
                        first ? "" : ",", a, model.type, anim.frameBegin, anim.frameEnd, rate);
                first = false;
                for (int32 f = 0; f <= last; f++) {
                    const int16* box = (const int16*)level->frameData + start + f * step;
                    fprintf(audit, "%s[%d,%d,%d,%d,%d,%d]", f ? "," : "",
                            box[0], box[1], box[2], box[3], box[4], box[5]);
                }
                fprintf(audit, "]}");
                ticksTotal += ticks;
            }
        }
        fprintf(audit, "]}\n");
        fclose(audit);
        printf("      aiming: %d ticks, %d unique points, %d bytes\n",
                ticksTotal, int32(points.size()), int32(data.size()));
        return data;
    }

    // ItemObj::getSpheres for collideSpheres, which GBA/32X (FAST_HITMASK)
    // run only for a swinging blade against Lara. Exact upstream data: each
    // mesh's sphere and each node's flags and offset, and per game tick of
    // every animation of the two models the stored frame getFrame picks --
    // root offset and TR's packed 10-bit angles, deduplicated. Layout (LE):
    //   upstream's gSinCosTable, 4096 u32 (sin << 16 | cos & $FFFF), which
    //   is round(sin/cos(2 pi k / 4096) * 16384) exactly
    //   u16 modelCount, at 16384
    //   per model, 12 bytes: u16 type, u16 meshCount, u32 meshRecords,
    //     u16 firstAnim, u16 animCount
    //   mesh records, 16 bytes: int16 cx, cy, cz, radius; u16 flags;
    //     int16 x, y, z (the node's; mesh 0's are zero)
    //   per model, per anim: u32 tickDirectory
    //   tick directory: u16 ticks; ticks x u32 frame offset
    //   frames: int16 x, y, z; meshCount x u32 angles, w0 | (w1 << 16) as GBA reads
    //     Lara's frames then carry, per mesh, the fast path's yaw-0 centre:
    //     int16 e03 >> 10, e13 >> 14, e23 >> 10 (x and z in sixteenths)
    //   the fast path, at 16410: u16 bound, u16 yawCount, 4 x { u16 yaw,
    //     u16 0, u32 table } -- a table per distinct blade yaw: per blade
    //     anim u16 tickBase, u16 ticks; per tick meshCount x int16 x, y, z,
    //     the exact centre less the blade's position
    // No record crosses a 64 KiB boundary: the host binds a bank per record.
    //
    // The fast path (src/trap_fx3.asm): at pitch 0 the chain's row 1 never
    // sees the yaw, so her centre's y is the yaw-0 one exactly, and x and z
    // are the yaw-0 centre turned by the yaw's table sine and cosine to
    // within `bound` units -- proven here over every stored frame and all
    // 4,096 table yaws (and again against upstream's own code by
    // tools/spherecheck.cpp --table in `make check`).
    struct SphereChain {
        int32 stack[16][12];
        int32 top;
        const int16* sinCos;        // cos, sin per table entry

        void rot(int32 angle, int32 a, int32 b)
        {
            const int32 k = (angle & 0xFFFF) >> 4;
            const int32 c = sinCos[k * 2], s = sinCos[k * 2 + 1];
            int32* e = stack[top];
            for (int32 r = 0; r < 12; r += 4) {
                const int32 x = e[a + r], y = e[b + r];
                e[a + r] = (x * c - y * s) >> 14;
                e[b + r] = (y * c + x * s) >> 14;
            }
        }
        void trans(int32 x, int32 y, int32 z)
        {
            int32* e = stack[top];
            for (int32 r = 0; r < 12; r += 4) e[r + 3] += e[r] * x + e[r + 1] * y + e[r + 2] * z;
        }
        void frame(int32 x, int32 y, int32 z, uint32 v)
        {
            const uint32 hi = v >> 16, lo = v & 0xFFFF;
            const int16 ax = int16((hi & 0x3FF0) << 2);
            const int16 ay = int16(((hi & 0x000F) << 12) | ((lo & 0xFC00) >> 4));
            const int16 az = int16((lo & 0x03FF) << 6);
            trans(x, y, z);
            if (ay) rot(ay, 0, 2);
            if (ax) rot(ax, 2, 1);
            if (az) rot(az, 1, 0);
        }
        void push() { if (top + 1 < 16) { memcpy(stack[top + 1], stack[top], sizeof(stack[0])); top++; } }
        void pop() { if (top) top--; }

        // ItemObj::getJoint(jointIndex, offset) for an item turned by
        // matrixRotateYXZ(ax, ay, az): the raw e03, e13, e23.
        void joint(TR1_PC* level, const TR1_PC::Model& model, const uint16* f, int32 jointIndex,
                   int32 ox, int32 oy, int32 oz, int32 ax, int32 ay, int32 az, int32* out)
        {
            top = 0;
            memset(stack[0], 0, sizeof(stack[0]));
            stack[0][0] = stack[0][5] = stack[0][10] = 0x4000;
            if (int16(ay)) rot(ay, 0, 2);
            if (int16(ax)) rot(ax, 2, 1);
            if (int16(az)) rot(az, 1, 0);
            frame(int16(f[6]), int16(f[7]), int16(f[8]), uint32(f[10]) | (uint32(f[11]) << 16));
            for (int32 i = 0; i < jointIndex; i++) {
                const TR1_PC::Node* node = (const TR1_PC::Node*)(level->nodesData
                    + model.nodeIndex + i * 4);
                if (node->flags & 1) pop();
                if (node->flags & 2) push();
                frame(int16(node->pos.x), int16(node->pos.y), int16(node->pos.z),
                      uint32(f[12 + i * 2]) | (uint32(f[13 + i * 2]) << 16));
            }
            trans(ox, oy, oz);
            out[0] = stack[top][3]; out[1] = stack[top][7]; out[2] = stack[top][11];
        }

        // getSpheres(.., true) at pitch 0: the raw e03, e13, e23 per mesh.
        void run(TR1_PC* level, const TR1_PC::Model& model, const uint16* f, int32 yaw, int32* out)
        {
            top = 0;
            memset(stack[0], 0, sizeof(stack[0]));
            stack[0][0] = stack[0][5] = stack[0][10] = 0x4000;
            if (int16(yaw)) rot(yaw, 0, 2);
            for (int32 i = 0; i < model.count; i++) {
                const uint32 angles = uint32(f[10 + i * 2]) | (uint32(f[11 + i * 2]) << 16);
                const int16* mesh = (const int16*)((const uint8*)level->meshData
                    + level->meshOffsets[model.start + i]);
                if (i == 0) {
                    frame(int16(f[6]), int16(f[7]), int16(f[8]), angles);
                } else {
                    const TR1_PC::Node* node = (const TR1_PC::Node*)(level->nodesData
                        + model.nodeIndex + (i - 1) * 4);
                    if (node->flags & 1) pop();
                    if (node->flags & 2) push();
                    frame(int16(node->pos.x), int16(node->pos.y), int16(node->pos.z), angles);
                }
                push();
                trans(mesh[0], mesh[1], mesh[2]);
                out[i * 3] = stack[top][3];
                out[i * 3 + 1] = stack[top][7];
                out[i * 3 + 2] = stack[top][11];
                pop();
            }
        }
    };

    std::vector<uint8> buildSphereData(TR1_PC* level, const ModelBlob* models)
    {
        std::vector<uint8> data;
        auto put16 = [&](int32 v) { data.push_back(uint8(v)); data.push_back(uint8(v >> 8)); };
        auto within = [&](int32 bytes) {
            if ((int32(data.size()) & 0xFFFF) + bytes > 0x10000)
                data.resize((data.size() | 0xFFFF) + 1, 0);
        };
        auto put32 = [&](int32 v) { put16(v & 0xFFFF); put16((v >> 16) & 0xFFFF); };
        auto set32 = [&](int32 at, int32 v) {
            for (int32 k = 0; k < 4; k++) data[at + k] = uint8(v >> (k * 8));
        };
        const int32 types[2] = { ITEM_LARA, 36 };
        int32 index[2] = { -1, -1 };
        for (int32 m = 0; m < level->modelsCount; m++)
            for (int32 t = 0; t < 2; t++)
                if (int32(level->models[m].type) == types[t] && models[m].valid) index[t] = m;
        if (index[0] < 0 || index[1] < 0) return data;
        bool blades = false;
        for (int32 i = 0; i < level->itemsCount; i++) blades |= level->items[i].type == 36;
        if (!blades) return data;

        std::vector<int16> sinCos(8192);
        for (int32 k = 0; k < 4096; k++) {
            const double a = k * 6.283185307179586 / 4096.0;
            const int32 sn = int32(lround(sin(a) * 16384.0)), cs = int32(lround(cos(a) * 16384.0));
            put16(cs); put16(sn);
            sinCos[k * 2] = int16(cs); sinCos[k * 2 + 1] = int16(sn);
        }
        SphereChain* chain = new SphereChain();
        chain->sinCos = sinCos.data();
        put16(2);
        int32 header = data.size();
        data.resize(header + 2 * 12, 0);
        const int32 fastHeader = data.size();       // 16410
        data.resize(fastHeader + 36, 0);
        std::map<std::vector<uint8>, int32> frames;
        std::vector<std::pair<int32, const uint16*> > laraFrames;
        std::vector<std::vector<const uint16*> > bladeTicks;
        std::vector<int32> dirSlots[2];
        for (int32 t = 0; t < 2; t++) {
            const TR1_PC::Model& model = level->models[index[t]];
            const int32 h = header + t * 12;
            data[h] = uint8(model.type); data[h + 1] = 0;
            data[h + 2] = uint8(model.count); data[h + 3] = 0;
            set32(h + 4, data.size());
            data[h + 8] = uint8(model.animIndex); data[h + 9] = uint8(model.animIndex >> 8);
            data[h + 10] = uint8(models[index[t]].animCount); data[h + 11] = 0;
            for (int32 i = 0; i < model.count; i++) {
                const int16* mesh = (const int16*)((const uint8*)level->meshData
                    + level->meshOffsets[model.start + i]);
                put16(mesh[0]); put16(mesh[1]); put16(mesh[2]); put16(mesh[3]);
                if (i == 0) { put16(0); put16(0); put16(0); put16(0); continue; }
                const TR1_PC::Node* node = (const TR1_PC::Node*)(level->nodesData
                    + model.nodeIndex + (i - 1) * 4);
                put16(node->flags); put16(node->pos.x); put16(node->pos.y); put16(node->pos.z);
            }
            for (int32 a = 0; a < models[index[t]].animCount; a++) {
                dirSlots[t].push_back(data.size());
                put32(0);
            }
        }
        // src/trap_fx3.asm reads the headers, mesh records and directory
        // slots in the data's first bank.
        if (data.size() > 0x10000) { printf("sphere headers past the first bank\n"); exit(1); }
        int32 unique = 0, ticksTotal = 0;
        for (int32 t = 0; t < 2; t++) {
            const TR1_PC::Model& model = level->models[index[t]];
            for (int32 k = 0; k < models[index[t]].animCount; k++) {
                const int32 a = model.animIndex + k;
                const TR1_PC::Animation& anim = level->anims[a];
                const int32 ticks = animTickCount[a];
                const int32 rate = anim.frameBegin == anim.frameEnd ? 1 : anim.frameRate;
                const int32 step = anim.frameSize ? anim.frameSize : 10 + model.count * 2;
                const int32 start = anim.frameOffset >> 1;
                within(2 + ticks * 4);
                set32(dirSlots[t][k], data.size());
                int32 directory = data.size();
                put16(ticks);
                data.resize(directory + 2 + ticks * 4, 0);
                if (t == 1) bladeTicks.push_back(std::vector<const uint16*>());
                for (int32 tick = 0; tick < ticks; tick++) {
                    int32 frame = tick / rate;
                    // getFrames tests indexB against the stored frameEnd, as
                    // buildAimData does.
                    if (tick % rate > (rate >> 1) && (frame + 1) * rate < anim.frameEnd)
                        frame++;
                    const int32 offset = start + frame * step;
                    if (offset < 0 || offset + 10 + model.count * 2 > level->frameDataSize) {
                        printf("sphere frame out of range: anim %d tick %d\n", a, tick); exit(1);
                    }
                    const uint16* f = (const uint16*)level->frameData + offset;
                    std::vector<uint8> key;
                    for (int32 w = 6; w < 9; w++) { key.push_back(uint8(f[w])); key.push_back(uint8(f[w] >> 8)); }
                    for (int32 i = 0; i < model.count; i++) {
                        // GBA reads the pair as a little-endian uint32 from
                        // TR's words (frame->angles + 1); DECODE_ANGLES takes
                        // X from the second word.
                        uint32 v = f[10 + i * 2] | (uint32(f[11 + i * 2]) << 16);
                        for (int32 b = 0; b < 4; b++) key.push_back(uint8(v >> (b * 8)));
                    }
                    if (t == 1) bladeTicks.back().push_back(f);
                    int32 at;
                    auto found = frames.find(key);
                    if (found == frames.end()) {
                        const int32 extra = t == 0 ? model.count * 6 : 0;
                        within(int32(key.size()) + extra);
                        at = data.size();
                        frames[key] = at;
                        data.insert(data.end(), key.begin(), key.end());
                        if (t == 0) {
                            int32 raw[3 * 64];
                            chain->run(level, model, f, 0, raw);
                            for (int32 i = 0; i < model.count; i++) {
                                const int32 v[3] = { raw[i * 3] >> 10, raw[i * 3 + 1] >> 14, raw[i * 3 + 2] >> 10 };
                                for (int32 c = 0; c < 3; c++) {
                                    if (v[c] < -32767 || v[c] > 32767) {
                                        printf("sphere centre out of range: anim %d tick %d mesh %d\n", a, tick, i);
                                        exit(1);
                                    }
                                    put16(v[c]);
                                }
                            }
                            laraFrames.push_back(std::make_pair(at, f));
                        }
                        unique++;
                    } else at = found->second;
                    set32(directory + 2 + tick * 4, at);
                }
                ticksTotal += ticks;
            }
        }

        // The bound: every stored frame of hers at every table yaw.
        const TR1_PC::Model& lara = level->models[index[0]];
        int32 bound = 0;
        for (size_t n = 0; n < laraFrames.size(); n++) {
            const int32 at = laraFrames[n].first + 6 + lara.count * 4;
            int32 raw[3 * 64];
            for (int32 k = 0; k < 4096; k++) {
                chain->run(level, lara, laraFrames[n].second, k << 4, raw);
                const int32 c = sinCos[k * 2], s = sinCos[k * 2 + 1];
                for (int32 i = 0; i < lara.count; i++) {
                    const uint8* p = &data[at + i * 6];
                    const int32 x0 = int16(p[0] | (p[1] << 8)), y0 = int16(p[2] | (p[3] << 8));
                    const int32 z0 = int16(p[4] | (p[5] << 8));
                    if ((raw[i * 3 + 1] >> 14) != y0) {
                        printf("sphere y depends on the yaw: frame %d mesh %d\n", int32(n), i);
                        exit(1);
                    }
                    const int32 ex = abs(((c * x0 + s * z0) >> 18) - (raw[i * 3] >> 14));
                    const int32 ez = abs(((c * z0 - s * x0) >> 18) - (raw[i * 3 + 2] >> 14));
                    bound = std::max(bound, std::max(ex, ez));
                }
            }
        }

        // The blades' own spheres, exact, per distinct yaw (a blade never turns).
        const TR1_PC::Model& blade = level->models[index[1]];
        std::vector<uint16> yaws;
        for (int32 i = 0; i < level->itemsCount; i++) {
            if (level->items[i].type != 36) continue;
            const uint16 yaw = uint16(level->items[i].angleY);
            if (std::find(yaws.begin(), yaws.end(), yaw) == yaws.end() && yaws.size() < 4)
                yaws.push_back(yaw);
        }
        int32 bladeTickCount = 0;
        for (size_t k = 0; k < bladeTicks.size(); k++) bladeTickCount += int32(bladeTicks[k].size());
        for (size_t y = 0; y < yaws.size(); y++) {
            within(int32(bladeTicks.size()) * 4 + bladeTickCount * blade.count * 6);
            const int32 table = data.size();
            set32(fastHeader + 4 + int32(y) * 8, 0);
            data[fastHeader + 4 + y * 8] = uint8(yaws[y]);
            data[fastHeader + 5 + y * 8] = uint8(yaws[y] >> 8);
            set32(fastHeader + 8 + int32(y) * 8, table);
            int32 base = 0;
            for (size_t k = 0; k < bladeTicks.size(); k++) {
                put16(base); put16(int32(bladeTicks[k].size()));
                base += int32(bladeTicks[k].size());
            }
            for (size_t k = 0; k < bladeTicks.size(); k++)
                for (size_t tick = 0; tick < bladeTicks[k].size(); tick++) {
                    int32 raw[3 * 64];
                    chain->run(level, blade, bladeTicks[k][tick], int16(yaws[y]), raw);
                    for (int32 i = 0; i < blade.count * 3; i++) put16(raw[i] >> 14);
                }
        }
        data[fastHeader] = uint8(bound); data[fastHeader + 1] = uint8(bound >> 8);
        data[fastHeader + 2] = uint8(yaws.size()); data[fastHeader + 3] = 0;
        delete chain;
        printf("      spheres: Lara and blade, %d ticks, %d unique frames, %d bytes; "
               "fast path within %d, %d blade yaws\n",
               ticksTotal, unique, int32(data.size()), bound, int32(yaws.size()));
        return data;
    }

    /* An animation's speed and acceleration, TR's own, in 16.16 -- which is
       where Lara's movement comes from: `ItemObj::move` reads them to get her
       horizontal speed, so a walk and a run differ because their animations
       do, not because anything says so. Eight bytes an animation, in a table
       of their own rather than widened into the record above: that one is
       read six times a frame per entity and a sixteen-byte stride is a shift
       where a twenty-four-byte one is a multiply. */
    int32*  animSpeed;      // two per animation: speed then accel

    uint16* animCmdStart;
    uint8*  animCmdCount;
    uint8*  cmdData;            // SNES_CMD_SIZE bytes each
    int32   cmdCount;

    uint16* animTransStart;
    uint8*  animTransCount;
    uint8*  animState;
    uint8*  transData;          // TRANS_SIZE bytes each
    int32   transCount;

    // FX.BIN (FX3 CPU ROM, olLevel.fxAt/fxBank): what src/fx.c needs beyond
    // the host tables. Layout (LE, offsets from the file's start):
    //   u16 poolFirst, poolCount     the hidden effect entities' slots
    //   u16 frames[5]                sprite frames per kind (0: absent)
    //   u16 siteCount
    //   per site, 6 bytes: u16 firstAnim, animCount, directory (0: none)
    //   directory: animCount x u16 tick map (0: never asked of that anim)
    //   tick map: u16 ticks; ticks x u16 point
    //   points: int16 x, y, z in sixteenths of a unit, at angle 0
    // A site is a getJoint(joint, offset) upstream asks: the creatures' bite
    // (Enemy::bite's blood) and her head for FX_LARA_BUBBLES. The tick map
    // resolves getFrame's nearest-keyframe choice per game tick, as the aim
    // and frame-box tables do. The host turns the point by the item's
    // angles on its own sine table (fx.c Fx_joint); the chain is upstream's
    // int32 one from the identity, so the host's answer is upstream's to
    // within the bound printed here, measured over every stored point and a
    // sweep of the angles the site can have.
    struct FxSite { int32 type, joint, ox, oy, oz; };

    std::vector<uint8> buildFxData(TR1_PC* level, const ModelBlob* models)
    {
        static const FxSite sites[] = {
            {  7,  6, 0, -14, 174 },    // Wolf: STATE_ATTACK and STATE_BITE
            {  8, 14, 0,  96, 335 },    // Bear: STATE_BITE
            {  9,  4, 0,  16,  45 },    // Bat: STATE_ATTACK
            { 19, 22, 0,  66, 318 },    // Raptor: ATTACK_1/2, BITE
            {  0, 14, 0,   0,  50 },    // Lara: JOINT_HEAD, fxBubbles
        };
        const int32 siteCount = int32(sizeof(sites) / sizeof(sites[0]));
        std::vector<uint8> data;
        auto put16 = [&](int32 v) { data.push_back(uint8(v)); data.push_back(uint8(v >> 8)); };
        auto set16 = [&](int32 at, int32 v) {
            if (v < 0 || v > 0xFFFF) { printf("FX.BIN offset out of range: %d\n", v); exit(1); }
            data[at] = uint8(v); data[at + 1] = uint8(v >> 8);
        };
        int32 poolFirst = -1, poolCount = 0;
        for (int32 i = level->itemsCount - fxPoolItems; i < level->itemsCount; i++) {
            const int32 s = itemSlot[i];
            if (s < 0) { printf("effect entity %d has no slot (%d entities)\n", i, level->itemsCount); exit(1); }
            if (poolFirst < 0) poolFirst = s;
            if (s != poolFirst + poolCount) { printf("effect slots are not consecutive\n"); exit(1); }
            poolCount++;
        }
        put16(poolFirst < 0 ? 0 : poolFirst);
        put16(poolCount);
        for (int32 k = 0; k < FX_KINDS; k++) put16(fxFrames[k]);
        put16(siteCount);
        const int32 siteAt = data.size();
        data.resize(siteAt + siteCount * 6, 0);

        // upstream's gSinCosTable, and the host's 1024-entry olSinTable
        std::vector<int16> sinCos(8192);
        for (int32 k = 0; k < 4096; k++) {
            const double a = k * 6.283185307179586 / 4096.0;
            sinCos[k * 2] = int16(lround(cos(a) * 16384.0));
            sinCos[k * 2 + 1] = int16(lround(sin(a) * 16384.0));
        }
        auto hostSin = [](int32 angle) {
            const int32 i = ((uint16(angle + 32)) >> 6) & 1023;
            return int32(lround(sin(6.283185307179586 * i / 1024.0) * 16384.0));
        };
        auto hostCos = [&](int32 angle) { return hostSin(angle + 0x4000); };
        SphereChain* chain = new SphereChain();
        chain->sinCos = sinCos.data();

        std::map<std::vector<int16>, int32> points;
        int32 bound = 0, ticksTotal = 0;
        for (int32 s = 0; s < siteCount; s++) {
            const FxSite& site = sites[s];
            int32 m = -1;
            for (int32 j = 0; j < level->modelsCount; j++)
                if (int32(level->models[j].type) == site.type && models[j].valid) m = j;
            if (m < 0) continue;
            const TR1_PC::Model& model = level->models[m];
            if (site.joint >= model.count) { printf("joint %d of type %d\n", site.joint, site.type); exit(1); }
            const int32 animCount = models[m].animCount;
            set16(siteAt + s * 6, model.animIndex);
            set16(siteAt + s * 6 + 2, animCount);
            set16(siteAt + s * 6 + 4, data.size());
            const int32 dir = data.size();
            data.resize(dir + animCount * 2, 0);
            for (int32 k = 0; k < animCount; k++) {
                const int32 a = model.animIndex + k;
                const TR1_PC::Animation& anim = level->anims[a];
                if (site.type == ITEM_LARA) {       // only where FX_LARA_BUBBLES fires
                    bool bubbles = false;
                    int32 ci = anim.commandsStart;
                    for (int32 n = 0; n < anim.commandsCount && ci >= 0 && ci < level->commandsCount; n++) {
                        const int32 op = level->commands[ci++];
                        const int32 args = op == 1 ? 3 : (op == 2 || op == 5 || op == 6) ? 2 : 0;
                        if (op == 6 && ci + 1 < level->commandsCount && (level->commands[ci + 1] & 0x3FFF) == 3)
                            bubbles = true;
                        ci += args;
                    }
                    if (!bubbles) continue;
                }
                const int32 ticks = animTickCount[a];
                const int32 rate = anim.frameBegin == anim.frameEnd ? 1 : anim.frameRate;
                const int32 step = anim.frameSize ? anim.frameSize : 10 + model.count * 2;
                const int32 start = anim.frameOffset >> 1;
                set16(dir + k * 2, data.size());
                const int32 map = data.size();
                put16(ticks);
                data.resize(map + 2 + ticks * 2, 0);
                for (int32 tick = 0; tick < ticks; tick++) {
                    int32 frame = tick / rate;
                    // getFrames tests indexB against the stored frameEnd, as
                    // buildAimData does.
                    if (tick % rate > (rate >> 1) && (frame + 1) * rate < anim.frameEnd)
                        frame++;
                    const int32 offset = start + frame * step;
                    if (offset < 0 || offset + 10 + model.count * 2 > level->frameDataSize) {
                        printf("joint frame out of range: anim %d tick %d\n", a, tick); exit(1);
                    }
                    const uint16* f = (const uint16*)level->frameData + offset;
                    int32 raw[3];
                    chain->joint(level, model, f, site.joint, site.ox, site.oy, site.oz, 0, 0, 0, raw);
                    std::vector<int16> point(3);
                    for (int32 c = 0; c < 3; c++) {
                        const int32 v = raw[c] >> 10;
                        if (v < -32768 || v > 32767) { printf("joint point out of range: anim %d\n", a); exit(1); }
                        point[c] = int16(v);
                    }
                    auto found = points.find(point);
                    int32 p;
                    if (found == points.end()) {
                        p = data.size();
                        points[point] = p;
                        for (int32 c = 0; c < 3; c++) put16(point[c]);
                        // fx.c's arithmetic against upstream's chain: creatures
                        // turn by yaw alone; she swims pitched (+-85 degrees)
                        // and leaning (+-22)
                        const int32 pitches = site.type == ITEM_LARA ? 9 : 1;
                        const int32 rolls = site.type == ITEM_LARA ? 5 : 1;
                        for (int32 yi = 0; yi < 64; yi++)
                        for (int32 pi = 0; pi < pitches; pi++)
                        for (int32 ri = 0; ri < rolls; ri++) {
                            const int32 ay = (yi * 1031) & 0xFFFF;
                            const int32 ax = pitches > 1 ? int16((pi - 4) * 3868) : 0;   // 85 / 4 degrees
                            const int32 az = rolls > 1 ? int16((ri - 2) * 2002) : 0;    // 11 degrees
                            int32 want[3];
                            chain->joint(level, model, f, site.joint, site.ox, site.oy, site.oz, ax, ay, az, want);
                            int64 x = point[0], y = point[1], z = point[2], t;
                            if (az) { t = (hostCos(az) * x - hostSin(az) * y) >> 14;
                                      y = (hostSin(az) * x + hostCos(az) * y) >> 14; x = t; }
                            if (ax) { t = (hostCos(ax) * y - hostSin(ax) * z) >> 14;
                                      z = (hostSin(ax) * y + hostCos(ax) * z) >> 14; y = t; }
                            t = (hostCos(ay) * x + hostSin(ay) * z) >> 14;
                            z = (hostCos(ay) * z - hostSin(ay) * x) >> 14; x = t;
                            const int64 got[3] = { x >> 4, y >> 4, z >> 4 };
                            for (int32 c = 0; c < 3; c++)
                                bound = std::max(bound, int32(std::abs(got[c] - (int64(want[c]) >> 14))));
                        }
                    } else p = found->second;
                    set16(map + 2 + tick * 2, p);
                }
                ticksTotal += ticks;
            }
        }
        delete chain;
        if (data.size() > 0x10000) { printf("FX.BIN past a bank\n"); exit(1); }
        printf("     effects: pool slots %d..%d; joints: %d ticks, %d points, within %d units of upstream; FX.BIN %d bytes\n",
               poolFirst, poolFirst + poolCount - 1, ticksTotal, int32(points.size()), bound, int32(data.size()));
        return data;
    }

    // Walk one sector's FloorData and, if it holds a trigger that activates
    // entities, record which. TR's stream is a sequence of words whose low
    // five bits say what they are, ended by the top bit of the last one; the
    // trigger entry is followed by its own list of commands, ended the same
    // way. Only ACTIVATE_OBJECT is kept -- cameras, flipmaps and the rest
    // have nothing on this target to act on yet.
    uint16 buildTrigger(TR1_PC* level, int32 floorIndex)
    {
        if (floorIndex <= 0 || floorIndex >= level->floorsCount)
            return SNES_TRIGGER_NONE;

        // A sector is asked about twice -- once for the grid the host probes,
        // once for whatever entity stands on it -- and both want the same
        // record rather than two that drift apart.
        if (triggerOfFloor[floorIndex] != -2)
            return (uint16)triggerOfFloor[floorIndex];

        const TR1_PC::FloorData* fd = level->floors + floorIndex;
        const TR1_PC::FloorData* end = level->floors + level->floorsCount;

        while (fd < end)
        {
            uint16 cmd = (fd++)->value;
            int32 type = cmd & 0x1F;
            bool last = (cmd >> 15) != 0;

            if (type == 1) {                        // portal
                if (fd < end) fd++;
            } else if (type == 2 || type == 3) {    // floor or ceiling slant
                if (fd < end) fd++;
            } else if (type == 4) {                 // trigger
                if (fd >= end) break;
                uint16 info = (fd++)->value;        // timer, once, mask

                uint8 items[SNES_TRIGGER_ITEMS];
                int32 count = 0;
                // The actions besides ACTIVATE_OBJECT the host has anything
                // to do about: END (the level is over) and SECRET (which
                // one). A bit each in byte 5, the secret's index in byte 6.
                // FX3 also keeps ACTIVATE_CAMERA and CAMERA_TARGET.
                uint8 actions = 0, secret = 0;
                int32 camIndex = -1, camWord = 0, target = -1;
                while (fd < end)
                {
                    uint16 c = (fd++)->value;
                    int32 action = (c >> 10) & 0x1F;
                    int32 args = c & 0x03FF;
                    bool done = (c >> 15) != 0;

                    if (action == 0 && count < SNES_TRIGGER_ITEMS &&
                        args >= 0 && args < level->itemsCount &&
                        itemSlot[args] >= 0)
                    {
                        items[count++] = (uint8)itemSlot[args];
                    }
                    if (action == 7) actions |= SNES_TRIGGER_ACTION_END;
                    if (action == 10) { actions |= SNES_TRIGGER_ACTION_SECRET; secret = (uint8)args; }
                    // The camera command carries a word of its own, and the
                    // list's end bit is that word's (upstream's FD_SET_END).
                    if (action == 1 && fd < end) {
                        uint16 w = (fd++)->value;
                        done = (w >> 15) != 0;
                        if (camIndex < 0) { camIndex = args; camWord = w; }
                        else if (packFx3) printf("  trigger with a second camera; the first is kept\n");
                    }
                    if (action == 6 && args < level->itemsCount && itemSlot[args] >= 0) {
                        if (target < 0) target = itemSlot[args];
                        else if (packFx3) printf("  trigger with a second camera target; the first is kept\n");
                    }
                    if (done) break;
                }
                if (packFx3) {
                    if (camIndex >= 0 && camIndex < level->camerasCount && camIndex < 255)
                        actions |= SNES_TRIGGER_ACTION_CAMERA;
                    if (target >= 0) actions |= SNES_TRIGGER_ACTION_TARGET;
                }

                if (count == 0 && actions == 0) {
                    triggerOfFloor[floorIndex] = SNES_TRIGGER_NONE;
                    return SNES_TRIGGER_NONE;
                }

                // The same sector's trigger is asked for once per room, so
                // there is no sharing to do -- but there is a cap, and a cap
                // that drops work silently is worse than one that says so.
                if (triggerCount >= SNES_TRIGGER_MAX) {
                    printf("  more than %d triggers; the rest are dropped\n",
                           SNES_TRIGGER_MAX);
                    triggerOfFloor[floorIndex] = SNES_TRIGGER_NONE;
                    return SNES_TRIGGER_NONE;
                }

                uint8* t = triggerData + triggerCount * SNES_TRIGGER_SIZE;
                memset(t, 0, SNES_TRIGGER_SIZE);
                t[0] = (uint8)((cmd >> 8) & 0x7F);   // TR's trigger type
                t[1] = (uint8)count;
                t[2] = (uint8)((info >> 9) & 0x1F);  // FD_MASK
                t[3] = (uint8)((info >> 8) & 1);     // FD_ONCE
                t[4] = (uint8)(info & 0xFF);         // FD_TIMER, in seconds
                t[5] = actions;
                t[6] = secret;
                for (int32 i = 0; i < count; i++) t[8 + i] = items[i];
                if (packFx3) {
                    triggerCam.resize((triggerCount + 1) * 4, 0xFF);
                    uint8* cam = &triggerCam[triggerCount * 4];
                    if (actions & SNES_TRIGGER_ACTION_CAMERA) {
                        cam[0] = uint8(camIndex);
                        cam[1] = uint8(camWord & 0xFF);                     // FD_TIMER
                        cam[2] = uint8(((camWord >> 8) & 1) |               // FD_ONCE
                                       (((camWord >> 9) & 0x1F) << 1));     // FD_SPEED
                    }
                    if (actions & SNES_TRIGGER_ACTION_TARGET) cam[3] = uint8(target);
                }
                if (getenv("OL_DUMP_TRIGGERS")) {
                    fprintf(stderr, "trigger %d: type %d mask %02x once %d timer %d items", (int)triggerCount, (int)t[0], (unsigned)t[2], (int)t[3], (int)t[4]);
                    for (int32 i = 0; i < count; i++) fprintf(stderr, " %d", (int)items[i]);
                    if (actions & SNES_TRIGGER_ACTION_END) fprintf(stderr, " END");
                    if (actions & SNES_TRIGGER_ACTION_SECRET) fprintf(stderr, " SECRET %d", (int)secret);
                    if (actions & SNES_TRIGGER_ACTION_CAMERA) fprintf(stderr, " CAMERA %d timer %d once %d speed %d",
                        camIndex, camWord & 0xFF, (camWord >> 8) & 1, (camWord >> 9) & 0x1F);
                    if (actions & SNES_TRIGGER_ACTION_TARGET) fprintf(stderr, " TARGET %d", target);
                    fprintf(stderr, "\n");
                }
                triggerOfFloor[floorIndex] = triggerCount;
                return (uint16)triggerCount++;
            }

            if (last) break;
        }

        triggerOfFloor[floorIndex] = SNES_TRIGGER_NONE;
        return SNES_TRIGGER_NONE;
    }

    // The trigger of the sector an entity stands in. A switch's own trigger
    // is what pulling it fires, and the host has no way to work out which
    // sector anything is in -- it cannot read the level.
    uint16 triggerUnderItem(TR1_PC* level, const TR1_PC::Item& item)
    {
        if (item.roomIndex < 0 || item.roomIndex >= level->roomsCount)
            return SNES_TRIGGER_NONE;

        const TR1_PC::Room* room = level->rooms + item.roomIndex;
        int32 sx = (item.pos.x - room->info.x) >> 10;
        int32 sz = (item.pos.z - room->info.z) >> 10;
        if (sx < 0 || sx >= room->xSectors || sz < 0 || sz >= room->zSectors)
            return SNES_TRIGGER_NONE;

        return buildTrigger(level, room->sectors[sx * room->zSectors + sz].floorIndex);
    }

    // The host needs the part of FloorData that changes geometry as well as
    // the trigger buildTrigger already resolves. TR orders these commands as
    // floor slant, ceiling slant, horizontal portal, then everything else,
    // so this is the same bounded prefix walk as Sector::getNextRoom.
    uint16 packHostSlant(int32 sx, int32 sz, int32 floorIndex, bool fx3)
    {
        if (fx3) return uint16((sx & 255) | ((sz & 255) << 8));
        if (sx < -16 || sx > 15 || sz < -16 || sz > 15) {
            printf("  FloorData %d slant (%d,%d) exceeds signed five-bit; "
                   "host sector table cannot represent it\n",
                   floorIndex, sx, sz);
            exit(1);
        }
        return (uint16)((sx & 31) | ((sz & 31) << 5));
    }

    void hostSectorInfo(TR1_PC* level, int32 floorIndex, bool fx3,
                        uint8& portal, uint16& floorSlant,
                        uint16& ceilingSlant)
    {
        portal = 0xFF;
        floorSlant = ceilingSlant = 0;
        if (floorIndex <= 0 || floorIndex >= level->floorsCount) return;

        const TR1_PC::FloorData* fd = level->floors + floorIndex;
        const TR1_PC::FloorData* end = level->floors + level->floorsCount;
        if (fd >= end) return;

        uint16 cmd = (fd++)->value;
        if ((cmd & 0x1F) == 2)                 // FLOOR_TYPE_FLOOR
        {
            if (fd >= end) return;
            uint16 s = (fd++)->value;
            floorSlant = packHostSlant((int8)(s & 0xFF), (int8)(s >> 8),
                                       floorIndex, fx3);
            if ((cmd >> 15) || fd >= end) return;
            cmd = (fd++)->value;
        }

        if ((cmd & 0x1F) == 3)                 // FLOOR_TYPE_CEILING
        {
            if (fd >= end) return;
            uint16 s = (fd++)->value;
            ceilingSlant = packHostSlant((int8)(s & 0xFF), (int8)(s >> 8),
                                         floorIndex, fx3);
            if ((cmd >> 15) || fd >= end) return;
            cmd = (fd++)->value;
        }

        if ((cmd & 0x1F) == 1 && fd < end)      // FLOOR_TYPE_PORTAL
        {
            uint16 room = fd->value;
            if (room < level->roomsCount && room < 0xFF) portal = (uint8)room;
        }
    }

    void buildRoom(TR1_PC* level, int32 roomIndex, RoomBlob& out)
    {
        TR1_PC::Room* room = level->rooms + roomIndex;

        OutFace* faces = new OutFace[room->qCount + room->tCount + 1];
        int32 faceCount = buildRoomFaces(level, room, faces);

        // OT_NEXT and OT_OFF reserve SNES_MAX_FACES words each in
        // gsu/mesh.gsu. LEVEL1 peaks at 546 faces (room 26), so 512 silently
        // removed visible geometry from two rooms.
        if (faceCount > SNES_MAX_FACES) {
            printf("  room %d: %d faces, over the %d the ordering table holds;"
                   " dropping the tail\n", roomIndex, faceCount, SNES_MAX_FACES);
            faceCount = SNES_MAX_FACES;
        }

        if (room->vCount > SNES_MAX_VERTS) {
            printf("  room %d: %d vertices, over the %d TV_BASE holds -- the "
                   "renderer would write into the ordering table\n",
                   roomIndex, room->vCount, SNES_MAX_VERTS);
            // Truncating vertices would break every face index, so this is
            // a refusal like the packer's others, not a warning.
            exit(1);
        }

        int32 faceBytes = 0;
        for (int32 i = 0; i < faceCount; i++)
            faceBytes += 2 + faces[i].count * 4;

        int32 geomBytes   = 4 + room->vCount * 8 + faceBytes;
        int32 portalBytes = room->pCount * SNES_PORTAL_SIZE;
        int32 sectorBytes = room->xSectors * room->zSectors * SNES_SECTOR_SIZE;
        int32 instBytes   = room->mCount * SNES_INST_SIZE;
        int32 itemBytes   = level->itemsCount * SNES_ITEM_SIZE;

        out.data = new uint8[geomBytes + portalBytes + sectorBytes
                             + instBytes + itemBytes];
        uint8* p = out.data;

        #define PUT16(v) { uint16 _t = (uint16)(v); memcpy(p, &_t, 2); p += 2; }
        #define PUT8(v)  { *p++ = (uint8)(v); }

        PUT16(room->vCount);
        PUT16(faceCount);

        for (int32 i = 0; i < room->vCount; i++)
        {
            // g matches the 32X room vertex: lighting >> 8, 0 bright, 31 dark.
            // The clamp is what lets the GSU mask instead of clamping; see
            // SNES_SHADE_MIN above.
            int32 g = room->vertices[i].lighting >> 8;
            if (g < SNES_SHADE_MIN) g = SNES_SHADE_MIN;
            if (g > SNES_SHADE_MAX) g = SNES_SHADE_MAX;

            PUT16(room->vertices[i].pos.x);
            PUT16(room->vertices[i].pos.y - room->info.yTop);
            PUT16(room->vertices[i].pos.z);
            PUT16(g);
        }

        for (int32 i = 0; i < faceCount; i++)
        {
            PUT8(faces[i].page);
            PUT8(faces[i].count);
            for (int32 j = 0; j < faces[i].count; j++)
            {
                PUT16(SNES_TV_BASE + faces[i].v[j] * SNES_TV_SIZE);
                PUT8(faces[i].u[j]);
                PUT8(faces[i].t[j]);
            }
        }

        out.portalOffset = (int32)(p - out.data);

        // Portals, in the same frame as the vertices, so the renderer can test
        // them with the matrix it already built for the room.
        for (int32 i = 0; i < room->pCount; i++)
        {
            const TR1_PC::Room::Portal& portal = room->portals[i];
            PUT16(portal.roomIndex);
            PUT16(portal.normal.x);
            PUT16(portal.normal.y);
            PUT16(portal.normal.z);
            for (int32 j = 0; j < 4; j++)
            {
                PUT16(portal.vertices[j].x);
                PUT16(portal.vertices[j].y - room->info.yTop);
                PUT16(portal.vertices[j].z);
            }
        }

        out.sectorOffset = (int32)(p - out.data);

        // Sectors, TR's own 1024-unit grid, x major. Heights are in sector
        // units like TR stores them; the renderer multiplies by 256. This is
        // what a camera needs to know where the floor is, and what the host
        // will need once it has a way to read it -- see the README.
        for (int32 i = 0; i < room->xSectors * room->zSectors; i++)
        {
            const TR1_PC::Room::Sector& sector = room->sectors[i];
            PUT8(sector.floor);
            PUT8(sector.ceiling);
            PUT8(sector.roomBelow);
            PUT8(sector.roomAbove);
            PUT16(buildTrigger(level, sector.floorIndex));
        }

        out.instOffset = (int32)(p - out.data);
        out.instCount  = 0;

        // Static mesh placements: where a mesh goes and which way it faces.
        // Position is relative to the room, like everything else here, and the
        // angle is the top byte of TR's -- which is the resolution the GSU's
        // sine table works at, and static meshes are placed on quadrants
        // anyway.
        for (int32 i = 0; i < room->mCount; i++)
        {
            const TR1_PC::Room::Mesh& inst = room->meshes[i];

            int32 slot = -1;
            for (int32 j = 0; j < level->staticMeshesCount; j++) {
                if (level->staticMeshes[j].id == inst.id) { slot = j; break; }
            }
            if (slot < 0) continue;

            int32 meshIndex = level->staticMeshes[slot].meshIndex;
            if (meshIndex < 0 || meshIndex >= level->meshOffsetsCount) continue;
            if (!meshUsed) continue;
            meshUsed[meshIndex] = 1;
            if (packFx3 && (inst.angleY & 0x3FFF)) {
                printf("room %d: a static turned %04X, off a quarter: InstTurn's marker is not safe\n",
                       roomIndex, inst.angleY & 0xFFFF);
                exit(1);
            }

            int32 g = inst.intensity >> 8;
            if (g < SNES_SHADE_MIN) g = SNES_SHADE_MIN;
            if (g > SNES_SHADE_MAX) g = SNES_SHADE_MAX;

            PUT16(meshIndex);                   // patched to an address later
            PUT8(0);                            // and a bank
            PUT8((inst.angleY >> 8) & 0xFF);
            PUT16(inst.pos.x - room->info.x);
            PUT16(inst.pos.y - room->info.yTop);
            PUT16(inst.pos.z - room->info.z);
            PUT8(g);
            {
                const TR1_PC::StaticMesh& sm = level->staticMeshes[slot];
                PUT8(radiusByte(sm.vbox.minX, sm.vbox.maxX, sm.vbox.minY,
                                sm.vbox.maxY, sm.vbox.minZ, sm.vbox.maxZ));
            }
            out.instCount++;
        }

        // FX3: the room's sprites, upstream's renderSprite at a room vertex
        // -- a static instance of the texture's quad, at the vertex (x and z
        // room-relative already, y less yTop as the statics'), marked to face
        // the camera (ROOM_SPRITE_TURN, mesh.gsu InstTurn), unshaded as the
        // pickups are.
        for (int32 i = 0; packFx3 && i < room->sCount; i++)
        {
            const TR1_PC::Room::Sprite& sp = room->sprites[i];
            if (sp.index >= room->vCount) continue;
            const int32 k = int32(std::find(roomSpriteTex.begin(), roomSpriteTex.end(), int32(sp.texture))
                                  - roomSpriteTex.begin());
            const TR1_PC::SpriteTexture& spr = level->spriteTextures[sp.texture];
            const TR1_PC::Room::Vertex& v = room->vertices[sp.index];
            roomSpriteRooms.push_back(roomIndex);
            roomSpriteInst.push_back(int32(p - out.data));
            roomSpriteWorld.push_back(room->info.x + v.pos.x);
            roomSpriteWorld.push_back(v.pos.y);
            roomSpriteWorld.push_back(room->info.z + v.pos.z);
            PUT16(roomSpriteMesh0 + k);         // patched to an address later
            PUT8(0);
            PUT8(ROOM_SPRITE_TURN);
            PUT16(v.pos.x);
            PUT16(v.pos.y - room->info.yTop);
            PUT16(v.pos.z);
            PUT8(SNES_SHADE_MAX);
            PUT8(radiusByte(spr.l, spr.r, spr.t, spr.b, 0, 0));
            out.instCount++;
        }

        out.itemOffset = (int32)(p - out.data);
        out.itemCount  = 0;

        // Entity placements. Same shape as a static mesh placement, but what
        // it points at is a model stream rather than a single mesh, and the
        // model is resolved from the entity type here so the renderer does
        // not need a lookup table at all. An entity whose type has no model
        // -- pickups are sprites in TR1 -- gets the sentinel and is skipped.
        // The index has to be the entity's place in the level, not in the
        // room, or two rooms would hand out the same slot.
        for (int32 i = 0; i < level->itemsCount; i++)
        {
            const TR1_PC::Item& item = level->items[i];
            if (item.roomIndex != roomIndex) continue;

            if (itemSlot[i] < 0) continue;

            int32 modelIndex = modelOfType(level, item.type);
            if (modelIndex < 0) continue;

            // Intensity is -1 where TR means "use the room's lighting", which
            // this renderer has no per-item version of; the middle of the
            // range is the honest answer until it does.
            int32 g = (item.intensity < 0) ? 16 : (item.intensity >> 8);
            if (g < SNES_SHADE_MIN) g = SNES_SHADE_MIN;
            if (g > SNES_SHADE_MAX) g = SNES_SHADE_MAX;

            PUT16(modelIndex);                  // patched to an address later
            PUT8(0);                            // and a bank
            PUT8(itemSlot[i]);                  // its slot in the host's array
            PUT8((item.angleY >> 8) & 0xFF);
            PUT8(g);
            PUT16(item.pos.x - room->info.x);
            PUT16(item.pos.y - room->info.yTop);
            PUT16(item.pos.z - room->info.z);
            out.itemCount++;
        }

        #undef PUT16
        #undef PUT8

        out.size        = (int32)(p - out.data);
        out.x           = room->info.x;
        out.y           = room->info.yTop;
        out.z           = room->info.z;
        out.yBottom     = room->info.yBottom - room->info.yTop;
        out.portalCount = room->pCount;
        out.xSectors    = room->xSectors;
        out.zSectors    = room->zSectors;
        out.flags       = room->flags;
        out.vCount      = room->vCount;
        out.qCount      = room->qCount;
        out.tCount      = room->tCount;
        out.faceCount   = faceCount;

        delete[] faces;
    }

    // FX3's weapons beyond the pistols, WEAPON.json's "kit": upstream's
    // inventory.init() hands her all four guns, and Lara::init puts the
    // shotgun on her back. Every level carries the four weapon models; this
    // only says where their meshes and the shotgun's animations already lie.
    //
    // meshes: the swap table the GSU indexes (WeaponNodeFx3 in mesh.gsu),
    //   1-based so that 0 is "no swap": 1-4 her own thighs L/R and hands R/L,
    //   5 + 4 * weapon + j for the pistols/magnums/uzis (j: thigh L, thigh R,
    //   hand R, hand L), 17/18 her torso and the shotgun's (on her back),
    //   19/20 the shotgun's hands R/L, 21/22 her head and the uzis' (the face
    //   weaponUpdateTargets gives her while Action is held).
    // shotgun: its AIM, DRAW and FIRE animations (ANIM_SHOTGUN_*), indexed by
    //   game tick as the pistols' are.
    // torso: per frame of those three, drawLaraNodes' "additional torso
    //   animation for shotgun" -- the frame's torso angles less
    //   DEF_TORSO_ANGLE_X/Y/Z -- in 16-bit angle units.
    void writeWeaponKit(FILE* f, TR1_PC* level, const ModelBlob* models,
                        int32** animPlace, const uint8* used, const int32* place)
    {
        const int32 body = level->getModelIndex(ITEM_LARA);
        int32 guns[4];
        for (int32 w = 0; w < 4; w++) {
            guns[w] = level->getModelIndex((ItemType)(ITEM_LARA_PISTOLS + w));
            if (guns[w] < 0 || level->models[guns[w]].count != 15) {
                printf("weapon model %d missing; FX3 hands her all four\n", ITEM_LARA_PISTOLS + w);
                exit(1);
            }
        }
        // Her own mesh at a joint is the one buildModel gave the node.
        auto bodyMesh = [&](int32 joint) {
            int32 m = level->models[body].start + joint;
            if (!meshDrawable(level, m)) m = level->models[guns[0]].start + joint;
            return m;
        };
        auto gunMesh = [&](int32 w, int32 joint) { return level->models[guns[w]].start + joint; };
        std::vector<int32> table;
        table.push_back(bodyMesh(1));  table.push_back(bodyMesh(4));
        table.push_back(bodyMesh(10)); table.push_back(bodyMesh(13));
        static const int32 pistolJoints[4] = { 1, 4, 10, 13 };
        for (int32 w = 0; w < 3; w++) {             // pistols, magnums, uzis
            const int32 model = w == 0 ? 0 : w == 1 ? 2 : 3;    // LARA_MAGNUMS = 3, LARA_UZIS = 4
            for (int32 j = 0; j < 4; j++) table.push_back(gunMesh(model, pistolJoints[j]));
        }
        table.push_back(bodyMesh(7));  table.push_back(gunMesh(1, 7));
        table.push_back(gunMesh(1, 10)); table.push_back(gunMesh(1, 13));
        table.push_back(bodyMesh(14)); table.push_back(gunMesh(3, 14));
        fprintf(f, ",\"kit\":{\"meshes\":[");
        for (size_t i = 0; i < table.size(); i++) {
            const int32 m = table[i];
            if (!meshDrawable(level, m) || !used[m] || place[m] < 0) {
                printf("weapon kit mesh %d (%d) is not in the level\n", (int)i + 1, m);
                exit(1);
            }
            fprintf(f, "%s{\"offset\":%d,\"radius\":%d}", i ? "," : "", place[m],
                    meshRadiusByte(level, m));
        }
        const int32 shotgun = guns[1];
        fprintf(f, "],\"shotgun\":[");
        std::vector<int32> torso;
        for (int32 k = 0; k < 3; k++) {
            const int32 a = level->models[shotgun].animIndex + k;
            const TR1_PC::Animation& anim = level->anims[a];
            if (anim.frameRate != 1 || !animPlace[shotgun] || animPlace[shotgun][k] < 0 ||
                anim.frameEnd < anim.frameBegin || anim.frameEnd - anim.frameBegin > 255 ||
                models[shotgun].animFrames[k] != anim.frameEnd - anim.frameBegin + 1) {
                printf("unsupported shotgun animation %d\n", k); exit(1);
            }
            fprintf(f, "%s{\"offset\":%d,\"last\":%d}", k ? "," : "", animPlace[shotgun][k],
                    anim.frameEnd - anim.frameBegin);
            const int32 step = anim.frameSize > 0 ? anim.frameSize : 10 + 15 * 2;
            for (int32 fr = 0; fr <= anim.frameEnd - anim.frameBegin; fr++) {
                const uint16* frame = level->frameData + (anim.frameOffset >> 1) + fr * step;
                const uint32 v = (uint32)frame[10 + 7 * 2] | ((uint32)frame[11 + 7 * 2] << 16);
                const int32 x = ((v >> 16) & 0x3FF0) << 2;
                const int32 y = (((v >> 16) & 0x000F) << 12) | ((v & 0xFC00) >> 4);
                const int32 z = (v & 0x03FF) << 6;
                torso.push_back((int16)(x - 1216));
                torso.push_back((int16)(y + 832));
                torso.push_back((int16)(z + 192));
            }
        }
        fprintf(f, "],\"torso\":[");
        for (size_t i = 0; i < torso.size(); i++) fprintf(f, "%s%d", i ? "," : "", torso[i]);
        fprintf(f, "]}");
    }

    void writeLevel(const char* dir, TR1_PC* level, bool fx3)
    {
        const int32 roomCount = level->roomsCount;

        // Entity types with no model but a sprite sequence: TR's pickups, which
        // are a texture rather than geometry. Each gets a synthetic model of
        // one billboard quad, appended after the real ones -- see buildSprite.
        spriteCount = 0;
        fxModel = fxAnim0 = fxMesh = -1;
        for (int32 i = 0; i < level->itemsCount; i++)
        {
            int32 t = level->items[i].type;
            if (fx3 && fxPoolItems && isFxType(t)) continue;    // the effects model's
            bool has = false;
            for (int32 j = 0; j < level->modelsCount; j++)
                if (level->models[j].type == t) has = true;
            if (has) continue;

            bool seen = false;
            for (int32 j = 0; j < spriteCount; j++)
                if (spriteType[j] == t) seen = true;
            if (seen) continue;

            for (int32 j = 0; j < level->spriteSequencesCount; j++)
            {
                if (level->spriteSequences[j].type != t) continue;
                if (spriteCount >= SNES_SPRITE_MAX) break;
                spriteType[spriteCount] = t;
                spriteIndex[spriteCount] = level->spriteSequences[j].start;
                spriteCount++;
                break;
            }
        }

        // Everything a sprite type needs is appended after the level's own, so
        // that an index into the mesh, model or animation tables means the
        // same thing whichever kind of entity it came from. The counts below
        // are the ones every table is sized and written by; the level's own
        // remain the bound on reading TR's data.
        // And after them, FX3's effects model: one mesh, one model, an
        // animation per kind (buildFxModel).
        const int32 fxCount = (fx3 && fxPoolItems) ? 1 : 0;
        // and the room sprites' quads after that (FX3)
        roomSpriteTex.clear();
        roomSpriteRooms.clear();
        roomSpriteInst.clear();
        roomSpriteWorld.clear();
        if (fx3) {
            for (int32 r = 0; r < roomCount; r++)
                for (int32 i = 0; i < level->rooms[r].sCount; i++) {
                    const int32 t = level->rooms[r].sprites[i].texture;
                    if (std::find(roomSpriteTex.begin(), roomSpriteTex.end(), t) == roomSpriteTex.end())
                        roomSpriteTex.push_back(t);
                }
        }
        const int32 realMeshCount = level->meshOffsetsCount;
        roomSpriteMesh0 = realMeshCount + spriteCount + fxCount;
        const int32 meshCount     = roomSpriteMesh0 + int32(roomSpriteTex.size());
        const int32 spriteModel0  = level->modelsCount;
        const int32 spriteAnim0   = level->animsCount;
        if (fxCount) {
            fxModel = level->modelsCount + spriteCount;
            fxAnim0 = level->animsCount + spriteCount;
            fxMesh  = realMeshCount + spriteCount;
        }

        meshUsed = new uint8[meshCount];
        memset(meshUsed, 0, meshCount);

        const int32 animsAll = level->animsCount + spriteCount + fxCount * FX_KINDS;
        const int32 animsN = animsAll > 0 ? animsAll : 1;
        animFirstFrame = new uint16[animsN];
        animFrameCount = new uint16[animsN];
        animTickCount  = new uint8[animsN];
        animFrameRate  = new uint8[animsN];
        animNextAnim   = new uint16[animsN];
        animNextFrame  = new uint16[animsN];
        memset(animFirstFrame, 0, animsN * 2);
        memset(animFrameCount, 0, animsN * 2);
        memset(animTickCount, 1, animsN);
        memset(animFrameRate, 1, animsN);
        memset(animNextAnim, 0, animsN * 2);
        memset(animNextFrame, 0, animsN * 2);
        animBox = new int16[animsN * 6];
        memset(animBox, 0, animsN * 12);
        animSpeed = new int32[animsN * 2];
        memset(animSpeed, 0, animsN * 8);

        for (int32 a = 0; a < level->animsCount; a++) {
            const TR1_PC::Animation& anim = level->anims[a];
            int32 ticks = (int32)anim.frameEnd - (int32)anim.frameBegin + 1;
            // Upstream's collision handlers test frameIndex against *global*
            // frame numbers; this port's frameIndex is local to the
            // animation. OL_DUMP_ANIMS=1 prints what to subtract.
            if (getenv("OL_DUMP_ANIMS")) {
                fprintf(stderr, "anim %d: frames %d..%d state %d next %d speed %d accel %d\n",
                        a, anim.frameBegin, anim.frameEnd, anim.state,
                        anim.nextAnimIndex, (int)((int32)anim.speed >> 16),
                        (int)((int32)anim.accel >> 16));
            }
            if (ticks < 1) ticks = 1;
            if (ticks > 255) {
                printf("  animation %d lasts %d ticks; host counter holds 255\n",
                       a, ticks);
                exit(1);
            }
            animTickCount[a] = (uint8)ticks;
            animFrameRate[a] = anim.frameRate ? anim.frameRate : 1;
        }

        animCmdStart = new uint16[animsN];
        animCmdCount = new uint8[animsN];
        memset(animCmdStart, 0, animsN * 2);
        memset(animCmdCount, 0, animsN);
        cmdData = new uint8[(level->commandsCount + 1) * SNES_CMD_SIZE];
        cmdCount = 0;

        animTransStart = new uint16[animsN];
        animTransCount = new uint8[animsN];
        animState      = new uint8[animsN];
        memset(animTransStart, 0, animsN * 2);
        memset(animTransCount, 0, animsN);
        memset(animState, 0, animsN);
        transData = new uint8[(level->rangesCount + 1) * SNES_TRANS_SIZE];
        transCount = 0;
        triggerData = new uint8[SNES_TRIGGER_MAX * SNES_TRIGGER_SIZE];
        triggerCount = 0;
        packFx3 = fx3;
        triggerCam.clear();
        triggerOfFloor = new int32[level->floorsCount > 0 ? level->floorsCount : 1];
        for (int32 i = 0; i < level->floorsCount; i++) triggerOfFloor[i] = -2;

        const int32 modelsAll = level->modelsCount + spriteCount + fxCount;
        const int32 modelCount = modelsAll > 0 ? modelsAll : 1;
        modelFirstAnim = new uint16[modelCount];
        modelAnimCount = new uint16[modelCount];
        modelBox = new int16[modelCount * 6];
        memset(modelFirstAnim, 0, modelCount * 2);
        memset(modelAnimCount, 0, modelCount * 2);
        memset(modelBox, 0, modelCount * 12);

        // Slot 0 is Lara, wherever TR put her: the host addresses her as
        // items[0] throughout, and she is item 0 in LEVEL1 by luck and item
        // 93 in LEVEL2. slotItem is the inverse, for writing the host's list
        // in slot order below.
        itemSlot = new int32[level->itemsCount > 0 ? level->itemsCount : 1];
        slotItem = new int32[SNES_ITEM_MAX];
        int32 slots = 0;
        for (int32 i = 0; i < level->itemsCount; i++)
        {
            itemSlot[i] = -1;
            if (level->items[i].type == ITEM_LARA && slots == 0) {
                itemSlot[i] = 0;
                slotItem[0] = i;
                slots = 1;
            }
        }
        for (int32 i = 0; i < level->itemsCount; i++)
        {
            if (itemSlot[i] == 0) continue;
            if (slots >= SNES_ITEM_MAX) continue;
            if (modelOfType(level, level->items[i].type) >= 0) {
                slotItem[slots] = i;
                itemSlot[i] = slots++;
            }
        }

        RoomBlob* blobs = new RoomBlob[roomCount];
        for (int32 i = 0; i < roomCount; i++) {
            buildRoom(level, i, blobs[i]);
        }

        // Only the meshes something actually places are worth carrying.
        // Models first: they are what marks their meshes used, and the rooms
        // have already marked the static ones.
        ModelBlob* models = new ModelBlob[modelCount];
        memset(models, 0, sizeof(ModelBlob) * modelCount);
        for (int32 i = 0; i < level->modelsCount; i++) {
            buildModel(level, i, models[i]);
            if (getenv("OL_DUMP_MODELS")) {
                fprintf(stderr, "model %d: type %d meshes %d anims %d..\n", i,
                        (int)level->models[i].type, (int)level->models[i].count,
                        (int)level->models[i].animIndex);
            }
        }

        MeshBlob* meshes = new MeshBlob[meshCount];
        memset(meshes, 0, sizeof(MeshBlob) * meshCount);
        for (int32 i = 0; i < realMeshCount; i++) {
            if (meshUsed[i]) buildMesh(level, i, meshes[i]);
        }

        // And one of each for every sprite type an entity uses.
        for (int32 i = 0; i < spriteCount; i++)
        {
            buildSpriteMesh(level, spriteIndex[i], meshes[realMeshCount + i]);
            if (!meshes[realMeshCount + i].valid) continue;
            buildSpriteModel(level, spriteIndex[i], spriteModel0 + i,
                             spriteAnim0 + i, realMeshCount + i,
                             models[spriteModel0 + i]);
        }
        if (fxCount) buildFxModel(level, meshes[fxMesh], models[fxModel]);
        for (int32 k = 0; k < int32(roomSpriteTex.size()); k++)
            buildSpriteMesh(level, roomSpriteTex[k], meshes[roomSpriteMesh0 + k]);

        // TR chains animations by global frame number; the host works in
        // frames local to a model, because that is what it hands the
        // renderer. The conversion happens once, here, so the other side
        // needs neither a divide per transition nor TR's own tables.
        //
        // The same conversion flattens the state graph. TR's is two nested
        // tables: an animation lists the states reachable from it, and each
        // of those lists the frame ranges it is reachable from. One list per
        // animation costs the cartridge one loop instead of two, and the
        // order is TR's so the first match still wins.
        for (int32 a = 0; a < level->animsCount; a++)
        {
            const TR1_PC::Animation& anim = level->anims[a];
            animState[a] = (uint8)anim.state;
            animTransStart[a] = (uint16)transCount;
            animTransCount[a] = 0;
            animCmdStart[a] = (uint16)cmdCount;
            animCmdCount[a] = 0;

            if (animFrameCount[a] == 0 || anim.frameRate == 0) continue;

            // The commands, unpacked from TR's stream into fixed records.
            // The frame a SOUND or EFFECT fires on is global in the file and
            // local here, like everything else the host is handed.
            {
                int32 ci = anim.commandsStart;
                for (int32 n = 0; n < anim.commandsCount; n++)
                {
                    if (ci < 0 || ci >= level->commandsCount) break;
                    int32 op = level->commands[ci++];
                    int32 a0 = 0, a1 = 0, a2 = 0, args = 0;

                    if (op == 1) args = 3;          /* OFFSET  x, y, z */
                    else if (op == 2) args = 2;     /* JUMP    vSpeed, hSpeed */
                    else if (op == 5 || op == 6) args = 2;  /* SOUND / EFFECT */

                    if (ci + args > level->commandsCount) break;
                    if (args > 0) a0 = level->commands[ci++];
                    if (args > 1) a1 = level->commands[ci++];
                    if (args > 2) a2 = level->commands[ci++];

                    if (op == 5 || op == 6) {
                        a0 = a0 - (int32)anim.frameBegin;
                        if (a0 < 0) a0 = 0;
                        if (a0 > 255) a0 = 255;
                    }

                    if (animCmdCount[a] == 255) break;
                    uint8* c = cmdData + cmdCount * SNES_CMD_SIZE;
                    c[0] = (uint8)op;
                    c[1] = 0;
                    memcpy(c + 2, &a0, 2);
                    memcpy(c + 4, &a1, 2);
                    memcpy(c + 6, &a2, 2);
                    cmdCount++;
                    animCmdCount[a]++;
                }
            }

            for (int32 si = 0; si < anim.statesCount; si++)
            {
                int32 sIdx = anim.statesStart + si;
                if (sIdx < 0 || sIdx >= level->statesCount) continue;
                const TR1_PC::AnimState& st = level->states[sIdx];

                for (int32 ri = 0; ri < st.rangesCount; ri++)
                {
                    int32 rIdx = st.rangesStart + ri;
                    if (rIdx < 0 || rIdx >= level->rangesCount) continue;
                    const TR1_PC::AnimRange& rg = level->ranges[rIdx];

                    int32 na = rg.nextAnimIndex;
                    if (na < 0 || na >= level->animsCount) continue;
                    if (animFrameCount[na] == 0) continue;

                    // Frame numbers are global in TR and local here, in both
                    // the range this transition is valid over and the frame
                    // it lands on.
                    int32 b = (int32)rg.frameBegin - (int32)anim.frameBegin;
                    int32 e = (int32)rg.frameEnd - (int32)anim.frameBegin;
                    if (b < 0) b = 0;

                    // One *past* the last frame is a legal upper bound, and
                    // clamping it away is fatal: animProcess advances the
                    // frame and then asks whether a transition applies, so
                    // the index it tests is one beyond the frame just drawn.
                    // TR's ranges are written for that. A one-frame idle --
                    // which is what a shut trapdoor is -- has the range 0..1
                    // over an animation with only frame 0, and clamping it to
                    // 0..0 means the door can never open.
                    if (e > animTickCount[a]) e = animTickCount[a];
                    if (e < b) continue;
                    if (b > 255 || e > 255) continue;

                    const TR1_PC::Animation& next = level->anims[na];
                    int32 nf = 0;
                    nf = (int32)rg.nextFrameIndex - (int32)next.frameBegin;
                    if (nf < 0) nf = 0;
                    if (nf >= animTickCount[na]) nf = animTickCount[na] - 1;

                    if (animTransCount[a] == 255) continue;
                    uint8* t = transData + transCount * SNES_TRANS_SIZE;
                    t[0] = (uint8)st.state;
                    t[1] = (uint8)b;
                    t[2] = (uint8)e;
                    t[3] = (uint8)nf;
                    t[4] = (uint8)(na & 0xFF);
                    t[5] = (uint8)(na >> 8);
                    transCount++;
                    animTransCount[a]++;
                }
            }
        }

        for (int32 a = 0; a < level->animsCount; a++)
        {
            const TR1_PC::Animation& anim = level->anims[a];
            int32 na = anim.nextAnimIndex;
            if (na < 0 || na >= level->animsCount || animFrameCount[na] == 0) {
                animNextAnim[a] = (uint16)a;             // loop on itself
                animNextFrame[a] = animFirstFrame[a];
                continue;
            }

            const TR1_PC::Animation& next = level->anims[na];
            int32 local = 0;
            local = (int32)anim.nextFrameIndex - (int32)next.frameBegin;
            if (local < 0) local = 0;
            if (local >= animTickCount[na]) local = animTickCount[na] - 1;

            animNextAnim[a]  = (uint16)na;
            animNextFrame[a] = (uint16)(animFirstFrame[na] + local);
        }

        // The host's own view of the level, right after the room table so
        // both live in bank 0 and the GSU can stage it without a bank switch.
        int32 hostItems = 0;
        for (int32 i = 0; i < level->itemsCount; i++) {
            if (itemSlot[i] >= 0) hostItems++;
        }
        int32 hostSectors = 0;
        for (int32 i = 0; i < roomCount; i++)
            hostSectors += blobs[i].xSectors * blobs[i].zSectors;

        // The link table: one entry per distinct (roomBelow, roomAbove,
        // portal, trigger, floor slants, ceiling slants), and the entry each
        // sector uses, in the order the sectors are written below. Entry 0
        // is the sector that links nowhere and is flat, so that a table
        // with nothing in it still has one; a level whose sectors need more
        // than the index can hold is refused rather than truncated.
        std::vector<uint64_t> linkTable;
        std::vector<uint16>   sectorLink;
        {
            std::map<uint64_t, uint16> linkIndex;
            const uint64_t none = 0xFFFFFFFFull;
            linkTable.push_back(none);
            linkIndex[none] = 0;
            for (int32 r = 0; r < roomCount; r++) {
                const TR1_PC::Room* room = level->rooms + r;
                for (int32 i = 0; i < room->xSectors * room->zSectors; i++) {
                    const TR1_PC::Room::Sector& sector = room->sectors[i];
                    uint8 portal;
                    uint16 fs, cs;
                    hostSectorInfo(level, sector.floorIndex, fx3, portal, fs, cs);
                    uint16 trigger = buildTrigger(level, sector.floorIndex);
                    if (trigger != SNES_TRIGGER_NONE && getenv("OL_DUMP_TRIGGERS")) {
                        fprintf(stderr, "trigger %d at room %d sector (%d,%d) world (%d,%d)\n",
                                (int)trigger, (int)r,
                                (int)(i / room->zSectors), (int)(i % room->zSectors),
                                (int)(room->info.x + (i / room->zSectors) * 1024 + 512),
                                (int)(room->info.z + (i % room->zSectors) * 1024 + 512));
                    }
                    uint64_t key = (uint64_t)sector.roomBelow
                                 | ((uint64_t)sector.roomAbove << 8)
                                 | ((uint64_t)portal << 16)
                                 | ((uint64_t)(trigger & 0xFF) << 24)
                                 | ((uint64_t)fs << 32)
                                 | ((uint64_t)cs << 48);
                    std::map<uint64_t, uint16>::iterator it = linkIndex.find(key);
                    if (it == linkIndex.end()) {
                        if (linkTable.size() >= 0xFFFF) {
                            printf("  more than 65534 distinct sector links\n");
                            exit(1);
                        }
                        it = linkIndex.insert(std::make_pair(key,
                                 (uint16)linkTable.size())).first;
                        linkTable.push_back(key);
                    }
                    sectorLink.push_back(it->second);
                }
            }
            if ((int32)sectorLink.size() != hostSectors) {
                printf("  sector count mismatch: %d links for %d sectors\n",
                       (int32)sectorLink.size(), hostSectors);
                exit(1);
            }
        }
        const int32 linkCount = (int32)linkTable.size();

        // The static meshes she can walk into: upstream's collideStatic
        // walks a room's placements, skips the plants and the ones flagged
        // as having no collision, turns the mesh's box to its quadrant and
        // tests. The walk and the skipping happen here, once; the host gets
        // the survivors in room order with the box already turned, and a
        // table of where each room's run starts.
        struct HostStatic { uint8 room; int16 x, y, z; int16 box[6]; };
        std::vector<HostStatic> statics;
        std::vector<uint16>     staticFirst;
        for (int32 r = 0; r < roomCount; r++) {
            const TR1_PC::Room* room = level->rooms + r;
            staticFirst.push_back((uint16)statics.size());
            for (int32 i = 0; i < room->mCount; i++) {
                const TR1_PC::Room::Mesh& inst = room->meshes[i];
                int32 slot = -1;
                for (int32 j = 0; j < level->staticMeshesCount; j++) {
                    if (level->staticMeshes[j].id == inst.id) { slot = j; break; }
                }
                if (slot < 0) continue;
                const TR1_PC::StaticMesh& sm = level->staticMeshes[slot];
                if (inst.id < 10) continue;                 // NO_STATIC_MESH_PLANTS
                if (sm.flags & 1) continue;                 // STATIC_MESH_FLAG_NO_COLLISION
                // upstream: (angleY / 0x4000 + 2) is the quadrant, 2 the identity
                int32 q = (inst.angleY / 0x4000 + 2) & 3;
                int32 minX = sm.cbox.minX, maxX = sm.cbox.maxX;
                int32 minZ = sm.cbox.minZ, maxZ = sm.cbox.maxZ;
                HostStatic hs;
                hs.room = (uint8)r;
                hs.x = (int16)inst.pos.x;
                hs.y = (int16)inst.pos.y;
                hs.z = (int16)inst.pos.z;
                // boxRotateYQ, upstream's
                switch (q) {
                    case 3:  hs.box[0] = minZ;  hs.box[1] = maxZ;
                             hs.box[4] = -maxX; hs.box[5] = -minX; break;
                    case 1:  hs.box[0] = -maxZ; hs.box[1] = -minZ;
                             hs.box[4] = minX;  hs.box[5] = maxX;  break;
                    case 0:  hs.box[0] = -maxX; hs.box[1] = -minX;
                             hs.box[4] = -maxZ; hs.box[5] = -minZ; break;
                    default: hs.box[0] = minX;  hs.box[1] = maxX;
                             hs.box[4] = minZ;  hs.box[5] = maxZ;  break;
                }
                hs.box[2] = sm.cbox.minY;
                hs.box[3] = sm.cbox.maxY;
                if (getenv("OL_DUMP_STATICS")) {
                    printf("  static %3d room %2d id %2d q %d at (%d,%d,%d) box x %d..%d y %d..%d z %d..%d\n",
                           (int32)statics.size(), r, inst.id, q,
                           inst.pos.x, inst.pos.y, inst.pos.z,
                           hs.box[0], hs.box[1], hs.box[2], hs.box[3], hs.box[4], hs.box[5]);
                }
                statics.push_back(hs);
            }
        }
        staticFirst.push_back((uint16)statics.size());
        const int32 staticCount = (int32)statics.size();
        if (staticCount > 255) {
            printf("  %d colliding static placements; the room table indexes 65535 but nobody has tested past 255\n", staticCount);
        }

        const std::vector<uint8> aimData = buildAimData(level, animsN, models, dir);
        const std::vector<uint8> sphereData = fx3 ? buildSphereData(level, models) : std::vector<uint8>();
        if (fx3) {
            char spath[512];
            sprintf(spath, "%s/SPHERE.BIN", dir);
            FILE* sf = fopen(spath, "wb");
            if (!sf) { printf("can't write %s\n", spath); exit(1); }
            if (!sphereData.empty()) fwrite(sphereData.data(), 1, sphereData.size(), sf);
            fclose(sf);
        }
        if (fx3) {
            // The level's fixed cameras and the triggers' camera actions:
            //   u16 cameraCount, u16 triggerCount
            //   cameras, 8 bytes: int16 x, y, z; u8 room; u8 flags (bit 0:
            //     TR's FIXED_CAMERA_FLAG_ONCE)
            //   triggers, 4 bytes: camera ($FF none), timer, once | speed
            //     << 1, target slot ($FF none)
            std::vector<uint8> cam;
            auto put16 = [&](int32 v) { cam.push_back(uint8(v)); cam.push_back(uint8(v >> 8)); };
            const int32 cameras = level->camerasCount < 255 ? level->camerasCount : 255;
            put16(cameras);
            put16(triggerCount);
            for (int32 i = 0; i < cameras; i++) {
                const TR1_PC::Camera& c = level->cameras[i];
                put16(c.pos.x); put16(c.pos.y); put16(c.pos.z);
                cam.push_back(uint8(c.roomIndex));
                cam.push_back(uint8((c.flags >> 8) & 1));
            }
            triggerCam.resize(triggerCount * 4, 0xFF);
            cam.insert(cam.end(), triggerCam.begin(), triggerCam.end());
            buildCameraPoses(level, animsN, models, cam);
            if (cam.size() > 0x10000) { printf("CAMERA.BIN past a bank\n"); exit(1); }
            char cpath[512];
            sprintf(cpath, "%s/CAMERA.BIN", dir);
            FILE* cf = fopen(cpath, "wb");
            if (!cf) { printf("can't write %s\n", cpath); exit(1); }
            fwrite(cam.data(), 1, cam.size(), cf);
            fclose(cf);
            int32 withCamera = 0, withTarget = 0;
            for (int32 i = 0; i < triggerCount; i++) {
                withCamera += triggerCam[i * 4] != 0xFF;
                withTarget += triggerCam[i * 4 + 3] != 0xFF;
            }
            printf("      cameras: %d fixed, %d triggers switch to one, %d name a target\n",
                   cameras, withCamera, withTarget);
        }
        if (fx3) {
            const std::vector<uint8> fxData = fxPoolItems ? buildFxData(level, models) : std::vector<uint8>();
            char xpath[512];
            sprintf(xpath, "%s/FX.BIN", dir);
            FILE* xf = fopen(xpath, "wb");
            if (!xf) { printf("can't write %s\n", xpath); exit(1); }
            if (!fxData.empty()) fwrite(fxData.data(), 1, fxData.size(), xf);
            fclose(xf);
        }
        const std::vector<uint8> navData = buildNavData(level, dir);
        const std::vector<uint8> zoneData = fx3 ? buildZoneData(level) : std::vector<uint8>();
        const std::vector<uint8> frameBoxes = fx3 ? buildFrameBoxes(level, animsN, models, dir) : std::vector<uint8>();
        const int32 frameExtra = fx3 ? int32(frameBoxes.size()) - animsN * 12 : 0;
        const int32 zoneBytes = fx3 ? int32(zoneData.size()) : level->boxesCount * 4;
        const int32 hostBytes = SNES_HOST_HEADER + roomCount * SNES_HOST_ROOM
                                   + hostSectors * SNES_HOST_SECTOR
                                   + linkCount * SNES_HOST_LINK
                                   + modelCount * SNES_HOST_MODEL
                                   + animsN * (SNES_HOST_ANIM + 12 + 8)
                                   + transCount * SNES_TRANS_SIZE
                                   + cmdCount * SNES_CMD_SIZE
                                   + triggerCount * SNES_TRIGGER_SIZE
                                   + hostItems * SNES_HOST_ITEM
                                   + level->boxesCount * 8
                                   + level->overlapsCount * 2
                                   + zoneBytes
                                   + staticCount * SNES_HOST_STATIC
                                   + (roomCount + 1) * 2
                                   + aimData.size() + navData.size() + 2 + frameExtra;

        if (!fx3 && hostBytes > 65534) {
            printf("  host table is %d bytes; game pak RAM bank holds 65534\n",
                   hostBytes);
            exit(1);
        }

        const int32 roomTableBytes = 4 + roomCount * (int32)sizeof(RoomEntry);
        const int32 hostBase = fx3 ? 0 : roomTableBytes;
        const int32 tableBytes = roomTableBytes + (fx3 ? 0 : hostBytes);
        if (!fx3 && tableBytes > 65536) {
            printf("host table plus room directory is %d bytes; GSU source bank holds 65536\n", tableBytes);
            exit(1);
        }

        // Place each room, then each mesh, in a 64 KB GSU bank, first fit,
        // never straddling: R14 is sixteen bits and wraps inside ROMBR's bank,
        // so a record split over the boundary would read its own beginning.
        RoomEntry* entries = new RoomEntry[roomCount];
        int32* placement = new int32[roomCount];
        int32* meshPlace = new int32[meshCount];
        int32 cursor = tableBytes;

        for (int32 i = 0; i < roomCount; i++)
        {
            int32 bank = cursor / GSU_BANK_SIZE;
            if (cursor % GSU_BANK_SIZE + blobs[i].size > GSU_BANK_SIZE) {
                bank++;
                cursor = bank * GSU_BANK_SIZE;
            }
            placement[i] = cursor;

            memset(entries + i, 0, sizeof(RoomEntry));
            entries[i].addr        = (uint16)(cursor % GSU_BANK_SIZE);
            entries[i].portalAddr  = (uint16)((cursor + blobs[i].portalOffset) % GSU_BANK_SIZE);
            entries[i].sectorAddr  = (uint16)((cursor + blobs[i].sectorOffset) % GSU_BANK_SIZE);
            entries[i].instAddr    = (uint16)((cursor + blobs[i].instOffset) % GSU_BANK_SIZE);
            entries[i].itemAddr    = (uint16)((cursor + blobs[i].itemOffset) % GSU_BANK_SIZE);
            entries[i].itemCount   = (uint8)blobs[i].itemCount;
            entries[i].bank        = (uint8)bank;    // relative; the linker adds the base
            entries[i].portalCount = (uint8)blobs[i].portalCount;
            entries[i].instCount   = (uint8)blobs[i].instCount;
            entries[i].x           = (int16)blobs[i].x;
            entries[i].y           = (int16)blobs[i].y;
            entries[i].z           = (int16)blobs[i].z;
            entries[i].yBottom     = (int16)blobs[i].yBottom;
            entries[i].xSectors    = (uint8)blobs[i].xSectors;
            entries[i].zSectors    = (uint8)blobs[i].zSectors;

            cursor += blobs[i].size;
        }

        int32* modelPlace = new int32[modelCount];
        int32** animPlace = new int32*[modelCount];
        for (int32 i = 0; i < modelCount; i++)
        {
            modelPlace[i] = -1;
            animPlace[i] = NULL;
            if (!models[i].valid) continue;

            int32 bank = cursor / GSU_BANK_SIZE;
            if (cursor % GSU_BANK_SIZE + models[i].size > GSU_BANK_SIZE) {
                bank++;
                cursor = bank * GSU_BANK_SIZE;
            }
            modelPlace[i] = cursor;
            cursor += models[i].size;

            // And every animation's frames, each in a bank of its own
            // choosing. A model's frames together can exceed one -- Lara's
            // come to 101 KB -- but an animation's never do.
            animPlace[i] = new int32[models[i].animCount];
            for (int32 k = 0; k < models[i].animCount; k++)
            {
                animPlace[i][k] = -1;
                if (models[i].animLen[k] <= 0) continue;

                int32 b = cursor / GSU_BANK_SIZE;
                if (cursor % GSU_BANK_SIZE + models[i].animLen[k] > GSU_BANK_SIZE) {
                    b++;
                    cursor = b * GSU_BANK_SIZE;
                }
                animPlace[i][k] = cursor;
                cursor += models[i].animLen[k];
            }
        }

        for (int32 i = 0; i < meshCount; i++)
        {
            meshPlace[i] = -1;
            if (!meshes[i].valid) continue;

            int32 bank = cursor / GSU_BANK_SIZE;
            if (cursor % GSU_BANK_SIZE + meshes[i].size > GSU_BANK_SIZE) {
                bank++;
                cursor = bank * GSU_BANK_SIZE;
            }
            meshPlace[i] = cursor;
            cursor += meshes[i].size;
        }

        // the font's pixels (buildFont), in one bank: TextRender sets ROMB once
        int32 fontPlace = -1;
        if (fx3 && !fontPixels.empty()) {
            const int32 size = int32(fontPixels.size());
            if (cursor % GSU_BANK_SIZE + size > GSU_BANK_SIZE)
                cursor = (cursor / GSU_BANK_SIZE + 1) * GSU_BANK_SIZE;
            fontPlace = cursor;
            cursor += size;
        }

        const int32 totalSize = ((cursor + GSU_BANK_SIZE - 1) / GSU_BANK_SIZE) * GSU_BANK_SIZE;
        uint8* image = new uint8[totalSize];
        memset(image, 0, totalSize);
        if (fontPlace >= 0)
            memcpy(image + fontPlace, fontPixels.data(), fontPixels.size());

        uint16 rc = (uint16)roomCount;
        memcpy(image, &rc, 2);
        uint16 hb = (uint16)hostBase;
        memcpy(image + 2, &hb, 2);          // where the host's table starts
        memcpy(image + 4, entries, roomCount * sizeof(RoomEntry));

        std::vector<uint8> hostData(hostBytes);
        {
            uint8* h = hostData.data();
            #define PUT16(v) { uint16 _t = (uint16)(v); memcpy(h, &_t, 2); h += 2; }
            #define PUT8(v)  { *h++ = (uint8)(v); }

            // These legacy 16-bit offsets are used only by GSU-2. FX3's
            // HOST.json below carries full offsets into HOST.RAW; OLH2
            // replaces this header and the footer before ROM placement.
            PUT16(hostBytes);
            PUT16(hostItems);
            PUT16(modelCount);
            PUT16(animsN);
            PUT16(transCount);
            PUT16(triggerCount);
            PUT16(cmdCount);
            PUT16(level->boxesCount);
            PUT16(level->overlapsCount);
            PUT16(roomCount);
            /* Where the entity list starts. Everything else can be found by
               adding up what is in front of it, and three separate pieces of
               code were doing that -- the packer, the host and the
               interpreter -- so every table added broke the two that were not
               being edited. This is the one anybody actually looks up. */
            PUT16(SNES_HOST_HEADER + roomCount * SNES_HOST_ROOM
                     + hostSectors * SNES_HOST_SECTOR
                     + linkCount * SNES_HOST_LINK
                     + modelCount * SNES_HOST_MODEL
                     + animsN * (SNES_HOST_ANIM + 12 + 8)
                     + transCount * SNES_TRANS_SIZE
                     + cmdCount * SNES_CMD_SIZE
                     + triggerCount * SNES_TRIGGER_SIZE + frameExtra);
            PUT16(hostSectors);
            PUT16(linkCount);
            PUT16(staticCount);

            // Where each room starts, in sectors and in the wrapped
            // coordinates everything else uses. The host works out which
            // sector something is in from these, exactly: the sector origin
            // is absolute and the offset within the room is small, so neither
            // term can wrap. Deriving it from a position alone cannot be
            // done -- TR's world is wider than sixteen bits.
            int32 hostSectorOffset = 0;
            for (int32 i = 0; i < roomCount; i++) {
                PUT8((blobs[i].x >> 10) & 0xFF);
                PUT8((blobs[i].z >> 10) & 0xFF);
                PUT16(blobs[i].x);
                PUT16(blobs[i].z);
                PUT16(hostSectorOffset);
                // The water flag rides in bit 7 of the width: the host's
                // water state machine asks it every tick, and a byte of
                // its own would move every table behind it for one bit.
                if (blobs[i].xSectors >= 128) {
                    printf("  room %d is %d sectors wide; the host width byte holds 127\n",
                           (int)i, (int)blobs[i].xSectors);
                    exit(1);
                }
                PUT8(blobs[i].xSectors | ((blobs[i].flags & 1) ? 0x80 : 0));
                PUT8(blobs[i].zSectors);
                hostSectorOffset += blobs[i].xSectors * blobs[i].zSectors;
                if (getenv("OL_DUMP_ROOMS")) {
                    fprintf(stderr, "room %d:%s x %d y %d..%d z %d  %dx%d sectors\n",
                            (int)i, (blobs[i].flags & 1) ? " water" : "",
                            (int)blobs[i].x, (int)blobs[i].y,
                            (int)(blobs[i].y + blobs[i].yBottom), (int)blobs[i].z,
                            (int)blobs[i].xSectors, (int)blobs[i].zSectors);
                }
                // OL_DUMP_SECTORS=<room>: that room's floor and ceiling
                // bytes as a grid, rows along x, columns along z, with the
                // trigger of each sector -- for planning a scene in the
                // runner without decoding the host table by hand.
                if (getenv("OL_DUMP_SECTORS") && atoi(getenv("OL_DUMP_SECTORS")) == i) {
                    const TR1_PC::Room* room = level->rooms + i;
                    fprintf(stderr, "room %d sectors (floor/ceiling/trigger), x %d.., z %d..:\n",
                            (int)i, (int)room->info.x, (int)room->info.z);
                    for (int32 sx = 0; sx < room->xSectors; sx++) {
                        fprintf(stderr, "  x%2d:", (int)sx);
                        for (int32 sz = 0; sz < room->zSectors; sz++) {
                            const TR1_PC::Room::Sector& sec = room->sectors[sx * room->zSectors + sz];
                            uint16 tr = buildTrigger(level, sec.floorIndex);
                            if (tr == SNES_TRIGGER_NONE)
                                fprintf(stderr, " %4d/%4d   ", (int)sec.floor, (int)sec.ceiling);
                            else
                                fprintf(stderr, " %4d/%4d t%-2d", (int)sec.floor, (int)sec.ceiling, (int)tr);
                        }
                        fprintf(stderr, "\n");
                    }
                }
            }

            // A compact, CPU-readable copy of every sector. The renderer's
            // six-byte records remain unchanged in the room blobs; these add
            // horizontal portals and slants so game logic can answer a query
            // at any point without asking the in-flight renderer. The heights
            // are the sector's own; everything else is the link table's.
            {
                int32 at = 0;
                for (int32 r = 0; r < roomCount; r++) {
                    const TR1_PC::Room* room = level->rooms + r;
                    for (int32 i = 0; i < room->xSectors * room->zSectors; i++) {
                        const TR1_PC::Room::Sector& sector = room->sectors[i];
                        PUT8(sector.floor);
                        PUT8(sector.ceiling);
                        PUT16(sectorLink[at++]);
                    }
                }
            }

            for (int32 i = 0; i < linkCount; i++) {
                uint64_t k = linkTable[i];
                PUT8(k);                // roomBelow
                PUT8(k >> 8);           // roomAbove
                PUT8(k >> 16);          // portal
                PUT8(k >> 24);          // trigger
                PUT16(k >> 32);         // floor slants, X low
                PUT16(k >> 48);         // ceiling slants
            }

            for (int32 i = 0; i < modelCount; i++) {
                PUT16(modelFirstAnim[i]);
                PUT16(modelAnimCount[i]);
                for (int32 k = 0; k < 6; k++) PUT16(modelBox[i * 6 + k]);
            }

            for (int32 a = 0; a < animsN; a++) {
                for (int32 k = 0; k < 6; k++) PUT16(animBox[a * 6 + k]);
            }
            if (fx3) {
                memcpy(h, frameBoxes.data() + animsN * 12, frameExtra);
                h += frameExtra;
            }

            for (int32 a = 0; a < animsN; a++) {
                PUT16(animSpeed[a * 2 + 0] & 0xFFFF);
                PUT16((animSpeed[a * 2 + 0] >> 16) & 0xFFFF);
                PUT16(animSpeed[a * 2 + 1] & 0xFFFF);
                PUT16((animSpeed[a * 2 + 1] >> 16) & 0xFFFF);
            }

            for (int32 a = 0; a < animsN; a++) {
                PUT8(animTickCount[a]);
                PUT8(animFrameRate[a]);
                PUT16(animNextAnim[a]);
                PUT16(animNextFrame[a]);
                PUT16(animTransStart[a]);
                PUT8(animTransCount[a]);
                PUT8(animState[a]);
                PUT16(animCmdStart[a]);
                PUT8(animCmdCount[a]);
                PUT8(0);
            }

            memcpy(h, transData, transCount * SNES_TRANS_SIZE);
            h += transCount * SNES_TRANS_SIZE;

            memcpy(h, cmdData, cmdCount * SNES_CMD_SIZE);
            h += cmdCount * SNES_CMD_SIZE;

            memcpy(h, triggerData, triggerCount * SNES_TRIGGER_SIZE);
            h += triggerCount * SNES_TRIGGER_SIZE;

            for (int32 s = 0; s < hostItems; s++)
            {
                const int32 i = slotItem[s];
                const TR1_PC::Item& item = level->items[i];

                int32 modelIndex = modelOfType(level, item.type);
                if (modelIndex < 0) continue;

                PUT8(item.type);
                PUT8(item.roomIndex);
                PUT8(modelIndex);
                // TR's own two facts about activation, in the host's flag
                // layout: the five-bit mask (TR bits 9-13, ITEM_FLAG_MASK
                // here) and the hidden bit (TR bit 8, ITEM_FLAG_ONCE's slot,
                // as upstream reads it). A full mask is "active from the
                // start"; the hidden bit is "invisible until a trigger".
                // Bit 6 additionally carries TR's animated/wild-aggression
                // bit 7; the FX3 loader moves it to ItemObj::flags2.
                PUT8(((item.flags >> 9) & 0x1F) | ((item.flags & 0x0100) ? 0x20 : 0)
                     | ((item.flags & 0x0080) ? 0x40 : 0)); // wild aggression -> flags2
                PUT16(item.pos.x);
                PUT16(item.pos.y);
                PUT16(item.pos.z);
                PUT16(item.angleY);
                PUT16(triggerUnderItem(level, item));
                // The sector it stands in, absolutely. Positions here are
                // sixteen bits and TR's world is not -- LEVEL1 spans 98
                // sectors, so a coordinate past 32767 wraps. That is harmless
                // for everything that works room-relative, which is all the
                // renderer does and most of what the host does, and fatal for
                // anything that indexes a table by absolute position. The
                // navigation boxes are such a table.
                PUT8((item.pos.x >> 10) & 0xFF);
                PUT8((item.pos.z >> 10) & 0xFF);
                if (getenv("OL_DUMP_ITEMS")) {
                    fprintf(stderr, "item %d (TR %d): type %d room %d at %d,%d,%d angle %d flags %04x trigger %d\n",
                            (int)s, (int)i, (int)item.type, (int)item.roomIndex,
                            (int)item.pos.x, (int)item.pos.y, (int)item.pos.z,
                            (int)item.angleY, (unsigned)item.flags,
                            (int)triggerUnderItem(level, item));
                }
            }

            // TR's navigation graph, last because it is the biggest thing
            // here and everything else is indexed from the front. Box bounds
            // are in sectors, as upstream's runtime Box has them. TR PC's
            // max is inclusive; GBA/32X store the exclusive sector edge.
            for (int32 i = 0; i < level->boxesCount; i++)
            {
                const TR1_PC::Box& b = level->boxes[i];
                PUT8(b.minZ >> 10);
                PUT8((b.maxZ + 1) >> 10);
                PUT8(b.minX >> 10);
                PUT8((b.maxX + 1) >> 10);
                PUT16(b.floor);
                PUT16(b.overlap);
            }
            for (int32 i = 0; i < level->overlapsCount; i++) {
                PUT16(level->overlaps[i]);
            }

            if (fx3) {
                memcpy(h, zoneData.data(), zoneData.size());
                h += zoneData.size();
            } else {
                // Preserve GSU-2's original ground-1/fly, unflipped layout.
                for (int32 i = 0; i < level->boxesCount; i++) {
                    PUT16(level->zones[0].ground1[i]);
                }
                for (int32 i = 0; i < level->boxesCount; i++) {
                    PUT16(level->zones[0].fly[i]);
                }
            }


            // The static meshes she can walk into, and where each room's run
            // of them starts. Last, so nothing in front of them moved.
            for (int32 i = 0; i < staticCount; i++) {
                const HostStatic& hs = statics[i];
                PUT8(hs.room);
                PUT8(0);
                PUT16(hs.x);
                PUT16(hs.y);
                PUT16(hs.z);
                for (int32 k = 0; k < 6; k++) PUT16(hs.box[k]);
            }
            for (int32 r = 0; r <= roomCount; r++) PUT16(staticFirst[r]);

            memcpy(h, aimData.data(), aimData.size());
            h += aimData.size();
            // Footer avoids changing every preceding host table offset.
            const int32 navBase = int32(h - hostData.data());
            memcpy(h, navData.data(), navData.size());
            h += navData.size();
            PUT16(navBase);
            if (h != hostData.data() + hostBytes) {
                printf("host table length mismatch\n"); exit(1);
            }

            #undef PUT16
            #undef PUT8
        }
        if (!fx3) memcpy(image + hostBase, hostData.data(), hostBytes);
        {
            const char* names[] = {"ROOM", "SECTOR", "LINK", "MODEL", "ANIM_BOX",
                "ANIM_SPEED", "ANIM", "TRANS", "CMD", "TRIGGER", "ITEM", "BOX",
                "OVERLAP", "ZONE", "STATIC", "STATIC_FIRST", "AIM", "NAV"};
            const int32 sizes[] = {roomCount * SNES_HOST_ROOM,
                hostSectors * SNES_HOST_SECTOR, linkCount * SNES_HOST_LINK,
                modelCount * SNES_HOST_MODEL, animsN * 12 + frameExtra, animsN * 8,
                animsN * SNES_HOST_ANIM, transCount * SNES_TRANS_SIZE,
                cmdCount * SNES_CMD_SIZE, triggerCount * SNES_TRIGGER_SIZE,
                hostItems * SNES_HOST_ITEM, level->boxesCount * 8,
                level->overlapsCount * 2, zoneBytes,
                staticCount * SNES_HOST_STATIC, (roomCount + 1) * 2,
                (int32)aimData.size(), (int32)navData.size()};
            char path[512];
            sprintf(path, "%s/HOST.RAW", dir);
            FILE* raw = fopen(path, "wb");
            if (!raw) { printf("can't write %s\n", path); exit(1); }
            const bool written = fwrite(hostData.data(), 1, hostData.size(), raw) == hostData.size();
            if (fclose(raw) || !written) { printf("can't finish %s\n", path); exit(1); }
            sprintf(path, "%s/HOST.json", dir);
            FILE* meta = fopen(path, "w");
            if (!meta) { printf("can't write %s\n", path); exit(1); }
            fprintf(meta, "{\"counts\":{\"items\":%d,\"models\":%d,\"anims\":%d,"
                "\"trans\":%d,\"triggers\":%d,\"cmds\":%d,\"boxes\":%d,"
                "\"overlaps\":%d,\"rooms\":%d,\"sectors\":%d,\"links\":%d,"
                "\"statics\":%d},\"tables\":{\n", hostItems, modelCount, animsN,
                transCount, triggerCount, cmdCount, level->boxesCount,
                level->overlapsCount, roomCount, hostSectors, linkCount, staticCount);
            int32 offset = SNES_HOST_HEADER;
            for (int32 i = 0; i < 18; i++) {
                fprintf(meta, "%s\"%s\":{\"offset\":%d,\"length\":%d}\n",
                        i ? "," : "", names[i], offset, sizes[i]);
                offset += sizes[i];
            }
            fprintf(meta, "}}\n");
            if (ferror(meta) || fclose(meta)) { printf("can't finish %s\n", path); exit(1); }
            if (offset + 2 != hostBytes) { printf("host manifest length mismatch\n"); exit(1); }
        }
        for (int32 i = 0; i < roomCount; i++) {
            memcpy(image + placement[i], blobs[i].data, blobs[i].size);
        }
        for (int32 i = 0; i < meshCount; i++) {
            if (meshPlace[i] >= 0)
                memcpy(image + meshPlace[i], meshes[i].data, meshes[i].size);
        }
        for (int32 i = 0; i < modelCount; i++) {
            if (modelPlace[i] < 0) continue;
            memcpy(image + modelPlace[i], models[i].data, models[i].size);
            for (int32 k = 0; k < models[i].animCount; k++) {
                if (animPlace[i][k] < 0) continue;
                memcpy(image + animPlace[i][k],
                       models[i].frames + models[i].animOff[k],
                       models[i].animLen[k]);
            }
        }

        // Now that every mesh has an address, the placements can point at one
        // instead of at an index. A mesh with nothing drawable gets the
        // sentinel bank and the renderer skips it.
        int32 placed = 0, dropped = 0;
        for (int32 i = 0; i < roomCount; i++)
        {
            uint8* inst = image + placement[i] + blobs[i].instOffset;
            for (int32 j = 0; j < blobs[i].instCount; j++, inst += SNES_INST_SIZE)
            {
                uint16 meshIndex;
                memcpy(&meshIndex, inst, 2);

                if (meshIndex >= meshCount || meshPlace[meshIndex] < 0) {
                    inst[2] = SNES_INST_NONE;
                    dropped++;
                    continue;
                }

                uint16 addr = (uint16)(meshPlace[meshIndex] % GSU_BANK_SIZE);
                memcpy(inst, &addr, 2);
                inst[2] = (uint8)(meshPlace[meshIndex] / GSU_BANK_SIZE);
                placed++;
            }
        }

        // FX3's room sprites, for the runner: each placement in LEVEL.BIN
        if (packFx3) {
            char rpath[512];
            sprintf(rpath, "%s/ROOMSPRITES.json", dir);
            FILE* rf = fopen(rpath, "wb");
            if (!rf) { printf("can't write %s\n", rpath); exit(1); }
            fprintf(rf, "{\"sprites\":[");
            for (size_t k = 0; k < roomSpriteRooms.size(); k++)
                fprintf(rf, "%s{\"room\":%d,\"offset\":%d,\"x\":%d,\"y\":%d,\"z\":%d}", k ? "," : "",
                        roomSpriteRooms[k], placement[roomSpriteRooms[k]] + roomSpriteInst[k],
                        roomSpriteWorld[k * 3], roomSpriteWorld[k * 3 + 1], roomSpriteWorld[k * 3 + 2]);
            fprintf(rf, "]}\n");
            fclose(rf);
            printf("  room sprites: %d placements of %d textures\n", int32(roomSpriteRooms.size()),
                   int32(roomSpriteTex.size()));
        }

        // Model nodes carry the same mesh reference, and want the same fix.
        // The header's frame pointer is an offset until now, for the same
        // reason: nothing knows where the model landed until it has landed.
        for (int32 i = 0; i < modelCount; i++)
        {
            if (modelPlace[i] < 0) continue;

            // The animation table: an address and a bank per animation, which
            // is what lets a model's frames live wherever they fit.
            uint8* at = image + modelPlace[i] + models[i].animTableOff;
            for (int32 k = 0; k < models[i].animCount; k++, at += SNES_ANIM_SIZE)
            {
                if (animPlace[i][k] < 0) continue;
                uint16 addr = (uint16)(animPlace[i][k] % GSU_BANK_SIZE);
                memcpy(at, &addr, 2);
                at[2] = (uint8)(animPlace[i][k] / GSU_BANK_SIZE);
            }

            uint8* node = image + modelPlace[i] + SNES_MODEL_HDR;
            for (int32 j = 0; j < models[i].nodeCount; j++, node += SNES_NODE_SIZE)
            {
                if (node[2] == SNES_INST_NONE) continue;

                uint16 meshIndex;
                memcpy(&meshIndex, node, 2);
                if (meshIndex >= meshCount || meshPlace[meshIndex] < 0) {
                    node[2] = SNES_INST_NONE;
                    continue;
                }
                uint16 addr = (uint16)(meshPlace[meshIndex] % GSU_BANK_SIZE);
                memcpy(node, &addr, 2);
                node[2] = (uint8)(meshPlace[meshIndex] / GSU_BANK_SIZE);
            }
        }

        // The pistol controller and renderer share the original animation
        // lengths and the two mesh sets. Keep these addresses out of the
        // host level blob: they are immutable cartridge data, consumed by
        // the GSU between frames. mkbanks adds the final ROM bank base.
        {
            char path[512];
            sprintf(path, "%s/WEAPON.json", dir);
            FILE* f = fopen(path, "wb");
            if (!f) { printf("can't write %s\n", path); exit(1); }
            const int32 body = level->getModelIndex(ITEM_LARA);
            const int32 guns = level->getModelIndex(ITEM_LARA_PISTOLS);
            bool valid = body >= 0 && guns >= 0 &&
                         models[body].nodeCount == 15 && models[guns].nodeCount == 15 &&
                         models[guns].animCount >= 4;
            fprintf(f, "{\"animations\":[");
            if (valid) for (int32 i = 0; i < 4; i++) {
                const TR1_PC::Animation& a = level->anims[level->models[guns].animIndex + i];
                // drawLaraNodes indexes these poses directly by game tick.
                if (a.frameRate != 1 || animPlace[guns][i] < 0 ||
                    a.frameEnd < a.frameBegin || a.frameEnd - a.frameBegin > 255) {
                    printf("unsupported pistol animation %d\n", i); exit(1);
                }
                fprintf(f, "%s{\"offset\":%d,\"last\":%d}", i ? "," : "",
                        animPlace[guns][i], a.frameEnd - a.frameBegin);
            }
            fprintf(f, "],\"meshes\":[");
            const int32 joints[4] = { 1, 4, 10, 13 }; // left/right thigh, right/left hand
            if (valid) for (int32 i = 0; i < 8; i++) {
                const int32 m = (i & 1) ? guns : body;
                const int32 mesh = level->models[m].start + joints[i >> 1];
                const int32 offset = meshUsed[mesh] ? meshPlace[mesh] : -1;
                fprintf(f, "%s{\"offset\":%d,\"radius\":%d}", i ? "," : "",
                        offset, offset < 0 ? 0 : meshRadiusByte(level, mesh));
            }
            // drawFlash's mesh, level.models[ITEM_MUZZLE_FLASH].start (FX3)
            const int32 flash = level->getModelIndex(166);
            const int32 flashMesh = flash >= 0 ? level->models[flash].start : -1;
            const int32 flashAt = (valid && flashMesh >= 0 && meshUsed[flashMesh]) ? meshPlace[flashMesh] : -1;
            fprintf(f, "],\"flash\":{\"offset\":%d,\"radius\":%d}", flashAt,
                    flashAt < 0 ? 0 : meshRadiusByte(level, flashMesh));
            if (fx3) writeWeaponKit(f, level, models, animPlace, meshUsed, meshPlace);
            fprintf(f, "}\n");
            fclose(f);
        }

        // And the entity placements point at a model stream.
        int32 itemsPlaced = 0, itemsDropped = 0;
        for (int32 i = 0; i < roomCount; i++)
        {
            uint8* item = image + placement[i] + blobs[i].itemOffset;
            for (int32 j = 0; j < blobs[i].itemCount; j++, item += SNES_ITEM_SIZE)
            {
                uint16 modelIndex;
                memcpy(&modelIndex, item, 2);

                if (modelIndex >= modelCount || modelPlace[modelIndex] < 0) {
                    item[2] = SNES_INST_NONE;
                    itemsDropped++;
                    continue;
                }

                uint16 addr = (uint16)(modelPlace[modelIndex] % GSU_BANK_SIZE);
                memcpy(item, &addr, 2);
                item[2] = (uint8)(modelPlace[modelIndex] / GSU_BANK_SIZE);
                itemsPlaced++;
            }
        }

        if (fx3) {
            // The inventory's models, which nothing in the level places: per
            // upstream InvSlot, 8 bytes -- u16 model stream address, u8 GSU
            // bank relative to the level's ($FF: not in this level), u8 its
            // first animation's frame rate, u16 that animation's frameEnd -
            // frameBegin (getAnimLength), u16 0. src/inventory_draw.c.
            // (a 27th: the closed passport, drawn while it is not chosen)
            static const uint8 invTypes[27] = {
                127, 133, 134, 135, 136, 117, 116, 115, 114, 150, 72, 99, 103, 100, 104,
                101, 105, 102, 106, 109, 108, 71, 95, 96, 97, 73, 81 };
            std::vector<uint8> inv(27 * 8, 0);
            int32 found = 0;
            for (int32 k = 0; k < 27; k++) {
                uint8* r = &inv[k * 8];
                r[2] = SNES_INST_NONE;
                int32 m = -1;
                for (int32 j = 0; j < level->modelsCount; j++)
                    if (int32(level->models[j].type) == invTypes[k]) m = j;
                if (m < 0 || modelPlace[m] < 0) continue;
                const uint16 addr = uint16(modelPlace[m] % GSU_BANK_SIZE);
                r[0] = uint8(addr); r[1] = uint8(addr >> 8);
                r[2] = uint8(modelPlace[m] / GSU_BANK_SIZE);
                const TR1_PC::Animation& anim = level->anims[level->models[m].animIndex];
                r[3] = uint8(anim.frameBegin == anim.frameEnd ? 1 : anim.frameRate);
                const int32 len = anim.frameEnd - anim.frameBegin;
                r[4] = uint8(len); r[5] = uint8(len >> 8);
                found++;
            }
            char ipath[512];
            sprintf(ipath, "%s/INV.BIN", dir);
            FILE* inf = fopen(ipath, "wb");
            if (!inf) { printf("can't write %s\n", ipath); exit(1); }
            fwrite(inv.data(), 1, inv.size(), inf);
            fclose(inf);
            printf("      inventory: %d of 27 slot models in the level\n", found);

            // FONT.BIN (buildFont): the glyphs' addresses, now they have one
            if (fontPlace >= 0) {
                fontInfo[58] = uint8(fontPlace / GSU_BANK_SIZE);
                const uint16 remap = uint16((fontPlace + fontRemap) % GSU_BANK_SIZE);
                fontInfo[60] = uint8(remap);
                fontInfo[61] = uint8(remap >> 8);
                const uint16 dark = uint16((fontPlace + fontDark) % GSU_BANK_SIZE);
                fontInfo[62] = uint8(dark);
                fontInfo[63] = uint8(dark >> 8);
                for (size_t g = 0; g < fontOffset.size(); g++) {
                    const uint16 addr = uint16((fontPlace + fontOffset[g]) % GSU_BANK_SIZE);
                    fontInfo[FONT_GLYPHS + g * 6] = uint8(addr);
                    fontInfo[FONT_GLYPHS + 1 + g * 6] = uint8(addr >> 8);
                }
            } else {
                fontInfo.clear();
            }
            sprintf(ipath, "%s/FONT.BIN", dir);
            FILE* ff = fopen(ipath, "wb");
            if (!ff) { printf("can't write %s\n", ipath); exit(1); }
            if (!fontInfo.empty()) fwrite(fontInfo.data(), 1, fontInfo.size(), ff);
            fclose(ff);
        }

        char path[512];
        sprintf(path, "%s/LEVEL.BIN", dir);
        FILE* f = fopen(path, "wb");
        if (!f) {
            printf("can't write \"%s\"\n", path);
        } else {
            fwrite(image, 1, totalSize, f);
            fclose(f);
            printf("  LEVEL.BIN: %d rooms, %d KB, %d GSU bank(s)\n",
                   roomCount, totalSize / 1024, totalSize / GSU_BANK_SIZE);
            int32 verts = 0, faces = 0, portals = 0, sectors = 0;
            for (int32 i = 0; i < roomCount; i++) {
                verts += blobs[i].vCount;
                faces += blobs[i].faceCount;
                portals += blobs[i].portalCount;
                sectors += blobs[i].xSectors * blobs[i].zSectors;
            }
            int32 meshN = 0, meshV = 0, meshF = 0;
            for (int32 i = 0; i < meshCount; i++) {
                if (meshPlace[i] < 0) continue;
                meshN++;
                meshV += meshes[i].vCount;
                meshF += meshes[i].faceCount;
            }
            printf("      %d vertices, %d faces, %d portals, %d sectors\n",
                   verts, faces, portals, sectors);
            printf("      %d static meshes (%d verts, %d faces), "
                   "%d placements, %d dropped\n",
                   meshN, meshV, meshF, placed, dropped);
            int32 modelN = 0, modelNodes = 0;
            for (int32 i = 0; i < modelCount; i++) {
                if (modelPlace[i] < 0) continue;
                modelN++;
                modelNodes += models[i].nodeCount;
            }
            int32 modelFrames = 0, modelBytes = 0;
            for (int32 i = 0; i < modelCount; i++) {
                if (modelPlace[i] < 0) continue;
                modelFrames += models[i].frameTotal;
                modelBytes += models[i].size;
                for (int32 k = 0; k < models[i].animCount; k++)
                    modelBytes += models[i].animLen[k];
            }
            printf("      %d models (%d nodes, %d frames, %d KB), "
                   "%d entities, %d dropped\n",
                   modelN, modelNodes, modelFrames, modelBytes / 1024,
                   itemsPlaced, itemsDropped);
            printf("      colour-keyed faces: %d of %d textured (source-index mask preserved)\n",
                   g_akillFaces, g_facesSeen);
            printf("      host table: %d entities, %d rooms/%d sectors/%d links, "
                   "%d models, %d anims, "
                   "%d transitions, %d commands, %d triggers, %d colliding "
                   "statics, %d bytes at $%04X\n",
                   hostItems, roomCount, hostSectors, linkCount, modelCount,
                   animsN, transCount, cmdCount, triggerCount, staticCount,
                   hostBytes, hostBase);
        }

        for (int32 i = 0; i < roomCount; i++) delete[] blobs[i].data;
        for (int32 i = 0; i < meshCount; i++) delete[] meshes[i].data;
        for (int32 i = 0; i < modelCount; i++) {
            delete[] models[i].data;
            delete[] models[i].frames;
            delete[] models[i].animOff;
            delete[] models[i].animLen;
            delete[] models[i].animFrames;
            delete[] animPlace[i];
        }
        delete[] animPlace;
        delete[] blobs;
        delete[] meshes;
        delete[] entries;
        delete[] placement;
        delete[] meshPlace;
        delete[] modelPlace;
        delete[] models;
        delete[] meshUsed;
        delete[] itemSlot;
        delete[] slotItem;
        delete[] animFirstFrame;
        delete[] animFrameCount;
        delete[] animTickCount;
        delete[] animFrameRate;
        delete[] animNextAnim;
        delete[] animNextFrame;
        delete[] modelFirstAnim;
        delete[] modelAnimCount;
        delete[] modelBox;
        delete[] animBox;
        delete[] animSpeed;
        delete[] animCmdStart;
        delete[] animCmdCount;
        delete[] cmdData;
        delete[] animTransStart;
        delete[] animTransCount;
        delete[] animState;
        delete[] transData;
        delete[] triggerData;
        delete[] triggerOfFloor;
        delete[] image;
    }

    // ------------------------------------------------------------------

    void process(const char* dir, TR1_PC* level, bool fx3)
    {
        printf("level: %d rooms, %d tiles, %d object textures\n",
               level->roomsCount, level->tilesCount, level->objectTexturesCount);

        if (fx3) appendDartPool(level);    // GSU-2 has no dart code to fly them
        if (fx3) appendFxPool(level);      // nor effect code; these go last
        buildPalette(level);

        char path[512];
        sprintf(path, "%s/PALETTE.BIN", dir);
        FILE* f = fopen(path, "wb");
        if (!f) {
            printf("can't write \"%s\" -- does the directory exist?\n", path);
            return;
        }
        fwrite(palette, 2, SNES_SHADES * SNES_COLORS, f);
        fclose(f);
        printf("  PALETTE.BIN: %d shade rows x %d colours\n",
               SNES_SHADES, SNES_COLORS);

        writeHUD(dir, level);
        if (fx3) buildFont(level);

        int32 pages = writeTextures(dir, level);
        printf("  %d texture pages, %d KB, one per GSU ROM bank\n", pages,
               pages * SNES_PAGE_SIZE / 1024);

        writeLevel(dir, level, fx3);
        if (fx3) writeTitle(dir);

        SoundSNES sound;
        sound.gunRegion = fx3;                  // FX3's four guns: GUNSND.BIN
        sound.write(dir, level);
    }
};

#endif
