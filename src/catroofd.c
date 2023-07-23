/* -*- Mode: C ; c-basic-offset: 2 -*- */
/* catroof - audio, midi and surface control device manager */
/* SPDX-FileCopyrightText: Copyright © 2023 Nedko Arnaudov */
/* SPDX-License-Identifier: GPL-3 */

#include "common.h"
#include <catroof/catroof.h>

static
bool
catroof_enum_alsa_card_cb(
  void * UNUSED(ctx),
  int card_no,
  const char * card_id_str,
  const char * card_description,
  void ** ctx_card)
{
  struct catroof_superdevice * superdev_ptr;

  printf("card#%d\n", card_no);
  printf("  id: \"%s\"\n", card_id_str);
  printf("  description: \"%s\"\n", card_description);
  printf("  Devices:\n");

  /* leak catroof_superdevice memory for now, TODO: maintain a list of superdevices and free it on exit */
  superdev_ptr = malloc(sizeof(struct catroof_superdevice));
  if (superdev_ptr == NULL) return false;

  superdev_ptr->data.alsa.card_no = card_no;
  strncpy(
    superdev_ptr->data.alsa.card_id_str,
    card_id_str,
    CATROOF_MAX_ID_STR_SIZE);
  strncpy(
    superdev_ptr->data.alsa.card_description,
    card_description,
    CATROOF_MAX_DESCR_STR_SIZE);

  *ctx_card = superdev_ptr;
  return true;
}

#define superdevice_ptr ((struct catroof_superdevice *)ctx_card)

static
bool
catroof_enum_alsa_device_cb(
  void * UNUSED(ctx),
  void * UNUSED(ctx_card), /* card memory is leaked, see above */
  unsigned int device_type,
  int device_no,
  const char * device_id_str,
  unsigned int playback_subdevices,
  unsigned int capture_subdevices)
{
  switch (device_type)
  {
  case CATROOF_DEVICE_TYPE_AUDIO:
    printf("    Audio device#%d\n", device_no);
    break;
  case CATROOF_DEVICE_TYPE_MIDI:
    printf("    MIDI device#%d\n", device_no);
    break;
  }
  printf("      id: \"%s\"\n", device_id_str);

  if (playback_subdevices == capture_subdevices)
  {
    printf("        %u duplex subdevice(s)\n", playback_subdevices);
  }
  else
  {
    if (capture_subdevices > 0)
    {
      printf("        %u capture subdevice(s)\n", capture_subdevices);
    }
    if (playback_subdevices > 0)
    {
      printf("        %u playback subdevice(s)\n", playback_subdevices);
    }
  }

  return true;
}

#undef superdevice_ptr

void catroof_enum_devices(void)
{
  if (!catroof_alsa_enum_devices(
        NULL,
        catroof_enum_alsa_card_cb,
        catroof_enum_alsa_device_cb))
  {
    fprintf(stderr, "ALSA device enumeration failed\n");
  }
}

int main(void)
{
  catroof_enum_devices();
  return 0;
}
