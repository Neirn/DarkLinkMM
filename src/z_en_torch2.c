/*
 * File: z_en_torch2.c
 * Overlay: ovl_En_Torch2
 * Description: Dark Link
 */

#include "z_en_torch2.h"

#include "modding.h"
#include "global.h"
#include "libc/math.h" // why do you need to exist to get rid of errors in vscode
#include "recomputils.h"
#include "recompconfig.h"
#include "proxymm_custom_actor.h"

#include "object_torch2/object_torch2.h"

#define FLAGS                                                                                 \
    (ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE | ACTOR_FLAG_UPDATE_CULLING_DISABLED | \
     ACTOR_FLAG_DRAW_CULLING_DISABLED)

typedef enum EnTorch2ActionStates {
    /* 0 */ ENTORCH2_WAIT,
    /* 1 */ ENTORCH2_ATTACK,
    /* 2 */ ENTORCH2_DEATH,
    /* 3 */ ENTORCH2_DAMAGE
} EnTorch2ActionStates;

void EnTorch2_Init(Actor* thisx, PlayState* play2);
void EnTorch2_Destroy(Actor* thisx, PlayState* play);
void EnTorch2_Update(Actor* thisx, PlayState* play2);
void EnTorch2_Draw(Actor* thisx, PlayState* play2);

ActorProfile En_Torch2_Profile = {
    /**/ ACTOR_ID_MAX,
    /**/ ACTORCAT_BOSS,
    /**/ FLAGS,
    /**/ GAMEPLAY_KEEP,
    /**/ sizeof(EnTorch2),
    /**/ EnTorch2_Init,
    /**/ EnTorch2_Destroy,
    /**/ EnTorch2_Update,
    /**/ EnTorch2_Draw,
};

s16 CUSTOM_ACTOR_EN_DARKLINK = ACTOR_ID_MAX;

RECOMP_CALLBACK("*", recomp_on_init) 
void OoTArwing_OnRecompInit() {
    CUSTOM_ACTOR_EN_DARKLINK = CustomActor_Register(&En_Torch2_Profile);
}

// Spawn Arwing in South Clock Town
//RECOMP_HOOK("Actor_SpawnTransitionActors")
//void OoTArwing_OnRoomLoad(PlayState* play, ActorContext* actorCtx) {
//    if (play->sceneId == SCENE_00KEIKOKU) {
//        //Actor_SpawnAsChildAndCutscene(&play->actorCtx, play, CUSTOM_ACTOR_EN_DARKLINK, -367.0f, 0.0f, -245.0f, 0, 0, 0, 0xFFFF, 0, 0, 0);
//    }
//}

RECOMP_HOOK("Player_Update")
void SpawnArwingWithL(Actor* thisx, PlayState* play) {
    Input* input = CONTROLLER1(&play->state);
    if (CHECK_BTN_ALL(input->press.button, BTN_L)) {
        Actor_SpawnAsChildAndCutscene(&play->actorCtx, play, CUSTOM_ACTOR_EN_DARKLINK, \
            thisx->world.pos.x, thisx->world.pos.y + Player_GetHeight((Player*)thisx) + 5.0f, thisx->world.pos.z,\
            thisx->world.rot.x, thisx->world.rot.y, thisx->world.rot.z, \
            0xFFFF, 0, 0, 0);
    }
}

static DamageTable sDamageTable = {
    /* Deku nut      */ DMG_ENTRY(0, 0x1),
    /* Deku stick    */ DMG_ENTRY(2, 0x0),
    /* Slingshot     */ DMG_ENTRY(1, 0x0),
    /* Explosive     */ DMG_ENTRY(2, 0x0),
    /* Boomerang     */ DMG_ENTRY(0, 0x1),
    /* Normal arrow  */ DMG_ENTRY(2, 0x0),
    /* Hammer swing  */ DMG_ENTRY(2, 0x0),
    /* Hookshot      */ DMG_ENTRY(0, 0x1),
    /* Kokiri sword  */ DMG_ENTRY(1, 0x0),
    /* Master sword  */ DMG_ENTRY(2, 0x0),
    /* Giant's Knife */ DMG_ENTRY(4, 0x0),
    /* Fire arrow    */ DMG_ENTRY(2, 0x0),
    /* Ice arrow     */ DMG_ENTRY(2, 0x0),
    /* Light arrow   */ DMG_ENTRY(2, 0x0),
    /* Unk arrow 1   */ DMG_ENTRY(2, 0x0),
    /* Unk arrow 2   */ DMG_ENTRY(2, 0x0),
    /* Unk arrow 3   */ DMG_ENTRY(2, 0x0),
    /* Fire magic    */ DMG_ENTRY(2, 0xE),
    /* Ice magic     */ DMG_ENTRY(0, 0x6),
    /* Light magic   */ DMG_ENTRY(3, 0xD),
    /* Shield        */ DMG_ENTRY(0, 0x0),
    /* Mirror Ray    */ DMG_ENTRY(0, 0x0),
    /* Kokiri spin   */ DMG_ENTRY(1, 0x0),
    /* Giant spin    */ DMG_ENTRY(4, 0x0),
    /* Master spin   */ DMG_ENTRY(2, 0x0),
    /* Kokiri jump   */ DMG_ENTRY(2, 0x0),
    /* Giant jump    */ DMG_ENTRY(8, 0x0),
    /* Master jump   */ DMG_ENTRY(4, 0x0),
    /* Unknown 1     */ DMG_ENTRY(0, 0x0),
    /* Unblockable   */ DMG_ENTRY(0, 0x0),
    /* Hammer jump   */ DMG_ENTRY(4, 0x0),
    /* Unknown 2     */ DMG_ENTRY(0, 0x0),
};

extern PlayerAgeProperties sPlayerAgeProperties[PLAYER_FORM_MAX];

static EffectBlureInit2 sBlureInit = {
    0,
    EFFECT_BLURE_ELEMENT_FLAG_8,
    0,
    { 255, 255, 255, 255 },
    { 255, 255, 255, 64 },
    { 255, 255, 255, 0 },
    { 255, 255, 255, 0 },
    4,
    0,
    EFF_BLURE_DRAW_MODE_SMOOTH,
    0,
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
};

static EffectTireMarkInit sTireMarkInit = {
    0,
    63,
    { 0, 0, 15, 100 },
};

static PlayerAgeProperties darkLinkProperties;

