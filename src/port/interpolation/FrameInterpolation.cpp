#include <libultraship/bridge.h>

#include <vector>
#include <map>
#include <unordered_map>
#include <math.h>
#include <string.h>
#include "port/Engine.h"
#include "FrameInterpolation.h"
#include "matrix.h"
#include "port/CoreMath.h"
#include "engine/math_util.h"

extern "C" {
extern Mat4* gInterpolationMatrix;
void guOrtho(Mtx* dest, float left, float right, float bottom, float oTop, float oNear, float oFar, float oScale);
}
/*
Frame interpolation.

The idea of this code is to interpolate all matrices.

The code contains two approaches. The first is to interpolate
all inputs in transformations, such as angles, scale and distances,
and then perform the same transformations with the interpolated values.
After evaluation for some reason some animations such rolling look strange.

The second approach is to simply interpolate the final matrices. This will
more or less simply interpolate the world coordinates for movements.
This will however make rotations ~180 degrees get the "paper effect".
The mitigation is to identify this case for actors and interpolate the
matrix but in model coordinates instead, by "removing" the rotation-
translation before interpolating, create a rotation matrix with the
interpolated angle which is then applied to the matrix.

Currently the code contains both methods but only the second one is currently
used.

Both approaches build a tree of instructions, containing matrices
at leaves. Every node is built from OPEN_DISPS/CLOSE_DISPS and manually
inserted FrameInterpolation_OpenChild/FrameInterpolation_Close child calls.
These nodes contain information that should suffice to identify the matrix,
so we can find it in an adjacent frame.

We can interpolate an arbitrary amount of frames between two original frames,
given a specific interpolation factor (0=old frame, 0.5=average of frames,
1.0=new frame).
*/

static bool invert_matrix(const float m[16], float invOut[16]);

using namespace std;

#define SNOW_INTERP_RING 8
#define SNOW_MAX_PARTICLES 140
#define SNOW_MAX_GROUPS (SNOW_MAX_PARTICLES / 5)
#define SNOW_MAX_GFX (1 + SNOW_MAX_GROUPS * 6 + 2)
#define SNOW_MAX_VTX (SNOW_MAX_GROUPS * 15)

struct SnowRecord {
    Gfx* dl = nullptr;
    float v1[3], v2[3], v3[3];
    s32 mode;
    s16 count;
    float positions[SNOW_MAX_PARTICLES * 3];
};

static SnowRecord sCurSnow;
static SnowRecord sPrevSnow;
static Gfx sSnowGfxBuf[SNOW_INTERP_RING][SNOW_MAX_GFX];
static Vtx sSnowVtxBuf[SNOW_INTERP_RING][SNOW_MAX_VTX];
static int sSnowRingIdx = 0;

namespace {

enum class Op {
    Marker,
    OpenChild,
    CloseChild,

    MatrixPush,
    MatrixPop,
    MatrixPut,
    MatrixMult,
    MatrixTranslate,
    Ortho,
    MatrixScale,
    MatrixRotate1Coord,
    MatrixRotateXYCoords,
    MatrixMult4x4,
    MatrixPosRotXYZ,
    MatrixPosRotZXY,
    MatrixMultVec3fNoTranslate,
    MatrixMultVec3f,
    MatrixMtxFToMtx,
    BillboardMatrix,
    AnimatedPartMatrix,
    MatrixToMtx,
    MatrixRotateAxis,
    SkinMatrixMtxFToMtx,
    SetTransformMatrix,
    SetMatrixTransformation,
    SetTranslateRotate,
    SetTextMatrix,
    SetMatrixPosRotScaleXY,
    SetApplyMatrixTransformations,
};

typedef pair<const void*, uintptr_t> label;

union Data {
    Data() {
    }

    struct {
        Mat4** matrix;
    } matrix_ptr;

    struct {
        const char* file;
        int line;
    } marker;

    struct {
        Mat4* matrix;
        MtxF mf;
        u8 mode;
    } matrix_mult;

    struct {
        Mat4* matrix;
        FVector pos;
        IRotator rot;
        FVector scale;
    } matrix_applytransformations;

    struct {
        Mat4* matrix;
        Vec3fInterp b;
    } matrix_translate;

    struct {
        Mtx* m;
        float oLeft;
        float oRight;
        float oBottom;
        float oTop;
        float oNear;
        float oFar;
        float oScale;
    } ortho;

    struct {
        Mat4* matrix;
        f32 scale;
    } matrix_scale;

    struct {
        Mat4* matrix;
        u32 coord;
        s16 value;
    } matrix_rotate_1_coord;

    struct {
        Mat4* matrix;
        s16 x;
        s16 y;
    } matrix_rotate_xy_coords;

    struct {
        Mat4* matrix;
        Vec3f src;
        Vec3f dest;
    } matrix_vec_translate;

    struct {
        Mat4* matrix;
        Vec3f src;
        Vec3f dest;
    } matrix_vec_no_translate;

    struct {
        Mat4* dest;
        Mat4 mtx1;
        Mat4 mtx2;
    } matrix_mult_4x4;

    struct {
        Mat4* matrix;
        Vec3fInterp pos;
        Vec3sInterp orientation;
    } matrix_pos_rot_xyz;

    struct {
        Mat4* matrix;
        s32 x;
        s32 y;
        u16 angle;
        f32 scale;
    } matrix_pos_rot_scale_xy;

    struct {
        Mat4* matrix;
        Vec3f translation;
        Vec3s rotation;
    } matrix_translate_rotate_zyx;

    struct {
        Mat4* matrix;
        f32 translateX, translateY, translateZ;
        Vec3s rot;
        // MtxF mtx;
        bool has_mtx;
    } matrix_set_translate_rotate_yxz;

    struct {
        MtxF src;
        Mtx* dest;
    } matrix_mtxf_to_mtx;

    struct {
        MtxF parent_mf;
        float translation[3];
        float scale[3];
        s16 roll;
        Mtx* dest;
    } billboard_matrix;

    struct {
        MtxF parent_mf;
        float translation[3];
        s16 rotation[3];
        Mtx* dest;
    } animated_part_matrix;

    struct {
        Mtx* dest;
        MtxF src;
        bool has_adjusted;
    } matrix_to_mtx;

    struct {
        MtxF mf;
    } matrix_replace_rotation;

    struct {
        f32 angle;
        Vec3f axis;
        u8 mode;
    } matrix_rotate_axis;

