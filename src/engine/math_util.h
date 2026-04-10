#ifndef MATH_UTIL_H
#define MATH_UTIL_H

#include <libultra/types.h>

#include "types.h"

/*
 * The sine and cosine tables overlap, but "#define gCosineTable (gSineTable +
 * 0x400)" doesn't give expected codegen; gSineTable and gCosineTable need to
 * be different symbols for code to match. Most likely the tables were placed
 * adjacent to each other, and gSineTable cut short, such that reads overflow
 * into gCosineTable.
 *
 * These kinds of out of bounds reads are undefined behavior, and break on
 * e.g. GCC (which doesn't place the tables next to each other, and probably
 * exploits array sizes for range analysis-based optimizations as well).
 * Thus, for non-IDO compilers we use the standard-compliant version.
 */
extern_s f32 gSineTable[];
#define gCosineTable (gSineTable + 0x400)

#define sins(x) gSineTable[(u16) (x) >> 4]
#define coss(x) gCosineTable[(u16) (x) >> 4]

#define min(a, b) ((a) <= (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define sqr(x) ((x) * (x))

extern_s void *vec3f_copy(Vec3f dest, Vec3f src);
extern_s void *vec3f_set(Vec3f dest, f32 x, f32 y, f32 z);
extern_s void *vec3f_add(Vec3f dest, Vec3f a);
extern_s void *vec3f_sum(Vec3f dest, Vec3f a, Vec3f b);
extern_s void *vec3s_copy(Vec3s dest, Vec3s src);
extern_s void *vec3s_set(Vec3s dest, s16 x, s16 y, s16 z);
extern_s void *vec3s_add(Vec3s dest, Vec3s a);
extern_s void *vec3s_sum(Vec3s dest, Vec3s a, Vec3s b);
extern_s void *vec3s_sub(Vec3s dest, Vec3s a);
extern_s void *vec3s_to_vec3f(Vec3f dest, Vec3s a);
extern_s void *vec3f_to_vec3s(Vec3s dest, Vec3f a);
extern_s void *find_vector_perpendicular_to_plane(Vec3f dest, Vec3f a, Vec3f b, Vec3f c);
extern_s void *vec3f_cross(Vec3f dest, Vec3f a, Vec3f b);
extern_s void *vec3f_normalize(Vec3f dest);
extern_s void mtxf_copy(Mat4 dest, Mat4 src);
extern_s void mtxf_identity(Mat4 mtx);
extern_s void mtxf_translate(Mat4 dest, Vec3f b);
extern_s void mtxf_lookat(Mat4 mtx, Vec3f from, Vec3f to, s16 roll);
extern_s void mtxf_rotate_zxy_and_translate(Mat4 dest, Vec3f translate, Vec3s rotate);
extern_s void mtxf_rotate_xyz_and_translate(Mat4 dest, Vec3f b, Vec3s c);
extern_s void mtxf_billboard(Mat4 dest, Mat4 mtx, Vec3f position, s16 angle);
extern_s void mtxf_align_terrain_normal(Mat4 dest, Vec3f upDir, Vec3f pos, s16 yaw);
extern_s void mtxf_align_terrain_triangle(Mat4 mtx, Vec3f pos, s16 yaw, f32 radius);
extern_s void mtxf_mul(Mat4 dest, Mat4 a, Mat4 b);
extern_s void mtxf_scale_vec3f(Mat4 dest, Mat4 mtx, Vec3f s);
extern_s void mtxf_mul_vec3s(Mat4 mtx, Vec3s b);
extern_s void mtxf_to_mtx(Mtx *dest, Mat4 src);
extern_s void mtxf_rotate_xy(Mtx *mtx, s16 angle);
extern_s void get_pos_from_transform_mtx(Vec3f dest, Mat4 objMtx, Mat4 camMtx);
extern_s void vec3f_get_dist_and_angle(Vec3f from, Vec3f to, f32 *dist, s16 *pitch, s16 *yaw);
extern_s void vec3f_set_dist_and_angle(Vec3f from, Vec3f to, f32  dist, s16  pitch, s16  yaw);
extern_s s32 approach_s32(s32 current, s32 target, s32 inc, s32 dec);
extern_s f32 approach_f32(f32 current, f32 target, f32 inc, f32 dec);
extern_s s16 atan2s(f32 y, f32 x);
extern_s void spline_get_weights(Vec4f result, f32 t, UNUSED s32 c);
extern_s void anim_spline_init(Vec4s *keyFrames);
extern_s s32 anim_spline_poll(Vec3f result);
f32 atan2f(f32 a, f32 b);

#endif // MATH_UTIL_H
