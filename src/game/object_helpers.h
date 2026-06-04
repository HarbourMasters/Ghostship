#ifndef OBJECT_HELPERS_H
#define OBJECT_HELPERS_H

#include <libultra/types.h>

#include "macros.h"
#include "types.h"

// used for chain chomp and wiggler
struct ChainSegment {
    f32 posX;
    f32 posY;
    f32 posZ;
    s16 pitch;
    s16 yaw;
    s16 roll;
};

#define WATER_DROPLET_FLAG_RAND_ANGLE                0x02
#define WATER_DROPLET_FLAG_RAND_OFFSET_XZ            0x04 // Unused
#define WATER_DROPLET_FLAG_RAND_OFFSET_XYZ           0x08 // Unused
#define WATER_DROPLET_FLAG_SET_Y_TO_WATER_LEVEL      0x20
#define WATER_DROPLET_FLAG_RAND_ANGLE_INCR_PLUS_8000 0x40
#define WATER_DROPLET_FLAG_RAND_ANGLE_INCR           0x80 // Unused

// Call spawn_water_droplet with this struct to spawn an object.
struct WaterDropletParams {
    s16 flags; // Droplet spawn flags, see defines above
    s16 model;
    const BehaviorScript *behavior;
    s16 moveAngleRange; // Only used for RAND_ANGLE_INCR flags
    s16 moveRange;      // Only used for RAND_OFFSET flags
    f32 randForwardVelOffset;
    f32 randForwardVelScale;
    f32 randYVelOffset;
    f32 randYVelScale;
    f32 randSizeOffset;
    f32 randSizeScale;
};

struct struct802A1230 {
    /*0x00*/ s16 unk00;
    /*0x02*/ s16 unk02;
};

struct Struct802A272C {
    Vec3f vecF;
    Vec3s vecS;
};

struct SpawnParticlesInfo {
    /*0x00*/ s8 behParam;
    /*0x01*/ s8 count;
    /*0x02*/ u8 model;
    /*0x03*/ s8 offsetY;
    /*0x04*/ s8 forwardVelBase;
    /*0x05*/ s8 forwardVelRange;
    /*0x06*/ s8 velYBase;
    /*0x07*/ s8 velYRange;
    /*0x08*/ s8 gravity;
    /*0x09*/ s8 dragStrength;
    /*0x0C*/ f32 sizeBase;
    /*0x10*/ f32 sizeRange;
};

extern_s Gfx *geo_update_projectile_pos_from_parent(s32 callContext, UNUSED struct GraphNode *node, Mat4 mtx);
extern_s Gfx *geo_update_layer_transparency(s32 callContext, struct GraphNode *node, UNUSED void *context);
extern_s Gfx *geo_switch_anim_state(s32 callContext, struct GraphNode *node, UNUSED void *context);
extern_s Gfx *geo_switch_area(s32 callContext, struct GraphNode *node, UNUSED void *context);
extern_s void obj_update_pos_from_parent_transformation(Mat4 a0, struct Object *a1);
extern_s void obj_apply_scale_to_matrix(struct Object *obj, Mat4 dst, Mat4 src);
extern_s void create_transformation_from_matrices(Mat4 a0, Mat4 a1, Mat4 a2);
extern_s void obj_set_held_state(struct Object *obj, const BehaviorScript *heldBehavior);
extern_s f32 lateral_dist_between_objects(struct Object *obj1, struct Object *obj2);
extern_s f32 dist_between_objects(struct Object *obj1, struct Object *obj2);
extern_s void cur_obj_forward_vel_approach_upward(f32 target, f32 increment);
extern_s s32 approach_f32_signed(f32 *value, f32 target, f32 increment);
extern_s f32 approach_f32_symmetric(f32 value, f32 target, f32 increment);
extern_s s16 approach_s16_symmetric(s16 value, s16 target, s16 increment);
extern_s s32 cur_obj_rotate_yaw_toward(s16 target, s16 increment);
extern_s s16 obj_angle_to_object(struct Object *obj1, struct Object *obj2);
extern_s s16 obj_turn_toward_object(struct Object *obj, struct Object *target, s16 angleIndex, s16 turnAmount);
extern_s void obj_set_parent_relative_pos(struct Object *obj, s16 relX, s16 relY, s16 relZ);
extern_s void obj_set_pos(struct Object *obj, s16 x, s16 y, s16 z);
extern_s void obj_set_angle(struct Object *obj, s16 pitch, s16 yaw, s16 roll);
extern_s struct Object *spawn_object_abs_with_rot(struct Object *parent, s16 uselessArg, u32 model,
                                         const BehaviorScript *behavior,
                                         s16 x, s16 y, s16 z, s16 pitch, s16 yaw, s16 roll);