    struct {
        Mat4* dest;
        Vec3f orientationVector;
        Vec3f positionVector;
        u16 rotationAngle;
        f32 scaleFactor;
    } set_transform_matrix_data;

    struct {
        Mat4* dest;
        Vec3f location;
        Vec3su rotation;
        f32 scale;
    } set_matrix_transformation_data;

    struct {
        Mat4* dest;
        Vec3f location;
        Vec3s rotation;
    } set_translate_rotate_data;

    struct {
        Mat3* dest;
        f32 arg1;
        f32 arg2;
        f32 arg3;
        s16 rot;
    } set_orientation_matrix_data;

    struct {
        Mat4* matrix;
        f32 x;
        f32 y;
        f32 arg3;
        f32 arg4;
    } matrix_text;

    struct {
        label key;
        size_t idx;
    } open_child;
};

struct Path {
    map<label, vector<Path>> children;
    map<Op, vector<Data>> ops;
    vector<pair<Op, size_t>> items;
};

struct Recording {
    Path root_path;
};

bool is_recording;
vector<Path*> current_path;
uint32_t camera_epoch;
uint32_t previous_camera_epoch;
Recording current_recording;
Recording previous_recording;

bool next_is_actor_pos_rot_matrix;
bool has_inv_actor_mtx;
MtxF inv_actor_mtx;
size_t inv_actor_mtx_path_index;

Data& append(Op op) {
    auto& m = current_path.back()->ops[op];
    current_path.back()->items.emplace_back(op, m.size());
    return m.emplace_back();
}

MtxF* Matrix_GetCurrent() {
    return (MtxF*)gInterpolationMatrix;
}

struct InterpolateCtx {
    float step;
    float w;
    unordered_map<Mtx*, MtxF> mtx_replacements;
    unordered_map<Gfx*, Gfx*> dl_replacements;
    MtxF tmp_mtxf, tmp_mtxf2;
    Mat3 tmp_mat3;
    Vec3f tmp_vec3f, tmp_vec3f2;
    Vec3s tmp_vec3s;
    int32_t tmp32[2];
    MtxF actor_mtx;

    MtxF* new_replacement(Mtx* addr) {
        return &mtx_replacements[addr];
    }

    void interpolate_mtxf(MtxF* res, MtxF* o, MtxF* n) {
        const float threshold = 1500.0f;

        float dx = fabsf(o->mf[3][0] - n->mf[3][0]);
        float dy = fabsf(o->mf[3][1] - n->mf[3][1]);
        float dz = fabsf(o->mf[3][2] - n->mf[3][2]);

        if (dx > threshold || dy > threshold || dz > threshold) {
            *res = *n;
            return;
        }

        float col_dot = o->mf[0][0] * n->mf[0][0] + o->mf[1][0] * n->mf[1][0] + o->mf[2][0] * n->mf[2][0];

        if (col_dot < 0.0f) {
            *res = *n;
            res->mf[3][0] = w * o->mf[3][0] + step * n->mf[3][0];
            res->mf[3][1] = w * o->mf[3][1] + step * n->mf[3][1];
            res->mf[3][2] = w * o->mf[3][2] + step * n->mf[3][2];
            return;
        }

        for (size_t i = 0; i < 4; i++) {
            for (size_t j = 0; j < 4; j++) {
                res->mf[i][j] = w * o->mf[i][j] + step * n->mf[i][j];
            }
        }
    }

    void translate_mtxf(Mat4 dest, Vec3f b) {
        mtxf_identity(dest);
        dest[3][0] = b[0];
        dest[3][1] = b[1];
        dest[3][2] = b[2];
    }

    float lerp(f32 o, f32 n) {
        return w * o + step * n;
    }

    s16 lerp_s16(s16 o, s16 n) {
        return w * o + step * n;
    }

    s32 lerp_s32(s32 o, s32 n) {
        return w * o + step * n;
    }

    void lerp_vec3s(Vec3s* res, Vec3s o, Vec3s n) {
        (*res)[0] = lerp_s16(o[0], n[0]);
        (*res)[1] = lerp_s16(o[1], n[1]);
        (*res)[2] = lerp_s16(o[2], n[2]);
    }

    void lerp_vec3f(Vec3f* res, Vec3f* o, Vec3f* n) {
        (*res)[0] = lerp((*o)[0], (*n)[0]);
        (*res)[1] = lerp((*o)[1], (*n)[1]);
        (*res)[2] = lerp((*o)[2], (*n)[2]);
    }

    float interpolate_angle(f32 o, f32 n) {
        if (o == n)
            return n;
        o = fmodf(o, 2 * M_PI);
        if (o < 0.0f) {
            o += 2 * M_PI;
        }
        n = fmodf(n, 2 * M_PI);
        if (n < 0.0f) {
            n += 2 * M_PI;
        }
        if (fabsf(o - n) > M_PI) {
            if (o < n) {
                o += 2 * M_PI;
            } else {
                n += 2 * M_PI;
            }
        }
        if (fabsf(o - n) > M_PI / 2) {
            // return n;
        }
        return lerp(o, n);
    }

    s16 interpolate_angle(s16 os, s16 ns) {
        if (os == ns)
            return ns;
        int o = (u16)os;
        int n = (u16)ns;
        u16 res;
        int diff = o - n;
        if (-0x8000 <= diff && diff <= 0x8000) {
            if (diff < -0x4000 || diff > 0x4000) {
                return ns;
            }
            res = (u16)(w * o + step * n);
        } else {
            if (o < n) {
                o += 0x10000;
            } else {
                n += 0x10000;
            }
            diff = o - n;
            if (diff < -0x4000 || diff > 0x4000) {
                return ns;
            }
            res = (u16)(w * o + step * n);
        }
        return res;
    }

    void interpolate_vecs(Vec3f* res, Vec3f* o, Vec3f* n) {
        (*res)[0] = interpolate_angle((*o)[0], (*n)[0]);
        (*res)[1] = interpolate_angle((*o)[1], (*n)[1]);
        (*res)[2] = interpolate_angle((*o)[2], (*n)[2]);
    }

    void interpolate_angles(Vec3s* res, Vec3s* o, Vec3s* n) {
        (*res)[0] = interpolate_angle((*o)[0], (*n)[0]);
        (*res)[1] = interpolate_angle((*o)[1], (*n)[1]);
        (*res)[2] = interpolate_angle((*o)[2], (*n)[2]);
    }

