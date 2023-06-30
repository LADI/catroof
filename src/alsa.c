/* -*- Mode: C ; c-basic-offset: 2 -*- */
/* catroof - audio, midi and surface control device manager */
/* SPDX-FileCopyrightText: Copyright © 2008-2023 Nedko Arnaudov */
/* SPDX-FileCopyrightText: Copyright © 2011 Devin Anderson */
/* SPDX-License-Identifier: GPL-3 */

/* catroof code related to ALSA subsystem (Linux) */
/* ALSA PCM and RAW MIDI device handling */

#include <stdbool.h>
#include <alsa/asoundlib.h>
#include <catroof/catroof.h>

static
unsigned int
catroof_alsa_rawmidi_get_info(
  snd_ctl_t * control,
  int device,
  snd_rawmidi_stream_t stream,
  char ** subdevice_name_ptr_ptr)
{
  int iret;
  snd_rawmidi_info_t * info;

  snd_rawmidi_info_alloca(&info);
  snd_rawmidi_info_set_device(info, device);
  snd_rawmidi_info_set_subdevice(info, 0);
  snd_rawmidi_info_set_stream(info, stream);

  iret = snd_ctl_rawmidi_info(control, info);
  if (iret != 0)
  {
    //"assert"(iret == -ENOENT)
    return 0;
  }

  if (subdevice_name_ptr_ptr != NULL)
    *subdevice_name_ptr_ptr = strdup(snd_rawmidi_info_get_name(info));

  return snd_rawmidi_info_get_subdevices_count(info);
}

static
unsigned int
catroof_alsa_pcm_get_info(
  snd_ctl_t * control,
  int device,
  snd_pcm_stream_t stream,
  char ** subdevice_name_ptr_ptr)
{
  int iret;
  snd_pcm_info_t * info;

  snd_pcm_info_alloca(&info);
  snd_pcm_info_set_device(info, device);
  snd_pcm_info_set_subdevice(info, 0);
  snd_pcm_info_set_stream(info, stream);

  iret = snd_ctl_pcm_info(control, info);
  if (iret != 0)
  {
    //"assert"(iret == -ENOENT)
    return 0;
  }

  if (subdevice_name_ptr_ptr != NULL)
    *subdevice_name_ptr_ptr = strdup(snd_pcm_info_get_name(info));

  return snd_pcm_info_get_subdevices_count(info);
}

static
void
catroof_alsa_compose_device_id(
  const char * card_id_str,
  int device_no,
  char * device_id_str)
{
  snprintf(
    device_id_str,
    CATROOF_MAX_DEVICE_ID_STR_SIZE,
    "%s,%d",
    card_id_str,
    device_no);
}

bool
catroof_alsa_enum_devices(
  void * ctx,
  catroof_enum_alsa_card_callback_fn cards_cb,
  catroof_enum_alsa_device_callback_fn devices_cb)
{
  int iret;
  snd_ctl_t * handle;
  snd_ctl_card_info_t * info;
  snd_pcm_info_t * pcminfo_capture;
  snd_pcm_info_t * pcminfo_playback;
  int card_no;
  char card_id_str[CATROOF_MAX_ID_STR_SIZE];
  char card_description[CATROOF_MAX_DESCR_STR_SIZE];
  char device_id_str[CATROOF_MAX_DEVICE_ID_STR_SIZE];
  int device_no;
  unsigned int capture_subdevices;
  unsigned int playback_subdevices;
  snd_rawmidi_info_t * rawmidiinfo_capture;
  snd_rawmidi_info_t * rawmidiinfo_playback;
  void * ctx_card;
  bool ret;
  char * name_capture;
  char * name_playback;

  ret = false;

  snd_ctl_card_info_alloca(&info);
  snd_pcm_info_alloca(&pcminfo_capture);
  snd_pcm_info_alloca(&pcminfo_playback);
  snd_rawmidi_info_alloca(&rawmidiinfo_capture);
  snd_rawmidi_info_alloca(&rawmidiinfo_playback);

  card_no = -1;
  while (snd_card_next(&card_no) >= 0 && card_no >= 0)
  {
    snprintf(card_id_str, sizeof(card_id_str), "hw:%d", card_no);

    if (snd_ctl_open(&handle, card_id_str, 0) >= 0 &&
        snd_ctl_card_info(handle, info) >= 0)
    {
      snprintf(card_id_str, sizeof(card_id_str), "hw:%s", snd_ctl_card_info_get_id(info));
      snprintf(card_description, sizeof(card_description), "%s", snd_ctl_card_info_get_id(info));

      if (!cards_cb(
        ctx,
        card_no,
        card_id_str,
        card_description,
        &ctx_card))
        goto close;

      device_no = -1;

      while (snd_ctl_pcm_next_device(handle, &device_no) >= 0 && device_no != -1)
      {
        capture_subdevices =
          catroof_alsa_pcm_get_info(
            handle,
            device_no,
            SND_PCM_STREAM_CAPTURE,
            &name_capture);
        free(name_capture);

        playback_subdevices =
          catroof_alsa_pcm_get_info(
            handle,
            device_no,
            SND_PCM_STREAM_PLAYBACK,
            &name_playback);
        free(name_playback);

        if (capture_subdevices == 0 &&
            playback_subdevices == 0)
          continue;

        catroof_alsa_compose_device_id(
          card_id_str,
          device_no,
          device_id_str);

        if (!devices_cb(
              ctx,
              ctx_card,
              CATROOF_DEVICE_TYPE_AUDIO,
              device_no,
              device_id_str,
              playback_subdevices,
              capture_subdevices))
          goto close;
      }

      device_no = -1;
      while (true)
      {
        iret = snd_ctl_rawmidi_next_device(handle, &device_no);
        if (iret) continue;
        if (device_no == -1)  break;

        capture_subdevices =
          catroof_alsa_rawmidi_get_info(
            handle,
            device_no,
            SND_RAWMIDI_STREAM_INPUT,
            &name_capture);
        free(name_capture);

        playback_subdevices =
          catroof_alsa_rawmidi_get_info(
            handle,
            device_no,
            SND_RAWMIDI_STREAM_OUTPUT,
            &name_playback);
        free(name_playback);

        if (capture_subdevices == 0 &&
            playback_subdevices == 0)
          continue;

        catroof_alsa_compose_device_id(
          card_id_str,
          device_no,
          device_id_str);


        if (!devices_cb(
              ctx,
              ctx_card,
              CATROOF_DEVICE_TYPE_MIDI,
              device_no,
              device_id_str,
              playback_subdevices,
              capture_subdevices))
          goto close;
      }

      ret = true;

    close:
      snd_ctl_close(handle);
    }
  }

  return ret;
}
