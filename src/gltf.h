#ifndef _H_GLTF
#define _H_GLTF

#include "json.h"

#define ACCESSOR_TYPES(E) \
    E( SCALAR ) \
    E( VEC2   ) \
    E( VEC3   ) \
    E( VEC4   ) \
    E( MAT2   ) \
    E( MAT3   ) \
    E( MAT4   )

struct GLTF {
    enum AccessorType { ACCESSOR_TYPES(DECL_ENUM) ACCESSOR_TYPE_MAX };

    JSON *root;
    JSON *asset;
    JSON *buffers;
    JSON *bufferViews;
    JSON *accessors;
    JSON *meshes;
    JSON *images;
    JSON *samplers;
    JSON *textures;
    JSON *materials;
    JSON *nodes;
    JSON *scenes;
    JSON *skins;
    JSON *animations;

    char *binaryData;
    int  binarySize;

    GLTF() : binaryData(NULL), binarySize(0) {
        root = new JSON(JSON::OBJECT);

        asset       = root->add(JSON::OBJECT, "asset");
        images      = root->add(JSON::ARRAY, "images");
        samplers    = root->add(JSON::ARRAY, "samplers");
        textures    = root->add(JSON::ARRAY, "textures");
        materials   = root->add(JSON::ARRAY, "materials");
        buffers     = root->add(JSON::ARRAY, "buffers");
        bufferViews = root->add(JSON::ARRAY, "bufferViews");
        accessors   = root->add(JSON::ARRAY, "accessors");
        meshes      = root->add(JSON::ARRAY, "meshes");
        nodes       = root->add(JSON::ARRAY, "nodes");
        scenes      = root->add(JSON::ARRAY, "scenes");
        skins       = NULL;//root->add(JSON::ARRAY, "skins");
        animations  = NULL;//root->add(JSON::ARRAY, "animations");

        asset->add("generator", "OpenLara");
        asset->add("version", "2.0");

        root->add("scene", 0);
    }

    ~GLTF() {
        delete[] binaryData;
        delete root;
    }

    int getBufferSize() {
        return 8 * 1024 * 1024 + binarySize;
    }

    int save(char *buffer) {
        struct Header {
            uint32 magic;
            uint32 version;
            uint32 length;
        };

        struct Chunk {
            uint32 length;
            uint32 type;
        };

        char *start = buffer;

        Header *header = (Header*)buffer;
        buffer += sizeof(Header);

        Chunk *chunk = (Chunk*)buffer;
        buffer += sizeof(Chunk);

        root->save(buffer);

        chunk->length = strlen(buffer);
        chunk->type   = FOURCC("JSON");

        buffer += chunk->length;

        while (( int(buffer - start) % 4) != 0) {
            *buffer++ = ' ';
            chunk->length++;
        }

        if (binaryData) {
            Chunk *chunk = (Chunk*)buffer;
            buffer += sizeof(Chunk);
            chunk->length = binarySize;
            chunk->type   = FOURCC("BIN\0");

            memcpy(buffer, binaryData, binarySize);

            buffer += binarySize;

            while (( (buffer - start) % 4) != 0) {
                *buffer++ = 0;
                chunk->length++;
            }
        }

        header->magic = FOURCC("glTF");
        header->version = 2;
        header->length  = buffer - start;

        return header->length;
    }

    JSON* addAccessor(int bufferView, int byteStride, int byteOffset, int count, AccessorType type, int format, bool normalized = false, const vec4 &vMin = vec4(0.0f), const vec4 &vMax = vec4(0.0f)) {
        static const char *AccessorTypeName[ACCESSOR_TYPE_MAX]  = { ACCESSOR_TYPES(DECL_STR) };

        JSON *item = accessors->add(JSON::OBJECT);

        item->add("bufferView", bufferView);
        
        if (byteStride) {
            //item->add("byteStride", byteStride);
        }

        if (byteOffset) {
            item->add("byteOffset", byteOffset);
        }
        
        item->add("count", count);
        item->add("type", AccessorTypeName[type]);
        item->add("componentType", format);

        if (normalized) {
            item->add("normalized", true);
        }

        if (vMin != vMax || vMin != vec4(0.0f)) {
            JSON *itemMin = item->add(JSON::ARRAY, "min");
            JSON *itemMax = item->add(JSON::ARRAY, "max");

            int k;
            switch (type) {
                case SCALAR : k = 1; break;
                case VEC2   : k = 2; break;
                case VEC3   : k = 3; break;
                case VEC4   : k = 4; break;
                default     : k = 0;
            }

            for (int i = 0; i < k; i++) {
                itemMin->add(NULL, vMin[i]);
                itemMax->add(NULL, vMax[i]);
            }
        }

        return item;
    }

    JSON* addBuffer(void *data, int size) {
        JSON *item = buffers->add(JSON::OBJECT);

        item->add("byteLength", size);
        
        binarySize = size;
        binaryData = new char[size];
        memcpy(binaryData, data, size);

        return item;
    }

