#include "better_aiming.h"
#include "foxhollow_compat.h"
#include "dolphin/pad.h"
#include "game/objects/object.h"
#include "main/camera_object.h"
#include "main/dll/dll_0044_cameramodeviewfinder.h"
#include "main/dll/dll_0052_cameramodeforcebehind.h"
#include "main/dll/ARW/dll_029A_arwarwing.h"
#include "main/dll/player_state.h"
#include "main/gamebit_ids.h"

#define BETTER_HOOK_CAPACITY 13

typedef int (*PlayerStateAimStaffFn)(GameObject*, PlayerState*, f32);
typedef void (*CameraModeForceBehindUpdateFn)(CameraObject*);
typedef void (*FirstPersonDoControlsFn)(CameraObject*);
typedef void (*IceblastUpdateFn)(GameObject*);
typedef u32 (*GetScreenResolutionFn)(void);
typedef s8 (*PadGetStickFn)(int);
typedef int (*GetYButtonItemFn)(s16*);
typedef void (*PlayerProcessQueuedItemCommandFn)(GameObject*, PlayerState*);
typedef void (*PlayerCastSpellFn)(GameObject*, PlayerState*, int);
typedef int (*PlayerCanCastBlasterSpellFn)(GameObject*, PlayerState*, int);
typedef GameObject* (*ObjGetPlayerObjectFn)(void);
typedef f32 (*MathTrigFn)(f32);
typedef void (*ObjFreeObjectFn)(GameObject*);
typedef int (*ResourceReleaseFn)(void*);
typedef void (*ObjSetXRotFn)(GameObject*, int);
typedef f32 (*CameraGetFovYFn)(void);
typedef f32 (*InterpolateFn)(f32, f32, f32);
typedef f32 (*AtanfFn)(f32);
typedef f32 (*MathTanfFn)(f32);
typedef f32* (*CameraGetWorldMatrixFn)(void);
typedef void (*MatrixTransformVectorFn)(const f32*, const f32*, f32*);
typedef void (*GameUIHudDrawFn)(int, int, int);
typedef int (*MinimapUpdateFn)(void);
typedef void (*CameraApplyFullViewportFn)(void);
typedef void (*ArwingReadControlsFn)(GameObject*, ArwingState*);
typedef void (*CloudRunnerUpdateFn)(GameObject*);

static FhMod* gMod;
static const FhModHost* gHost;
static PlayerStateAimStaffFn gPlayerStateAimStaff;
static CameraModeForceBehindUpdateFn gCameraModeForceBehindUpdate;
static FirstPersonDoControlsFn gFirstPersonDoControls;
static IceblastUpdateFn gIceblastUpdate;
static GetScreenResolutionFn gGetScreenResolution;
static PadGetStickFn gPadGetStickX;
static PadGetStickFn gPadGetStickY;
static PadGetStickFn gPadGetCX;
static PadGetStickFn gPadGetCY;
static GetYButtonItemFn gGetYButtonItem;
static PlayerProcessQueuedItemCommandFn gPlayerProcessQueuedItemCommand;
static PlayerCastSpellFn gPlayerCastSpell;
static PlayerCanCastBlasterSpellFn gPlayerCanCastBlasterSpell;
static ObjGetPlayerObjectFn gObjGetPlayerObject;
static ObjSetXRotFn gObjSetXRot;
static CameraGetFovYFn gCameraGetFovY;
static InterpolateFn gInterpolate;
static AtanfFn gAtanf;
static MathTanfFn gMathTanf;
static MathTrigFn gMathSinf;
static MathTrigFn gMathCosf;
static CameraGetWorldMatrixFn gCameraGetWorldMatrix;
static MatrixTransformVectorFn gMatrixTransformVector;
static GameUIHudDrawFn gGameUIHudDraw;
static MinimapUpdateFn gMinimapUpdate;
static CameraApplyFullViewportFn gCameraApplyFullViewport;
static ArwingReadControlsFn gArwingReadControls;
static CloudRunnerUpdateFn gCloudRunnerUpdate;
static f32* gTimeDelta;
static CameraModeViewfinderState** gViewfinderState;
static s16* gPlayerSelectedItem;
static u8* gPlayerIceSpellSustaining;
static GameObject** gPlayerSpawnedObjects;
static void*** gPlayerResourceAddress;
static ObjFreeObjectFn gObjFreeObject;
static ResourceReleaseFn gResourceRelease;
static void* gInstalledHookTargets[BETTER_HOOK_CAPACITY];
static int gInstalledHookCount;

typedef enum BetterAimMode {
    BETTER_AIM_MODE_ORIGINAL = 0,
    BETTER_AIM_MODE_FREE = 1,
    BETTER_AIM_MODE_CENTERED = 2
} BetterAimMode;

static BetterAimMode gFireAimMode = BETTER_AIM_MODE_FREE;
static BetterAimMode gFreezeAimMode = BETTER_AIM_MODE_ORIGINAL;
static int gModernBinocularControls = 1;
static int gInvertAimX;
static int gInvertAimY = 1;
static int gInvertBinocularX;
static int gInvertBinocularY = 1;
static int gFreezeBlastCrosshair;
static int gDirectSpellSwitching = 1;
static int gAimWhileMoving;
static int gBlasterHudFix = 1;
static int gInvertFlightX;
static int gInvertFlightY;
static int gFlightInputContext;
static f32 gAimSpeed = 1.0f;
static f32 gAimVerticalRangeDegrees = 135.0f;
static f32 gBinocularSpeed = 1.0f;
static f32 gBinocularInertia = 1.0f;
static f32 gBinocularVerticalRangeDegrees = 180.0f;
static f32 gFlightSensitivity = 1.0f;
static f32 gBinocularPitchSpeed;
static CameraObject* gLastBinocularCamera;
static uint64_t gLastBinocularFrame;