void EnTorch2_Init(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    EnTorch2 *this = (EnTorch2 *)thisx;
    Player* player = &this->player;

    this->input.cur.button = this->input.press.button = this->input.rel.button = 0;
    this->input.cur.stick_x = this->input.cur.stick_y = 0;
    player->currentShield = PLAYER_SHIELD_HEROS_SHIELD;
    player->heldItemAction = PLAYER_IA_SWORD_GILDED;
    player->heldItemId = ITEM_SWORD_GILDED;

    player->actor.room = -1;
    player->csId = CS_ID_NONE;
    player->transformation = PLAYER_FORM_HUMAN;

    // copied from oot age properties
    darkLinkProperties = sPlayerAgeProperties[PLAYER_FORM_ZORA];
    darkLinkProperties.unk_28 = 44.8f;
    darkLinkProperties.unk_3C = 15.0f;
    PlayerAgeProperties *fdProps = &sPlayerAgeProperties[PLAYER_FORM_FIERCE_DEITY];
    darkLinkProperties.unk_44 = sPlayerAgeProperties[PLAYER_FORM_FIERCE_DEITY].unk_44;
    for (int i = 0; i < 4; ++i) {
        darkLinkProperties.unk_4A[i] = fdProps->unk_4A[i];
        darkLinkProperties.unk_62[i] = fdProps->unk_62[i];
        darkLinkProperties.unk_7A[i] = fdProps->unk_7A[i];
    }
    darkLinkProperties.voiceSfxIdOffset = SFX_VOICE_BANK_SIZE * 0;
    darkLinkProperties.surfaceSfxIdOffset = 0x80;

    player->ageProperties = &darkLinkProperties;

    Player_SetModelGroup(player, PLAYER_MODELGROUP_ONE_HAND_SWORD);
    play->playerInit(player, play, &gDarkLinkSkel);
    // this->actor.naviEnemyId = NAVI_ENEMY_DARK_LINK;
    player->cylinder.base.acFlags = AC_ON | AC_TYPE_PLAYER;
    player->meleeWeaponQuads[0].base.atFlags = player->meleeWeaponQuads[1].base.atFlags = AT_ON | AT_TYPE_ENEMY;
    player->meleeWeaponQuads[0].base.acFlags = player->meleeWeaponQuads[1].base.acFlags = AC_ON | AC_HARD | AC_TYPE_PLAYER;
    player->meleeWeaponQuads[0].base.colMaterial = player->meleeWeaponQuads[1].base.colMaterial = COL_MATERIAL_METAL;
    player->meleeWeaponQuads[0].elem.atDmgInfo.damage = player->meleeWeaponQuads[1].elem.atDmgInfo.damage = 8;
    player->meleeWeaponQuads[0].elem.acElemFlags = player->meleeWeaponQuads[1].elem.acElemFlags = ACELEM_ON;
    player->shieldQuad.base.atFlags = AT_ON | AT_TYPE_ENEMY;
    player->shieldQuad.base.acFlags = AC_ON | AC_HARD | AC_TYPE_PLAYER;
    player->actor.colChkInfo.damageTable = &sDamageTable;
    player->actor.colChkInfo.health = gSaveContext.save.saveInfo.playerData.healthCapacity >> 3;
    player->actor.colChkInfo.cylRadius = 60;
    player->actor.colChkInfo.cylHeight = 100;

    Effect_Add(play, &player->meleeWeaponEffectIndex[0], EFFECT_BLURE2, 0, 0, &sBlureInit);
    Effect_Add(play, &player->meleeWeaponEffectIndex[1], EFFECT_BLURE2, 0, 0, &sBlureInit);
    Effect_Add(play, &player->meleeWeaponEffectIndex[2], EFFECT_TIRE_MARK, 0, 0, &sTireMarkInit);

    play->func_18780(player, play);

    this->actionState = ENTORCH2_WAIT;
    this->dodgeRollState = 0;
    this->swordJumpHeight = 0.0f;
    this->swordJumpState = 0;
    this->jumpslashTimer = 0;
    this->jumpslashFlag = false;
    this->counterState = this->staggerTimer = this->staggerCount = 0;
    this->lastSwordAnim = 0;
    this->alpha = 95;
    this->spawnPoint = player->actor.home.pos;
}

void EnTorch2_Destroy(Actor* thisx, PlayState* play) {
    s32 pad;
    Player* this = (Player*)thisx;

    Effect_Destroy(play, this->meleeWeaponEffectIndex[0]);
    Effect_Destroy(play, this->meleeWeaponEffectIndex[1]);
    Effect_Destroy(play, this->meleeWeaponEffectIndex[2]);
    Audio_RestorePrevBgm();
    Collider_DestroyCylinder(play, &this->cylinder);
    Collider_DestroyQuad(play, &this->meleeWeaponQuads[0]);
    Collider_DestroyQuad(play, &this->meleeWeaponQuads[1]);
    Collider_DestroyQuad(play, &this->shieldQuad);
}

#include "overlays/actors/ovl_Arms_Hook/z_arms_hook.h"

Actor* Actor_GetProjectileActor(PlayState* play, Actor* refActor, f32 radius) {
    Actor* actor;
    Vec3f spA8;
    Vec3f delta;
    Vec3f sp90;
    Vec3f sp84;

    actor = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
    while (actor != NULL) {
        if (((actor->id != ACTOR_ARMS_HOOK) && (actor->id != ACTOR_EN_ARROW)) || (actor == refActor)) {
            actor = actor->next;
        } else {
            //! @bug The projectile actor gets unsafely casted to a hookshot to check its timer, even though
            //! it can also be an arrow.
            //  Luckily, the field at the same offset in the arrow actor is the x component of a vector
            //  which will rarely ever be 0. So it's very unlikely for this bug to cause an issue.
            if ((Math_Vec3f_DistXYZ(&refActor->world.pos, &actor->world.pos) > radius) ||
                (((ArmsHook*)actor)->timer == 0)) {
                actor = actor->next;
            } else {
                delta.x = (actor->speed * 10.0f) * Math_SinS(actor->world.rot.y);
                delta.y = actor->velocity.y + (actor->gravity * 10.0f);
                delta.z = (actor->speed * 10.0f) * Math_CosS(actor->world.rot.y);

                spA8.x = actor->world.pos.x + delta.x;
                spA8.y = actor->world.pos.y + delta.y;
                spA8.z = actor->world.pos.z + delta.z;

                if (CollisionCheck_CylSideVsLineSeg(refActor->colChkInfo.cylRadius, refActor->colChkInfo.cylHeight,
                                                    0.0f, &refActor->world.pos, &actor->world.pos, &spA8, &sp90,
                                                    &sp84)) {
                    return actor;
                } else {
                    actor = actor->next;
                }
            }
        }
    }

    return NULL;
}

// copied from OOT
Actor* func_80033684(PlayState* play, Actor* explosiveActor) {
    Actor* actor = play->actorCtx.actorLists[ACTORCAT_EXPLOSIVES].first;

    while (actor != NULL) {
        if ((actor == explosiveActor) || (actor->params != 1)) {
            actor = actor->next;
        } else {
            if (Actor_WorldDistXYZToActor(explosiveActor, actor) <= (actor->shape.rot.z * 10) + 80.0f) {
                return actor;
            } else {
                actor = actor->next;
            }
        }
    }

    return NULL;
}

Actor* EnTorch2_GetAttackItem(PlayState* play, Player* this) {
    Actor* rangedItem = Actor_GetProjectileActor(play, &this->actor, 4000.0f);

    if (rangedItem != NULL) {
        return rangedItem;
    } else {
        return func_80033684(play, &this->actor);
    }
}

s32 EnTorch2_SwingSword(PlayState* play, Input* input, EnTorch2* this) {
    f32 noAttackChance = 0.0f;
    s32 attackDelay = 7;
    Player* player = GET_PLAYER(play);

    if ((this->player.speedXZ < 0.0f) || (player->speedXZ < 0.0f)) {
        return 0;
    }
    if (gSaveContext.save.saveInfo.playerData.health < 0x50) {
        attackDelay = 15;
        noAttackChance += 0.3f;
    }
    if (this->alpha != 255) {
        noAttackChance += 2.0f;
    }
    if ((((play->gameplayFrames & attackDelay) == 0) || (this->swordJumpState != 0)) && (noAttackChance <= Rand_ZeroOne())) {
        if (this->swordJumpState == 0) {
            switch ((s32)(Rand_ZeroOne() * 7.0f)) {
                case 1:
                case 5:
                    this->stickAngle += 0x4000;
                    this->stickTilt = 127.0f;
                    break;
                case 2:
                case 6:
                    this->stickAngle -= 0x4000;
                    this->stickTilt = 127.0f;
                    break;
            }
        }
        input->cur.button = BTN_B;
        return 1;
    }
    return 0;
}

void EnTorch2_Backflip(EnTorch2* this, Input* input, Actor* thisx) {
    thisx->world.rot.y = thisx->shape.rot.y = thisx->yawTowardsPlayer;
    this->stickAngle = thisx->yawTowardsPlayer + 0x8000;
    this->stickTilt = 127.0f;
    this->zTargetFlag = true;
    input->cur.button = BTN_A;
    this->player.invincibilityTimer = 10;
    this->counterState = 0;
}

f32 D_801305B0 = 0.7950898f;
s8 D_801305B4 = 35;

