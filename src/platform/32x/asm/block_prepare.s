#include "common.i"
.data

.global _block_prepare_start
.global _block_prepare_end

.align 4
_block_prepare_start:

#include "transformMesh.i"
#include "transformRoom.i"

.align 2
var_gVerticesBase:
        .long   _gVerticesBase
var_gMatrixPtr:
        .long   _gMatrixPtr
var_gLightAmbient:
        .long   _gLightAmbient
var_divTable:
        .long   _divTable
var_viewportRel:
        .long   _viewportRel

#include "faceAddMeshQuads.i"
#include "faceAddMeshTriangles.i"

.align 2
var_gVertices_fam:
        .long   _gVertices
var_gFacesBase_fam:
        .long   _gFacesBase
var_gVerticesBase_fam:
        .long   _gVerticesBase
const_FACE_CLIPPED_fam:
        .long   FACE_CLIPPED
const_FACE_TRIANGLE_fam:
        .long   FACE_TRIANGLE
var_gOT_fam:
        .long   _gOT

#include "faceAddRoomQuads.i"
#include "faceAddRoomTriangles.i"

.align 2
var_gVertices_far:
        .long   _gVertices
var_gFacesBase_far:
        .long   _gFacesBase
var_gVerticesBase_far:
        .long   _gVerticesBase
const_FACE_CLIPPED_far:
        .long   FACE_CLIPPED
const_FACE_GOURAUD_far:
        .long   FACE_GOURAUD
const_FACE_TRIANGLE_far:
        .long   FACE_TRIANGLE
var_gOT_far:
        .long   _gOT

_block_prepare_end:
