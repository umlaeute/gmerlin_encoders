/*****************************************************************

  e_mpegaudio.c

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
#include <stdlib.h>
#include <config.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/subprocess.h>
#include <gmerlin_encoders.h>
#include "mpa_common.h"

typedef struct
  {
  char * error_msg;
  char * filename;

  bg_mpa_common_t com;
  } e_mpa_t;

static void * create_mpa()
  {
  e_mpa_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_mpa(void * priv)
  {
  e_mpa_t * mpa;
  mpa = (e_mpa_t*)priv;

  free(mpa);
  }

static const char * get_error_mpa(void * priv)
  {
  e_mpa_t * mpa;
  mpa = (e_mpa_t*)priv;
  return mpa->error_msg;
  }

static void set_parameter_mpa(void * data, char * name,
                              bg_parameter_value_t * v)
  {
  e_mpa_t * mpa;
  mpa = (e_mpa_t*)data;
  if(!name)
    {
    return;
    }
  bg_mpa_set_parameter(&(mpa->com), name, v);
  }

static int open_mpa(void * data, const char * filename,
                    bg_metadata_t * metadata)
  {
  e_mpa_t * mpa;
  mpa = (e_mpa_t*)data;

  mpa->filename = bg_strdup(mpa->filename, filename);
  
  return 1;
  }

static int add_audio_stream_mpa(void * data, gavl_audio_format_t * format)
  {
  e_mpa_t * mpa;

  mpa = (e_mpa_t*)data;
  bg_mpa_set_format(&mpa->com, format);
  return 0;
  }

static int start_mpa(void * data)
  {
  int result;
  e_mpa_t * e = (e_mpa_t*)data;
  result = bg_mpa_start(&e->com, e->filename);
  if(!result)
    e->error_msg = bg_sprintf("Cannot find mp2enc executable");
  return result;
  }



static void write_audio_frame_mpa(void * data, gavl_audio_frame_t * frame,
                                  int stream)
  {
  e_mpa_t * mpa;
  mpa = (e_mpa_t*)data;
  bg_mpa_write_audio_frame(&mpa->com, frame);
  }

static void get_audio_format_mpa(void * data, int stream,
                                 gavl_audio_format_t * ret)
  {
  e_mpa_t * mpa;
  mpa = (e_mpa_t*)data;
  bg_mpa_get_format(&mpa->com, ret);
  
  }

static void close_mpa(void * data, int do_delete)
  {
  e_mpa_t * mpa;
  mpa = (e_mpa_t*)data;

  bg_mpa_close(&mpa->com);

  if(do_delete)
    remove(mpa->filename);
  
  if(mpa->filename)
    free(mpa->filename);

  }

static const char * get_extension_mpa(void * data)
  {
  e_mpa_t * mpa;
  mpa = (e_mpa_t*)data;
  return bg_mpa_get_extension(&mpa->com);
  }

static bg_parameter_info_t * get_parameters_mpa(void * data)
  {
  return bg_mpa_get_parameters();
  }


bg_encoder_plugin_t the_plugin =
  {
    common:
    {
      name:            "e_mpegaudio",       /* Unique short name */
      long_name:       "MPEG-1 layer 1/2 audio encoder",
      mimetypes:       NULL,
      extensions:      "mpa",
      type:            BG_PLUGIN_ENCODER_AUDIO,
      flags:           BG_PLUGIN_FILE,
      priority:        5,
      create:            create_mpa,
      destroy:           destroy_mpa,
      get_error:         get_error_mpa,
      get_parameters:    get_parameters_mpa,
      set_parameter:     set_parameter_mpa,
    },
    max_audio_streams:   1,
    max_video_streams:   0,

    get_extension:       get_extension_mpa,

    open:                open_mpa,
    add_audio_stream:    add_audio_stream_mpa,
    get_audio_format:    get_audio_format_mpa,

    start:               start_mpa,
    write_audio_frame:   write_audio_frame_mpa,
    close:               close_mpa
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
