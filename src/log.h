/* -*- Mode: C ; c-basic-offset: 2 -*- */
/* catroof - audio, midi and surface control device manager */
/* SPDX-FileCopyrightText: Copyright © 2008-2023 Nedko Arnaudov */
/* SPDX-FileCopyrightText: Juuso Alasuutari <juuso.alasuutari@gmail.com> */
/* SPDX-FileCopyrightText: Copyright © 2002 Robert Ham <rah@bash.sh> */
/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef LOG_H__70B2BC62_C528_43F7_BC0B_B1D7D3C2F5CB__INCLUDED
#define LOG_H__70B2BC62_C528_43F7_BC0B_B1D7D3C2F5CB__INCLUDED

#include "config.h"

#define ANSI_BOLD_ON    "\033[1m"
#define ANSI_BOLD_OFF   "\033[22m"
#define ANSI_COLOR_RED  "\033[31m"
#define ANSI_COLOR_YELLOW "\033[33m"
#define ANSI_RESET      "\033[0m"

#include <stdio.h>

#if HAVE_CDBUS_1
#include <cdbus/log.h>
#else

#ifdef __cplusplus
extern "C"
#endif
typedef
void
(* cdbus_log_function)(
  unsigned int level,
  const char * file,
  unsigned int line,
  const char * func,
  const char * format,
  ...)
#if defined (__GNUC__)
  __attribute__((format(printf, 5, 6)))
#endif
  ;

extern cdbus_log_function cdbus_log;

#define CDBUS_LOG_LEVEL_DEBUG        0
#define CDBUS_LOG_LEVEL_INFO         1
#define CDBUS_LOG_LEVEL_WARN         2
#define CDBUS_LOG_LEVEL_ERROR        3
#define CDBUS_LOG_LEVEL_ERROR_PLAIN  4
#endif

/* fallback for old gcc versions,
   http://gcc.gnu.org/onlinedocs/gcc/Function-Names.html */
#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

#ifdef __cplusplus
extern "C"
#endif
void
catroof_log(
  unsigned int level,
  const char * file,
  unsigned int line,
  const char * func,
  const char * format,
  ...)
#if defined (__GNUC__)
  __attribute__((format(printf, 5, 6)))
#endif
  ;

#define log_debug(fmt, args...)       catroof_log(CDBUS_LOG_LEVEL_DEBUG,       __FILE__, __LINE__, __func__, fmt, ## args)
#define log_info(fmt, args...)        catroof_log(CDBUS_LOG_LEVEL_INFO,        __FILE__, __LINE__, __func__, fmt, ## args)
#define log_warn(fmt, args...)        catroof_log(CDBUS_LOG_LEVEL_WARN,        __FILE__, __LINE__, __func__, fmt, ## args)
#define log_error(fmt, args...)       catroof_log(CDBUS_LOG_LEVEL_ERROR,       __FILE__, __LINE__, __func__, fmt, ## args)
#define log_error_plain(fmt, args...) catroof_log(CDBUS_LOG_LEVEL_ERROR_PLAIN, __FILE__, __LINE__, __func__, fmt, ## args)

#endif /* #ifndef LOG_H__70B2BC62_C528_43F7_BC0B_B1D7D3C2F5CB__INCLUDED */