    void interpolate_branch(Path* old_path, Path* new_path) {
        for (auto& item : new_path->items) {
            Data& new_op = new_path->ops[item.first][item.second];

            if (item.first == Op::OpenChild) {
                if (auto it = old_path->children.find(new_op.open_child.key);
                    it != old_path->children.end() && new_op.open_child.idx < it->second.size()) {
                    interpolate_branch(&it->second[new_op.open_child.idx],
                                       &new_path->children.find(new_op.open_child.key)->second[new_op.open_child.idx]);
                } else {
                    interpolate_branch(&new_path->children.find(new_op.open_child.key)->second[new_op.open_child.idx],
                                       &new_path->children.find(new_op.open_child.key)->second[new_op.open_child.idx]);
                }
                continue;
            }

            if (auto it = old_path->ops.find(item.first); it != old_path->ops.end()) {
                if (item.second < it->second.size()) {
                    Data& old_op = it->second[item.second];
                    switch (item.first) {
                        case Op::OpenChild:
                        case Op::CloseChild:
                        case Op::Marker:
                            break;

                        case Op::MatrixPush:
                            Matrix_Push((Matrix**)&gInterpolationMatrix);
                            break;

                        case Op::MatrixPop:
                            Matrix_Pop((Matrix**)&gInterpolationMatrix);
                            break;

                            // Unused on SF64
                            // case Op::MatrixPut:
                            //     interpolate_mtxf(&tmp_mtxf, &old_op.matrix_put.src, &new_op.matrix_put.src);
                            //     Matrix_Put(&tmp_mtxf);
                            //     break;

                        case Op::MatrixMult:
                            interpolate_mtxf(&tmp_mtxf, &old_op.matrix_mult.mf, &new_op.matrix_mult.mf);
                            mtxf_mul(*gInterpolationMatrix, tmp_mtxf.mf, new_op.matrix_mult.mf.mf);
                            // Matrix_Mult(gInterpolationMatrix, (Matrix*) &tmp_mtxf, new_op.matrix_mult.mode);
                            break;

                        case Op::MatrixTranslate:
                            Vec3f temp;

                            temp[0] = lerp(old_op.matrix_translate.b.x, new_op.matrix_translate.b.x);
                            temp[1] = lerp(old_op.matrix_translate.b.y, new_op.matrix_translate.b.y);
                            temp[2] = lerp(old_op.matrix_translate.b.z, new_op.matrix_translate.b.z);

                            translate_mtxf(*gInterpolationMatrix, temp);
                            break;
                        case Op::Ortho: {
                            // Only interpolate if the difference is significant to avoid jitter
                            constexpr float THRESHOLD = 2.0f;
                            constexpr float LARGE_JUMP = 500.0f;
                            float leftDiff = fabsf(old_op.ortho.oLeft - new_op.ortho.oLeft);
                            if (leftDiff > LARGE_JUMP) {
                                break;
                            }
                            if (leftDiff < THRESHOLD && fabsf(old_op.ortho.oRight - new_op.ortho.oRight) < THRESHOLD &&
                                fabsf(old_op.ortho.oBottom - new_op.ortho.oBottom) < THRESHOLD &&
                                fabsf(old_op.ortho.oTop - new_op.ortho.oTop) < THRESHOLD &&
                                fabsf(old_op.ortho.oNear - new_op.ortho.oNear) < THRESHOLD &&
                                fabsf(old_op.ortho.oFar - new_op.ortho.oFar) < THRESHOLD &&
                                fabsf(old_op.ortho.oScale - new_op.ortho.oScale) < THRESHOLD) {
                                break;
                            }
                            float left = lerp(old_op.ortho.oLeft, new_op.ortho.oLeft);
                            float right = lerp(old_op.ortho.oRight, new_op.ortho.oRight);
                            float bottom = lerp(old_op.ortho.oBottom, new_op.ortho.oBottom);
                            float oTop = lerp(old_op.ortho.oTop, new_op.ortho.oTop);
                            float oNear = lerp(old_op.ortho.oNear, new_op.ortho.oNear);
                            float oFar = lerp(old_op.ortho.oFar, new_op.ortho.oFar);
                            float scale = lerp(old_op.ortho.oScale, new_op.ortho.oScale);
                            guOrtho(new_replacement(new_op.ortho.m), left, right, bottom, oTop, oNear, oFar, scale);
                            break;
                        }
                        case Op::MatrixPosRotXYZ: {
                            Vec3f tempF;
                            Vec3s tempS;

                            tempF[0] = lerp(old_op.matrix_pos_rot_xyz.pos.x, new_op.matrix_pos_rot_xyz.pos.x);
                            tempF[1] = lerp(old_op.matrix_pos_rot_xyz.pos.y, new_op.matrix_pos_rot_xyz.pos.y);
                            tempF[2] = lerp(old_op.matrix_pos_rot_xyz.pos.z, new_op.matrix_pos_rot_xyz.pos.z);

                            tempS[0] =
                                lerp(old_op.matrix_pos_rot_xyz.orientation.x, new_op.matrix_pos_rot_xyz.orientation.x);
                            tempS[1] =
                                lerp(old_op.matrix_pos_rot_xyz.orientation.y, new_op.matrix_pos_rot_xyz.orientation.y);
                            tempS[2] =
                                lerp(old_op.matrix_pos_rot_xyz.orientation.z, new_op.matrix_pos_rot_xyz.orientation.z);

                            mtxf_rotate_xyz_and_translate(*gInterpolationMatrix, tempF, tempS);
                            break;
                        }
                        case Op::MatrixPosRotZXY: {
                            Vec3f tempF;
                            Vec3s tempS;

                            tempF[0] = lerp(old_op.matrix_pos_rot_xyz.pos.x, new_op.matrix_pos_rot_xyz.pos.x);
                            tempF[1] = lerp(old_op.matrix_pos_rot_xyz.pos.y, new_op.matrix_pos_rot_xyz.pos.y);
                            tempF[2] = lerp(old_op.matrix_pos_rot_xyz.pos.z, new_op.matrix_pos_rot_xyz.pos.z);

                            tempS[0] =
                                lerp(old_op.matrix_pos_rot_xyz.orientation.x, new_op.matrix_pos_rot_xyz.orientation.x);
                            tempS[1] =
                                lerp(old_op.matrix_pos_rot_xyz.orientation.y, new_op.matrix_pos_rot_xyz.orientation.y);
                            tempS[2] =
                                lerp(old_op.matrix_pos_rot_xyz.orientation.z, new_op.matrix_pos_rot_xyz.orientation.z);

                            mtxf_rotate_zxy_and_translate(*gInterpolationMatrix, tempF, tempS);
                            break;
                        }

                        case Op::MatrixScale:
                            // mtxf_scale(*gInterpolationMatrix, lerp(old_op.matrix_scale.scale,
                            // new_op.matrix_scale.scale));
                            break;

                        case Op::MatrixRotate1Coord: {
                            //                            s16 v = interpolate_angle(old_op.matrix_rotate_1_coord.value,
                            //                                                      new_op.matrix_rotate_1_coord.value);
                            //                            switch (new_op.matrix_rotate_1_coord.coord) {
                            //                                case 0:
                            //                                    mtxf_rotate_x(*gInterpolationMatrix, v);
                            //                                    break;
                            //
                            //                                case 1:
                            //                                    mtxf_rotate_y(*gInterpolationMatrix, v);
                            //                                    break;
                            //
                            //                                case 2:
                            //                                    mtxf_s16_rotate_z(*gInterpolationMatrix, v);
                            //                                    break;
                            //                            }
                            break;
                        }
                        case Op::MatrixRotateXYCoords: {
                            s16 x =
                                interpolate_angle(old_op.matrix_rotate_xy_coords.x, new_op.matrix_rotate_xy_coords.x);
                            s16 y =
                                interpolate_angle(old_op.matrix_rotate_xy_coords.y, new_op.matrix_rotate_xy_coords.y);
                            // mtxf_rotate_xy(*gInterpolationMatrix, y);
                            break;
                        }
                        case Op::MatrixMultVec3fNoTranslate: {
                            interpolate_vecs(&tmp_vec3f, &old_op.matrix_vec_no_translate.src,
                                             &new_op.matrix_vec_no_translate.src);
                            interpolate_vecs(&tmp_vec3f2, &old_op.matrix_vec_no_translate.dest,
                                             &new_op.matrix_vec_no_translate.dest);
                            // Matrix_MultVec3fNoTranslate(gInterpolationMatrix, &tmp_vec3f, &tmp_vec3f2);
                            break;
                        }
                        case Op::MatrixMultVec3f: {
                            interpolate_vecs(&tmp_vec3f, &old_op.matrix_vec_translate.src,
                                             &new_op.matrix_vec_translate.src);
                            interpolate_vecs(&tmp_vec3f2, &old_op.matrix_vec_translate.dest,
                                             &new_op.matrix_vec_translate.dest);
                            // Matrix_MultVec3f(gInterpolationMatrix, &tmp_vec3f, &tmp_vec3f2);
                            break;
                        }

                        case Op::MatrixMtxFToMtx:
                            interpolate_mtxf(new_replacement(new_op.matrix_mtxf_to_mtx.dest),
                                             &old_op.matrix_mtxf_to_mtx.src, &new_op.matrix_mtxf_to_mtx.src);
                            break;

                        case Op::BillboardMatrix: {
                            // Re-derive the billboard matrix from the interpolated parent rather than
                            // element-wise lerping the final matrix. This correctly handles camera
                            // roll changes and avoids any drift caused by interpolating a camera-
                            // dependent view-space matrix directly.
                            MtxF interp_parent;
                            interpolate_mtxf(&interp_parent, &old_op.billboard_matrix.parent_mf,
                                             &new_op.billboard_matrix.parent_mf);
                            s16 interp_roll = lerp_s16(old_op.billboard_matrix.roll, new_op.billboard_matrix.roll);
                            float sx = w * old_op.billboard_matrix.scale[0] + step * new_op.billboard_matrix.scale[0];
                            float sy = w * old_op.billboard_matrix.scale[1] + step * new_op.billboard_matrix.scale[1];
                            float sz = w * old_op.billboard_matrix.scale[2] + step * new_op.billboard_matrix.scale[2];
                            const float* t = new_op.billboard_matrix.translation;
                            const float (*p)[4] = interp_parent.mf;
                            MtxF* result = new_replacement(new_op.billboard_matrix.dest);
                            float (*r)[4] = result->mf;
                            // Billboard rotation uses the camera roll angle; columns are screen-aligned
                            // basis vectors scaled by the object's visual scale.
                            float cs = cosf((float)interp_roll * (float)M_PI / 32768.0f);
                            float sn = sinf((float)interp_roll * (float)M_PI / 32768.0f);
                            r[0][0] = cs * sx;
                            r[0][1] = sn * sx;
                            r[0][2] = 0.0f;
                            r[0][3] = 0.0f;
                            r[1][0] = -sn * sy;
                            r[1][1] = cs * sy;
                            r[1][2] = 0.0f;
                            r[1][3] = 0.0f;
                            r[2][0] = 0.0f;
                            r[2][1] = 0.0f;
                            r[2][2] = sz;
                            r[2][3] = 0.0f;
                            // Translation = parent_matrix × billboard_local_offset + parent_translation
                            r[3][0] = p[0][0] * t[0] + p[1][0] * t[1] + p[2][0] * t[2] + p[3][0];
                            r[3][1] = p[0][1] * t[0] + p[1][1] * t[1] + p[2][1] * t[2] + p[3][1];
                            r[3][2] = p[0][2] * t[0] + p[1][2] * t[1] + p[2][2] * t[2] + p[3][2];
                            r[3][3] = 1.0f;
                            break;
                        }

                        case Op::AnimatedPartMatrix: {
                            MtxF interp_parent;
                            interpolate_mtxf(&interp_parent, &old_op.animated_part_matrix.parent_mf,
                                             &new_op.animated_part_matrix.parent_mf);

                            // Lerp local translation
                            float tx = lerp(old_op.animated_part_matrix.translation[0],
                                            new_op.animated_part_matrix.translation[0]);
                            float ty = lerp(old_op.animated_part_matrix.translation[1],
                                            new_op.animated_part_matrix.translation[1]);
                            float tz = lerp(old_op.animated_part_matrix.translation[2],
                                            new_op.animated_part_matrix.translation[2]);

                            // Lerp local rotation (s16 angles, short-path)
                            s16 rx = interpolate_angle(old_op.animated_part_matrix.rotation[0],
                                                       new_op.animated_part_matrix.rotation[0]);
                            s16 ry = interpolate_angle(old_op.animated_part_matrix.rotation[1],
                                                       new_op.animated_part_matrix.rotation[1]);
                            s16 rz = interpolate_angle(old_op.animated_part_matrix.rotation[2],
                                                       new_op.animated_part_matrix.rotation[2]);

                            // Rebuild local bone matrix: rotate XYZ then translate
                            float cx = cosf((float)(u16)rx * (float)M_PI / 32768.0f);
                            float sx_ = sinf((float)(u16)rx * (float)M_PI / 32768.0f);
                            float cy = cosf((float)(u16)ry * (float)M_PI / 32768.0f);
                            float sy_ = sinf((float)(u16)ry * (float)M_PI / 32768.0f);
                            float cz = cosf((float)(u16)rz * (float)M_PI / 32768.0f);
                            float sz_ = sinf((float)(u16)rz * (float)M_PI / 32768.0f);

                            // mtxf_rotate_xyz_and_translate: R = Rx * Ry * Rz (XYZ order)
                            // Same convention used by the game's math_util.c
                            float m00 = cy * cz;
                            float m01 = cy * sz_;
                            float m02 = -sy_;
                            float m10 = sx_ * sy_ * cz - cx * sz_;
                            float m11 = sx_ * sy_ * sz_ + cx * cz;
                            float m12 = sx_ * cy;
                            float m20 = cx * sy_ * cz + sx_ * sz_;
                            float m21 = cx * sy_ * sz_ - sx_ * cz;
                            float m22 = cx * cy;

                            // Combined = parent × local  (local columns into parent space)
                            const float (*p)[4] = interp_parent.mf;
                            MtxF* result = new_replacement(new_op.animated_part_matrix.dest);
                            float (*r2)[4] = result->mf;

                            r2[0][0] = p[0][0] * m00 + p[1][0] * m01 + p[2][0] * m02;
                            r2[0][1] = p[0][1] * m00 + p[1][1] * m01 + p[2][1] * m02;
                            r2[0][2] = p[0][2] * m00 + p[1][2] * m01 + p[2][2] * m02;
                            r2[0][3] = 0.0f;
                            r2[1][0] = p[0][0] * m10 + p[1][0] * m11 + p[2][0] * m12;
                            r2[1][1] = p[0][1] * m10 + p[1][1] * m11 + p[2][1] * m12;
                            r2[1][2] = p[0][2] * m10 + p[1][2] * m11 + p[2][2] * m12;
                            r2[1][3] = 0.0f;
                            r2[2][0] = p[0][0] * m20 + p[1][0] * m21 + p[2][0] * m22;
                            r2[2][1] = p[0][1] * m20 + p[1][1] * m21 + p[2][1] * m22;
                            r2[2][2] = p[0][2] * m20 + p[1][2] * m21 + p[2][2] * m22;
                            r2[2][3] = 0.0f;
                            r2[3][0] = p[0][0] * tx + p[1][0] * ty + p[2][0] * tz + p[3][0];
                            r2[3][1] = p[0][1] * tx + p[1][1] * ty + p[2][1] * tz + p[3][1];
                            r2[3][2] = p[0][2] * tx + p[1][2] * ty + p[2][2] * tz + p[3][2];
                            r2[3][3] = 1.0f;
                            break;
                        }

                        case Op::MatrixToMtx: {
                            //*new_replacement(new_op.matrix_to_mtx.dest) = *Matrix_GetCurrent();
                            if (old_op.matrix_to_mtx.has_adjusted && new_op.matrix_to_mtx.has_adjusted) {
                                interpolate_mtxf(&tmp_mtxf, &old_op.matrix_to_mtx.src, &new_op.matrix_to_mtx.src);
                                // Matrix_MtxFMtxFMult(&actor_mtx, &tmp_mtxf,
                                //                         new_replacement(new_op.matrix_to_mtx.dest));
                            } else {
                                interpolate_mtxf(new_replacement(new_op.matrix_to_mtx.dest), &old_op.matrix_to_mtx.src,
                                                 &new_op.matrix_to_mtx.src);
                            }
                            break;
                        }

                        case Op::MatrixRotateAxis: {
                            lerp_vec3f(&tmp_vec3f, &old_op.matrix_rotate_axis.axis, &new_op.matrix_rotate_axis.axis);
                            auto tmp =
                                interpolate_angle(old_op.matrix_rotate_axis.angle, new_op.matrix_rotate_axis.angle);
                            // Matrix_RotateAxis((Matrix*) &tmp_vec3f, tmp, 1.0f, 1.0f, 1.0f,
                            // new_op.matrix_rotate_axis.mode);
                            break;
                        }

                        case Op::SetTransformMatrix: {
                            lerp_vec3f(&tmp_vec3f, &old_op.set_transform_matrix_data.orientationVector,
                                       &new_op.set_transform_matrix_data.orientationVector);

                            lerp_vec3f(&tmp_vec3f2, &old_op.set_transform_matrix_data.positionVector,
                                       &new_op.set_transform_matrix_data.positionVector);

                            u16 rotationAngleTemp = lerp_s16(old_op.set_transform_matrix_data.rotationAngle,
                                                             new_op.set_transform_matrix_data.rotationAngle);
                            f32 scaleFactorTemp = lerp(old_op.set_transform_matrix_data.scaleFactor,
                                                       new_op.set_transform_matrix_data.scaleFactor);

                            // set_transform_matrix(*gInterpolationMatrix, tmp_vec3f, tmp_vec3f2, rotationAngleTemp,
                            // scaleFactorTemp);

                            break;
                        }

                        case Op::SetMatrixTransformation: {

                            lerp_vec3f(&tmp_vec3f, &old_op.set_matrix_transformation_data.location,
                                       &new_op.set_matrix_transformation_data.location);

                            lerp_vec3s(&tmp_vec3s, *(Vec3s*)&old_op.set_matrix_transformation_data.rotation,
                                       *(Vec3s*)&new_op.set_matrix_transformation_data.rotation);

                            f32 scaleFactorTemp = lerp(old_op.set_matrix_transformation_data.scale,
                                                       new_op.set_matrix_transformation_data.scale);

                            //                            mtxf_set_matrix_transformation(*gInterpolationMatrix,
                            //                            tmp_vec3f, *(Vec3su*)&tmp_vec3s, scaleFactorTemp);
                            break;
                        }

                        case Op::SetTranslateRotate: {
                            lerp_vec3f(&tmp_vec3f, &old_op.set_translate_rotate_data.location,
                                       &new_op.set_translate_rotate_data.location);

                            lerp_vec3s(&tmp_vec3s, old_op.set_translate_rotate_data.rotation,
                                       new_op.set_translate_rotate_data.rotation);

                            // mtxf_translate_rotate(*gInterpolationMatrix, tmp_vec3f, tmp_vec3s);
                            break;
                        }
                        case Op::SetMatrixPosRotScaleXY: {
                            tmp32[0] = lerp_s32(old_op.matrix_pos_rot_scale_xy.x, new_op.matrix_pos_rot_scale_xy.x);
                            tmp32[1] = lerp_s32(old_op.matrix_pos_rot_scale_xy.y, new_op.matrix_pos_rot_scale_xy.y);

                            tmp_vec3s[0] =
                                lerp_s16(old_op.matrix_pos_rot_scale_xy.angle, new_op.matrix_pos_rot_scale_xy.angle);

                            tmp_vec3f[0] =
                                lerp(old_op.matrix_pos_rot_scale_xy.scale, new_op.matrix_pos_rot_scale_xy.scale);

                            // mtxf_translation_x_y_rotate_z_scale_x_y(*gInterpolationMatrix, tmp32[0], tmp32[1],
                            // tmp_vec3s[0], tmp_vec3f[0]);
                            break;
                        }
                    }
                }
            }
        }
    }
};

} // anonymous namespace

