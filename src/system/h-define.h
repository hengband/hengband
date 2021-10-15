/*!
 * @file h-define.h
 * @brief 変愚蛮怒で新しく追加された主要なマクロ定義ヘッダ / Define some simple constants
 * @date 2014/08/16
 * @author
 * 不明(変愚蛮怒開発チーム？)
 */

#ifndef INCLUDED_H_DEFINE_H
#define INCLUDED_H_DEFINE_H

/*
 * Refer to the member at offset of structure
 */
#define atoffset(TYPE, STRUCT_PTR, OFFSET) (*(TYPE*)(((char*)STRUCT_PTR) + (OFFSET)))

#endif
