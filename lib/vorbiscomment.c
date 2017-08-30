/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <config.h>


#include <string.h>


#include <gavl/gavl.h>
#include <gavl/metadata.h>
#include <gavl/metatags.h>
#include <gavl/numptr.h>

#include <vorbiscomment.h>

#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "vorbiscomment"

static const struct
  {
  const char * gavl_name;
  const char * vorbis_name;
  }
tags[] =
  {
    { GAVL_META_ARTIST,      "ARTIST"       },
    { GAVL_META_TITLE,       "TITLE"        },
    { GAVL_META_ALBUM,       "ALBUM"        },
    { GAVL_META_ALBUMARTIST, "ALBUM ARTIST" },
    { GAVL_META_ALBUMARTIST, "ALBUMARTIST"  },
    { GAVL_META_GENRE,       "GENRE"        },
    { GAVL_META_COPYRIGHT,   "COPYRIGHT"    },
    { GAVL_META_TRACKNUMBER, "TRACKNUMBER"  },
    { GAVL_META_COMMENT,     "COMMENT"      },
    { /* End */ }
  };

static const char * get_vendor(const gavl_dictionary_t * m)
  {
  const char * ret = gavl_dictionary_get_string(m, GAVL_META_SOFTWARE);
  if(!ret)
    ret = PACKAGE"-"VERSION;
  return ret;
  }
  
int bg_vorbis_comment_bytes(const gavl_dictionary_t * m_stream,
                            const gavl_dictionary_t * m_global,
                            int framing)
  {
  int ret = 0;
  const char * str;
  int i = 0;

  /* Vendor string */
  str = get_vendor(m_stream);
  if(!str)
    {
    /* Vendor string missing */
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Couldn't build vorbis comment, vendor string missing");
    return 0;
    }

  ret += 4 + strlen(str); // Vendor length + vendor
  ret += 4; // Number of tags
  
  while(tags[i].gavl_name)
    {
    str = gavl_dictionary_get_string(m_global, tags[i].gavl_name);
    if(str)
      {
      ret += 4 + strlen(tags[i].vorbis_name) + 1 + strlen(str);
      }
    i++;
    }

  if((str = gavl_dictionary_get_string(m_global, GAVL_META_DATE)) ||
     (str = gavl_dictionary_get_string(m_global, GAVL_META_YEAR)))
    ret += 4 + 5 + strlen(str);
  
  if(framing)
    ret++;

  return ret;
  }

int bg_vorbis_comment_write(uint8_t * buf,
                            const gavl_dictionary_t * m_stream,
                            const gavl_dictionary_t * m_global,
                            int framing)
  {
  int len1;
  int len2;
  int len_total;
  uint8_t * ptr;
  int num_tags = 0;
  uint8_t * num_tags_pos;
  int i = 0;
  const char * str;
  
  ptr = buf;

  /* Vendor string */
  
  str = get_vendor(m_stream);
  if(!str)
    {
    /* Vendor string missing */
    return 0;
    }

  len1 = strlen(str);
  GAVL_32LE_2_PTR(len1, ptr); ptr += 4;
  memcpy(ptr, str, len1); ptr += len1;  

  num_tags_pos = ptr;
  ptr += 4;

  while(tags[i].gavl_name)
    {
    int j = 0;

    while((str = gavl_dictionary_get_string_array(m_global, tags[i].gavl_name, j)))
      {
      len1 = strlen(tags[i].vorbis_name);
      len2 = strlen(str);
      
      len_total = len1 + 1 + len2;

      GAVL_32LE_2_PTR(len_total, ptr); ptr += 4;
      memcpy(ptr, tags[i].vorbis_name, len1); ptr += len1;
      *ptr = '='; ptr++;
      memcpy(ptr, str, len2); ptr += len2;
      
      num_tags++;
      j++;
      }
    i++;
    }

  /* Date needs special attention */
  if((str = gavl_dictionary_get_string(m_global, GAVL_META_DATE)) ||
     (str = gavl_dictionary_get_string(m_global, GAVL_META_YEAR)))
    {
    len1 = 5; // DATE=
    len2 = strlen(str);
    len_total = len1 + len2;
    GAVL_32LE_2_PTR(len_total, ptr); ptr += 4;
    memcpy(ptr, "DATE=", len1); ptr += len1;
    memcpy(ptr, str, len2); ptr += len2;
    num_tags++;
    }
  
  GAVL_32LE_2_PTR(num_tags, num_tags_pos);

  if(framing)
    {
    *ptr = 0x01;
    ptr++;
    }
  
  return ptr - buf;
  }