FrameInterpolationResult FrameInterpolation_Interpolate(float step) {
    InterpolateCtx ctx;
    ctx.step = step;
    ctx.w = 1.0f - step;
    ctx.interpolate_branch(&previous_recording.root_path, &current_recording.root_path);

    // Snow particle interpolation: build a patched DL with lerped vertex positions.
    if (sPrevSnow.dl && sCurSnow.dl && sCurSnow.count > 0) {
        int ring = sSnowRingIdx;
        sSnowRingIdx = (sSnowRingIdx + 1) % SNOW_INTERP_RING;

        int groups = sCurSnow.count / 5;
        int dl_size = 1 + groups * 6 + 2;
        int min_count = (sPrevSnow.count < sCurSnow.count) ? sPrevSnow.count : sCurSnow.count;

        Gfx* out_dl = sSnowGfxBuf[ring];
        Vtx* out_vtx = sSnowVtxBuf[ring];

        memcpy(out_dl, sCurSnow.dl, (size_t)dl_size * sizeof(Gfx));

        // Texture-coordinate layout for the 3 snowflake triangle vertices (from gSnowTempVtx).
        static const short kTC[3][2] = { { 0, 0 }, { 0, 960 }, { 960, 0 } };

        int vtx_off = 0;
        for (int g = 0; g < groups; g++) {
            for (int k = 0; k < 15; k++) {
                int pidx = g * 5 + k / 3;
                int vt = k % 3;

                float px, py, pz;
                if (pidx < min_count) {
                    float ox = sPrevSnow.positions[pidx * 3 + 0];
                    float oy = sPrevSnow.positions[pidx * 3 + 1];
                    float oz = sPrevSnow.positions[pidx * 3 + 2];
                    float nx = sCurSnow.positions[pidx * 3 + 0];
                    float ny = sCurSnow.positions[pidx * 3 + 1];
                    float nz = sCurSnow.positions[pidx * 3 + 2];
                    px = ox * ctx.w + nx * step;
                    py = oy * ctx.w + ny * step;
                    pz = oz * ctx.w + nz * step;
                } else {
                    px = sCurSnow.positions[pidx * 3 + 0];
                    py = sCurSnow.positions[pidx * 3 + 1];
                    pz = sCurSnow.positions[pidx * 3 + 2];
                }

                const float* voff = (vt == 0) ? sCurSnow.v1 : (vt == 1) ? sCurSnow.v2 : sCurSnow.v3;

                Vtx& ov = out_vtx[vtx_off + k];
                ov.v.ob[0] = px + voff[0];
                ov.v.ob[1] = py + voff[1];
                ov.v.ob[2] = pz + voff[2];
                ov.v.flag = 0;
                ov.v.tc[0] = kTC[vt][0];
                ov.v.tc[1] = kTC[vt][1];
                ov.v.cn[0] = 0x7F;
                ov.v.cn[1] = 0x7F;
                ov.v.cn[2] = 0x7F;
                ov.v.cn[3] = 0xFF;
            }
            // Patch the gSPVertex command's vertex pointer (words.w1 is uintptr_t on PC).
            out_dl[1 + g * 6].words.w1 = (uintptr_t)(&out_vtx[vtx_off]);
            vtx_off += 15;
        }

        ctx.dl_replacements[sCurSnow.dl] = out_dl;
    }

    return { ctx.mtx_replacements, ctx.dl_replacements };
}