extern_s struct Object *spawn_object_rel_with_rot(struct Object *parent, u32 model, const BehaviorScript *behavior,
                                         s16 xOff, s16 yOff, s16 zOff, s16 pitch, s16 yaw, UNUSED s16 roll);
extern_s struct Object *spawn_obj_with_transform_flags(struct Object *sp20, s32 model, const BehaviorScript *sp28);
extern_s struct Object *spawn_water_droplet(struct Object *parent, struct WaterDropletParams *params);
extern_s struct Object *spawn_object_at_origin(struct Object *, s32, u32, const BehaviorScript *);
extern_s struct Object *spawn_object_at_origin(struct Object *parent, UNUSED s32 unusedArg, u32 model, const BehaviorScript *behavior);
extern_s struct Object *spawn_object(struct Object *parent, s32 model, const BehaviorScript *behavior);
extern_s struct Object *try_to_spawn_object(s16 offsetY, f32 scale, struct Object *parent, s32 model, const BehaviorScript *behavior);
extern_s struct Object *spawn_object_with_scale(struct Object *parent, s32 model, const BehaviorScript *behavior, f32 scale);
extern_s struct Object *spawn_object_relative(s16 behaviorParam, s16 relativePosX, s16 relativePosY, s16 relativePosZ,
                                     struct Object *parent, s32 model, const BehaviorScript *behavior);
extern_s struct Object *spawn_object_relative_with_scale(s16 behaviorParam, s16 relativePosX, s16 relativePosY,
                                                s16 relativePosZ, f32 scale, struct Object *parent,
                                                s32 model, const BehaviorScript *behavior);
