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
#include <catroof/catroof.h>

#define SYSFS_ROOT "/sys/devices"

/* N is [1..+INF], have catroof device indeces 1-based,
 * which is both natural and intentionally made not to match
 * the ALSA card nubering which is 0-based. */
#define CATROOF_DEVICE_NO_START 1
static unsigned long catroof_device_no = CATROOF_DEVICE_NO_START;

static bool catroof_scan_sysfs_internal(const char * dirpath);

static
bool
catroof_scan_sysfs_subdir(
  void * ctx,
  catroof_sysfs_device_callback_fn callback,
  const char * devpath,
  const char * devtype,
  const char * devprefix)
{
  bool success;
  size_t devprefix_len;
  char * dirpath;
  DIR * dir;
  struct dirent * dentry_ptr;
  char * entry_fullpath;
  struct stat st;

  success = false;

  devprefix_len = devprefix != NULL ? strlen(devprefix) : 0;

  dirpath = catdup4(SYSFS_ROOT, devpath, "/", devtype);
  if (dirpath == NULL)
  {
    log_error("OOM when composing sysfs dirpath");
    goto exit;
  }

  dir = opendir(dirpath);
  if (dir == NULL)
  {
    log_error("Cannot open directory '%s': %d (%s)", dirpath, errno, strerror(errno));
    goto free_dirpath;
  }

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
      if (S_ISDIR(st.st_mode) &&
          (devprefix == NULL ||
           strncmp(devprefix, dentry_ptr->d_name, devprefix_len) == 0))
      {
        if (!callback(ctx, devpath, devtype, dentry_ptr->d_name))
        {
          log_error("sysfs dir scan abort");
          goto free_fullpath;
        }
      }
    }
  }

  success = true;

free_fullpath:
  free(entry_fullpath);
close:
  closedir(dir);
free_dirpath:
  free(dirpath);
exit:
  return success;
}

static
bool
catroof_sysfs_device_callback_print(
  void * ctx,
  const char * devpath,
  const char * devtype,
  const char * devid)
{
  if (strcmp(devtype, "sound") == 0)
  {
    printf("              [SOUND] %s\n", devid);
  }
  else if (strcmp(devtype, "input") == 0)
  {
    printf("              [INPUT] %s\n", devid);
    char * input = catdup("input/", devid);
    if (input == NULL)
    {
      log_error("OOM when composing sysfs input event id");
      return false;
    }
    catroof_scan_sysfs_subdir(
      ctx,
      catroof_sysfs_device_callback_print,
      devpath,
      input,
      "event");
    free(input);
  }
  else if (strcmp(devtype, "tty") == 0)
  {
    printf("              [TTY] %s\n", devid);
  }
  else if (strncmp(devid, "event", 5) == 0)
  {
    printf("              [EVENT] %s\n", devid);
  }
  else if (strcmp(devtype, "block") == 0)
  {
    printf("              [BLOCK] %s\n", devid);
  }
  else
  {
    printf("              [???] devtype=\"%s\"\n", devid);
    ASSERT_NO_PASS;
  }
  return true;
}

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
  bool has_block;
  bool has_tty;
  char * manufacturer;
  char * product;
  char * serial;
  char * vendor;
  char * model;
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
  has_block = false;
  has_tty = false;
  manufacturer = NULL;
  product = NULL;
  vendor = NULL;
  model = NULL;
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
      if (S_ISDIR(st.st_mode) && strcmp(dentry_ptr->d_name, "block") == 0)
      {
        has_block = true;
      }
      if (S_ISDIR(st.st_mode) && strcmp(dentry_ptr->d_name, "input") == 0)
      {
        has_input = true;
      }
      if (S_ISDIR(st.st_mode) && strcmp(dentry_ptr->d_name, "tty") == 0)
      {
        has_tty = true;
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
      if (strcmp(dentry_ptr->d_name, "vendor") == 0)
      {
        if (vendor == NULL)
          vendor = read_file_contents(entry_fullpath);
      }
      if (strcmp(dentry_ptr->d_name, "model") == 0)
      {
        if (model == NULL)
          model = read_file_contents(entry_fullpath);
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
      (has_sound || has_input || has_block || has_tty ||
       wwid != NULL ||
       (manufacturer != NULL && product != NULL) ||
       (vendor != NULL && model != NULL)))
  {
    printf("-------------------------------------------------------------------------\n");
    printf("% 2lu % 10s ", catroof_device_no, basename(subsystem));
    printf("%s", device_path);
    printf("\n");
    if (manufacturer != NULL && product != NULL)
    {
      printf("              [MNFCTR] %s\n", manufacturer);
      printf("              [PRODCT] %s\n", product);
      if (serial != NULL) printf("              [SERIAL] %s\n", serial);
    }
    if (vendor != NULL && model != NULL)
    {
      printf("              [VENDOR] %s\n", vendor);
      printf("              [MODEL] %s\n", model);
    }
    if (wwid != NULL) printf("              [WWID] %s\n", wwid);
    if (has_sound)
    {
      if (!catroof_scan_sysfs_subdir(
            NULL,
            catroof_sysfs_device_callback_print,
            device_path,
            "sound",
            "card"))
      {
        catroof_sysfs_device_callback_print(
          NULL,
          device_path,
          "sound",
          "[ERROR]");
      }
    }
    if (has_block)
    {
      if (!catroof_scan_sysfs_subdir(
            NULL,
            catroof_sysfs_device_callback_print,
            device_path,
            "block",
            NULL))
      {
        catroof_sysfs_device_callback_print(
          NULL,
          device_path,
          "block",
          "[ERROR]");
      }
    }
    if (has_input)
    {
      if (!catroof_scan_sysfs_subdir(
            NULL,
            catroof_sysfs_device_callback_print,
            device_path,
            "input",
            NULL))
      {
        catroof_sysfs_device_callback_print(
          NULL,
          device_path,
          "input",
          "[ERROR]");
      }
    }
    if (has_tty)
    {
      if (!catroof_scan_sysfs_subdir(
            NULL,
            catroof_sysfs_device_callback_print,
            device_path,
            "tty",
            NULL))
      {
        catroof_sysfs_device_callback_print(
          NULL,
          device_path,
          "tty",
          "[ERROR]");
      }
    }
    catroof_device_no++;
  }

  free(manufacturer);
  free(product);
  free(vendor);
  free(model);
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
  printf(" N  SUBSYSTEM DEVPATH\n");
  return catroof_scan_sysfs_internal(SYSFS_ROOT);
}
