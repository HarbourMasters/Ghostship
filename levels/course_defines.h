/* Define lists for list of course for macros. Each of the following fields are described:
 * Argument 1: Course macro for define.
 * Argument 2: Star collection dance cutscenes
 * Each hex digit determines what dance cutscene to play for the stars in the course. The last digit is
 * unused. See determine_dance_cutscene() in camera.c for details.
 * Cutscene Digits:
 *      0: Lakitu flies away after the dance
 *      1: The camera rotates around mario
 *      2: The camera goes to a closeup of mario
 *      3: Bowser keys and the grand star
 *      4: Default, used for 100 coin stars, 8 red coin stars in bowser levels, and secret stars
 * 
 * #region [PU] Added info for each course
 * Argument 3: Human readable name of the course.
 * #endregion
 */
DEFINE_COURSE(      COURSE_NONE,     0x44444440, "Course Hub (Castle Grounds)") // (0)
DEFINE_COURSE(      COURSE_BOB,      0x00022240, "Bob Omb Battlefield") // (1)
DEFINE_COURSE(      COURSE_WF,       0x00002040, "Whomp's Fortress") // (2)
DEFINE_COURSE(      COURSE_JRB,      0x22222240, "Jolly Roger Bay") // (3)
DEFINE_COURSE(      COURSE_CCM,      0x00220040, "Cool Cool Mountain") // (4)
DEFINE_COURSE(      COURSE_BBH,      0x22222240, "Big Boo's Haunt") // (5)
DEFINE_COURSE(      COURSE_HMC,      0x22222240, "Hazy Maze Cave") // (6)
DEFINE_COURSE(      COURSE_LLL,      0x21212140, "Lethal Lava Land") // (7)
DEFINE_COURSE(      COURSE_SSL,      0x20222240, "Shifting Sand Land") // (8)
DEFINE_COURSE(      COURSE_DDD,      0x22222240, "Dire Dire Docks") // (9)
DEFINE_COURSE(      COURSE_SL,       0x02020240, "Snowman's Land") // (10)
DEFINE_COURSE(      COURSE_WDW,      0x22102240, "Wet Dry World") // (11)
DEFINE_COURSE(      COURSE_TTM,      0x00000040, "Tall Tall Mountain") // (12)
DEFINE_COURSE(      COURSE_THI,      0x11112140, "Tiny Huge Island") // (13)
DEFINE_COURSE(      COURSE_TTC,      0x22222240, "Tick Tock Clock") // (14)
DEFINE_COURSE(      COURSE_RR,       0x00000040, "Rainbow Ride") // (15)
DEFINE_COURSES_END()
DEFINE_BONUS_COURSE(COURSE_BITDW,    0x34444440, "Bowser in the Dark World") // (16)
DEFINE_BONUS_COURSE(COURSE_BITFS,    0x34444440, "Bowser in the Fire Sea") // (17)
DEFINE_BONUS_COURSE(COURSE_BITS,     0x34444440, "Bowser in the Sky") // (18)
DEFINE_BONUS_COURSE(COURSE_PSS,      0x24444440, "Princess's Secret Slide") // (19)
DEFINE_BONUS_COURSE(COURSE_COTMC,    0x44444440, "Cavern of the Metal Cap") // (20)
DEFINE_BONUS_COURSE(COURSE_TOTWC,    0x04444440, "Tower of the Wing Cap") // (21)
DEFINE_BONUS_COURSE(COURSE_VCUTM,    0x24444440, "Vanish Cap Under the Moat") // (22)
DEFINE_BONUS_COURSE(COURSE_WMOTR,    0x04444440, "Winged Mario over the Rainbow") // (23)
DEFINE_BONUS_COURSE(COURSE_SA,       0x24444440, "Secret Aquarium") // (24)
DEFINE_BONUS_COURSE(COURSE_CAKE_END, 0x44444440, "The End (Cake Scene)") // (25)