s32 func_800354B4(PlayState* play, Actor* actor, f32 range, s16 arg3, s16 arg4, s16 arg5) {
    Player* player = GET_PLAYER(play);
    s16 var1;
    s16 var2;

    var1 = (s16)(actor->yawTowardsPlayer + 0x8000) - player->actor.shape.rot.y;
    var2 = actor->yawTowardsPlayer - arg5;

    if ((actor->xzDistToPlayer <= range) && (player->meleeWeaponState != 0) && (arg4 >= ABS(var1)) &&
        (arg3 >= ABS(var2))) {
        return true;
    } else {
        return false;
    }
}

void EnTorch2_Update(Actor *thisx, PlayState *play2) {
    PlayState *play = play2;
    Player *player2 = GET_PLAYER(play2);
    Player *player = player2;
    EnTorch2 *this = (EnTorch2 *)thisx;
    Player *player1 = &this->player;
    Input *input = &this->input;
    Camera *mainCam;
    s16 sp66;
    s8 stickY;
    u32 pad54;
    Actor *attackItem;
    s16 sp5A;

    sp5A = player->actor.shape.rot.y - player1->actor.shape.rot.y;
    input->cur.button = 0;
    mainCam = Play_GetCamera(play, CAM_ID_MAIN);
    attackItem = EnTorch2_GetAttackItem(play, player1);
    switch (this->actionState) {
        case ENTORCH2_WAIT:
            player1->actor.shape.rot.y = player1->actor.world.rot.y = player1->actor.yawTowardsPlayer;
            player1->skelAnime.curFrame = 0.0f;
            player1->skelAnime.playSpeed = 0.0f;
            player1->actor.world.pos.x = (Math_SinS(player1->actor.world.rot.y) * 25.0f) + this->spawnPoint.x;
            player1->actor.world.pos.z = (Math_CosS(player1->actor.world.rot.y) * 25.0f) + this->spawnPoint.z;
            if ((player1->actor.xzDistToPlayer <= 120.0f) || Actor_IsTargeted(play, &player1->actor) ||
                (attackItem != NULL)) {
                if (attackItem != NULL) {
                    this->dodgeRollState = 1;
                    this->stickAngle = player1->actor.yawTowardsPlayer;
                    this->stickTilt = 127.0f;
                    input->cur.button = BTN_A;
                    this->zTargetFlag = false;
                    sp66 = mainCam->camDir.y - this->stickAngle;
                    this->input.cur.stick_x = this->stickTilt * Math_SinS(sp66);
                    stickY = this->stickTilt * Math_CosS(sp66);
                    if (stickY) {
                    }
                    this->input.cur.stick_y = stickY;
                }
                Audio_PlayBgm_StorePrevBgm(NA_BGM_MINI_BOSS);
                this->actionState = ENTORCH2_ATTACK;
            }
            break;

        case ENTORCH2_ATTACK:
            this->stickTilt = 0.0f;

            // Handles Dark Link's sword clanking on Link's sword

            if ((player1->meleeWeaponQuads[0].base.acFlags & AC_BOUNCED) ||
                (player1->meleeWeaponQuads[1].base.acFlags & AC_BOUNCED)) {
                player1->meleeWeaponQuads[0].base.acFlags &= ~AC_BOUNCED;
                player1->meleeWeaponQuads[1].base.acFlags &= ~AC_BOUNCED;
                player1->meleeWeaponQuads[0].base.atFlags |= AT_BOUNCED;
                player1->meleeWeaponQuads[1].base.atFlags |= AT_BOUNCED;
                player1->cylinder.base.acFlags &= ~AC_HIT;

                if (this->lastSwordAnim != player1->meleeWeaponAnimation) {
                    this->staggerCount++;
                    this->lastSwordAnim = player1->meleeWeaponAnimation;
                }
                /*! @bug
                 *  player1 code is needed to reset this->counterState, and should run regardless
                 *  of how much health Link has. Without it, this->counterState stays at 2 until
                 *  something else resets it, preventing Dark Link from using his shield and
                 *  creating a hole in his defenses. player1 also makes Dark Link harder at low
                 *  health, while the other health checks are intended to make him easier.
                 */
                if ((gSaveContext.save.saveInfo.playerData.health < 0x50) && (this->counterState != 0)) {
                    this->counterState = 0;
                    this->staggerTimer = 50;
                }
            }
            if ((this->counterState != 0) && (player1->meleeWeaponState != 0)) {
                CollisionCheck_SetAC(play, &play->colChkCtx, &player1->meleeWeaponQuads[0].base);
                CollisionCheck_SetAC(play, &play->colChkCtx, &player1->meleeWeaponQuads[1].base);
            }

            // Ignores hits when jumping on Link's sword
            if ((player1->invincibilityTimer < 0) && (this->actionState != ENTORCH2_DAMAGE) &&
                (player1->cylinder.base.acFlags & AC_HIT)) {
                player1->cylinder.base.acFlags &= ~AC_HIT;
            }

            // Handles Dark Link rolling to dodge item attacks

            if (this->dodgeRollState != 0) {
                this->stickTilt = 127.0f;
            } else if (attackItem != NULL) {
                this->dodgeRollState = 1;
                this->stickAngle = player1->actor.yawTowardsPlayer;
                this->stickTilt = 127.0f;
                input->cur.button = BTN_A;
            } else if (this->jumpslashTimer == 0) {

                // Handles Dark Link's initial reaction to jumpslashes

                if (((player->meleeWeaponState != 0) || (player->actor.velocity.y > -3.0f)) &&
                    (player->meleeWeaponAnimation == PLAYER_MWA_JUMPSLASH_START)) {
                    player1->actor.world.rot.y = player1->actor.shape.rot.y = player1->actor.yawTowardsPlayer;

                    if (play->gameplayFrames % 2) {
                        this->stickAngle = player1->actor.yawTowardsPlayer + 0x4000;
                    } else {
                        this->stickAngle = player1->actor.yawTowardsPlayer - 0x4000;
                    }
                    this->stickTilt = 127.0f;
                    this->jumpslashTimer = 15;
                    this->jumpslashFlag = false;
                    input->cur.button |= BTN_A;

                    // Handles jumping on Link's sword

                } else if (this->swordJumpState != 0) {
                    this->stickTilt = 0.0f;
                    player->stateFlags3 |= PLAYER_STATE3_4;
                    Math_SmoothStepToF(&player1->actor.world.pos.x,
                                       (Math_SinS(player->actor.shape.rot.y - 0x3E8) * 45.0f) +
                                           player->actor.world.pos.x,
                                       1.0f, 5.0f, 0.0f);
                    Math_SmoothStepToF(&player1->actor.world.pos.z,
                                       (Math_CosS(player->actor.shape.rot.y - 0x3E8) * 45.0f) +
                                           player->actor.world.pos.z,
                                       1.0f, 5.0f, 0.0f);
                    this->swordJumpTimer--;
                    if (((u32)this->swordJumpTimer == 0) ||
                        ((player->invincibilityTimer > 0) && (player1->meleeWeaponState == 0))) {
                        player1->actor.world.rot.y = player1->actor.shape.rot.y = player1->actor.yawTowardsPlayer;
                        input->cur.button = BTN_A;
                        player->stateFlags3 &= ~PLAYER_STATE3_4;
                        this->stickTilt = 127.0f;
                        player->skelAnime.curFrame = 3.0f;
                        this->stickAngle = player1->actor.yawTowardsPlayer + 0x8000;
                        this->swordJumpTimer = this->swordJumpState = 0;
                        player1->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED;
                    } else if (this->swordJumpState == 1) {
                        if (this->swordJumpTimer < 16) {
                            EnTorch2_SwingSword(play, input, this);
                            this->swordJumpState++;
                        } else if (this->swordJumpTimer == 19) {
                            // func_800F4190(&player1->actor.projectedPos, NA_SE_VO_LI_AUTO_JUMP);
                            AudioSfx_PlaySfx(NA_SE_VO_LI_AUTO_JUMP, &player1->actor.projectedPos, 4, &D_801305B0, &gSfxDefaultFreqAndVolScale, &D_801305B4);
                        }
                    }
                } else {
                    // player1 does nothing, as this->holdShieldTimer is never set.
                    if (this->holdShieldTimer != 0) {
                        this->holdShieldTimer--;
                        input->cur.button = BTN_R;
                    }

                    // Handles Dark Link's reaction to sword attack other than jumpslashes

                    if (func_800354B4(play, &player1->actor, 120.0f, 0x7FFF, 0x7FFF, player1->actor.world.rot.y)) {
                        if ((player->meleeWeaponAnimation == PLAYER_MWA_STAB_1H) &&
                            (player1->actor.xzDistToPlayer < 90.0f)) {

                            // Handles the reaction to a one-handed stab. If the conditions are satisfied,
                            // Dark Link jumps on Link's sword. Otherwise he backflips away.

                            if ((player1->meleeWeaponState == 0) && (this->counterState == 0) &&
                                (player->invincibilityTimer == 0) &&
                                (player->meleeWeaponAnimation == PLAYER_MWA_STAB_1H) &&
                                (player1->actor.xzDistToPlayer <= 85.0f) && Actor_IsTargeted(play, &player1->actor)) {

                                this->stickTilt = 0.0f;
                                this->swordJumpState = 1;
                                player->stateFlags3 |= PLAYER_STATE3_4;
                                player1->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
                                this->swordJumpTimer = 27;
                                player->meleeWeaponState = 0;
                                player->speedXZ = 0.0f;
                                player1->invincibilityTimer = -7;
                                player1->speedXZ = 0.0f;
                                player->skelAnime.curFrame = 2.0f;
                                // LinkAnimation_Update(play, &player->skelAnime);
                                PlayerAnimation_Update(play, &player->skelAnime);
                                this->holdShieldTimer = 0;
                                input->cur.button = BTN_A;
                            } else {
                                EnTorch2_Backflip(this, input, &player1->actor);
                            }
                        } else {

                            // Handles reactions to all other sword attacks

                            this->stickAngle = thisx->yawTowardsPlayer;
                            input->cur.button = BTN_B;

                            if (player->meleeWeaponAnimation <= PLAYER_MWA_FORWARD_COMBO_2H) {
                                this->stickTilt = 0.0f;
                            } else if (player->meleeWeaponAnimation <= PLAYER_MWA_RIGHT_COMBO_2H) {
                                this->stickTilt = 127.0f;
                                this->stickAngle += 0x4000;
                            } else if (player->meleeWeaponAnimation <= PLAYER_MWA_LEFT_COMBO_2H) {
                                this->stickTilt = 127.0f;
                                this->stickAngle -= 0x4000;
                            } else if (player->meleeWeaponAnimation <= PLAYER_MWA_BACKSLASH_LEFT) {
                                input->cur.button = BTN_R;
                            } else if (player->meleeWeaponAnimation <= PLAYER_MWA_BIG_SPIN_2H) {
                                EnTorch2_Backflip(this, input, &player1->actor);
                            } else {
                                EnTorch2_Backflip(this, input, &player1->actor);
                            }
                            if (!CHECK_BTN_ANY(input->cur.button, BTN_A | BTN_R) && (player1->meleeWeaponState == 0) &&
                                (player->meleeWeaponState != 0)) {
                                this->counterState = 1;
                            }
                        }
                    } else {

                        // Handles movement and attacks when not reacting to Link's actions

                        this->stickAngle = thisx->yawTowardsPlayer;
                        if ((90.0f >= player1->actor.xzDistToPlayer) && (player1->actor.xzDistToPlayer > 70.0f) &&
                            (ABS(sp5A) >= 0x7800) &&
                            (player1->actor.isLockedOn || !(player->stateFlags1 & PLAYER_STATE1_400000))) {
                            EnTorch2_SwingSword(play, input, this);
                        } else {
                            f32 sp50 = 0.0f;

                            if (((player1->actor.xzDistToPlayer <= 70.0f) ||
                                 ((player1->actor.xzDistToPlayer <= 80.0f + sp50) && (player->meleeWeaponState != 0))) &&
                                (player1->meleeWeaponState == 0)) {
                                if (!EnTorch2_SwingSword(play, input, this) && (player1->meleeWeaponState == 0) &&
                                    (this->counterState == 0)) {
                                    EnTorch2_Backflip(this, input, &player1->actor);
                                }
                            } else if (player1->actor.xzDistToPlayer <= 50 + sp50) {
                                this->stickTilt = 127.0f;
                                this->stickAngle = player1->actor.yawTowardsPlayer;
                                if (!player1->actor.isLockedOn) {
                                    Math_SmoothStepToS(&this->stickAngle, player->actor.shape.rot.y + 0x7FFF, 1, 0x2328, 0);
                                }
                            } else if (player1->actor.xzDistToPlayer > 100.0f + sp50) {
                                if ((player->meleeWeaponState == 0) ||
                                    !((player->meleeWeaponAnimation >= PLAYER_MWA_SPIN_ATTACK_1H) &&
                                      (player->meleeWeaponAnimation <= PLAYER_MWA_BIG_SPIN_2H)) ||
                                    (player1->actor.xzDistToPlayer >= 280.0f)) {
                                    this->stickTilt = 127.0f;
                                    this->stickAngle = player1->actor.yawTowardsPlayer;
                                    if (!player1->actor.isLockedOn) {
                                        Math_SmoothStepToS(&this->stickAngle, player->actor.shape.rot.y + 0x7FFF, 1, 0x2328,
                                                           0);
                                    }
                                } else {
                                    EnTorch2_Backflip(this, input, &player1->actor);
                                }
                            } else if (((ABS(sp5A) < 0x7800) && (ABS(sp5A) >= 0x3000)) ||
                                       !EnTorch2_SwingSword(play, input, this)) {
                                this->stickAngle = player1->actor.yawTowardsPlayer;
                                this->stickTilt = 127.0f;
                                if (!player1->actor.isLockedOn) {
                                    Math_SmoothStepToS(&this->stickAngle, player->actor.shape.rot.y + 0x7FFF, 1, 0x2328, 0);
                                }
                            }
                        }
                    }
                }

                // Handles Dark Link's counterattack to jumpslashes

            } else if (this->jumpslashFlag && (this->alpha == 255) && (player1->actor.velocity.y > 0)) {
                input->cur.button |= BTN_B;
            } else if (!this->jumpslashFlag && (player1->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
                player1->actor.world.rot.y = player1->actor.shape.rot.y = player1->actor.yawTowardsPlayer;
                this->stickAngle = player1->actor.yawTowardsPlayer;
                if (this->alpha != 255) {
                    this->stickAngle += 0x8000;
                    this->stickTilt = 127.0f;
                    this->zTargetFlag = true;
                }
                input->cur.button |= BTN_A;
                this->jumpslashFlag = true;
                player1->invincibilityTimer = 10;
            }

            // Rotates Dark Link's stick angle from Link-relative to camera-relative.

            sp66 = mainCam->camDir.y - this->stickAngle;
            this->input.cur.stick_x = this->stickTilt * Math_SinS(sp66);
            stickY = this->stickTilt * Math_CosS(sp66);
            if (this->alpha) {
            }
            this->input.cur.stick_y = stickY;

            if ((this->alpha != 255) && ((play->gameplayFrames % 8) == 0)) {
                this->alpha++;
            }
            break;

        case ENTORCH2_DAMAGE:
            player1->meleeWeaponState = 0;
            input->cur.stick_x = input->cur.stick_y = 0;
            // TODO: investigate why this condition needs to be commented out
            if (/* (player1->invincibilityTimer > 0) && */ (player1->actor.world.pos.y < (player1->actor.floorHeight - 160.0f))) {
                player1->stateFlags3 &= ~PLAYER_STATE3_1;
                player1->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED;
                player1->invincibilityTimer = 0;
                player1->actor.velocity.y = 0.0f;
                player1->actor.world.pos.y = this->spawnPoint.y + 40.0f;
                player1->actor.world.pos.x = (Math_SinS(player->actor.shape.rot.y) * -120.0f) + player->actor.world.pos.x;
                player1->actor.world.pos.z = (Math_CosS(player->actor.shape.rot.y) * -120.0f) + player->actor.world.pos.z;
                // #if OOT_VERSION < NTSC_1_2
                //                 if (Actor_WorldDistXYZToPoint(&player1->actor, &this->spawnPoint) > 1000.0f)
                // #else
                if (Actor_WorldDistXYZToPoint(&player1->actor, &this->spawnPoint) > 800.0f)
                // #endif
                {
                    f32 sp50 = Rand_ZeroOne() * 20.0f;
                    s16 sp4E = Rand_CenteredFloat(4000.0f);

                    player1->actor.shape.rot.y = player1->actor.world.rot.y =
                        Math_Vec3f_Yaw(&this->spawnPoint, &player->actor.world.pos);
                    player1->actor.world.pos.x =
                        (Math_SinS(player1->actor.world.rot.y + sp4E) * (25.0f + sp50)) + this->spawnPoint.x;
                    player1->actor.world.pos.z =
                        (Math_CosS(player1->actor.world.rot.y + sp4E) * (25.0f + sp50)) + this->spawnPoint.z;
                    player1->actor.world.pos.y = this->spawnPoint.y;
                } else {
                    player1->actor.world.pos.y = player1->actor.floorHeight;
                }
                // #if OOT_VERSION >= NTSC_1_2
                //                 Math_Vec3f_Copy(&player1->actor.home.pos, &player1->actor.world.pos);
                // #endif
                play->func_18780(player1, play);
                this->actionState = ENTORCH2_ATTACK;
                this->stickTilt = 0.0f;
                if (this->alpha != 255) {
                    this->staggerCount = 0;
                    this->staggerTimer = 0;
                }
            }
            break;

        case ENTORCH2_DEATH:
            if (this->alpha - 13 <= 0) {
                this->alpha = 0;
                Actor_Kill(&player1->actor);
                return;
            }
            this->alpha -= 13;
            player1->actor.shape.shadowAlpha -= 13;
            break;
    }

    // Causes Dark Link to shield in place when Link is using magic attacks other than the spin attack

    if ((gSaveContext.magicState == MAGIC_STATE_METER_FLASH_1) &&
        (player->meleeWeaponState == 0 || !((player->meleeWeaponAnimation >= PLAYER_MWA_SPIN_ATTACK_1H) &&
                                            (player->meleeWeaponAnimation <= PLAYER_MWA_BIG_SPIN_2H)))) {
        this->stickTilt = 0.0f;
        input->cur.stick_x = 0;
        input->cur.stick_y = 0;
        input->cur.button = BTN_R;
    }

    if ((this->actionState == ENTORCH2_ATTACK) && (player1->actor.xzDistToPlayer <= 610.0f) && this->zTargetFlag) {
        input->cur.button |= BTN_Z;
    }

    // Updates Dark Link's "controller". The conditional seems to cause him to
    // stop targeting and hold shield if he's been holding it long enough.

    pad54 = input->prev.button ^ input->cur.button;
    input->press.button = input->cur.button & pad54;
    if (CHECK_BTN_ANY(input->cur.button, BTN_R)) {
        input->cur.button = ((this->counterState == 0) && (player1->meleeWeaponState == 0)) ? BTN_R : input->cur.button ^ BTN_R;
    }
    input->rel.button = input->prev.button & pad54;
    input->prev.button = input->cur.button & (u16) ~(BTN_A | BTN_B);
    PadUtils_UpdateRelXY(input);

    input->press.stick_x += (s8)(input->cur.stick_x - input->prev.stick_x);
    input->press.stick_y += (s8)(input->cur.stick_y - input->prev.stick_y);

    // Handles Dark Link being damaged

    if ((player1->actor.colChkInfo.health == 0) && this->deathFlag) {
        player1->csAction = PLAYER_CSACTION_24;
        player1->csActor = &player->actor;
        player1->cv.haltActorsDuringCsAction = true;
        this->deathFlag = false;
    }
    if ((player1->invincibilityTimer == 0) && (player1->actor.colChkInfo.health != 0) &&
        (player1->cylinder.base.acFlags & AC_HIT) && !(player1->stateFlags1 & PLAYER_STATE1_4000000) &&
        !(player1->meleeWeaponQuads[0].base.atFlags & AT_HIT) && !(player1->meleeWeaponQuads[1].base.atFlags & AT_HIT)) {

        if (!Actor_ApplyDamage(&player1->actor)) {
            Audio_RestorePrevBgm();
            player1->actor.flags &= ~(ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE);
            // player1->knockbackType = PLAYER_KNOCKBACK_LARGE;
            // player1->knockbackSpeed = 6.0f;
            // player1->knockbackYVelocity = 6.0f;
            // player1->knockbackDamage = player1->actor.colChkInfo.damage;
            // player1->knockbackRot = player1->actor.yawTowardsPlayer + 0x8000;
            player1->unk_B75 = 2;
            player1->unk_B78 = 6.0f;
            player1->unk_B7C = 6.0f;
            player1->unk_B74 = player1->actor.colChkInfo.damage;
            player1->unk_B76 = player1->actor.yawTowardsPlayer + 0x8000;
            this->deathFlag++;
            this->actionState = ENTORCH2_DEATH;
            Enemy_StartFinishingBlow(play, &player1->actor);
            Item_DropCollectibleRandom(play, &player1->actor, &thisx->world.pos, 0xC0);
            player1->stateFlags3 &= ~PLAYER_STATE3_4;
        } else {
            Audio_PlayBgm_StorePrevBgm(NA_BGM_MINI_BOSS);
            if (player1->actor.colChkInfo.damageEffect == 1) {
                if (this->alpha == 255) {
                    Actor_SetColorFilter(&player1->actor, COLORFILTER_COLORFLAG_BLUE, 255, COLORFILTER_BUFFLAG_OPA, 80);
                } else {
                    Actor_SetColorFilter(&player1->actor, COLORFILTER_COLORFLAG_BLUE, 255, COLORFILTER_BUFFLAG_XLU, 80);
                }
            } else {
                player1->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
                player1->unk_B74 = player1->actor.colChkInfo.damage;
                player1->unk_B75 = 1;
                player1->unk_B7C = 6.0f;
                player1->unk_B78 = 8.0f;
                player1->unk_B76 = player1->actor.yawTowardsPlayer + 0x8000;
                // Actor_SetDropFlag(&player1->actor, &player1->cylinder.elem, true);
                Actor_SetDropFlag(&player1->actor, &player1->cylinder.elem);
                player1->stateFlags3 &= ~PLAYER_STATE3_4;
                player1->stateFlags3 |= PLAYER_STATE3_1;
                this->actionState = ENTORCH2_DAMAGE;
                if (this->alpha == 255) {
                    Actor_SetColorFilter(&player1->actor, COLORFILTER_COLORFLAG_RED, 255, COLORFILTER_BUFFLAG_OPA, 12);
                } else {
                    Actor_SetColorFilter(&player1->actor, COLORFILTER_COLORFLAG_RED, 255, COLORFILTER_BUFFLAG_XLU, 12);
                }
            }
        }
        player1->actor.colChkInfo.damage = 0;
        player1->unk_B74 = 0;
    }

    // Handles being frozen by a deku nut

    if ((player1->actor.colorFilterTimer == 0) || (player1->actor.colorFilterParams & 0x4000)) {
        player1->stateFlags3 &= ~PLAYER_STATE3_4;
    } else {
        player1->stateFlags3 |= PLAYER_STATE3_4;
        player1->stateFlags1 &= ~PLAYER_STATE1_4000000;
        player1->invincibilityTimer = 0;
        input->press.stick_x = input->press.stick_y = 0;
        /*! @bug
         *  Setting cur.button to 0 clears the Z-trigger, causing Dark Link to break his
         *  lock on Link. If he presses A while not locked on, he'll put his sword away.
         *  player1 clears his held item param permanently and makes him unable to attack.
         */
        input->cur.button = 0;
        input->press.button = 0;
        player1->speedXZ = 0.0f;
    }

    play->playerUpdate(player1, play, input);

    /*
     * Handles sword clanks and removes their recoil for both Links. Dark Link staggers
     * if he's had to counter with enough different sword animations in a row.
     */
    if (player1->speedXZ == -18.0f) {
        u8 staggerThreshold = (u32)Rand_CenteredFloat(2.0f) + 6;

        if (gSaveContext.save.saveInfo.playerData.health < 0x50) {
            staggerThreshold = (u32)Rand_CenteredFloat(2.0f) + 3;
        }
        if (player1->actor.xzDistToPlayer > 80.0f) {
            player1->speedXZ = 1.2f;
        } else if (player1->actor.xzDistToPlayer < 70.0f) {
            player1->speedXZ = -1.5f;
        } else {
            player1->speedXZ = 1.0f;
        }
        if (staggerThreshold < this->staggerCount) {
            player1->skelAnime.playSpeed *= 0.6f;
            // func_800F4190(&player1->actor.projectedPos, NA_SE_PL_DAMAGE);
            AudioSfx_PlaySfx(NA_SE_PL_DAMAGE, &player1->actor.projectedPos, 4, &D_801305B0, &gSfxDefaultFreqAndVolScale, &D_801305B4);
            this->staggerTimer = 0;
            this->staggerCount = 0;
        }
    }
    if (player->speedXZ == -18.0f) {
        if (player1->actor.xzDistToPlayer > 80.0f) {
            player->speedXZ = 1.2f;
        } else if (player1->actor.xzDistToPlayer < 70.0f) {
            player->speedXZ = -1.5f;
        } else {
            player->speedXZ = 1.0f;
        }
    }
    /*
     * player1 ensures Dark Link's counter animation mirrors Link's exactly.
     */
    if ((this->counterState != 0) && (this->counterState == 1)) {
        if (player1->meleeWeaponState == 0) {
            this->counterState = 0;
        } else {
            this->counterState = 2;
            player1->meleeWeaponState = 1;
            player1->skelAnime.curFrame = player->skelAnime.curFrame - player->skelAnime.playSpeed;
            player1->skelAnime.playSpeed = player->skelAnime.playSpeed;
            // LinkAnimation_Update(play, &player1->skelAnime);
            PlayerAnimation_Update(play, &player1->skelAnime);
            Collider_ResetQuadAT(play, &player1->meleeWeaponQuads[0].base);
            Collider_ResetQuadAT(play, &player1->meleeWeaponQuads[1].base);
        }
    }
    if (this->staggerTimer != 0) {
        this->staggerTimer--;
        if (this->staggerTimer == 0) {
            this->counterState = 0;
            this->staggerCount = 0;
        }
    }
    if (this->dodgeRollState != 0) {
        if (this->dodgeRollState == 1) {
            player1->invincibilityTimer = 20;
        }
        this->dodgeRollState = (player1->invincibilityTimer > 0) ? 2 : 0;
    }
    if (player1->invincibilityTimer != 0) {
        player1->cylinder.base.colMaterial = COL_MATERIAL_NONE;
        player1->cylinder.elem.elemMaterial = ELEM_MATERIAL_UNK5;
    } else {
        player1->cylinder.base.colMaterial = COL_MATERIAL_HIT5;
        player1->cylinder.elem.elemMaterial = ELEM_MATERIAL_UNK1;
    }
    /*
     * Handles the jump movement onto Link's sword. Dark Link doesn't move during the
     * sword jump. Instead, his shape y-offset is increased (see below). Once the sword
     * jump is finished, the offset is added to his position to fix the discrepancy.
     */
    if (this->swordJumpState != 0) {
        Math_SmoothStepToF(&this->swordJumpHeight, 2630.0f, 1.0f, 2000.0f, 0.0f);
        player1->actor.velocity.y -= 0.6f;
    } else if (this->swordJumpHeight != 0) {
        player1->actor.world.pos.y += this->swordJumpHeight * 0.01f;
        this->swordJumpHeight = 0;
    }
    if ((this->actionState == ENTORCH2_WAIT) || (player1->invincibilityTimer < 0)) {
        this->zTargetFlag = false;
    } else {
        this->zTargetFlag = true;
    }
    if (this->jumpslashTimer != 0) {
        this->jumpslashTimer--;
    }
    player1->actor.focus.pos = player1->actor.world.pos;
    player1->actor.focus.pos.y += 20.0f;
    player1->actor.shape.yOffset = this->swordJumpHeight;
}

s32 Player_OverrideLimbDrawGameplayCommon(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot,
                                          Actor* thisx);

s32 EnTorch2_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, Actor* thisx,
                              Gfx** gfx) {
    EnTorch2* this = (EnTorch2 *)thisx;

    return Player_OverrideLimbDrawGameplayCommon(play, limbIndex, dList, pos, rot, &this->player.actor);
}