bool camera_interpolation = true;

void FrameInterpolation_ShouldInterpolateFrame(bool shouldInterpolate) {
    camera_interpolation = shouldInterpolate;
    is_recording = shouldInterpolate;
}

bool check_if_recording() {
    return (is_recording && GameEngine::GetInterpolationFPS() != 30);
}

void FrameInterpolation_StartRecord(void) {
    previous_recording = move(current_recording);
    current_recording = {};
    sPrevSnow = sCurSnow;
    sCurSnow.dl = nullptr;
    sCurSnow.count = 0;
    current_path.clear();
    current_path.push_back(&current_recording.root_path);
    if (!camera_interpolation) {
        // default to interpolating
        camera_interpolation = true;
        is_recording = false;
        return;
    }
    if (GameEngine::GetInterpolationFPS() != 30) {
        is_recording = true;
    }
}

void FrameInterpolation_StopRecord(void) {
    previous_camera_epoch = camera_epoch;
    is_recording = false;
}

void FrameInterpolation_RecordOpenChild(const void* a, uintptr_t b) {
    if (!check_if_recording()) {
        return;
    }
    label key = { a, b };
    auto& m = current_path.back()->children[key];
    append(Op::OpenChild).open_child = { key, m.size() };
    current_path.push_back(&m.emplace_back());
}

