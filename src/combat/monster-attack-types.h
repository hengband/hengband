#pragma once

#define MAX_MBE 34

typedef struct mbe_info_type {
    int power; /* The attack "power" */
    int explode_type; /* Explosion effect */
} mbe_info_type;

extern const mbe_info_type mbe_info[MAX_MBE];