extern Vec3f *sPlayerCurBodyPartPos;
extern Vec3f D_801C0994[];
extern Vec3f sPlayerGetItemRefPos;
extern AnimatedMaterial gameplay_keep_Matanimheader_054F18[];
extern Gfx gameplay_keep_DL_054C90[];
void Player_UpdateShieldCollider(PlayState *play, Player *player, ColliderQuad *collider, Vec3f quadSrc[4]);
void func_8012536C(void);

void EnTorch2_PostLimbDrawGameplay(PlayState *play, s32 limbIndex, Gfx **dList, Vec3s *rot, Actor *thisx, Gfx **gfx) {
    EnTorch2 *this = (EnTorch2 *)thisx;
    Player *player = &this->player;

    if (limbIndex == PLAYER_LIMB_LEFT_HAND) {
        Math_Vec3f_Copy(&player->leftHandWorld.pos, sPlayerCurBodyPartPos);

        if (player->actor.scale.y >= 0.0f) {
            Actor *heldActor;
            MtxF sp230;

            if (!Player_IsHoldingHookshot(player) && ((heldActor = player->heldActor) != NULL)) {
                if (player->stateFlags1 & PLAYER_STATE1_CARRYING_ACTOR) {
                    heldActor->world.rot.y = heldActor->shape.rot.y =
                        player->actor.shape.rot.y + player->leftHandWorld.rot.y;
                }
            } else {
                static f32 sMeleeWeaponLengths[PLAYER_MELEEWEAPON_MAX] = {
                    0.0f,    // PLAYER_MELEEWEAPON_NONE
                    3000.0f, // PLAYER_MELEEWEAPON_SWORD_KOKIRI
                    3000.0f, // PLAYER_MELEEWEAPON_SWORD_RAZOR
                    4000.0f, // PLAYER_MELEEWEAPON_SWORD_GILDED
                    5500.0f, // PLAYER_MELEEWEAPON_SWORD_TWO_HANDED
                    -1.0f,   // PLAYER_MELEEWEAPON_DEKU_STICK
                    2500.0f, // PLAYER_MELEEWEAPON_ZORA_BOOMERANG
                };

                if ((player->transformation == PLAYER_FORM_FIERCE_DEITY) ||
                    ((player->transformation != PLAYER_FORM_ZORA) &&
                     ((player->itemAction == PLAYER_IA_DEKU_STICK) ||
                      ((player->meleeWeaponState != PLAYER_MELEE_WEAPON_STATE_0) &&
                       (player->meleeWeaponAnimation != PLAYER_MWA_GORON_PUNCH_RIGHT) &&
                       (player->meleeWeaponAnimation != PLAYER_MWA_GORON_PUNCH_BUTT))))) {
                    if (player->itemAction == PLAYER_IA_DEKU_STICK) {
                        D_801C0994->x = player->unk_B0C * 5000.0f;
                    } else {
                        D_801C0994->x = sMeleeWeaponLengths[Player_GetMeleeWeaponHeld(player)];
                    }
                    func_80126B8C(play, player);
                }

                Matrix_Get(&player->leftHandMf);
                Matrix_MtxFToYXZRot(&player->leftHandMf, &player->leftHandWorld.rot, false);
            }
        }
    } else if (limbIndex == PLAYER_LIMB_RIGHT_HAND) {
        Actor *heldActor = player->heldActor;

        if (player->actor.scale.y >= 0.0f) {
            if (player->rightHandType == PLAYER_MODELTYPE_RH_FF) {
                Matrix_Get(&player->shieldMf);
            } else if (player->rightHandType == PLAYER_MODELTYPE_RH_SHIELD) {
                // Coordinates of the shield quad collider vertices, in the right hand limb's own model space.
                static Vec3f sRightHandLimbModelShieldQuadVertices[4] = {
                    {-4500.0f, -3000.0f, -600.0f},
                    {1500.0f, -3000.0f, -600.0f},
                    {-4500.0f, 3000.0f, -600.0f},
                    {1500.0f, 3000.0f, -600.0f},
                };

                Matrix_Get(&player->shieldMf);
                Player_UpdateShieldCollider(play, player, &player->shieldQuad, sRightHandLimbModelShieldQuadVertices);
            }

            if ((player->getItemDrawIdPlusOne != (GID_NONE + 1)) ||
                ((func_800B7118(player) == 0) && (heldActor != NULL))) {
                if (!(player->stateFlags1 & PLAYER_STATE1_400) && (player->getItemDrawIdPlusOne != (GID_NONE + 1)) &&
                    (player->exchangeItemAction != PLAYER_IA_NONE)) {
                    Math_Vec3f_Copy(&sPlayerGetItemRefPos, &player->leftHandWorld.pos);
                } else {
                    sPlayerGetItemRefPos.x =
                        (player->bodyPartsPos[PLAYER_BODYPART_RIGHT_HAND].x + player->leftHandWorld.pos.x) * 0.5f;
                    sPlayerGetItemRefPos.y =
                        (player->bodyPartsPos[PLAYER_BODYPART_RIGHT_HAND].y + player->leftHandWorld.pos.y) * 0.5f;
                    sPlayerGetItemRefPos.z =
                        (player->bodyPartsPos[PLAYER_BODYPART_RIGHT_HAND].z + player->leftHandWorld.pos.z) * 0.5f;
                }

                if (player->getItemDrawIdPlusOne == (GID_NONE + 1)) {
                    Math_Vec3f_Copy(&heldActor->world.pos, &sPlayerGetItemRefPos);
                }
            }
        }
    } else if (limbIndex == PLAYER_LIMB_LEFT_FOREARM) {
        // do nothing
    } else if (limbIndex == PLAYER_LIMB_RIGHT_FOREARM) {
        // do nothing
    } else if (limbIndex == PLAYER_LIMB_TORSO) {
        // do nothing
    } else if (limbIndex == PLAYER_LIMB_HEAD) {
        if ((player->stateFlags1 & (PLAYER_STATE1_2 | PLAYER_STATE1_100)) && (player->av2.actionVar2 != 0)) {
            static Vec3f D_801C0E40[PLAYER_FORM_MAX] = {
                {0.0f, 0.0f, 0.0f},        // PLAYER_FORM_FIERCE_DEITY
                {-578.3f, -1100.9f, 0.0f}, // PLAYER_FORM_GORON
                {-189.5f, -594.87f, 0.0f}, // PLAYER_FORM_ZORA
                {-570.0f, -812.0f, 0.0f},  // PLAYER_FORM_DEKU
                {-230.0f, -520.0f, 0.0f},  // PLAYER_FORM_HUMAN
            };
            Vec3f *temp_s0_7 = &D_801C0E40[player->transformation];

            OPEN_DISPS(play->state.gfxCtx);

            Matrix_Push();
            AnimatedMat_DrawXlu(play, Lib_SegmentedToVirtual(gameplay_keep_Matanimheader_054F18));
            Matrix_Translate(temp_s0_7->x, temp_s0_7->y, 0.0f, MTXMODE_APPLY);
            if (player->transformation == PLAYER_FORM_ZORA) {
                Matrix_Scale(0.7f, 0.7f, 0.7f, MTXMODE_APPLY);
            }

            MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
            gDPSetEnvColor(POLY_XLU_DISP++, 0, 0, 255, (u8)player->av2.actionVar2);
            gSPDisplayList(POLY_XLU_DISP++, gameplay_keep_DL_054C90);

            Matrix_Pop();

            CLOSE_DISPS(play->state.gfxCtx);
        }
        if (player->actor.scale.y >= 0.0f) {
            static Vec3f sPlayerFocusHeadLimbModelPos = {1100.0f, -700.0f, 0.0f};

            Matrix_MultVec3f(&sPlayerFocusHeadLimbModelPos, &player->actor.focus.pos);
        }
    } else if ((limbIndex == PLAYER_LIMB_HAT) && (player->stateFlags3 & PLAYER_STATE3_100000)) {
        Vec3f sp5C;
        Vec3f sp50;

        Matrix_MultVecX(3000.0f, &sp5C);
        Matrix_MultVecX(2300.0f, &sp50);
        if (func_80126440(play, NULL, player->meleeWeaponInfo, &sp5C, &sp50)) {
            EffectBlure_AddVertex(Effect_GetByIndex(player->meleeWeaponEffectIndex[0]), &player->meleeWeaponInfo[0].tip,
                                  &player->meleeWeaponInfo[0].base);
        }
    } else if (limbIndex == PLAYER_LIMB_RIGHT_SHIN) {
        // do nothing
    } else if (limbIndex == PLAYER_LIMB_WAIST) {
        // do nothing
    } else if (limbIndex == PLAYER_LIMB_SHEATH) {
        if (player->actor.scale.y >= 0.0f) {
            if ((player->rightHandType != PLAYER_MODELTYPE_RH_SHIELD) &&
                (player->rightHandType != PLAYER_MODELTYPE_RH_FF)) {
                static Vec3f sSheathLimbModelShieldOnBackPos = {630.0f, 100.0f, -30.0f};
                static Vec3s sSheathLimbModelShieldOnBackZyxRot = {0, 0, 0x7FFF};

                Matrix_TranslateRotateZYX(&sSheathLimbModelShieldOnBackPos, &sSheathLimbModelShieldOnBackZyxRot);
                Matrix_Get(&player->shieldMf);
            }
        }
    } else if (player->actor.scale.y >= 0.0f) {
        Player_SetFeetPos(play, player, limbIndex);
    }

    func_8012536C();
}

