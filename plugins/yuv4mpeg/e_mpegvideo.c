/*****************************************************************

  e_mpegvideo.c

  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include <string.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin_encoders.h>
#include <yuv4mpeg.h>
#include "y4m_common.h"
#include "mpv_common.h"

typedef struct
  {
  char * error_msg;
  bg_y4m_common_t com;
  bg_mpv_common_t mpv;
  } e_mpv_t;

static void * create_mpv()
  {
  e_mpv_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

static const char * get_error_mpv(void * priv)
  {
  e_mpv_t * y4m;
  y4m = (e_mpv_t*)priv;
  return y4m->error_msg;
  }

static const char * extension_mpeg1  = ".m1v";
static const char * extension_mpeg2  = ".m2v";

static const char * get_extension_mpv(void * data)
  {
  e_mpv_t * e = (e_mpv_t*)data;

  switch(e->mpv.format)
    {
    case FORMAT_MPEG1:
    case FORMAT_VCD:
    case FORMAT_MPEG2:
      return extension_mpeg1;
      break;
    case FORMAT_SVCD:
    case FORMAT_DVD:
      return extension_mpeg2;
    }
  return (char*)0;
  }

static ssize_t write_func(void * data, const void *buf, size_t len)
  {
  size_t result;
  result = fwrite(buf, 1, len, (FILE*)(data));
  
  if(result != len)
    return -len;
  return 0;
  }

static int open_mpv(void * data, const char * filename,
                    bg_metadata_t * metadata)
  {
  e_mpv_t * e = (e_mpv_t*)data;
  char * commandline;

  commandline = bg_mpv_make_commandline(&e->mpv, filename);

  fprintf(stderr, "launching %s...", commandline);
  
  e->com.file = popen(commandline, "w");
  if(!e->com.file)
    {
    fprintf(stderr, "failed\n");
    return 0;
    }
  fprintf(stderr, "done\n");

  free(commandline);
  
  /* Copy filename for later reusal */
  e->com.filename = bg_strdup(e->com.filename, filename);
  
  /* Set up writer */
  e->com.writer.data  = e->com.file;
  e->com.writer.write = write_func;
  return 1;
  }

static void add_video_stream_mpv(void * data, gavl_video_format_t* format)
  {
  e_mpv_t * e = (e_mpv_t*)data;
  gavl_video_format_copy(&(e->com.format), format);

  /* Set chroma mode for yuv4mpeg */
  switch(e->mpv.format)
    {
    case FORMAT_MPEG1:
    case FORMAT_VCD:
      e->com.chroma_mode = Y4M_CHROMA_420JPEG;
      break;
    case FORMAT_MPEG2:
    case FORMAT_SVCD:
    case FORMAT_DVD:
      e->com.chroma_mode = Y4M_CHROMA_420MPEG2;
      break;
    default:
      fprintf(stderr, "ERROR: Unknown MPEG format\n");
    }
  bg_y4m_set_pixelformat(&e->com);
  }

static void get_video_format_mpv(void * data, int stream,
                                 gavl_video_format_t * ret)
  {
  e_mpv_t * e = (e_mpv_t*)data;
  gavl_video_format_copy(ret, &(e->com.format));
  }

static int start_mpv(void * data)
  {
  e_mpv_t * e = (e_mpv_t*)data;
  return bg_y4m_write_header(&e->com);
  }

static void write_video_frame_mpv(void * data,
                                  gavl_video_frame_t* frame,
                                  int stream)
  {
  e_mpv_t * e = (e_mpv_t*)data;
  bg_y4m_write_frame(&(e->com), frame);
  }

static void close_mpv(void * data, int do_delete)
  {
  e_mpv_t * e = (e_mpv_t*)data;
  pclose(e->com.file);
  if(do_delete)
    remove(e->com.filename);
  }

static void destroy_mpv(void * data)
  {
  e_mpv_t * e = (e_mpv_t*)data;
  bg_y4m_cleanup(&e->com);
  free(e);
  }

/* Per stream parameters */

static bg_parameter_info_t * get_parameters_mpv(void * data)
  {
  return bg_mpv_get_parameters();
  }

static void set_parameter_mpv(void * data, char * name,
                              bg_parameter_value_t * val)
  {
  e_mpv_t * e = (e_mpv_t*)data;
  bg_mpv_set_parameter(&e->mpv, name, val);
  }

#undef SET_ENUM

bg_encoder_plugin_t the_plugin =
  {
    common:
    {
      name:           "e_mpegvideo",       /* Unique short name */
      long_name:      "MPEG-1/2 video encoder",
      mimetypes:      NULL,
      extensions:     "m1v m2v",
      type:           BG_PLUGIN_ENCODER_VIDEO,
      flags:          BG_PLUGIN_FILE,
      priority:       BG_PLUGIN_PRIORITY_MAX,
      create:         create_mpv,
      destroy:        destroy_mpv,
      get_parameters: get_parameters_mpv,
      set_parameter:  set_parameter_mpv,
      get_error:      get_error_mpv,
    },

    max_audio_streams:  0,
    max_video_streams:  1,

    //    get_video_parameters: get_video_parameters_mpv,

    get_extension:        get_extension_mpv,

    open:                 open_mpv,

    add_video_stream:     add_video_stream_mpv,

    //    set_video_parameter:  set_video_parameter_mpv,

    get_video_format:     get_video_format_mpv,

    start:                start_mpv,

    write_video_frame: write_video_frame_mpv,
    close:             close_mpv,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;