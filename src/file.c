/* -*- Mode: C ; c-basic-offset: 2 -*- */
/* SPDX-FileCopyrightText: Copyright © 2010-2023 Nedko Arnaudov */
/* SPDX-FileCopyrightText: Copyright © 2010 Nikita Zlobin <nick87720z@gmail.com> */
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "file.h"

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

char * read_file_contents(const char * filename)
{
  int fd;
  struct stat st;
  char * buffer;

  if (stat(filename, &st) != 0)
  {
    return NULL;
  }

  fd = open(filename, O_RDONLY);
  if (fd == -1)
  {
    return NULL;
  }

  buffer = malloc(st.st_size + 1);
  if (buffer == NULL)
  {
    close(fd);
    return NULL;
  }

  ssize_t sret = read(fd, buffer, (size_t)st.st_size);
  if (sret == -1 || sret == 0 || sret > (ssize_t)st.st_size)
  {
    free(buffer);
    buffer = NULL;
  }
  else
  {
    //printf("[%zd]\n", sret);
    if (sret > 0 && buffer[sret - 1] == '\n')
      buffer[sret - 1] = 0;
    else
      buffer[sret] = 0;
  }

  close(fd);

  return buffer;
}