Gfx D_80116280[] = {
    gsDPSetRenderMode(G_RM_FOG_SHADE_A, AA_EN | Z_CMP | Z_UPD | IM_RD | CLR_ON_CVG | CVG_DST_WRAP | ZMODE_XLU |
                                            FORCE_BL | GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)),
    gsDPSetAlphaCompare(G_AC_THRESHOLD),
    gsSPEndDisplayList(),
};

Hilite* Hilite_DrawOpa(Vec3f* object, Vec3f* eye, Vec3f* lightDir, GraphicsContext* gfxCtx);
Hilite* Hilite_DrawXlu(Vec3f* object, Vec3f* eye, Vec3f* lightDir, GraphicsContext* gfxCtx);

void func_8002EBCC(Actor* actor, PlayState* play, s32 flag) {
    Hilite* hilite;
    Vec3f lightDir;
    Gfx* displayListHead;
    Gfx* displayList;

    lightDir.x = play->envCtx.dirLight1.params.dir.x;
    lightDir.y = play->envCtx.dirLight1.params.dir.y;
    lightDir.z = play->envCtx.dirLight1.params.dir.z;

    hilite = Hilite_DrawOpa(&actor->world.pos, &play->view.eye, &lightDir, play->state.gfxCtx);

    if (flag != 0) {
        displayList = GRAPH_ALLOC(play->state.gfxCtx, 2 * sizeof(Gfx));
        displayListHead = displayList;

        OPEN_DISPS(play->state.gfxCtx);

        gDPSetHilite1Tile(displayListHead++, 1, hilite, 16, 16);
        gSPEndDisplayList(displayListHead);
        gSPSegment(POLY_OPA_DISP++, 0x07, displayList);

        CLOSE_DISPS(play->state.gfxCtx);
    }
}

