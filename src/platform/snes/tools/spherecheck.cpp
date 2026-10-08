/* Upstream's ItemObj::getSpheres/collideSpheres, run on the raw level.
 * sphereupstream.inc is extracted verbatim from src/fixed by
 * tools/mkspherecheck.py; this file only supplies GBA's types (int32 Matrix,
 * FAST_HITMASK) and upstream's `level` shape filled from the PHD through
 * the packer's own TR1 loader -- not from SPHERE.BIN, which it checks.
 * argv: level.phd levelId
 * stdin, per line: blade anim frame x y z angleY, then Lara anim frame x y
 *   z angleY pitch (frames local to the animation, as the port keeps them)
 * stdout, per line: blade count, its spheres x y z r; Lara's; the mask
 * argv: level.phd levelId --joint
 * stdin, per line: type anim frame angleX angleY angleZ joint ox oy oz
 * stdout, per line: getJoint(joint, offset)'s x y z (FX.BIN's oracle)
 * argv: level.phd levelId --camera
 * stdin: type anim frame; stdout: followY, middleY, middleX, middleZ from
 * the original interpolated bounding box (CAMERA.BIN's oracle). */
#include "../../gba/packer/common.h"
#include "../../gba/packer/TR1_PC.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace up {
typedef ::int8 int8; typedef ::uint8 uint8; typedef ::int16 int16;
typedef ::uint16 uint16; typedef ::int32 int32; typedef ::uint32 uint32;
#undef X_ROTX
#undef X_ROTY
#undef X_ROTXY
#undef DP33
#undef sincos
#undef DECODE_ANGLES
#undef X_SQR
#undef ASSERT
#define FIXED_SHIFT_UP 14
#define FIXED_SHIFT FIXED_SHIFT_UP
#define MATRIX_FIXED_SHIFT 0
#define FAST_HITMASK
#define ASSERT(x)
#define X_SQR(x) ((x) * (x))
enum { ITEM_TRAP_SWING_BLADE = 36, MAX_MATRICES = 64 };
struct vec3s { int16 x, y, z; };
struct vec3i { int32 x, y, z;
    vec3i operator-(const vec3i& b) const { vec3i r = { x - b.x, y - b.y, z - b.z }; return r; } };
static vec3i _vec3i(int32 x, int32 y, int32 z) { vec3i r = { x, y, z }; return r; }
struct AABBs { int16 minX, maxX, minY, maxY, minZ, maxZ; };
static AABBs tmpBox;
typedef uint16 divTableInt;
#define DIV_TABLE_SIZE 1025
struct Matrix { int32 e00, e01, e02, e03, e10, e11, e12, e13, e20, e21, e22, e23; };
struct Sphere { vec3i center; int32 radius; };
struct AnimFrame { AABBs box; vec3s pos; uint16 angles[1]; };
struct Anim { uint32 frameOffset; uint8 frameRate; uint16 frameBegin, frameEnd; };
struct Model { uint8 type; int8 count; uint16 start; uint16 nodeIndex; uint16 animIndex; };
struct ModelNode { vec3s pos; uint16 flags; };
struct Mesh { vec3s center; int16 radius; };
struct Level { Model models[256]; Anim* anims; const uint16* animFrames;
               const Mesh** meshes; ModelNode* nodes; } level;
static Matrix gMatrixStack[MAX_MATRICES];
static Matrix* gMatrixPtr = gMatrixStack;
#define matrixGet() *gMatrixPtr
#define matrixPush matrixPush_c
#define matrixPop() gMatrixPtr--
#define matrixSetIdentity matrixSetIdentity_c
#define matrixTranslateRel matrixTranslateRel_c
#define matrixRotateY matrixRotateY_c
#define matrixRotateX matrixRotateX_c
#define matrixRotateZ matrixRotateZ_c
#define matrixRotateYXZ matrixRotateYXZ_c
#define matrixFrame matrixFrame_c
static void matrixTranslateAbs(int32, int32, int32) {}   // getSpheres(.., false) only
struct Lara;
struct ItemObj {
    vec3i pos; vec3s angle; uint8 type; uint16 animIndex; uint16 frameIndex;
    int32 getFrames(const AnimFrame* &frameA, const AnimFrame* &frameB, int32 &animFrameRate) const;
    const AnimFrame* getFrame() const;
    const AABBs& getBoundingBox(bool lerp) const;
    int32 getSpheres(Sphere* spheres, bool flag) const;
    uint32 collideSpheres(Lara* lara) const;
    vec3i getJoint(int32 jointIndex, const vec3i &offset) const;
};
struct Lara : ItemObj {};
static Sphere gSpheres[2][34];
#include "sphereupstream.inc"
}

/* --table SPHERE.BIN: the fast path's data against upstream's getSpheres.
 * Every stored frame of Lara's (reached through her tick directories) at
 * every one of the table's 4,096 yaws: y must be the stored yaw-0 y exactly,
 * and x and z the stored yaw-0 centre turned by the table's sine and cosine,
 * as src/trap_fx3.asm turns it, to within the stored bound. And every blade
 * yaw table's centres must be upstream's exactly. */