extern_s void cur_obj_move_using_vel(void);
extern_s void obj_copy_graph_y_offset(struct Object *dst, struct Object *src);
extern_s void obj_copy_pos_and_angle(struct Object *dst, struct Object *src);
extern_s void obj_copy_pos(struct Object *dst, struct Object *src);
extern_s void obj_copy_angle(struct Object *dst, struct Object *src);
extern_s void obj_set_gfx_pos_from_pos(struct Object *obj);
extern_s void linear_mtxf_mul_vec3f(Mat4 m, Vec3f dst, Vec3f v);
extern_s void linear_mtxf_transpose_mul_vec3f(Mat4 m, Vec3f dst, Vec3f v);
extern_s void obj_apply_scale_to_transform(struct Object *obj);
extern_s void obj_copy_scale(struct Object *dst, struct Object *src);
extern_s void obj_scale_xyz(struct Object *obj, f32 xScale, f32 yScale, f32 zScale);
extern_s void obj_scale(struct Object *obj, f32 scale);
extern_s void cur_obj_scale(f32 scale);
extern_s void cur_obj_init_animation_with_sound(s32 animIndex);
extern_s void cur_obj_init_animation_with_accel_and_sound(s32 animIndex, f32 accel);
extern_s void cur_obj_init_animation(s32 animIndex);
extern_s void obj_init_animation_with_sound(struct Object *obj, const struct Animation * const* animations, s32 animIndex);
extern_s void cur_obj_enable_rendering(void);
extern_s void cur_obj_disable_rendering(void);
extern_s void cur_obj_unhide(void);
extern_s void cur_obj_hide(void);
extern_s void cur_obj_set_pos_relative(struct Object *other, f32 dleft, f32 dy, f32 dforward);
extern_s void cur_obj_set_pos_relative_to_parent(f32 dleft, f32 dy, f32 dforward);
extern_s void cur_obj_enable_rendering_2(void);
extern_s void obj_set_face_angle_to_move_angle(struct Object *obj);
extern_s u32 get_object_list_from_behavior(const BehaviorScript *behavior);
extern_s struct Object *cur_obj_nearest_object_with_behavior(const BehaviorScript *behavior);
extern_s f32 cur_obj_dist_to_nearest_object_with_behavior(const BehaviorScript* behavior);
extern_s struct Object *cur_obj_find_nearest_object_with_behavior(const BehaviorScript * behavior, f32 *dist);
extern_s struct Object *find_unimportant_object(void);
extern_s s32 count_unimportant_objects(void);
extern_s s32 count_objects_with_behavior(const BehaviorScript *behavior);
extern_s struct Object *cur_obj_find_nearby_held_actor(const BehaviorScript *behavior, f32 maxDist);
extern_s void cur_obj_change_action(s32 action);
extern_s void cur_obj_set_vel_from_mario_vel(f32 f12,f32 f14);
extern_s BAD_RETURN(s16) cur_obj_reverse_animation(void);
extern_s BAD_RETURN(s32) cur_obj_extend_animation_if_at_end(void);
extern_s s32 cur_obj_check_if_near_animation_end(void);
extern_s s32 cur_obj_check_if_at_animation_end(void);
extern_s s32 cur_obj_check_anim_frame(s32 frame);
extern_s s32 cur_obj_check_anim_frame_in_range(s32 startFrame, s32 rangeLength);
extern_s s32 cur_obj_check_frame_prior_current_frame(s16 *a0);
extern_s s32 mario_is_in_air_action(void);
extern_s s32 mario_is_dive_sliding(void);
extern_s void cur_obj_set_y_vel_and_animation(f32 sp18, s32 sp1C);
extern_s void cur_obj_unrender_set_action_and_anim(s32 sp18, s32 sp1C);
extern_s void cur_obj_get_thrown_or_placed(f32 forwardVel, f32 velY, s32 thrownAction);
extern_s void cur_obj_get_dropped(void);
extern_s void cur_obj_set_model(s32 modelID);
extern_s void mario_set_flag(s32 flag);
extern_s s32 cur_obj_clear_interact_status_flag(s32 flag);
extern_s void obj_mark_for_deletion(struct Object *obj);
extern_s void cur_obj_disable(void);
extern_s void cur_obj_become_intangible(void);
extern_s void cur_obj_become_tangible(void);
extern_s void obj_become_tangible(struct Object *obj);
extern_s void cur_obj_update_floor_height(void);
extern_s struct Surface *cur_obj_update_floor_height_and_get_floor(void);
extern_s void cur_obj_apply_drag_xz(f32 dragStrength);
extern_s void cur_obj_move_y(f32 gravity, f32 bounciness, f32 buoyancy);
extern_s void cur_obj_unused_resolve_wall_collisions(f32 offsetY, f32 radius);
extern_s s16 abs_angle_diff(s16 x0, s16 x1);
extern_s void cur_obj_move_xz_using_fvel_and_yaw(void);
extern_s void cur_obj_move_y_with_terminal_vel(void);
extern_s void cur_obj_compute_vel_xz(void);
extern_s f32 increment_velocity_toward_range(f32 value, f32 center, f32 zeroThreshold, f32 increment);
extern_s s32 obj_check_if_collided_with_object(struct Object *obj1, struct Object *obj2);
extern_s void cur_obj_set_behavior(const BehaviorScript *behavior);
extern_s void obj_set_behavior(struct Object *obj, const BehaviorScript *behavior);
extern_s s32 cur_obj_has_behavior(const BehaviorScript *behavior);
extern_s s32 obj_has_behavior(struct Object *obj, const BehaviorScript *behavior);
extern_s f32 cur_obj_lateral_dist_from_mario_to_home(void);
extern_s f32 cur_obj_lateral_dist_to_home(void);
extern_s void cur_obj_set_pos_to_home(void);
extern_s void cur_obj_set_pos_to_home_and_stop(void);
extern_s void cur_obj_shake_y(f32 amount);
extern_s void cur_obj_start_cam_event(UNUSED struct Object *obj, s32 cameraEvent);
extern_s void set_mario_interact_true_if_in_range(UNUSED s32 sp0, UNUSED s32 sp4, f32 sp8);
extern_s void obj_set_billboard(struct Object *obj);
extern_s void cur_obj_set_hitbox_radius_and_height(f32 radius, f32 height);
extern_s void cur_obj_set_hurtbox_radius_and_height(f32 radius, f32 height);
extern_s void obj_spawn_loot_blue_coins(struct Object *obj, s32 numCoins, f32 sp28, s16 posJitter);
extern_s void obj_spawn_loot_yellow_coins(struct Object *obj, s32 numCoins, f32 sp28);
extern_s void cur_obj_spawn_loot_coin_at_mario_pos(void);
extern_s s32 cur_obj_advance_looping_anim(void);
extern_s s32 cur_obj_resolve_wall_collisions(void);
extern_s void cur_obj_update_floor_and_walls(void);
extern_s void cur_obj_move_standard(s16 steepSlopeAngleDegrees);
extern_s void cur_obj_move_using_vel_and_gravity(void);
extern_s void cur_obj_move_using_fvel_and_gravity(void);
extern_s s16 cur_obj_angle_to_home(void);
extern_s void obj_set_gfx_pos_at_obj_pos(struct Object *obj1, struct Object *obj2);
extern_s void obj_translate_local(struct Object *obj, s16 posIndex, s16 localTranslateIndex);
extern_s void obj_build_transform_from_pos_and_angle(struct Object *obj, s16 posIndex, s16 angleIndex);
extern_s void obj_set_throw_matrix_from_transform(struct Object *obj);
extern_s void obj_build_transform_relative_to_parent(struct Object *obj);
extern_s void obj_create_transform_from_self(struct Object *obj);
extern_s void  cur_obj_rotate_face_angle_using_vel(void);
extern_s s32 cur_obj_follow_path(UNUSED s32 unused);
extern_s void chain_segment_init(struct ChainSegment *segment);
extern_s f32 random_f32_around_zero(f32 diameter);
extern_s void obj_scale_random(struct Object *obj, f32 rangeLength, f32 minScale);
extern_s void obj_translate_xyz_random(struct Object *obj, f32 rangeLength);
extern_s void obj_translate_xz_random(struct Object *obj, f32 rangeLength);
extern_s void cur_obj_set_pos_via_transform(void);
extern_s void cur_obj_spawn_particles(struct SpawnParticlesInfo *info);
extern_s s16 cur_obj_reflect_move_angle_off_wall(void);