void func_8002ED80(Actor* actor, PlayState* play, s32 flag) {
    Hilite* hilite;
    Vec3f lightDir;
    Gfx* displayListHead;
    Gfx* displayList;

    lightDir.x = play->envCtx.dirLight1.params.dir.x;
    lightDir.y = play->envCtx.dirLight1.params.dir.y;
    lightDir.z = play->envCtx.dirLight1.params.dir.z;

    hilite = Hilite_DrawXlu(&actor->world.pos, &play->view.eye, &lightDir, play->state.gfxCtx);

    if (flag != 0) {
        displayList = GRAPH_ALLOC(play->state.gfxCtx, 2 * sizeof(Gfx));
        displayListHead = displayList;

        OPEN_DISPS(play->state.gfxCtx);

        gDPSetHilite1Tile(displayListHead++, 1, hilite, 16, 16);
        gSPEndDisplayList(displayListHead);
        gSPSegment(POLY_XLU_DISP++, 0x07, displayList);

        CLOSE_DISPS(play->state.gfxCtx);
    }
}

void EnTorch2_Draw(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    EnTorch2* this = (EnTorch2 *)thisx;
    Player *player = &this->player;
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);
    // func_80093C80(play);

    gSPSegment(POLY_OPA_DISP++, 0x0C, gCullBackDList);
    gSPSegment(POLY_XLU_DISP++, 0x0C, gCullBackDList);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    // apparently whatever's below here goes unused?
    if (play->roomCtx.curRoom.type == ROOM_TYPE_3) {
        OPEN_DISPS(play->state.gfxCtx);
        gDPSetColorDither(POLY_OPA_DISP++, G_CD_DISABLE);
        CLOSE_DISPS(play->state.gfxCtx);
    }

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    if (this->alpha == 255) {
        gDPSetEnvColor(POLY_OPA_DISP++, 255, 0, 0, this->alpha);
        gSPSegment(POLY_OPA_DISP++, 0x0C, D_80116280 + 2);
        func_8002EBCC(&player->actor, play, 0);
        func_8002ED80(&player->actor, play, 0);
        POLY_OPA_DISP =
            // SkelAnime_DrawFlex(play, player->skelAnime.skeleton, player->skelAnime.jointTable, player->skelAnime.dListCount,
            //                    EnTorch2_OverrideLimbDraw, EnTorch2_PostLimbDraw, &player->actor, POLY_OPA_DISP);
            SkelAnime_DrawFlex(play, player->skelAnime.skeleton, player->skelAnime.jointTable, player->skelAnime.dListCount,
                               EnTorch2_OverrideLimbDraw, EnTorch2_PostLimbDrawGameplay, &player->actor, POLY_OPA_DISP);
    } else {
        gDPSetEnvColor(POLY_XLU_DISP++, 255, 0, 0, this->alpha);
        gSPSegment(POLY_XLU_DISP++, 0x0C, D_80116280);
        func_8002EBCC(&player->actor, play, 0);
        func_8002ED80(&player->actor, play, 0);
        POLY_XLU_DISP =
            // SkelAnime_DrawFlex(play, player->skelAnime.skeleton, player->skelAnime.jointTable, player->skelAnime.dListCount,
            //                    EnTorch2_OverrideLimbDraw, EnTorch2_PostLimbDraw, &player->actor, POLY_XLU_DISP);
            SkelAnime_DrawFlex(play, player->skelAnime.skeleton, player->skelAnime.jointTable, player->skelAnime.dListCount,
                               EnTorch2_OverrideLimbDraw, EnTorch2_PostLimbDrawGameplay, &player->actor, POLY_XLU_DISP);
    }
    CLOSE_DISPS(play->state.gfxCtx);
}

