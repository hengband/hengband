#pragma once

/*!
 * @file z-form.h
 * @brief Low level text formatting
 * @date 2023/04/30
 * @author Ben Harrison, 1997
 */

#include "system/h-basic.h"
#include <string>

std::string vformat(const char *fmt, va_list vp);
std::string format(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void plog_fmt(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void quit_fmt(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void core_fmt(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
