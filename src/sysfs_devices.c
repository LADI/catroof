/* -*- Mode: C ; c-basic-offset: 2 -*- */
/* catroof - audio, midi and surface control device manager */
/* SPDX-FileCopyrightText: Copyright © 2023 Nedko Arnaudov */
/* SPDX-License-Identifier: GPL-3 */

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>

#include "common.h"
#include "file.h"

#define SYSFS_ROOT "/sys/devices"

/* N is [1..+INF], have catroof device indeces 1-based,
 * which is both natural and intentionally made not to match
 * the ALSA card nubering which is 0-based. */
#define CATROOF_DEVICE_NO_START 1
static unsigned long catroof_device_no = CATROOF_DEVICE_NO_START;

/* recurse sysfs */
static bool catroof_scan_sysfs_internal(const char * dirpath)
{
  DIR * dir;
  struct dirent * dentry_ptr;
  char * entry_fullpath;
  struct stat st;
  bool success;
  bool has_subsystem;
  bool has_sound;
  bool has_input;
  char * manufacturer;
  char * product;
  char * serial;
  char subsystem[1024];
  size_t len;
  const char * device_path;
  char * wwid;

  success = false;

  dir = opendir(dirpath);
  if (dir == NULL)
  {
    log_error("Cannot open directory '%s': %d (%s)", dirpath, errno, strerror(errno));
    goto exit;
  }

  has_subsystem = false;
  has_sound = false;
  has_input = false;
  manufacturer = NULL;
  product = NULL;
  serial = NULL;
  wwid = NULL;
  while ((dentry_ptr = readdir(dir)) != NULL)
  {
    if (strcmp(dentry_ptr->d_name, ".") == 0 ||
        strcmp(dentry_ptr->d_name, "..") == 0)
    {
      continue;
    }

    entry_fullpath = catdup3(dirpath, "/", dentry_ptr->d_name);
    if (entry_fullpath == NULL)
    {
      log_error("catdup() failed");
      goto close;
    }

    if (lstat(entry_fullpath, &st) != 0)
    {
      log_error("failed to stat '%s': %d (%s)", entry_fullpath, errno, strerror(errno));
    }
    else
    {
      if (S_ISLNK(st.st_mode) && strcmp(dentry_ptr->d_name, "subsystem") == 0)
      {
        memset(subsystem, 0, sizeof(subsystem));
        ssize_t sret = readlink(entry_fullpath, subsystem, sizeof(subsystem));
        if (sret == -1)
        {
          log_error("readlink() failed for \"%s\"", entry_fullpath);
        }
        else if ((size_t)sret == sizeof(subsystem))
        {
          log_error("readlink() truncation for \"%s\"", entry_fullpath);
        }
        has_subsystem = true;
      }
      if (S_ISDIR(st.st_mode) && strcmp(dentry_ptr->d_name, "sound") == 0)
      {
        has_sound = true;
      }
      if (strcmp(dentry_ptr->d_name, "input") == 0)
      {
        has_input = true;
      }
      if (strcmp(dentry_ptr->d_name, "manufacturer") == 0)
      {
        if (manufacturer == NULL)
          manufacturer = read_file_contents(entry_fullpath);
      }
      if (strcmp(dentry_ptr->d_name, "product") == 0)
      {
        if (product == NULL)
          product = read_file_contents(entry_fullpath);
      }
      if (strcmp(dentry_ptr->d_name, "serial") == 0)
      {
        if (serial == NULL)
          serial = read_file_contents(entry_fullpath);
      }
      if (strcmp(dentry_ptr->d_name, "wwid") == 0)
      {
        wwid = read_file_contents(entry_fullpath);
      }
      if (S_ISDIR(st.st_mode))
      {
        if (!catroof_scan_sysfs_internal(entry_fullpath))
        {
          goto free;
        }
      }
      else
      {
        len = strlen(SYSFS_ROOT);
        if (strncmp(SYSFS_ROOT, dirpath, len) != 0)
        {
          ASSERT_NO_PASS;
        }
        else
        {
          device_path = dirpath + len;
        }
      }
    }

    free(entry_fullpath);
  }

  if (has_subsystem &&
      (has_sound || has_input ||
       wwid != NULL ||
       (manufacturer != NULL && product != NULL)))
  {
    printf("-------------------------------------------------------------------------\n");
    printf("% 2lu % 9s ", catroof_device_no, basename(subsystem));
    printf("%s", device_path);
    printf("\n");
    if (manufacturer != NULL && product != NULL)
    {
      printf("             [MNFCTR] %s\n", manufacturer);
      printf("             [PRODCT] %s\n", product);
      if (serial != NULL) printf("             [SERIAL] %s\n", serial);
    }
    if (wwid != NULL) printf("             [WWID] %s\n", wwid);
    if (has_sound)
    {
      printf("             [SOUND] ALSA CARD NO:");
      //char * cmd = catdupv("ls -la \"", SYSFS_ROOT, device_path, "/sound\"", NULL);
      //system(cmd);
      //free(cmd);
      for (int cardno = 0; cardno < /* FIXME */ 9; cardno++)
      {
        char cardno_str[2];
        cardno_str[0] = '0' + cardno;
        cardno_str[1] = 0;
        char * path = catdupv(SYSFS_ROOT, device_path, "/sound/card", cardno_str, NULL);
        //printf("%s\n", path);
        if (lstat(path, &st) == 0)
        {
          printf(" %s", cardno_str);
        }
        free(path);
      }
      printf("\n");
    }
    if (has_input) printf("             [INPUT]\n");
    catroof_device_no++;
  }

  free(manufacturer);
  free(product);
  free(serial);
  free(wwid);

  success = true;

  goto close;

free:
  free(entry_fullpath);
close:
  closedir(dir);
exit:
  return success;
}

bool catroof_scan_sysfs(void)
{
  printf("=========================================================================\n");
  printf(" N SUBSYSTEM DEVPATH\n");
  return catroof_scan_sysfs_internal(SYSFS_ROOT);
}
