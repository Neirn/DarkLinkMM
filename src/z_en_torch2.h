#ifndef Z_EN_TORCH2_H
#define Z_EN_TORCH2_H

#include "ultra64.h"
#include "z64actor.h"
#include "z64player.h"

typedef struct EnTorch2 {
    Player player;
    f32 stickTilt;
    s16 stickAngle;
    f32 swordJumpHeight;
    s32 holdShieldTimer;
    u8 zTargetFlag;
    u8 deathFlag;
    Input input;
    u8 swordJumpState;
    Vec3f spawnPoint;
    u8 jumpslashTimer;
    u8 jumpslashFlag;
    u8 actionState;
    u8 swordJumpTimer;
    u8 counterState;
    u8 dodgeRollState;
    u8 staggerCount;
    u8 staggerTimer;
    u8 lastSwordAnim;
    u8 alpha;
} EnTorch2;

#endif
