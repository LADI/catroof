/* -*- Mode: C ; c-basic-offset: 2 -*- */
/* catroof - audio, midi and surface control device manager */
/* SPDX-FileCopyrightText: Copyright © 2023 Nedko Arnaudov */
/* SPDX-License-Identifier: GPL-3 */

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

#include "common.h"

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
  char subsystem[1024];
  size_t len;
  const char * device_path;

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

  if (has_subsystem && (has_sound || has_input))
  {
#if 0
    printf("devpath:   %s\n", device_path);
    printf("subsystem: %s", basename(subsystem));
    if (has_sound) printf(" [SOUND]");
    if (has_input) printf(" [INPUT]");
    printf("\n");
#else
    printf("-------------------------------------------------------------------------\n");
    printf("% 2lu % 9s ", catroof_device_no, basename(subsystem));
    if (has_sound) printf("[SOUND]\t");
    if (has_input) printf("[INPUT]\t");
    printf("%s", device_path);
    printf("\n");
#endif
    catroof_device_no++;
  }

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
  printf(" N SUBSYSTEM DEVTYPE\tDEVPATH\n");
  return catroof_scan_sysfs_internal(SYSFS_ROOT);
}
