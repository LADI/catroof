/* -*- Mode: C ; c-basic-offset: 2 -*- */
/* catroof - audio, midi and surface control device manager */
/* SPDX-FileCopyrightText: Copyright © 2023 Nedko Arnaudov */
/* SPDX-License-Identifier: GPL-3 */

#ifndef CATROOF_H__7C90451F_CF8C_472E_8012_4079B57DF3F6__INCLUDED
#define CATROOF_H__7C90451F_CF8C_472E_8012_4079B57DF3F6__INCLUDED

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Adjust editor indent */
#endif

#define CATROOF_DEVICE_TYPE_INVALID 0 /* invalid value (for nullable types) */
#define CATROOF_DEVICE_TYPE_AUDIO   1 /* PCM samples */
#define CATROOF_DEVICE_TYPE_MIDI    2 /* MIDI events */
#define CATROOF_DEVICE_TYPE_SURFACE 3 /* Control Surface (human interaction device) */
#define CATROOF_DEVICE_TYPE_RATBAG  4 /* rodent input device via ratbag */
#define CATROOF_DEVICE_TYPE_CPU     5 /* CPU (isolated) core */
#define CATROOF_DEVICE_TYPE_FPU     6 /* FPU (isolated) core */
#define CATROOF_DEVICE_TYPE_GPU     7 /* GPU core */

#define CATROOF_MAX_ID_STR_SIZE 2048
#define CATROOF_MAX_DEVICE_ID_STR_SIZE (CATROOF_MAX_ID_STR_SIZE * 2)
#define CATROOF_MAX_DESCR_STR_SIZE 2048

struct catroof_superdevice
{
  unsigned int superdevice_type; /* one of CATROOF_DEVICE_TYPE_XXX constants */
  union
  {
    struct
    {
      int card_no;
      char card_id_str[CATROOF_MAX_ID_STR_SIZE];
      char card_description[CATROOF_MAX_DESCR_STR_SIZE];
    } alsa;
  } data;
};

typedef
bool
(* catroof_enum_alsa_card_callback_fn)(
  void * ctx,
  int card_no,
  const char * card_id_str,
  const char * card_description,
  void ** ctx_card);

typedef
bool
(* catroof_enum_alsa_device_callback_fn)(
  void * ctx,
  void * ctx_card,
  unsigned int device_type,
  int device_no,
  const char * device_id_str,
  unsigned int capture_subdevices,
  unsigned int playback_subdevices);

/**
 * Enumerate ALSA devices
 */
bool
catroof_alsa_enum_devices(
  void * ctx,
  catroof_enum_alsa_card_callback_fn cards_cb,
  catroof_enum_alsa_device_callback_fn devices_cb);

typedef
bool
(* catroof_sysfs_device_callback_fn)(
  void * ctx,
  const char * devpath,
  const char * devtype,
  const char * devid);

/**
 * Enumerate all devices
 */
void catroof_enum_devices(void);

#if 0
{ /* Adjust editor indent */
#endif
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* #ifndef CATROOF_H__7C90451F_CF8C_472E_8012_4079B57DF3F6__INCLUDED */