    JSON* addBufferView(int buffer, int byteStride, int byteOffset, int byteLength) {
        JSON *item = bufferViews->add(JSON::OBJECT);

        item->add("buffer", buffer);
        if (byteStride) {
            item->add("byteStride", byteStride);
        }
        if (byteOffset) {
            item->add("byteOffset", byteOffset);
        }
        item->add("byteLength", byteLength);

        return item;
    }

    JSON* addMesh(const char *name, int material, int INDICES, int POSITION = -1, int NORMAL = -1, int TEXCOORD = -1, int COLOR = -1, int JOINTS = -1, int WEIGHTS = -1) {
        JSON *item = meshes->add(JSON::OBJECT);

        if (name) item->add("name", name);
        JSON *primitives = item->add(JSON::ARRAY, "primitives");
        JSON *part = primitives->add(JSON::OBJECT);
        JSON *attributes = part->add(JSON::OBJECT, "attributes");
        if (POSITION >= 0) attributes->add("POSITION",   POSITION);
        if (NORMAL   >= 0) attributes->add("NORMAL",     NORMAL);
        if (TEXCOORD >= 0) attributes->add("TEXCOORD_0", TEXCOORD);
        if (COLOR    >= 0) attributes->add("COLOR_0",    COLOR);
        if (JOINTS   >= 0) attributes->add("JOINTS_0",   JOINTS);
        if (WEIGHTS  >= 0) attributes->add("WEIGHTS_0",  WEIGHTS);
        part->add("indices", INDICES);
        part->add("material", material);

        return item;
    }

    JSON* addImage(const char *uri) {
        JSON *item = images->add(JSON::OBJECT);

        item->add("uri", uri);

        return item;
    }

    JSON* addSampler(int magFilter, int minFilter, int wrapS, int wrapT) {
        JSON *item = samplers->add(JSON::OBJECT);

        item->add("magFilter", magFilter);
        item->add("minFilter", minFilter);
        item->add("wrapS",     wrapS);
        item->add("wrapT",     wrapT);

        return item;
    }

    JSON* addTexture(const char *name, int sampler, int source) {
        JSON *item = textures->add(JSON::OBJECT);

        if (name) item->add("name", name);
        item->add("sampler", sampler);
        item->add("source",  source);

        return item;
    }

    JSON* addMaterial(const char *name, int baseColorTextureIndex, int baseColorTextureTexCoord, float roughnessFactor, float metallicFactor) {
        JSON *item = materials->add(JSON::OBJECT);

        if (name) item->add("name", name);

        item->add("alphaMode", "MASK");

        JSON *pbr = item->add(JSON::OBJECT, "pbrMetallicRoughness");
        pbr->add("roughnessFactor", roughnessFactor);
        pbr->add("metallicFactor",  metallicFactor);

        JSON *baseTex = pbr->add(JSON::OBJECT, "baseColorTexture");
        baseTex->add("index",    baseColorTextureIndex);
        baseTex->add("texCoord", baseColorTextureTexCoord);

        return item;
    }

    JSON* addNode(const char *name, int mesh, int skin, const vec3 &translation, const quat &rotation) {
        JSON *item = nodes->add(JSON::OBJECT);

        if (name)      item->add("name", name);
        if (mesh >= 0) item->add("mesh", mesh);
        if (skin >= 0) item->add("skin", skin);

        if (translation != vec3(0.0f)) {
            JSON *v = item->add(JSON::ARRAY, "translation");
            v->add(NULL, translation.x);
            v->add(NULL, translation.y);
            v->add(NULL, translation.z);
        }

        if (rotation != quat(0.0f, 0.0f, 0.0f, 1.0f)) {
            JSON *v = item->add(JSON::ARRAY, "rotation");
            v->add(NULL, rotation.x);
            v->add(NULL, rotation.y);
            v->add(NULL, rotation.z);
            v->add(NULL, rotation.w);
        }

        return item;
    }

    JSON *addSkin(const char *name, int inverseBindMatrices, int skeleton, int *joints, int jointsCount) {
        if (!skins) {
            skins = root->add(JSON::ARRAY, "skins");
        }

        JSON *item = skins->add(JSON::OBJECT);

        if (name) {
            item->add("name", name);
        }

        if (inverseBindMatrices >= 0) {
            item->add("inverseBindMatrices", inverseBindMatrices);
        }

        item->add("skeleton", skeleton);

        JSON *v = item->add(JSON::ARRAY, "joints");
        for (int i = 0; i < jointsCount; i++) {
            v->add(NULL, joints[i]);
        }

        return item;
    }

    JSON* addAnimation(const char *name, JSON **samplers, JSON **channels) {
        if (!animations) {
            animations = root->add(JSON::ARRAY, "animations");
        }
        JSON *item = animations->add(JSON::OBJECT);

        if (name)     item->add("name", name);
        if (samplers) *samplers = item->add(JSON::ARRAY, "samplers");
        if (channels) *channels = item->add(JSON::ARRAY, "channels");

        return item;
    }


    JSON* addScene(const char *name, JSON **nodes) {
        JSON *item = scenes->add(JSON::OBJECT);

        item->add("name", name);
        if (nodes) {
            *nodes = item->add(JSON::ARRAY, "nodes");
        }

        return item;
    }
};

#undef ACCESSOR_TYPES

#endif
