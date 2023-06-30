/* -*- Mode: C ; c-basic-offset: 2 -*- */
/* catroof - audio, midi and surface control device manager */
/* SPDX-FileCopyrightText: Copyright © 2023 Nedko Arnaudov */
/* SPDX-License-Identifier: GPL-3 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <catroof/catroof.h>

#define CATROOF_MAX_ID_STR_SIZE 2048
#define CATROOF_MAX_DESCR_STR_SIZE 2048

struct catroof_superdevice
{
  int card_no;
  char card_id_str[CATROOF_MAX_ID_STR_SIZE];
  char card_description[CATROOF_MAX_DESCR_STR_SIZE];
};

static
bool
catroof_enum_alsa_card_cb(
  void * ctx __attribute__((unused)),
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

  superdev_ptr = malloc(sizeof(struct catroof_superdevice));
  if (superdev_ptr == NULL) return false;

  superdev_ptr->card_no = card_no;
  strncpy(superdev_ptr->card_id_str, card_id_str, CATROOF_MAX_ID_STR_SIZE);
  strncpy(superdev_ptr->card_description, card_description, CATROOF_MAX_DESCR_STR_SIZE);

  *ctx_card = superdev_ptr;
  return true;
}

#define superdevice_ptr ((struct catroof_superdevice *)ctx_card)

static
bool
catroof_enum_alsa_device_cb(
  void * ctx __attribute__((unused)),
  void * ctx_card,
  unsigned int device_type,
  int device_no,
  unsigned int capture_subdevices,
  unsigned int playback_subdevices)
{
  char device_id_str[CATROOF_MAX_ID_STR_SIZE * 2];

  snprintf(
    device_id_str,
    sizeof(device_id_str),
    "%s,%d",
    superdevice_ptr->card_id_str,
    device_no);

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
