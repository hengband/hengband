#pragma once

/*!
 * @file z-form.h
 * @brief Low level text formatting
 * @date 2023/04/30
 * @author Ben Harrison, 1997
 */

#include "system/h-basic.h"
#include <string>

uint32_t vstrnfmt(char *buf, uint32_t max, const char *fmt, va_list vp);
uint32_t strnfmt(char *buf, uint32_t max, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
std::string format(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void plog_fmt(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void quit_fmt(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void core_fmt(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