#define WAYPOINT_FLAGS_END -1
#define WAYPOINT_FLAGS_INITIALIZED 0x8000
#define WAYPOINT_MASK_00FF 0x00FF
#define WAYPOINT_FLAGS_PLATFORM_ON_TRACK_PAUSE 3

#define PATH_NONE 0
#define PATH_REACHED_END -1
#define PATH_REACHED_WAYPOINT 1

extern_s void obj_set_hitbox(struct Object *obj, struct ObjectHitbox *hitbox);
extern_s s32 signum_positive(s32 x);
extern_s f32 absf(f32 x);
extern_s s32 absi(s32 a0);
extern_s s32 cur_obj_wait_then_blink(s32 timeUntilBlinking, s32 numBlinks);
extern_s s32 cur_obj_is_mario_ground_pounding_platform(void);
extern_s void spawn_mist_particles(void);
extern_s void spawn_mist_particles_with_sound(u32 sp18);
extern_s void cur_obj_push_mario_away(f32 radius);
extern_s void cur_obj_push_mario_away_from_cylinder(f32 radius, f32 extentY);
extern_s s32 cur_obj_set_direction_table(s8 *a0);
extern_s s32 cur_obj_progress_direction_table(void);
extern_s void stub_obj_helpers_3(UNUSED s32 sp0, UNUSED s32 sp4);
extern_s void cur_obj_scale_over_time(s32 a0, s32 a1, f32 sp10, f32 sp14);
extern_s void cur_obj_set_pos_to_home_with_debug(void);
extern_s s32 cur_obj_is_mario_on_platform(void);
extern_s s32 jiggle_bbh_stair(s32 a0);
extern_s void cur_obj_call_action_function(void (*actionFunctions[])(void));
extern_s void spawn_base_star_with_no_lvl_exit(void);
extern_s s32 bit_shift_left(s32 a0);
extern_s s32 cur_obj_mario_far_away(void);
extern_s s32 is_mario_moving_fast_or_in_air(s32 speedThreshold);
extern_s s32 is_item_in_array(s8 item, s8 *array);
extern_s void cur_obj_enable_rendering_if_mario_in_room(void);
extern_s s32 cur_obj_set_hitbox_and_die_if_attacked(struct ObjectHitbox *hitbox, s32 deathSound, s32 noLootCoins);
extern_s void obj_explode_and_spawn_coins(f32 sp18, s32 sp1C);
extern_s void obj_set_collision_data(struct Object *obj, const void *segAddr);
extern_s void cur_obj_if_hit_wall_bounce_away(void);
extern_s s32 cur_obj_hide_if_mario_far_away_y(f32 distY);
extern_s Gfx *geo_offset_klepto_held_object(s32 callContext, struct GraphNode *node, UNUSED Mat4 mtx);
extern_s Gfx *geo_offset_klepto_debug(s32 callContext, struct GraphNode *node, UNUSED Mat4 mtx);
extern_s s32 obj_is_hidden(struct Object *obj);
extern_s void enable_time_stop(void);
extern_s void disable_time_stop(void);
extern_s void set_time_stop_flags(s32 flags);
extern_s void clear_time_stop_flags(s32 flags);
extern_s s32 cur_obj_can_mario_activate_textbox(f32 radius, f32 height, UNUSED s32 unused);
extern_s s32 cur_obj_can_mario_activate_textbox_2(f32 radius, f32 height);
extern_s s32 cur_obj_update_dialog(s32 actionArg, s32 dialogFlags, s32 dialogID, UNUSED s32 unused);
extern_s s32 cur_obj_update_dialog_with_cutscene(s32 actionArg, s32 dialogFlags, s32 cutsceneTable, s32 dialogID);
extern_s s32 cur_obj_has_model(u16 modelID);
extern_s void cur_obj_align_gfx_with_floor(void);
extern_s s32 mario_is_within_rectangle(s16 minX, s16 maxX, s16 minZ, s16 maxZ);
extern_s void cur_obj_shake_screen(s32 shake);
extern_s s32 obj_attack_collided_from_other_object(struct Object *obj);
extern_s s32 cur_obj_was_attacked_or_ground_pounded(void);
extern_s void obj_copy_behavior_params(struct Object *dst, struct Object *src);
extern_s void cur_obj_init_animation_and_anim_frame(s32 animIndex, s32 animFrame);
extern_s s32 cur_obj_init_animation_and_check_if_near_end(s32 animIndex);
extern_s void cur_obj_init_animation_and_extend_if_at_end(s32 animIndex);
extern_s s32 cur_obj_check_grabbed_mario(void);
extern_s s32 player_performed_grab_escape_action(void);
extern_s void cur_obj_unused_play_footstep_sound(s32 animFrame1, s32 animFrame2, s32 sound);
extern_s void enable_time_stop_including_mario(void);
extern_s void disable_time_stop_including_mario(void);
extern_s s32 cur_obj_check_interacted(void);
extern_s void cur_obj_spawn_loot_blue_coin(void);
extern_s void cur_obj_spawn_star_at_y_offset(f32 targetX, f32 targetY, f32 targetZ, f32 offsetY);

#endif // OBJECT_HELPERS_H
