#ifndef ANIMSTATE_H
#define ANIMSTATE_H

enum AnimState {
    IDLE = 1,
    IDLE_ATTACK,
    ATTACK_IDLE,
    IDLE_KICK,
    KICK_IDLE,
    IDLE_WALK,
    WALK_IDLE,
    WALK,
    IDLE_WALKBACK,
    WALKBACK_IDLE,
    WALKBACK,
    IDLE_TURN,
    TURN_IDLE,
    IDLE_DYING,
    IDLE_TALK,
    TALK,
    TALK_IDLE
};

#endif // ANIMSTATE_H