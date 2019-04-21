#ifndef _H_EXTENSION
#define _H_EXTENSION

#include "mesh.h"

#if defined(_DEBUG) && defined(_OS_WIN) && defined(_GAPI_GL) && !defined(_GAPI_GLES)
    #define GEOMETRY_EXPORT

    #include "animation.h"
    #include "gltf.h"
#endif

namespace Extension {

#ifdef GEOMETRY_EXPORT
    void exportTexture(const char *name, Texture *tex) {
        char *data32 = new char[tex->width * tex->height * 4];

        tex->bind(sDiffuse);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data32);

        Texture::SaveBMP(name, data32, tex->width, tex->height);

        delete[] data32;
    }

    void exportModel(IGame *game, TR::Model &model) {
        TR::Level   *level = game->getLevel();
        MeshBuilder *mesh  = game->getMesh();

        struct ModelVertex {
            vec3   coord;
            vec3   normal;
            vec2   texCoord;
            ubyte4 joints;
            ubyte4 weights;
        };

        char name[256];
        sprintf(name, "models/dump/%d.glb", int(model.type));

        FILE *file = fopen(name, "wb");
        if (!file) {
            LOG("dump: can't dump model to file \"%s\"!\n", name);
            return;
        }

        Index  *indices  = new Index[64 * 1024];
        Vertex *vertices = new Vertex[64 * 1024];

        int iCount = 0, vCount = 0;

        int animRate = 1, animFrames = 0;

        Animation *anim = NULL;
        if (model.animation != 0xFFFF) {
            anim = new Animation(level, &model, true);
            anim->setAnim(0);
            animRate   = max((int)(anim->anims + anim->index)->frameRate, 1);
            animFrames = anim->framesCount / animRate;
        }

    // get model geometry
        MeshBuilder::Geometry geom[3];
        for (int transp = 0; transp < 3; transp++) {
            int blendMask = mesh->getBlendMask(transp);

            for (int j = 0; j < model.mCount; j++) {
                bool forceOpaque = false;
                TR::Entity::fixOpaque(model.type, forceOpaque);

                int index = level->meshOffsets[model.mStart + j];
                if (index || model.mStart + j <= 0) {
                    TR::Mesh &m = level->meshes[index];
                    mesh->buildMesh(geom[transp], blendMask, m, level, indices, vertices, iCount, vCount, 0, j, 0, 0, 0, 0, COLOR_WHITE, false, forceOpaque);
                }
            }
        }

        for (int i = 0; i < iCount; i += 3) { // CCW -> CW
            swap(indices[i], indices[i + 2]);
        }

        int timelineOffset    = 0;
        int translationOffset = timelineOffset + animFrames * sizeof(float);
        int rotationOffset    = translationOffset + animFrames * sizeof(vec3);
        int verticesOffset    = rotationOffset + model.mCount * animFrames * sizeof(quat);
        int indicesOffset     = verticesOffset + vCount * sizeof(ModelVertex);
        int bufferSize        = indicesOffset + iCount * sizeof(Index);

        char *bufferData = new char[bufferSize];

        vec4 vMin(+INF), vMax(-INF);

        mat4 flip = mat4(
            -1, 0, 0, 0,
            0, -1, 0, 0,
            0,  0, 1, 0,
            0,  0, 0, 1
        );
        mat4 flipInv = flip.inverseOrtho();

        ModelVertex *gVertices = (ModelVertex*)(bufferData + verticesOffset);
        for (int i = 0; i < vCount; i++) {
            Vertex      &src = vertices[i];
            ModelVertex &dst = gVertices[i];

            dst.coord    = src.coord;
            dst.normal   = src.normal;
            dst.texCoord = src.texCoord;
            dst.normal   = dst.normal.normal();

            dst.coord    = flip * dst.coord;
            dst.normal   = flip * dst.normal;

            dst.texCoord *= (1.0f / 32767.0f);
            dst.joints   = ubyte4(uint8(src.coord.w), 0, 0, 0);
            dst.weights  = ubyte4(255, 0, 0, 0);

            vMin.x = min(vMin.x, dst.coord.x);
            vMin.y = min(vMin.y, dst.coord.y);
            vMin.z = min(vMin.z, dst.coord.z);

            vMax.x = max(vMax.x, dst.coord.x);
            vMax.y = max(vMax.y, dst.coord.y);
            vMax.z = max(vMax.z, dst.coord.z);
        }
        memcpy(bufferData + indicesOffset, indices, sizeof(Index) * iCount);

        GLTF *gltf = new GLTF();

        gltf->addSampler(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);
        gltf->addImage("objects.bmp");
        gltf->addTexture("texture", 0, 0);
        gltf->addMaterial("material", 0, 0, 1.0f, 0.0f);

        delete[] vertices;
        delete[] indices;

        gltf->addAccessor(0, 0, iCount, GLTF::SCALAR, GL_UNSIGNED_SHORT); // 0
        gltf->addAccessor(1, (int)OFFSETOF(ModelVertex, coord),    vCount, GLTF::VEC3, GL_FLOAT, false, vMin, vMax); // 1
        gltf->addAccessor(1, (int)OFFSETOF(ModelVertex, normal),   vCount, GLTF::VEC3, GL_FLOAT); // 2
        gltf->addAccessor(1, (int)OFFSETOF(ModelVertex, texCoord), vCount, GLTF::VEC2, GL_FLOAT); // 3
        gltf->addAccessor(1, (int)OFFSETOF(ModelVertex, joints),   vCount, GLTF::VEC4, GL_UNSIGNED_BYTE); // 4
        gltf->addAccessor(1, (int)OFFSETOF(ModelVertex, weights),  vCount, GLTF::VEC4, GL_UNSIGNED_BYTE, true); // 5

        gltf->addBufferView(0, 0, indicesOffset, sizeof(Index) * iCount); // 0
        gltf->addBufferView(0, sizeof(ModelVertex), verticesOffset, sizeof(ModelVertex) * vCount);    // 1

        sprintf(name, "%d_mesh", int(model.type));
        gltf->addMesh(name, 0, 0, 1, 2, 3, -1, 4, 5);

        JSON *nodes;
        gltf->addScene("Scene", &nodes);

        sprintf(name, "%d", int(model.type));
        gltf->addNode(name, 0, 0, vec3(0, 0, 0), quat(0, 0, 0, 1));
        nodes->add(NULL, 0); // mesh
        nodes->add(NULL, 1); // skeleton

        int  joints[64];
        bool links[32][32];
        memset(links, 0, sizeof(links));

        if (anim) {
            JSON *samplers;
            JSON *channels;
            gltf->addAnimation("0_anim", &samplers, &channels);

            { // timeline (input)
                float *timeline = (float*)(bufferData + timelineOffset);
                for (int i = 0; i < animFrames; i++) {
                    timeline[i] = i * animRate / 30.0f;
                }
                gltf->addBufferView(0, 0, timelineOffset, animFrames * sizeof(float)); // 2
                gltf->addAccessor(2, 0, animFrames, GLTF::SCALAR, GL_FLOAT, false, vec4(timeline[0], 0, 0, 0), vec4(timeline[animFrames - 1], 0, 0, 1)); // 6
            }

            { // root translation (output)
                vec3 *translation = (vec3*)(bufferData + translationOffset);
                gltf->addBufferView(0, 0, translationOffset, animFrames * sizeof(vec3)); // 3
                gltf->addAccessor(3, 0, animFrames, GLTF::VEC3, GL_FLOAT); // 7+

                JSON *sampler = samplers->add(JSON::OBJECT); // 0
                sampler->add("input", 6); // timeline
                sampler->add("output", 7); // translations for this node

                JSON *channel = channels->add(JSON::OBJECT);
                channel->add("sampler", 0);
                JSON *target = channel->add(JSON::OBJECT, "target");
                target->add("node", 1); // 0 - mesh, 1+ - skeleton (1 = root)
                target->add("path", "translation");

                for (int j = 0; j < animFrames; j++) {
                    TR::AnimFrame *frame = anim->getFrame(anim->anims + anim->index, j);

                    translation[j] = frame->pos;
                    translation[j] = flip * translation[j];
                }
            }

            { // rotations (output)
                gltf->addBufferView(0, 0, rotationOffset, model.mCount * animFrames * sizeof(quat)); // 4

                quat *rotation = (quat*)(bufferData + rotationOffset);
                quat *ptr = rotation;
                for (int i = 0; i < model.mCount; i++) {
                    for (int j = 0; j < animFrames; j++) {
                        TR::AnimFrame *frame = anim->getFrame(anim->anims + anim->index, j);

                        vec3 angles = frame->getAngle(level->version, i);

                        mat4 matrix;
                        matrix.identity();
                        matrix.rotateYXZ(angles);
                        matrix = flip * matrix * flipInv;

                        *ptr = matrix.getRot();
                        ptr++;
                    }
                    gltf->addAccessor(4, i * animFrames * sizeof(quat), animFrames, GLTF::VEC4, GL_FLOAT); // 8+

                    JSON *sampler = samplers->add(JSON::OBJECT); // 1+
                    sampler->add("input", 6); // timeline
                    sampler->add("output", 8 + i); // rotations for this node

                    JSON *channel = channels->add(JSON::OBJECT);
                    channel->add("sampler", 1 + i); // 0 - translation, 1+ - rotation samplers
                    JSON *target = channel->add(JSON::OBJECT, "target");
                    target->add("node", 1 + i); // 0 - mesh, 1+ - skeleton
                    target->add("path", "rotation");
                }
            }
        }

        gltf->addBuffer(bufferData, bufferSize); // 0

        delete[] bufferData;

        {
            int nIndex = 0;
            int sIndex = 0;
            int stack[16];

            TR::Node *node = (TR::Node*)&level->nodesData[model.node];

            for (int i = 0; i < model.mCount; i++) {
                if (i > 0) {
                    links[nIndex][i] = true;
                }

                nIndex = i;
                if (node[i].flags & 0x01) nIndex = stack[--sIndex];
                if (node[i].flags & 0x02) stack[sIndex++] = nIndex;
            }
        }

        delete anim;

        vec3 jointPos = vec3(0);

        for (int i = 0; i < model.mCount; i++) {
            sprintf(name, "joint_%d", i + 1);

            JSON* node = gltf->addNode(name, -1, -1, jointPos, quat(0, 0, 0, 1));
            JSON* children = new JSON(JSON::ARRAY, "children");

            for (int j = 0; j < model.mCount; j++) {
                if (links[i][j]) {
                    children->add(NULL, j + 1);
                }
            }

            if (children->nodes) {
                node->add(children);
            } else {
                delete children;
            }

            joints[i] = i + 1;
            TR::Node &t = *((TR::Node*)&level->nodesData[model.node] + i);
            jointPos = flip * vec3((float)t.x, (float)t.y, (float)t.z);
        }

        gltf->addSkin("skin", -1, 1, joints, model.mCount);


        char *buffer = new char[gltf->getBufferSize()];
        int size = gltf->save(buffer);
        delete gltf;

        fwrite(buffer, 1, size, file);

        delete[] buffer;

        LOG("export model: %s\n", name);

        fclose(file);
    }

    void exportGeometry(IGame *game, Texture *atlasRooms, Texture *atlasObjects, Texture *atlasSprites) {
        CreateDirectory("models",       NULL);
        CreateDirectory("models/dump/", NULL);

        exportTexture("models/dump/rooms",   atlasRooms);
        exportTexture("models/dump/objects", atlasObjects);
        exportTexture("models/dump/sprites", atlasSprites);

        TR::Level   *level = game->getLevel();
        MeshBuilder *mesh  = game->getMesh();

        for (int i = 0; i < level->modelsCount; i++) {
            exportModel(game, level->models[i]);
        }
    }
#endif

}

#endif