static std::vector<unsigned char> sph;
static unsigned rd16(unsigned at) { return sph[at] | (sph[at + 1] << 8); }
static unsigned rd32(unsigned at) { return rd16(at) | (rd16(at + 2) << 16); }
static int rs16(unsigned at) { return (short)rd16(at); }

static int checkTable(const char* path)
{
    FILE* f = std::fopen(path, "rb");
    if (!f) { std::fprintf(stderr, "can't open %s\n", path); return 1; }
    int c;
    while ((c = std::fgetc(f)) != EOF) sph.push_back((unsigned char)c);
    std::fclose(f);
    if (sph.size() < 16446) { std::printf("no sphere data\n"); return 1; }
    const unsigned bound = rd16(16410), yawCount = rd16(16412);
    long frames = 0, bladeTicks = 0, checked = 0;
    for (int t = 0; t < 2; t++) {
        const unsigned h = 16386 + t * 12;
        const unsigned type = rd16(h), count = rd16(h + 2), meshes = rd32(h + 4);
        const unsigned first = rd16(h + 8), anims = rd16(h + 10);
        std::vector<char> seen(sph.size(), 0);
        for (unsigned k = 0; k < anims; k++) {
            const unsigned dir = rd32(meshes + count * 16 + k * 4), ticks = rd16(dir);
            for (unsigned tick = 0; tick < ticks; tick++) {
                up::ItemObj it;
                it.type = (up::uint8)type; it.animIndex = first + k;
                it.frameIndex = up::level.anims[first + k].frameBegin + tick;
                it.pos = { 0, 0, 0 };
                up::Sphere s[34];
                if (t == 1) {
                    for (unsigned y = 0; y < yawCount; y++) {
                        const unsigned table = rd32(16414 + y * 8 + 4);
                        const unsigned rec = table + anims * 4 + (rd16(table + k * 4) + tick) * count * 6;
                        it.angle = { 0, (up::int16)rd16(16414 + y * 8), 0 };
                        it.getSpheres(s, true);
                        for (unsigned i = 0; i < count; i++)
                            if (s[i].center.x != rs16(rec + i * 6) || s[i].center.y != rs16(rec + i * 6 + 2) ||
                                s[i].center.z != rs16(rec + i * 6 + 4)) {
                                std::printf("blade yaw %u anim %u tick %u mesh %u: %d %d %d, table %d %d %d\n",
                                            rd16(16414 + y * 8), k, tick, i, s[i].center.x, s[i].center.y,
                                            s[i].center.z, rs16(rec + i * 6), rs16(rec + i * 6 + 2), rs16(rec + i * 6 + 4));
                                return 1;
                            }
                        bladeTicks++;
                    }
                    continue;
                }
                const unsigned frame = rd32(dir + 2 + tick * 4);
                if (seen[frame]) continue;
                seen[frame] = 1;
                frames++;
                const unsigned centres = frame + 6 + count * 4;
                for (int yaw = 0; yaw < 4096; yaw++) {
                    it.angle = { 0, (up::int16)(yaw << 4), 0 };
                    it.getSpheres(s, true);
                    const int cs = (short)(up::gSinCosTable[yaw] & 0xFFFF), sn = (int)up::gSinCosTable[yaw] >> 16;
                    for (unsigned i = 0; i < count; i++) {
                        const int x0 = rs16(centres + i * 6), y0 = rs16(centres + i * 6 + 2);
                        const int z0 = rs16(centres + i * 6 + 4);
                        const int ax = (cs * x0 + sn * z0) >> 18, az = (cs * z0 - sn * x0) >> 18;
                        if (s[i].center.y != y0 || std::abs(s[i].center.x - ax) > (int)bound ||
                            std::abs(s[i].center.z - az) > (int)bound) {
                            std::printf("Lara anim %u tick %u yaw %d mesh %u: upstream %d %d %d, fast %d %d %d "
                                        "(bound %u)\n", k, tick, yaw << 4, i, s[i].center.x, s[i].center.y,
                                        s[i].center.z, ax, y0, az, bound);
                            return 1;
                        }
                        checked++;
                    }
                }
            }
        }
    }
    std::printf("table ok: %ld frames x 4096 yaws (%ld centres) within %u; %ld blade ticks exact over %u yaws\n",
                frames, checked, bound, bladeTicks, yawCount);
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 3) { std::fprintf(stderr, "usage: spherecheck level.phd id [--table SPHERE.BIN]\n"); return 2; }
    FileStream f(argv[1], false);
    if (!f.isValid()) { std::fprintf(stderr, "can't open %s\n", argv[1]); return 1; }
    TR1_PC tr(f, LevelID(atoi(argv[2])));
    up::Anim* anims = new up::Anim[tr.animsCount];
    for (int i = 0; i < tr.animsCount; i++) {
        anims[i].frameOffset = tr.anims[i].frameOffset;
        anims[i].frameRate = tr.anims[i].frameRate;
        anims[i].frameBegin = tr.anims[i].frameBegin;
        anims[i].frameEnd = tr.anims[i].frameEnd;
    }
    const int nodes = tr.nodesDataSize / 4;
    up::ModelNode* nodeList = new up::ModelNode[nodes > 0 ? nodes : 1];
    for (int i = 0; i < nodes; i++) {
        const TR1_PC::Node* n = (const TR1_PC::Node*)(tr.nodesData + i * 4);
        nodeList[i].flags = (up::uint16)n->flags;
        nodeList[i].pos.x = (up::int16)n->pos.x;
        nodeList[i].pos.y = (up::int16)n->pos.y;
        nodeList[i].pos.z = (up::int16)n->pos.z;
    }
    const up::Mesh** meshes = new const up::Mesh*[tr.meshOffsetsCount];
    for (int i = 0; i < tr.meshOffsetsCount; i++)
        meshes[i] = (const up::Mesh*)((const uint8*)tr.meshData + tr.meshOffsets[i]);
    for (int i = 0; i < tr.modelsCount; i++) {
        up::Model& m = up::level.models[tr.models[i].type & 255];
        m.type = (up::uint8)tr.models[i].type;
        m.count = (up::int8)tr.models[i].count;
        m.start = tr.models[i].start;
        m.nodeIndex = (up::uint16)(tr.models[i].nodeIndex / 4);
        m.animIndex = tr.models[i].animIndex;
    }
    up::level.anims = anims;
    up::level.animFrames = tr.frameData;
    up::level.meshes = meshes;
    up::level.nodes = nodeList;

    if (argc >= 5 && !strcmp(argv[3], "--table"))
        return checkTable(argv[4]);
    if (argc >= 4 && !strcmp(argv[3], "--camera")) {
        int type, anim, frame;
        while (std::scanf("%d %d %d", &type, &anim, &frame) == 3) {
            up::ItemObj it;
            it.type = (up::uint8)type; it.animIndex = anim;
            it.frameIndex = anims[anim].frameBegin + frame;
            const up::AABBs& b = it.getBoundingBox(true);
            std::printf("%d %d %d %d\n", b.maxY + ((b.minY - b.maxY) * 3 >> 2),
                        (b.minY + b.maxY) >> 1, (b.minX + b.maxX) >> 1,
                        (b.minZ + b.maxZ) >> 1);
        }
        return 0;
    }
    if (argc >= 4 && !strcmp(argv[3], "--joint")) {
        int type, anim, frame, ax, ay, az, joint, ox, oy, oz;
        while (std::scanf("%d %d %d %d %d %d %d %d %d %d", &type, &anim, &frame, &ax, &ay, &az,
                          &joint, &ox, &oy, &oz) == 10) {
            up::ItemObj it;
            it.type = (up::uint8)type; it.animIndex = anim;
            it.frameIndex = anims[anim].frameBegin + frame;
            it.pos = { 0, 0, 0 };
            it.angle = { (up::int16)ax, (up::int16)ay, (up::int16)az };
            const up::vec3i j = it.getJoint(joint, up::_vec3i(ox, oy, oz));
            std::printf("%d %d %d\n", j.x, j.y, j.z);
        }
        return 0;
    }

    int ba, bf, bx, by, bz, bay, la, lf, lx, ly, lz, lay, lp;
    while (std::scanf("%d %d %d %d %d %d %d %d %d %d %d %d %d", &ba, &bf, &bx, &by, &bz, &bay,
                      &la, &lf, &lx, &ly, &lz, &lay, &lp) == 13) {
        up::ItemObj blade; up::Lara lara;
        blade.type = 36; blade.animIndex = ba; blade.frameIndex = anims[ba].frameBegin + bf;
        blade.pos = { bx, by, bz }; blade.angle = { 0, (up::int16)bay, 0 };
        lara.type = 0; lara.animIndex = la; lara.frameIndex = anims[la].frameBegin + lf;
        lara.pos = { lx, ly, lz }; lara.angle = { (up::int16)lp, (up::int16)lay, 0 };
        up::uint32 mask = blade.collideSpheres(&lara);
        for (int k = 0; k < 2; k++) {
            const up::ItemObj& it = k ? (const up::ItemObj&)lara : blade;
            int n = it.getSpheres(up::gSpheres[k], true);
            std::printf("%d", n);
            for (int i = 0; i < n; i++)
                std::printf(" %d %d %d %d", up::gSpheres[k][i].center.x, up::gSpheres[k][i].center.y,
                            up::gSpheres[k][i].center.z, up::gSpheres[k][i].radius);
            std::printf(" ");
        }
        std::printf("%u\n", mask);
    }
    return 0;
}
