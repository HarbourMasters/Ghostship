// breakable_box.inc.c

void bhv_breakable_box_loop(void) {
    obj_set_hitbox(o, &sBreakableBoxHitbox);
    if (game_value(GAME_VALUE_BREAKABLE_BOX_SETS_MODEL, TRUE)) {
        cur_obj_set_model(MODEL_BREAKABLE_BOX_SMALL);
    }

    if (o->oTimer == 0) {
        breakable_box_init();
    }

    if (cur_obj_was_attacked_or_ground_pounded()) {
        u32 marioAction = game_value(GAME_VALUE_BREAKABLE_BOX_MARIO_ACTION, 0);
        if (marioAction != 0) {
            set_mario_action(gMarioState, marioAction, 0);
        }
        obj_explode_and_spawn_coins(46.0f, 1);
        create_sound_spawner(SOUND_GENERAL_BREAK_BOX);
    }
}