void FrameInterpolation_RecordCloseChild(void) {
    if (!check_if_recording()) {
        return;
    }
    // append(Op::CloseChild);
    if (has_inv_actor_mtx && current_path.size() == inv_actor_mtx_path_index) {
        has_inv_actor_mtx = false;
    }
    current_path.pop_back();
}

void FrameInterpolation_DontInterpolateCamera(void) {
    camera_epoch = previous_camera_epoch + 1;
}

int FrameInterpolation_GetCameraEpoch(void) {
    return (int)camera_epoch;
}

void FrameInterpolation_Record_SetTextMatrix(Mat4* matrix, f32 x, f32 y, f32 arg3, f32 arg4) {
    if (!check_if_recording()) {
        return;
    }
    append(Op::SetTextMatrix).matrix_text = { matrix, x, y, arg3, arg4 };
}

void FrameInterpolation_RecordActorPosRotMatrix(void) {
    if (!check_if_recording()) {
        return;
    }
    next_is_actor_pos_rot_matrix = true;
}

// void FrameInterpolation_RecordMatrixPush(Mat4* matrix) {
//     if (!check_if_recording()) {
//         return;
//     }
//
//     append(Op::MatrixPush).matrix_ptr = { (Mat4**) matrix };
// }

void FrameInterpolation_RecordMarker(const char* file, int line) {
    if (!check_if_recording()) {
        return;
    }

    append(Op::Marker).marker = { file, line };
}

