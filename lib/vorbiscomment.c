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
#include <gavl/gavf.h>

#include <vorbiscomment.h>

#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "vorbiscomment"

#include <gmerlin/utils.h>

static const struct
  {
  const char * gavl_name;
  const char * vorbis_name;
  gavl_type_t type;
  }
tags[] =
  {
    { GAVL_META_ARTIST,      "ARTIST",       GAVL_TYPE_STRING },
    { GAVL_META_TITLE,       "TITLE",        GAVL_TYPE_STRING },
    { GAVL_META_ALBUM,       "ALBUM",        GAVL_TYPE_STRING },
    { GAVL_META_ALBUMARTIST, "ALBUM ARTIST", GAVL_TYPE_STRING },
    { GAVL_META_ALBUMARTIST, "ALBUMARTIST",  GAVL_TYPE_STRING },
    { GAVL_META_GENRE,       "GENRE",        GAVL_TYPE_STRING },
    { GAVL_META_COPYRIGHT,   "COPYRIGHT",    GAVL_TYPE_STRING },
    { GAVL_META_TRACKNUMBER, "TRACKNUMBER",  GAVL_TYPE_INT    },
    { GAVL_META_COMMENT,     "COMMENT",      GAVL_TYPE_STRING },
    { /* End */ }
  };

static const char * get_vendor(const gavl_dictionary_t * m)
  {
  const char * ret = gavl_dictionary_get_string(m, GAVL_META_SOFTWARE);
  if(!ret)
    ret = PACKAGE"-"VERSION;
  return ret;
  }

int bg_vorbis_comment_write(gavf_io_t * output,
                            const gavl_dictionary_t * m_stream,
                            const gavl_dictionary_t * m_global,
                            int framing)
  {
  int len1;
  int len2;
  int len_total;
  int num_tags = 0;
  int64_t num_tags_pos;
  int64_t old_pos;
  int64_t start_pos;
  int i = 0;
  int year;
  
  const char * str;
  
  /* Vendor string */

  start_pos = gavf_io_position(output);
  
  str = get_vendor(m_stream);
  if(!str)
    {
    /* Vendor string missing */
    return 0;
    }

  len1 = strlen(str);

  gavf_io_write_32_le(output, len1);
  gavf_io_write_data(output, (uint8_t*)str, len1);
  
  num_tags_pos = gavf_io_position(output);
  gavf_io_write_32_le(output, 0); // Filled in later
  
  while(tags[i].gavl_name)
    {
    
    switch(tags[i].type)
      {
      case GAVL_TYPE_STRING:
        {
        int j = 0;
        while((str = gavl_dictionary_get_string_array(m_global, tags[i].gavl_name, j)))
          {
          len1 = strlen(tags[i].vorbis_name);
          len2 = strlen(str);
          len_total = len1 + 1 + len2;

          gavf_io_write_32_le(output, len_total);
          gavf_io_write_data(output, (uint8_t*)tags[i].vorbis_name, len1);
          gavf_io_write_data(output, (uint8_t*)"=", 1);
          gavf_io_write_data(output, (uint8_t*)str, len2);
      
          num_tags++;
          j++;
          }
        }
        break;
      case GAVL_TYPE_INT:
        {
        int val_i;
        if(gavl_dictionary_get_int(m_global, tags[i].gavl_name, &val_i))
          {
          char * tmp_string = bg_sprintf("%d", val_i);

          len1 = strlen(tags[i].vorbis_name);
          len2 = strlen(tmp_string);
          len_total = len1 + 1 + len2;
          
          gavf_io_write_32_le(output, len_total);
          gavf_io_write_data(output, (uint8_t*)tags[i].vorbis_name, len1);
          gavf_io_write_data(output, (uint8_t*)"=", 1);
          gavf_io_write_data(output, (uint8_t*)tmp_string, len2);
          free(tmp_string);
          num_tags++;
          }
        }
        break;
      default:
        break;
      }
    
    i++;
    }

  /* Date needs special attention */
  if((str = gavl_dictionary_get_string(m_global, GAVL_META_DATE)) &&
    !gavl_string_ends_with(str, "99-99"))
    {
    len1 = 5; // DATE=
    len2 = strlen(str);
    
    len_total = len1 + len2;
    gavf_io_write_32_le(output, len_total);

    gavf_io_write_data(output, (uint8_t*)"DATE=", 5);
    gavf_io_write_data(output, (uint8_t*)str, len2);
    num_tags++;
    }
  else if((year = gavl_dictionary_get_year(m_global, GAVL_META_YEAR)) ||
          (year = gavl_dictionary_get_year(m_global, GAVL_META_DATE)))
    {
    char * tmp_string;
    len1 = 5; // DATE=

    tmp_string = bg_sprintf("%d", year);
    
    len2 = strlen(tmp_string);
    
    len_total = len1 + len2;

    gavf_io_write_32_le(output, len_total);
    gavf_io_write_data(output, (uint8_t*)"DATE=", 5);
    gavf_io_write_data(output, (uint8_t*)tmp_string, len2);

    free(tmp_string);
    num_tags++;
    }

  old_pos = gavf_io_position(output);
  gavf_io_seek(output, num_tags_pos, SEEK_SET);

  gavf_io_write_32_le(output, num_tags); // Filled in later

  gavf_io_seek(output, old_pos, SEEK_SET);

  if(framing)
    gavf_io_write_8(output, 0x01);
  
  return gavf_io_position(output) - start_pos;
  }
