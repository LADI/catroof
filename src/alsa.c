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
  int card_no = -1;
  char card_id_str[1024];
  char card_description[1024];
  int device_no;
  unsigned int pcm_capture_subdevices;
  unsigned int pcm_playback_subdevices;
  snd_rawmidi_info_t * rawmidiinfo_capture;
  snd_rawmidi_info_t * rawmidiinfo_playback;
  unsigned int rawmidi_capture_subdevices;
  unsigned int rawmidi_playback_subdevices;
  void * ctx_card;
  bool ret;

  ret = false;

  snd_ctl_card_info_alloca(&info);
  snd_pcm_info_alloca(&pcminfo_capture);
  snd_pcm_info_alloca(&pcminfo_playback);
  snd_rawmidi_info_alloca(&rawmidiinfo_capture);
  snd_rawmidi_info_alloca(&rawmidiinfo_playback);

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
        char * name_capture;
        pcm_capture_subdevices =
          catroof_alsa_pcm_get_info(
            handle,
            device_no,
            SND_PCM_STREAM_CAPTURE,
            &name_capture);

        char * name_playback;
        pcm_playback_subdevices =
          catroof_alsa_pcm_get_info(
            handle,
            device_no,
            SND_PCM_STREAM_PLAYBACK,
            &name_playback);

        if (pcm_capture_subdevices == 0 &&
            pcm_playback_subdevices == 0)
          continue;

        if (!devices_cb(
              ctx,
              ctx_card,
              CATROOF_DEVICE_TYPE_AUDIO,
              device_no,
              pcm_playback_subdevices,
              pcm_capture_subdevices))
          goto close;
      }

      for (int device = -1;;)
      {
        iret = snd_ctl_rawmidi_next_device(handle, &device);
        if (iret) continue;
        if (device == -1)  break;

        char * name_capture;
        rawmidi_capture_subdevices =
          catroof_alsa_rawmidi_get_info(
            handle,
            device,
            SND_RAWMIDI_STREAM_INPUT,
            &name_capture);

        char * name_playback;
        rawmidi_playback_subdevices =
          catroof_alsa_rawmidi_get_info(
            handle,
            device,
            SND_RAWMIDI_STREAM_OUTPUT,
            &name_playback);

        if (rawmidi_capture_subdevices == 0 &&
            rawmidi_playback_subdevices == 0)
          continue;

        if (!devices_cb(
              ctx,
              ctx_card,
              CATROOF_DEVICE_TYPE_MIDI,
              device,
              rawmidi_playback_subdevices,
              rawmidi_capture_subdevices))
          goto close;
      }

      ret = true;

    close:
      snd_ctl_close(handle);
    }
  }

  return ret;
}