// void FrameInterpolation_RecordMatrixPop(Mat4* matrix) {
//     if (!check_if_recording()) {
//         return;
//     }
//     append(Op::MatrixPop).matrix_ptr = { (Mat4**) matrix };
// }

void FrameInterpolation_RecordMatrixPut(MtxF* src) {
    if (!check_if_recording()) {
        return;
    }
    //    append(Op::MatrixPut).matrix_put = { matrix, *src };
}

void FrameInterpolation_RecordMatrixMult(Mat4* matrix, MtxF* mf, u8 mode) {
    if (!check_if_recording()) {
        return;
    }
    append(Op::MatrixMult).matrix_mult = { matrix, *mf, mode };
}

void FrameInterpolation_ApplyMatrixTransformations(Mat4* matrix, FVector pos, IRotator rot, FVector scale) {
    if (!check_if_recording()) {
        return;
    }
    append(Op::SetApplyMatrixTransformations).matrix_applytransformations = { matrix, pos, rot, scale };
}

void FrameInterpolation_RecordMatrixTranslate(Mat4* matrix, Vec3f b) {
    if (!check_if_recording()) {
        return;
    }
    // Note: Vec3f decays to pointer when passed as parameter. Cast directly, not &b.
    append(Op::MatrixTranslate).matrix_translate = { matrix, *((Vec3fInterp*)b) };
}

void FrameInterpolation_RecordOrtho(Mtx* m, f32 left, f32 right, f32 bottom, f32 oTop, f32 oNear, f32 oFar, f32 scale) {
    if (!check_if_recording()) {
        return;
    }
    append(Op::Ortho).ortho = { m, left, right, bottom, oTop, oNear, oFar, scale };
}

void FrameInterpolation_RecordMatrixScale(Mat4* matrix, f32 scale) {
    if (!check_if_recording()) {
        return;
    }
    append(Op::MatrixScale).matrix_scale = { matrix, scale };
}

void FrameInterpolation_RecordMatrixMultVec3fNoTranslate(Mat4* matrix, Vec3f src, Vec3f dest) {
    if (!check_if_recording()) {
        return;
    }
    // append(Op::MatrixMultVec3fNoTranslate).matrix_vec_no_translate = { matrix, src, dest };
}

void FrameInterpolation_RecordSetTransformMatrix(Mat4* dest, Vec3f orientationVector, Vec3f positionVector,
                                                 u16 rotationAngle, f32 scaleFactor) {
    if (!check_if_recording()) {
        return;
    }
    append(Op::SetTransformMatrix).set_transform_matrix_data = {
        dest,
        { orientationVector[0], orientationVector[1], orientationVector[2] },
        { positionVector[0], positionVector[1], positionVector[2] },
        rotationAngle,
        scaleFactor
    };
}

void FrameInterpolation_RecordTranslateRotate(Mat4* dest, Vec3f pos, Vec3s rotation) {
    if (!check_if_recording()) {
        return;
    }

    append(Op::SetTranslateRotate).set_translate_rotate_data = { dest,
                                                                 { pos[0], pos[1], pos[2] },
                                                                 { rotation[0], rotation[1], rotation[2] } };
}

void FrameInterpolation_RecordSetMatrixTransformation(Mat4* dest, Vec3f location, Vec3su rotation, f32 scale) {
    if (!check_if_recording()) {
        return;
    }
    append(Op::SetMatrixTransformation).set_matrix_transformation_data = {
        dest, { location[0], location[1], location[2] }, { rotation[0], rotation[1], rotation[2] }, scale
    };
}

void FrameInterpolation_RecordCalculateOrientationMatrix(Mat3* dest, f32 x, f32 y, f32 z, s16 rot) {
    if (!check_if_recording()) {
        return;
    }

    //  append(Op::SetMatrixTransformation).set_calculate_orientation_matrix_data = { dest, x, y, z, rot};
}

// Make a template for deref

void FrameInterpolation_RecordMatrixPosRotXYZ(Mat4* out, Vec3f pos, Vec3s orientation) {
    if (!check_if_recording()) {
        return;
    }
    // Note: Vec3f and Vec3s decay to pointers when passed as parameters.
    // We cast the pointer directly (not &pos which would be pointer-to-pointer).
    append(Op::MatrixPosRotXYZ).matrix_pos_rot_xyz = { out, *((Vec3fInterp*)pos), *((Vec3sInterp*)orientation) };
}

void FrameInterpolation_RecordMatrixPosRotZXY(Mat4* out, Vec3f pos, Vec3s orientation) {
    if (!check_if_recording()) {
        return;
    }
    // Note: Vec3f and Vec3s decay to pointers when passed as parameters.
    // We cast the pointer directly (not &pos which would be pointer-to-pointer).
    append(Op::MatrixPosRotXYZ).matrix_pos_rot_xyz = { out, *((Vec3fInterp*)pos), *((Vec3sInterp*)orientation) };
}

void FrameInterpolation_RecordMatrixPosRotScaleXY(Mat4* matrix, s32 x, s32 y, u16 angle, f32 scale) {
    if (!check_if_recording()) {
        return;
    }
    append(Op::SetMatrixPosRotScaleXY).matrix_pos_rot_scale_xy = { matrix, x, y, angle, scale };
}