static f32 clampf_local(f32 v, f32 lo, f32 hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static f32 absf_local(f32 v) {
    return v < 0.0f ? -v : v;
}

static int string_equal(const char* a, const char* b) {
    if (a == 0 || b == 0) {
        return 0;
    }
    while (*a != 0 && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static s32 round_to_s32(f32 v) {
    return v >= 0.0f ? (s32)(v + 0.5f) : (s32)(v - 0.5f);
}

static f32 normalized_input(f32 value) {
    return clampf_local(value / 56.0f, -1.0f, 1.0f);
}

static f32 interpolate_toward(f32 current, f32 target, f32 rate, f32 dt) {
    rate = clampf_local(rate, 0.0f, 1.0f);
    if (gInterpolate != 0) {
        return current + gInterpolate(target - current, rate, dt);
    }
    return current + (target - current) * clampf_local(rate * dt, 0.0f, 1.0f);
}

static s16 angle_add_scaled(s16 before, s16 after, f32 scale) {
    s16 delta = (s16)(after - before);
    return (s16)(before + (s16)round_to_s32((f32)delta * scale));
}

static int is_original_mode_value(const char* value) {
    return value != 0 && (string_equal(value, "original") || string_equal(value, "vanilla"));
}

static BetterAimMode parse_aim_mode(const char* value) {
    if (is_original_mode_value(value)) {
        return BETTER_AIM_MODE_ORIGINAL;
    }
    if (value != 0 && string_equal(value, "centered")) {
        return BETTER_AIM_MODE_CENTERED;
    }
    return BETTER_AIM_MODE_FREE;
}

static void load_config(void) {
    gFireAimMode = parse_aim_mode(gHost->configString(gMod, "fireAimMode", "free"));
    gFreezeAimMode = parse_aim_mode(gHost->configString(gMod, "freezeAimMode", "original"));
    const char* binocularMode = gHost->configString(gMod, "binocularMode", "modern");
    gModernBinocularControls = !is_original_mode_value(binocularMode);
    gInvertAimX = gHost->configBool(gMod, "invertAimX", 0);
    gInvertAimY = gHost->configBool(gMod, "invertAimY", 1);
    gInvertBinocularX = gHost->configBool(gMod, "invertBinocularX", 0);
    gInvertBinocularY = gHost->configBool(gMod, "invertBinocularY", 1);
    gFreezeBlastCrosshair = gHost->configBool(gMod, "freezeBlastCrosshair", 0);
    gDirectSpellSwitching = gHost->configBool(gMod, "directSpellSwitching", 1);
    gAimWhileMoving = gHost->configBool(gMod, "aimWhileMoving", 0);
    gBlasterHudFix = gHost->configBool(gMod, "blasterHudFix", 1);
    gInvertFlightX = gHost->configBool(gMod, "invertFlightX", 0);
    gInvertFlightY = gHost->configBool(gMod, "invertFlightY", 0);
    gAimSpeed = clampf_local(gHost->configFloat(gMod, "aimSpeed", 1.0f), 0.01f, 5.0f);
    gAimVerticalRangeDegrees = clampf_local(gHost->configFloat(gMod, "aimVerticalRangeDegrees", 135.0f), 0.0f, 180.0f);
    gBinocularSpeed = clampf_local(gHost->configFloat(gMod, "binocularSpeed", 1.0f), 0.01f, 5.0f);
    gBinocularInertia = clampf_local(gHost->configFloat(gMod, "binocularInertia", 1.0f), 0.0f, 10.0f);
    gBinocularVerticalRangeDegrees =
        clampf_local(gHost->configFloat(gMod, "binocularVerticalRangeDegrees", 180.0f), 0.0f, 180.0f);
    gFlightSensitivity = clampf_local(gHost->configFloat(gMod, "flightSensitivity", 1.0f), 0.1f, 2.0f);
}

static f32 aim_pitch_for_input(PlayerState* inner, f32 a) {
    f32 pitchScale = inner->focusObject != 0 ? 8192.0f : 14336.0f;
    f32 base = 0.5f * pitchScale * a;
    if (gCameraGetFovY == 0 || gAtanf == 0 || gMathTanf == 0) {
        return base;
    }
    f32 halfFov = gCameraGetFovY() * 0.008726646259971648f;
    f32 normalizedY = (a < 0.0f ? 0.5f : 0.25f) * a;
    f32 ray = gAtanf(normalizedY * gMathTanf(halfFov));
    return base + ray * 10430.378350470453f;
}

typedef struct BetterAimVerticalLimits {
    f32 negative;
    f32 positive;
} BetterAimVerticalLimits;

static f32 aim_pitch_degrees(PlayerState* inner, f32 a) {
    return aim_pitch_for_input(inner, a) * 0.0054931640625f;
}

static f32 solve_aim_input_for_pitch(PlayerState* inner, f32 targetDegrees, int negative) {
    if (targetDegrees <= 0.0f) {
        return 0.0f;
    }

    f32 lo = 0.0f;
    f32 hi = 1.0f;
    for (;;) {
        f32 degrees = negative ? -aim_pitch_degrees(inner, -hi) : aim_pitch_degrees(inner, hi);
        if (degrees >= targetDegrees || hi >= 8.0f) {
            break;
        }
        hi *= 2.0f;
    }
    if (hi > 8.0f) {
        hi = 8.0f;
    }

    for (int i = 0; i < 18; i++) {
        f32 mid = 0.5f * (lo + hi);
        f32 degrees = negative ? -aim_pitch_degrees(inner, -mid) : aim_pitch_degrees(inner, mid);
        if (degrees < targetDegrees) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    return 0.5f * (lo + hi);
}

static BetterAimVerticalLimits aim_vertical_limits(PlayerState* inner) {
    BetterAimVerticalLimits limits = {0.0f, 0.0f};
    f32 requestedRange = gAimVerticalRangeDegrees;
    if (requestedRange <= 0.0f) {
        return limits;
    }

    f32 originalNegative = -aim_pitch_degrees(inner, -1.0f);
    f32 originalPositive = aim_pitch_degrees(inner, 1.0f);
    f32 originalRange = originalNegative + originalPositive;
    if (originalRange <= 0.0001f) {
        return limits;
    }

    f32 targetRange = requestedRange * (originalRange / 103.0f);
    f32 negativeDegrees = targetRange * (originalNegative / originalRange);
    f32 positiveDegrees = targetRange - negativeDegrees;
    const f32 maxSideDegrees = 89.9945f;

    if (negativeDegrees > maxSideDegrees) {
        positiveDegrees += negativeDegrees - maxSideDegrees;
        negativeDegrees = maxSideDegrees;
    }
    if (positiveDegrees > maxSideDegrees) {
        negativeDegrees += positiveDegrees - maxSideDegrees;
        positiveDegrees = maxSideDegrees;
    }
    negativeDegrees = clampf_local(negativeDegrees, 0.0f, maxSideDegrees);
    positiveDegrees = clampf_local(positiveDegrees, 0.0f, maxSideDegrees);

    limits.negative = solve_aim_input_for_pitch(inner, negativeDegrees, 1);
    limits.positive = solve_aim_input_for_pitch(inner, positiveDegrees, 0);
    return limits;
}

static void set_aim_screen(PlayerState* inner, BetterAimMode mode, int forceVisible) {
    if (gGetScreenResolution == 0) {
        return;
    }
    if ((inner->flags360 & PLAYER_FLAG_AIM_READY) == 0 && !forceVisible) {
        return;
    }
    if (forceVisible) {
        inner->flags360 |= PLAYER_FLAG_AIM_READY;
    }
    u32 res = gGetScreenResolution();
    f32 halfH = (f32)(res >> 17);
    f32 halfW = (f32)((u16)res >> 1);
    if (mode == BETTER_AIM_MODE_CENTERED) {
        inner->aimScreenX = halfW;
        inner->aimScreenY = halfH;
        return;
    }
    inner->aimScreenX = 0.5f * inner->aimInputX * halfW + halfW;
    if (inner->aimInputZ < 0.0f) {
        inner->aimScreenY = 0.5f * inner->aimInputZ * halfH + halfH;
    } else {
        inner->aimScreenY = 0.25f * inner->aimInputZ * halfH + halfH;
    }
}

static int is_blaster_spell(s16 item) {
    return item == GAMEBIT_STAFF_ABILITY_FIRE_BLASTER || item == GAMEBIT_STAFF_ABILITY_FREEZE_BLAST;
}

static BetterAimMode current_staff_aim_mode(void) {
    if (gPlayerSelectedItem == 0) {
        return BETTER_AIM_MODE_ORIGINAL;
    }
    if (*gPlayerSelectedItem == GAMEBIT_STAFF_ABILITY_FIRE_BLASTER) {
        return gFireAimMode;
    }
    if (*gPlayerSelectedItem == GAMEBIT_STAFF_ABILITY_FREEZE_BLAST) {
        return gFreezeAimMode;
    }
    return BETTER_AIM_MODE_ORIGINAL;
}

static int freeze_crosshair_requested(void) {
    return gFreezeAimMode != BETTER_AIM_MODE_ORIGINAL && gFreezeBlastCrosshair &&
           gPlayerSelectedItem != 0 && *gPlayerSelectedItem == GAMEBIT_STAFF_ABILITY_FREEZE_BLAST;
}

static void end_freeze_blast_sustain(void) {
    if (gPlayerIceSpellSustaining == 0 || *gPlayerIceSpellSustaining == 0) {
        return;
    }

    *gPlayerIceSpellSustaining = 0;
    if (gPlayerSpawnedObjects != 0 && gObjFreeObject != 0) {
        for (int i = 0; i < 7; i++) {
            if (gPlayerSpawnedObjects[i] != 0) {
                gObjFreeObject(gPlayerSpawnedObjects[i]);
                gPlayerSpawnedObjects[i] = 0;
            }
        }
    }
    if (gPlayerResourceAddress != 0 && *gPlayerResourceAddress != 0 && gResourceRelease != 0) {
        gResourceRelease(*gPlayerResourceAddress);
        *gPlayerResourceAddress = 0;
    }
}

static void prepare_direct_spell_switch(PlayerState* inner) {
    if (!gDirectSpellSwitching || gGetYButtonItem == 0 || gPlayerSelectedItem == 0 ||
        gPlayerIceSpellSustaining == 0 || *gPlayerIceSpellSustaining == 0 ||
        (inner->buttonsJustPressed & PAD_BUTTON_Y) == 0) {
        return;
    }

    s16 item = -1;
    if (gGetYButtonItem(&item) != 1 || !is_blaster_spell(item) || item == *gPlayerSelectedItem) {
        return;
    }

    end_freeze_blast_sustain();
}

static int is_staff_aim_control_mode(s16 mode) {
    return mode >= 0x2a && mode <= 0x2f;
}

static void better_player_process_queued_item_command(GameObject* obj, PlayerState* state) {
    if (gDirectSpellSwitching && obj != 0 && state != 0 && gPlayerSelectedItem != 0 &&
        gPlayerCastSpell != 0 && gPlayerCanCastBlasterSpell != 0 &&
        is_staff_aim_control_mode(state->baddie.controlMode)) {
        s16 item = state->queuedItemCommand;
        if (is_blaster_spell(item) && item != *gPlayerSelectedItem &&
            gPlayerCanCastBlasterSpell(obj, state, item) != 0) {
            end_freeze_blast_sustain();
            gPlayerCastSpell(obj, state, item);
            state->queuedItemCommand = -1;
            state->deferredItemCommand = -1;
            return;
        }
    }
    gPlayerProcessQueuedItemCommand(obj, state);
}

static int is_staff_aiming_now(void) {
    if (gObjGetPlayerObject == 0) {
        return 0;
    }
    GameObject* player = gObjGetPlayerObject();
    if (player == 0 || player->extra == 0) {
        return 0;
    }
    PlayerState* state = (PlayerState*)player->extra;
    return is_staff_aim_control_mode(state->baddie.controlMode);
}

static int is_blaster_active_now(void) {
    if (gObjGetPlayerObject == 0) {
        return 0;
    }
    GameObject* player = gObjGetPlayerObject();
    if (player == 0 || player->extra == 0) {
        return 0;
    }
    PlayerState* state = (PlayerState*)player->extra;
    if (!is_staff_aim_control_mode(state->baddie.controlMode)) {
        return 0;
    }
    return is_blaster_spell(state->animState);
}

static int should_capture_cstick(void) {
    return gAimWhileMoving && is_staff_aiming_now() && current_staff_aim_mode() != BETTER_AIM_MODE_ORIGINAL;
}

enum {
    FLIGHT_INPUT_NONE = 0,
    FLIGHT_INPUT_ARWING = 1,
    FLIGHT_INPUT_CLOUDRUNNER = 2
};

static s8 apply_flight_input(s8 value, int inverted) {
    f32 limit = gFlightInputContext == FLIGHT_INPUT_ARWING ? 72.0f : 70.0f;
    f32 scaled = (f32)value * gFlightSensitivity;
    if (inverted) {
        scaled = -scaled;
    }
    scaled = clampf_local(scaled, -limit, limit);
    return (s8)round_to_s32(scaled);
}

static s8 better_pad_get_stick_x(int port) {
    s8 value = gPadGetStickX(port);
    if (gFlightInputContext == FLIGHT_INPUT_NONE) {
        return value;
    }
    return apply_flight_input(value, gInvertFlightX);
}

static s8 better_pad_get_stick_y(int port) {
    s8 value = gPadGetStickY(port);
    if (gFlightInputContext == FLIGHT_INPUT_NONE) {
        return value;
    }
    return apply_flight_input(value, gInvertFlightY);
}

static void better_arwing_read_controls(GameObject* obj, ArwingState* state) {
    int previousContext = gFlightInputContext;
    gFlightInputContext = FLIGHT_INPUT_ARWING;
    gArwingReadControls(obj, state);
    gFlightInputContext = previousContext;
}

static void better_cloudrunner_update(GameObject* obj) {
    int previousContext = gFlightInputContext;
    gFlightInputContext = FLIGHT_INPUT_CLOUDRUNNER;
    gCloudRunnerUpdate(obj);
    gFlightInputContext = previousContext;
}

static s8 better_pad_get_cx(int port) {
    if (should_capture_cstick()) {
        return 0;
    }
    return gPadGetCX(port);
}

static s8 better_pad_get_cy(int port) {
    if (should_capture_cstick()) {
        return 0;
    }
    return gPadGetCY(port);
}

static void apply_aim_movement(GameObject* obj, PlayerState* inner, f32 savedVelocityY) {
    if (!gAimWhileMoving || gMathSinf == 0 || gMathCosf == 0) {
        return;
    }

    f32 magnitude = clampf_local(inner->baddie.inputMagnitude, 0.0f, 1.0f);
    f32 normalized = clampf_local((magnitude - 0.2f) / 0.8f, 0.0f, 1.0f);
    const f32 maxSpeed = 2.3993998f;
    f32 speed = (maxSpeed - 0.05f) * normalized * inner->speedScale;
    f32 heading = 3.1415927f * (f32)inner->inputHeading / 32768.0f;
    f32 desiredX = speed * -gMathSinf(heading);
    f32 desiredZ = speed * -gMathCosf(heading);
    f32 yaw = 3.1415927f * (f32)inner->targetYaw / 32768.0f;
    f32 sinYaw = gMathSinf(yaw);
    f32 cosYaw = gMathCosf(yaw);

    inner->maxSpeed = maxSpeed;
    inner->baddie.animSpeedA = -desiredZ * cosYaw - desiredX * sinYaw;
    inner->baddie.animSpeedB = desiredX * cosYaw - desiredZ * sinYaw;
    inner->baddie.animSpeedC = speed;
    obj->anim.velocityY = savedVelocityY;
}

static int better_player_state_aim_staff(GameObject* obj, PlayerState* state, f32 fv) {
    PlayerState* inner = (PlayerState*)obj->extra;
    f32 savedMoveX = state->baddie.moveInputX;
    f32 savedMoveZ = state->baddie.moveInputZ;
    f32 savedVelocityY = obj->anim.velocityY;
    f32 dt = gTimeDelta != 0 ? *gTimeDelta : 1.0f;
    int activeMove = obj->anim.currentMove == 0x43e;

    prepare_direct_spell_switch(inner);

    BetterAimMode mode = current_staff_aim_mode();
    f32 rawAimX = savedMoveX;
    f32 rawAimZ = savedMoveZ;
    if (mode != BETTER_AIM_MODE_ORIGINAL && gAimWhileMoving && gPadGetCX != 0 && gPadGetCY != 0) {
        rawAimX = (f32)gPadGetCX(0);
        rawAimZ = (f32)gPadGetCY(0);
    }
    f32 inputX = normalized_input(rawAimX) * (gInvertAimX ? -1.0f : 1.0f);
    f32 inputZ = normalized_input(rawAimZ) * (gInvertAimY ? -1.0f : 1.0f);

    if (!activeMove) {
        int result = gPlayerStateAimStaff(obj, state, fv);
        BetterAimMode postMode = current_staff_aim_mode();
        if (postMode != BETTER_AIM_MODE_ORIGINAL || freeze_crosshair_requested()) {
            set_aim_screen(inner, postMode, freeze_crosshair_requested());
        }
        return result;
    }

    BetterAimVerticalLimits verticalLimits = aim_vertical_limits(inner);

    if (mode == BETTER_AIM_MODE_ORIGINAL) {
        f32 targetX = inputX;
        f32 targetZ = inputZ < 0.0f ? inputZ * verticalLimits.negative : inputZ * verticalLimits.positive;
        f32 rate = 0.1f * gAimSpeed;
        f32 nextX = interpolate_toward(inner->aimInputX, targetX, rate, dt);
        f32 nextZ = interpolate_toward(inner->aimInputZ, targetZ, rate, dt);
        f32 fakeZ = clampf_local(nextZ, -1.0f, 1.0f);

        inner->aimInputX = nextX;
        inner->aimInputZ = fakeZ;
        state->baddie.moveInputX = nextX * 56.0f;
        state->baddie.moveInputZ = fakeZ * 56.0f;

        int result = gPlayerStateAimStaff(obj, state, fv);

        inner->aimInputX = nextX;
        inner->aimInputZ = nextZ;
        inner->bodyLeanHalf = (s16)round_to_s32(-10240.0f * nextX);
        set_aim_screen(inner, BETTER_AIM_MODE_FREE, freeze_crosshair_requested());
        state->baddie.moveInputX = savedMoveX;
        state->baddie.moveInputZ = savedMoveZ;
        return result;
    }

    f32 nextZ = clampf_local(inner->aimInputZ + inputZ * 0.04f * gAimSpeed * dt,
                             -verticalLimits.negative, verticalLimits.positive);
    f32 nextX = inner->aimInputX;
    s16 yawBefore = inner->targetYaw;

    if (mode == BETTER_AIM_MODE_CENTERED) {
        nextX = 0.0f;
        inner->targetYaw = (s16)(inner->targetYaw - (s16)round_to_s32(384.0f * inputX * gAimSpeed * dt));
        inner->yaw = inner->targetYaw;
        obj->anim.rotX = inner->targetYaw;
    } else {
        nextX = clampf_local(inner->aimInputX + inputX * 0.04f * gAimSpeed * dt, -1.0f, 1.0f);
    }

    f32 fakeX = clampf_local(nextX, -0.75f, 0.75f);
    if (mode == BETTER_AIM_MODE_FREE && nextX >= 0.999f && inputX > 0.0f) {
        fakeX = 0.75f + 0.25f * inputX;
    } else if (mode == BETTER_AIM_MODE_FREE && nextX <= -0.999f && inputX < 0.0f) {
        fakeX = -0.75f + 0.25f * inputX;
    }

    inner->aimInputX = fakeX;
    inner->aimInputZ = nextZ;
    state->baddie.moveInputX = fakeX * 56.0f;
    state->baddie.moveInputZ = nextZ * 56.0f;

    int result = gPlayerStateAimStaff(obj, state, fv);

    if (mode == BETTER_AIM_MODE_FREE && absf_local(fakeX) > 0.75f) {
        inner->targetYaw = angle_add_scaled(yawBefore, inner->targetYaw, gAimSpeed);
        inner->yaw = inner->targetYaw;
        obj->anim.rotX = inner->targetYaw;
    } else if (mode == BETTER_AIM_MODE_FREE) {
        inner->targetYaw = yawBefore;
        inner->yaw = yawBefore;
        obj->anim.rotX = yawBefore;
    }

    inner->aimInputX = nextX;
    inner->aimInputZ = nextZ;
    inner->bodyLeanHalf = (s16)round_to_s32(-10240.0f * nextX);
    set_aim_screen(inner, mode, freeze_crosshair_requested());
    state->baddie.moveInputX = savedMoveX;
    state->baddie.moveInputZ = savedMoveZ;
    if (gAimWhileMoving && result == 0) {
        apply_aim_movement(obj, inner, savedVelocityY);
    }
    return result;
}

static int is_staff_aim_camera(GameObject* target) {
    if (target == 0 || target->extra == 0) {
        return 0;
    }
    PlayerState* inner = (PlayerState*)target->extra;
    return inner->baddie.controlMode == 0x2c && target->anim.currentMove == 0x43e;
}

static f32 centered_pitch(PlayerState* inner) {
    return aim_pitch_for_input(inner, inner->aimInputZ);
}

static void better_force_behind_update(CameraObject* camera) {
    GameObject* target = (GameObject*)camera->anim.targetObj;
    BetterAimMode mode = current_staff_aim_mode();
    if (!is_staff_aim_camera(target) || mode == BETTER_AIM_MODE_ORIGINAL) {
        gCameraModeForceBehindUpdate(camera);
        return;
    }

    PlayerState* inner = (PlayerState*)target->extra;
    f32 pitchScale = inner->focusObject != 0 ? 8192.0f : 14336.0f;
    s16 aimYaw = (s16)(10240.0f * inner->aimInputX);
    s16 targetYaw = (s16)((0x8000 - target->anim.rotX) + (aimYaw >> 1));
    f32 savedAimZ = inner->aimInputZ;
    s16 targetPitch;

    if (mode == BETTER_AIM_MODE_CENTERED) {
        targetPitch = (s16)round_to_s32(clampf_local(centered_pitch(inner), -16383.0f, 16383.0f));
        inner->aimInputZ = (f32)((s32)targetPitch * 2) / pitchScale;
    } else {
        s16 fullPitch = (s16)(pitchScale * inner->aimInputZ);
        targetPitch = (s16)(fullPitch >> 1);
    }

    camera->anim.rotX = (f32)(s32)targetYaw;
    camera->anim.rotY = (f32)(s32)targetPitch;
    gCameraModeForceBehindUpdate(camera);
    inner->aimInputZ = savedAimZ;
}

static f32 apply_binocular_inertia(f32 current, f32 target, f32 dt) {
    if (gBinocularInertia <= 0.0001f) {
        return target;
    }
    f32 rate = clampf_local(0.12f / gBinocularInertia, 0.001f, 1.0f);
    if (gInterpolate != 0) {
        return current + gInterpolate(target - current, rate, dt);
    }
    return current + (target - current) * clampf_local(rate * dt, 0.0f, 1.0f);
}

static void apply_binocular_focus_rotation(CameraObject* camera) {
    GameObject* focus = (GameObject*)camera->anim.targetObj;
    if (focus == 0) {
        return;
    }

    focus->anim.rotX = (s16)(0x8000 - (s16)round_to_s32(camera->anim.rotX));
    if (focus->anim.classId == 1 && gObjSetXRot != 0) {
        gObjSetXRot(focus, focus->anim.rotX);
    }
}

static void better_first_person_controls_original(CameraObject* camera, CameraModeViewfinderState* vf) {
    f32 oldYaw = camera->anim.rotX;
    f32 oldPitch = camera->anim.rotY;
    f32 oldYawSpeed = vf->yawSpeed;
    f32 oldFov = camera->fov;
    s8 stickX = gPadGetStickX(0);
    s8 stickY = gPadGetStickY(0);
    f32 inputX = (f32)stickX * (gInvertBinocularX ? -1.0f : 1.0f);
    f32 inputY = (f32)stickY * (gInvertBinocularY ? -1.0f : 1.0f);
    f32 dt = gTimeDelta != 0 ? *gTimeDelta : 1.0f;

    gFirstPersonDoControls(camera);

    f32 zoom = clampf_local((60.0f - oldFov) / 50.0f, 0.0f, 1.0f);
    f32 axisRate = (6.0f - 4.0f * zoom) * gBinocularSpeed;
    f32 targetYawSpeed = inputX * axisRate;
    f32 yawSpeed = interpolate_toward(oldYawSpeed, targetYawSpeed, 0.12f, dt);

    if (absf_local(yawSpeed) < 5.0f) {
        yawSpeed = 0.0f;
    }

    f32 targetPitch = 15360.0f * (inputY / 120.0f);
    f32 pitchRate = (1.0f / (32.0f * zoom + 16.0f)) * gBinocularSpeed;
    f32 nextPitch = interpolate_toward(oldPitch, targetPitch, pitchRate, dt);
    f32 maxPitch = gBinocularVerticalRangeDegrees * 91.02222222222222f;
    nextPitch = clampf_local(nextPitch, -maxPitch, maxPitch);

    vf->yawSpeed = yawSpeed;
    camera->anim.rotX = oldYaw + yawSpeed * dt;
    camera->anim.rotY = nextPitch;
    apply_binocular_focus_rotation(camera);
}

static void better_first_person_controls(CameraObject* camera) {
    CameraModeViewfinderState* vf = gViewfinderState != 0 ? *gViewfinderState : 0;
    if (vf == 0 || gPadGetStickX == 0 || gPadGetStickY == 0) {
        gBinocularPitchSpeed = 0.0f;
        gLastBinocularCamera = 0;
        gFirstPersonDoControls(camera);
        return;
    }

    if (!gModernBinocularControls) {
        gBinocularPitchSpeed = 0.0f;
        gLastBinocularCamera = 0;
        better_first_person_controls_original(camera, vf);
        return;
    }

    uint64_t frame = gHost->frameCount(gMod);
    if (camera != gLastBinocularCamera || frame > gLastBinocularFrame + 1) {
        gBinocularPitchSpeed = 0.0f;
    }
    gLastBinocularCamera = camera;
    gLastBinocularFrame = frame;

    f32 oldYaw = camera->anim.rotX;
    f32 oldPitch = camera->anim.rotY;
    f32 oldYawSpeed = vf->yawSpeed;
    f32 oldPitchSpeed = gBinocularPitchSpeed;
    f32 oldFov = camera->fov;
    s8 stickX = gPadGetStickX(0);
    s8 stickY = gPadGetStickY(0);
    f32 inputX = (f32)stickX * (gInvertBinocularX ? -1.0f : 1.0f);
    f32 inputY = (f32)stickY * (gInvertBinocularY ? -1.0f : 1.0f);
    f32 dt = gTimeDelta != 0 ? *gTimeDelta : 1.0f;

    gFirstPersonDoControls(camera);

    f32 zoom = clampf_local((60.0f - oldFov) / 50.0f, 0.0f, 1.0f);
    f32 axisRate = (6.0f - 4.0f * zoom) * gBinocularSpeed;
    f32 targetYawSpeed = inputX * axisRate;
    f32 targetPitchSpeed = inputY * axisRate;
    f32 yawSpeed = apply_binocular_inertia(oldYawSpeed, targetYawSpeed, dt);
    f32 pitchSpeed = apply_binocular_inertia(oldPitchSpeed, targetPitchSpeed, dt);

    if (absf_local(yawSpeed) < 5.0f) {
        yawSpeed = 0.0f;
    }
    if (absf_local(pitchSpeed) < 5.0f) {
        pitchSpeed = 0.0f;
    }

    f32 nextYaw = oldYaw + yawSpeed * dt;
    f32 nextPitch = oldPitch + pitchSpeed * dt;
    f32 maxPitch = gBinocularVerticalRangeDegrees * 91.02222222222222f;
    if (nextPitch > maxPitch) {
        nextPitch = maxPitch;
        if (pitchSpeed > 0.0f) {
            pitchSpeed = 0.0f;
        }
    } else if (nextPitch < -maxPitch) {
        nextPitch = -maxPitch;
        if (pitchSpeed < 0.0f) {
            pitchSpeed = 0.0f;
        }
    }

    vf->yawSpeed = yawSpeed;
    gBinocularPitchSpeed = pitchSpeed;
    camera->anim.rotX = nextYaw;
    camera->anim.rotY = nextPitch;

    apply_binocular_focus_rotation(camera);
}

static void apply_blaster_hud_fix(void) {
    if (gBlasterHudFix && is_blaster_active_now()) {
        gCameraApplyFullViewport();
    }
}

static void better_game_ui_hud_draw(int a, int b, int c) {
    apply_blaster_hud_fix();
    gGameUIHudDraw(a, b, c);
}

static int better_minimap_update(void) {
    apply_blaster_hud_fix();
    return gMinimapUpdate();
}

static void better_iceblast_update(GameObject* obj) {
    if (gFreezeAimMode != BETTER_AIM_MODE_CENTERED || obj == 0 || obj->extra == 0 || gCameraGetWorldMatrix == 0 || gMatrixTransformVector == 0) {
        gIceblastUpdate(obj);
        return;
    }

    f32 beforeTimer = *(f32*)obj->extra;
    gIceblastUpdate(obj);
    f32 afterTimer = *(f32*)obj->extra;
    if (afterTimer <= beforeTimer) {
        return;
    }

    f32 localForward[3] = {0.0f, 0.0f, 1.0f};
    f32 worldForward[3];
    gMatrixTransformVector(gCameraGetWorldMatrix(), localForward, worldForward);
    f32 oldX = obj->anim.velocityX;
    f32 oldY = obj->anim.velocityY;
    f32 oldZ = obj->anim.velocityZ;
    f32 newX = -3.0f * worldForward[0];
    f32 newY = -3.0f * worldForward[1];
    f32 newZ = -3.0f * worldForward[2];
    f32 dt = gTimeDelta != 0 ? *gTimeDelta : 1.0f;
    obj->anim.localPosX += (newX - oldX) * dt;
    obj->anim.localPosY += (newY - oldY) * dt;
    obj->anim.localPosZ += (newZ - oldZ) * dt;
    obj->anim.velocityX = newX;
    obj->anim.velocityY = newY;
    obj->anim.velocityZ = newZ;
}

static void* resolve_required(const char* name) {
    void* p = gHost->symbolAddress(gMod, name);
    if (p == 0) {
        gHost->log(gMod, FH_LOG_ERROR, name);
    }
    return p;
}

static int install_hook(const char* name, void* replacement, void** original) {
    if (gInstalledHookCount >= BETTER_HOOK_CAPACITY) {
        return FH_MOD_ERROR;
    }

    void* target = resolve_required(name);
    if (target == 0) {
        return FH_MOD_ERROR;
    }

    int result = gHost->hookInstall(gMod, target, replacement, original);
    if (result == FH_MOD_OK) {
        gInstalledHookTargets[gInstalledHookCount] = target;
        gInstalledHookCount++;
    }
    return result;
}

static void remove_installed_hooks(void) {
    if (gHost == 0 || gMod == 0) {
        gInstalledHookCount = 0;
        return;
    }

    while (gInstalledHookCount > 0) {
        gInstalledHookCount--;
        gHost->hookRemove(gMod, gInstalledHookTargets[gInstalledHookCount]);
        gInstalledHookTargets[gInstalledHookCount] = 0;
    }
}

static int fail_initialize(void) {
    remove_installed_hooks();
    gMod = 0;
    gHost = 0;
    gLastBinocularCamera = 0;
    gBinocularPitchSpeed = 0.0f;
    gFlightInputContext = FLIGHT_INPUT_NONE;
    return FH_MOD_ERROR;
}

FH_MOD_EXPORT int fh_mod_initialize(FhMod* mod, const FhModHost* host) {
    if (mod == 0 || host == 0 || host->abiVersion != FH_MOD_ABI_VERSION || host->structSize < sizeof(FhModHost)) {
        return FH_MOD_ERROR;
    }

    gMod = mod;
    gHost = host;
    gInstalledHookCount = 0;
    gFlightInputContext = FLIGHT_INPUT_NONE;
    load_config();

    gTimeDelta = (f32*)resolve_required("timeDelta");
    gGetScreenResolution = (GetScreenResolutionFn)resolve_required("getScreenResolution");
    gPadGetStickX = (PadGetStickFn)resolve_required("padGetStickX");
    gPadGetStickY = (PadGetStickFn)resolve_required("padGetStickY");
    gGetYButtonItem = (GetYButtonItemFn)resolve_required("getYButtonItem");
    gPlayerCastSpell = (PlayerCastSpellFn)resolve_required("playerCastSpell");
    gPlayerCanCastBlasterSpell = (PlayerCanCastBlasterSpellFn)resolve_required("playerCanCastBlasterSpell");
    gObjGetPlayerObject = (ObjGetPlayerObjectFn)resolve_required("Obj_GetPlayerObject");
    gObjSetXRot = (ObjSetXRotFn)resolve_required("objSetXRot");
    gCameraGetFovY = (CameraGetFovYFn)resolve_required("Camera_GetFovY");
    gInterpolate = (InterpolateFn)resolve_required("interpolate");
    gAtanf = (AtanfFn)resolve_required("atanf");
    gMathTanf = (MathTanfFn)resolve_required("mathTanf");
    gMathSinf = (MathTrigFn)resolve_required("mathSinf");
    gMathCosf = (MathTrigFn)resolve_required("mathCosf");
    gCameraGetWorldMatrix = (CameraGetWorldMatrixFn)resolve_required("Camera_GetWorldMatrix");
    gMatrixTransformVector = (MatrixTransformVectorFn)resolve_required("Matrix_TransformVector");
    gViewfinderState = (CameraModeViewfinderState**)resolve_required("gCameraModeViewfinderState");
    gPlayerSelectedItem = (s16*)resolve_required("gPlayerSelectedItem");
    gPlayerIceSpellSustaining = (u8*)resolve_required("gPlayerIceSpellSustaining");
    gPlayerSpawnedObjects = (GameObject**)resolve_required("gPlayerSpawnedObjects");
    gPlayerResourceAddress = (void***)resolve_required("gPlayerResource");
    gObjFreeObject = (ObjFreeObjectFn)resolve_required("Obj_FreeObject");
    gResourceRelease = (ResourceReleaseFn)resolve_required("Resource_Release");
    gCameraApplyFullViewport = (CameraApplyFullViewportFn)resolve_required("Camera_ApplyFullViewport");

    if (gTimeDelta == 0 || gGetScreenResolution == 0 || gPadGetStickX == 0 || gPadGetStickY == 0 ||
        gGetYButtonItem == 0 || gPlayerCastSpell == 0 || gPlayerCanCastBlasterSpell == 0 ||
        gObjGetPlayerObject == 0 || gObjSetXRot == 0 || gCameraGetFovY == 0 ||
        gInterpolate == 0 || gAtanf == 0 || gMathTanf == 0 || gMathSinf == 0 || gMathCosf == 0 ||
        gCameraGetWorldMatrix == 0 || gMatrixTransformVector == 0 || gViewfinderState == 0 ||
        gPlayerSelectedItem == 0 || gPlayerIceSpellSustaining == 0 || gPlayerSpawnedObjects == 0 ||
        gPlayerResourceAddress == 0 || gObjFreeObject == 0 || gResourceRelease == 0 ||
        gCameraApplyFullViewport == 0) {
        return fail_initialize();
    }

    if (install_hook("padGetStickX", (void*)better_pad_get_stick_x, (void**)&gPadGetStickX) != FH_MOD_OK) {
        return fail_initialize();
    }
    if (install_hook("padGetStickY", (void*)better_pad_get_stick_y, (void**)&gPadGetStickY) != FH_MOD_OK) {
        return fail_initialize();
    }
    if (install_hook("arwarwing_readControls", (void*)better_arwing_read_controls,
                     (void**)&gArwingReadControls) != FH_MOD_OK) {
        return fail_initialize();
    }
    if (install_hook("SB_CloudRunner_update", (void*)better_cloudrunner_update,
                     (void**)&gCloudRunnerUpdate) != FH_MOD_OK) {
        return fail_initialize();
    }
    if (install_hook("padGetCX", (void*)better_pad_get_cx, (void**)&gPadGetCX) != FH_MOD_OK) {
        return fail_initialize();
    }
    if (install_hook("padGetCY", (void*)better_pad_get_cy, (void**)&gPadGetCY) != FH_MOD_OK) {
        return fail_initialize();
    }
    if (install_hook("playerProcessQueuedItemCommand", (void*)better_player_process_queued_item_command,
                     (void**)&gPlayerProcessQueuedItemCommand) != FH_MOD_OK) {
        return fail_initialize();
    }
    if (install_hook("playerStateAimStaff", (void*)better_player_state_aim_staff,
                     (void**)&gPlayerStateAimStaff) != FH_MOD_OK) {
        return fail_initialize();
    }
    if (install_hook("CameraModeForceBehind_update", (void*)better_force_behind_update,
                     (void**)&gCameraModeForceBehindUpdate) != FH_MOD_OK) {
        return fail_initialize();
    }
    if (install_hook("firstPersonDoControls", (void*)better_first_person_controls,
                     (void**)&gFirstPersonDoControls) != FH_MOD_OK) {
        return fail_initialize();
    }
    if (install_hook("iceblast_update", (void*)better_iceblast_update, (void**)&gIceblastUpdate) != FH_MOD_OK) {
        return fail_initialize();
    }
    if (install_hook("GameUI_hudDraw", (void*)better_game_ui_hud_draw, (void**)&gGameUIHudDraw) != FH_MOD_OK) {
        return fail_initialize();
    }
    if (install_hook("Minimap_update", (void*)better_minimap_update, (void**)&gMinimapUpdate) != FH_MOD_OK) {
        return fail_initialize();
    }

    return FH_MOD_OK;
}

FH_MOD_EXPORT void fh_mod_config_changed(FhMod* mod) {
    if (mod == gMod && gHost != 0) {
        load_config();
    }
}

FH_MOD_EXPORT void fh_mod_shutdown(FhMod* mod) {
    if (mod == gMod) {
        remove_installed_hooks();
        gMod = 0;
        gHost = 0;
        gLastBinocularCamera = 0;
        gBinocularPitchSpeed = 0.0f;
        gFlightInputContext = FLIGHT_INPUT_NONE;
    }
}