static Player *sPlayerFromGetIdleAnim;

RECOMP_HOOK("Player_GetIdleAnim")
void fixDinkIdle_on_Player_GetIdleAnim(Player *this) {
    if (this->actor.id == CUSTOM_ACTOR_EN_DARKLINK) {
        sPlayerFromGetIdleAnim = this;
        this->actor.id = ACTOR_PLAYER;
    } else {
        sPlayerFromGetIdleAnim = NULL;
    }
}

RECOMP_HOOK_RETURN("Player_GetIdleAnim")
void fixDarkLinkIdle_on_return_Player_GetIdleAnim(void) {
    if (sPlayerFromGetIdleAnim) {
        sPlayerFromGetIdleAnim->actor.id = CUSTOM_ACTOR_EN_DARKLINK;
    }
}

static Actor *sFocusActorFromUpdateHostileLockOn;
static u32 sStoredActorFlags;

RECOMP_HOOK("Player_UpdateHostileLockOn")
void fixDarkLinkFightingAnims_on_Player_UpdateHostileLockOn(Player *this) {
    if (this->actor.id == CUSTOM_ACTOR_EN_DARKLINK && this->focusActor && this->focusActor->id == ACTOR_PLAYER) {
        sFocusActorFromUpdateHostileLockOn = this->focusActor;
        sStoredActorFlags = this->focusActor->flags;
        this->focusActor->flags |= (ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE);
    } else {
        sFocusActorFromUpdateHostileLockOn = NULL;
    }
}