void FrameInterpolation_RecordMatrixMultVec3f(Mat4* matrix, Vec3f src, Vec3f dest) {
    if (!check_if_recording()) {
        return;
    }
    // append(Op::MatrixMultVec3f).matrix_vec_translate = { matrix, src, dest };
}

void FrameInterpolation_RecordMatrixRotate1Coord(Mat4* matrix, u32 coord, s16 value) {
    if (!check_if_recording()) {
        return;
    }
    append(Op::MatrixRotate1Coord).matrix_rotate_1_coord = { matrix, coord, value };
}

void FrameInterpolation_RecordMatrixRotateXYCoords(Mat4* matrix, s16 x, s16 y) {
    if (!check_if_recording()) {
        return;
    }
    append(Op::MatrixRotateXYCoords).matrix_rotate_xy_coords = { matrix, x, y };
}

void FrameInterpolation_RecordMatrixMtxFToMtx(MtxF* src, Mtx* dest) {
    if (!check_if_recording()) {
        return;
    }
    append(Op::MatrixMtxFToMtx).matrix_mtxf_to_mtx = { .src = *src, .dest = dest };
}

void FrameInterpolation_RecordBillboardMatrix(MtxF* parent, float tx, float ty, float tz, float sx, float sy, float sz,
                                              s16 roll, Mtx* dest) {
    if (!check_if_recording()) {
        return;
    }
    auto& d = append(Op::BillboardMatrix).billboard_matrix;
    d.parent_mf = *parent;
    d.translation[0] = tx;
    d.translation[1] = ty;
    d.translation[2] = tz;
    d.scale[0] = sx;
    d.scale[1] = sy;
    d.scale[2] = sz;
    d.roll = roll;
    d.dest = dest;
}

void FrameInterpolation_RecordAnimatedPartMatrix(MtxF* parent, float tx, float ty, float tz, s16 rx, s16 ry, s16 rz,
                                                 Mtx* dest) {
    if (!check_if_recording()) {
        return;
    }
    auto& d = append(Op::AnimatedPartMatrix).animated_part_matrix;
    d.parent_mf = *parent;
    d.translation[0] = tx;
    d.translation[1] = ty;
    d.translation[2] = tz;
    d.rotation[0] = rx;
    d.rotation[1] = ry;
    d.rotation[2] = rz;
    d.dest = dest;
}

void FrameInterpolation_RecordMatrixToMtx(Mtx* dest, char* file, s32 line) {
    if (!check_if_recording()) {
        return;
    }
    auto& d = append(Op::MatrixToMtx).matrix_to_mtx = { dest };
    if (has_inv_actor_mtx) {
        d.has_adjusted = true;
        // Matrix_MtxFMtxFMult(&inv_actor_mtx, Matrix_GetCurrent(), &d.src);
    } else {
        d.src = *Matrix_GetCurrent();
    }
}

void FrameInterpolation_RecordMatrixRotateAxis(f32 angle, Vec3f* axis, u8 mode) {
    if (!check_if_recording()) {
        return;
    }
    // append(Op::MatrixRotateAxis).matrix_rotate_axis = { angle, axis, mode };
}

void FrameInterpolation_RecordSkinMatrixMtxFToMtx(MtxF* src, Mtx* dest) {
    if (!check_if_recording()) {
        return;
    }
    FrameInterpolation_RecordMatrixMtxFToMtx(src, dest);
}

void FrameInterpolation_RecordSnowParticles(Gfx* dl, f32 v1x, f32 v1y, f32 v1z, f32 v2x, f32 v2y, f32 v2z, f32 v3x,
                                            f32 v3y, f32 v3z, s32 mode, s16 count, const f32* positions3f) {
    if (!check_if_recording() || count <= 0 || count > SNOW_MAX_PARTICLES) {
        return;
    }
    sCurSnow.dl = dl;
    sCurSnow.v1[0] = v1x;
    sCurSnow.v1[1] = v1y;
    sCurSnow.v1[2] = v1z;
    sCurSnow.v2[0] = v2x;
    sCurSnow.v2[1] = v2y;
    sCurSnow.v2[2] = v2z;
    sCurSnow.v3[0] = v3x;
    sCurSnow.v3[1] = v3y;
    sCurSnow.v3[2] = v3z;
    sCurSnow.mode = mode;
    sCurSnow.count = count;
    for (int i = 0; i < count; i++) {
        sCurSnow.positions[i * 3 + 0] = positions3f[i * 3 + 0];
        sCurSnow.positions[i * 3 + 1] = positions3f[i * 3 + 1];
        sCurSnow.positions[i * 3 + 2] = positions3f[i * 3 + 2];
    }
}

// https://stackoverflow.com/questions/1148309/inverting-a-4x4-matrix
static bool invert_matrix(const float m[16], float invOut[16]) {
    float inv[16], det;
    int i;

    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] +
             m[13] * m[6] * m[11] - m[13] * m[7] * m[10];

    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] -
             m[12] * m[6] * m[11] + m[12] * m[7] * m[10];

    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] +
             m[12] * m[5] * m[11] - m[12] * m[7] * m[9];

    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] -
              m[12] * m[5] * m[10] + m[12] * m[6] * m[9];

    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] -
             m[13] * m[2] * m[11] + m[13] * m[3] * m[10];

    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] +
             m[12] * m[2] * m[11] - m[12] * m[3] * m[10];

    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] -
             m[12] * m[1] * m[11] + m[12] * m[3] * m[9];

    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] +
              m[12] * m[1] * m[10] - m[12] * m[2] * m[9];

    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] +
             m[13] * m[2] * m[7] - m[13] * m[3] * m[6];

    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] -
             m[12] * m[2] * m[7] + m[12] * m[3] * m[6];

    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] +
              m[12] * m[1] * m[7] - m[12] * m[3] * m[5];

    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] -
              m[12] * m[1] * m[6] + m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] -
             m[9] * m[2] * m[7] + m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] +
             m[8] * m[2] * m[7] - m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] -
              m[8] * m[1] * m[7] + m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] -
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0) {
        return false;
    }

    det = 1.0 / det;

    for (i = 0; i < 16; i++) {
        invOut[i] = inv[i] * det;
    }

    return true;
}