#pragma once

/*
 * Special Object Flags
 */
enum sof_type {
    IDENT_SENSE = 0x01, /* Item has been "sensed" */
    IDENT_XXX02 = 0x02,
    IDENT_EMPTY = 0x04, /* Item charges are known */
    IDENT_KNOWN = 0x08, /* Item abilities are known */
    IDENT_STORE = 0x10, /* Item is storebought !!!! */
    IDENT_FULL_KNOWN = 0x20, /* Item information is known */
    IDENT_BROKEN = 0x80, /* Item is permanently worthless */
};
