/* -*- Mode: C ; c-basic-offset: 2 -*- */
/* catroof - audio, midi and surface control device manager */
/* SPDX-FileCopyrightText: Copyright © 2009-2023 Nedko Arnaudov */
/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef ASSERT_H__F7392646_744A_47F8_8846_2CFCBEF9E714__INCLUDED
#define ASSERT_H__F7392646_744A_47F8_8846_2CFCBEF9E714__INCLUDED

#include "log.h"

#include <assert.h>

/* TODO enable asserts only in debug mode and/or have prerelease and release modes */

#define ASSERT(expr)                                                  \
  do                                                                  \
  {                                                                   \
    if (!(expr))                                                      \
    {                                                                 \
      log_error("ASSERT(" #expr ") failed. function %s in %s:%4u\n",  \
                __FUNCTION__,                                         \
                __FILE__,                                             \
                __LINE__);                                            \
      assert(false);                                                  \
    }                                                                 \
  }                                                                   \
  while(false)

#define ASSERT_NO_PASS                                                \
  do                                                                  \
  {                                                                   \
    log_error("Code execution taboo point. function %s in %s:%4u\n",  \
              __FUNCTION__,                                           \
              __FILE__,                                               \
              __LINE__);                                              \
    assert(false);                                                    \
  }                                                                   \
  while(false)

#endif /* #ifndef ASSERT_H__F7392646_744A_47F8_8846_2CFCBEF9E714__INCLUDED */
