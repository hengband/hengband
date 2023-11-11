/*!
 * @file artifact-info.h
 * @brief アーティファクトの発動効果取得関数ヘッダ
 */

#pragma once

enum class RandomArtActType : short;
class ItemEntity;
RandomArtActType activation_index(const ItemEntity *o_ptr);