RECOMP_HOOK_RETURN("Player_UpdateHostileLockOn")
void fixDarkLinkFightingAnims_on_return_Player_UpdateHostileLockOn(void) {
    if (sFocusActorFromUpdateHostileLockOn) {
        sFocusActorFromUpdateHostileLockOn->flags = sStoredActorFlags;
    }
}

static Player *sPlayerfunc_808339D4;

RECOMP_HOOK("func_808339D4")
void fixDarkLinkDmg_on_func_808339D4(PlayState *play, Player *this, s32 damage) {
    if (this->actor.id == CUSTOM_ACTOR_EN_DARKLINK) {
        sPlayerfunc_808339D4 = this;
        this->actor.id = ACTOR_PLAYER;
    } else {
        sPlayerfunc_808339D4 = NULL;
    }
}

RECOMP_HOOK_RETURN("func_808339D4")
void fixDarkLinkDmg_on_return_func_808339D4(void) {
    if (sPlayerfunc_808339D4) {
        sPlayerfunc_808339D4->actor.id = CUSTOM_ACTOR_EN_DARKLINK;
    }
}

static Player *sPlayerForPlayerResetCylinder;

RECOMP_HOOK("Player_ResetCylinder")
void fixDarkLinkCylinder_on_Player_ResetCylinder(Player *this) {
    sPlayerForPlayerResetCylinder = this;
}

RECOMP_HOOK_RETURN("Player_ResetCylinder")
void fixDarkLinkCylinder_on_return_Player_ResetCylinder(void) {
    if (sPlayerForPlayerResetCylinder->actor.id == CUSTOM_ACTOR_EN_DARKLINK) {
        sPlayerForPlayerResetCylinder->cylinder.base.acFlags = AC_ON | AC_TYPE_PLAYER;
    }
}
