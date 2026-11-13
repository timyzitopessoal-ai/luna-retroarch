/* LunaGames: contexto OpenGL emprestado pelo app.
 *
 * O LunaGames cria a janela e o contexto (um GtkGLArea no Linux) e chama o
 * RetroArch com ele já ativo. Aqui não há janela nem troca de buffers: o app
 * mostra o quadro depois que o RetroArch desenha no framebuffer dele. */
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include "../../configuration.h"
#include "../video_driver.h"
#include "../../luna/luna_host.h"

typedef struct { int unused; } gfx_ctx_luna_data_t;

static unsigned luna_ctx_last_w, luna_ctx_last_h;

static void *luna_ctx_init(void *video_driver)
{
   if (!luna_host.get_proc)
      return NULL; /* fora do LunaGames este contexto não existe */
   luna_ctx_last_w = luna_host.width;
   luna_ctx_last_h = luna_host.height;
   return calloc(1, sizeof(gfx_ctx_luna_data_t));
}

static void luna_ctx_destroy(void *data) { free(data); }
static enum gfx_ctx_api luna_ctx_get_api(void *data) { return GFX_CTX_OPENGL_API; }

static bool luna_ctx_bind_api(void *data, enum gfx_ctx_api api,
      unsigned major, unsigned minor)
{
   return api == GFX_CTX_OPENGL_API;
}

static void luna_ctx_swap_interval(void *data, int interval) { }
static bool luna_ctx_set_video_mode(void *data, unsigned w, unsigned h, bool fs) { return true; }

static void luna_ctx_get_video_size(void *data, unsigned *width, unsigned *height)
{
   *width  = luna_host.width;
   *height = luna_host.height;
}

static float luna_ctx_get_refresh_rate(void *data) { return luna_host.refresh; }

static void luna_ctx_check_window(void *data, bool *quit, bool *resize,
      unsigned *width, unsigned *height)
{
   *quit = luna_host.quit_requested ? true : false;
   if (luna_host.width != luna_ctx_last_w || luna_host.height != luna_ctx_last_h)
   {
      luna_ctx_last_w = luna_host.width;
      luna_ctx_last_h = luna_host.height;
      *width          = luna_host.width;
      *height         = luna_host.height;
      *resize         = true;
   }
}

static bool luna_ctx_has_focus(void *data) { return luna_host.focused ? true : false; }
static bool luna_ctx_suppress_screensaver(void *data, bool enable) { return false; }
static void luna_ctx_swap_buffers(void *data) { }

static void luna_ctx_input_driver(void *data, const char *name,
      input_driver_t **input, void **input_data)
{
   *input      = NULL;
   *input_data = NULL;
}

static gfx_ctx_proc_t luna_ctx_get_proc_address(const char *name)
{
   void *addr = luna_host.get_proc ? luna_host.get_proc(name) : NULL;
   return *((gfx_ctx_proc_t*)(&addr));
}

static uint32_t luna_ctx_get_flags(void *data)
{
   uint32_t flags = 0;
   BIT32_SET(flags, GFX_CTX_FLAGS_GL_CORE_CONTEXT);
#if defined(HAVE_SLANG) && defined(HAVE_SPIRV_CROSS)
   BIT32_SET(flags, GFX_CTX_FLAGS_SHADERS_SLANG);
#endif
#ifdef HAVE_GLSL
   BIT32_SET(flags, GFX_CTX_FLAGS_SHADERS_GLSL);
#endif
   return flags;
}

static void luna_ctx_set_flags(void *data, uint32_t flags) { }

const gfx_ctx_driver_t gfx_ctx_luna = {
   luna_ctx_init,
   luna_ctx_destroy,
   luna_ctx_get_api,
   luna_ctx_bind_api,
   luna_ctx_swap_interval,
   luna_ctx_set_video_mode,
   luna_ctx_get_video_size,
   luna_ctx_get_refresh_rate,
   NULL, /* get_video_output_size */
   NULL, /* get_video_output_prev */
   NULL, /* get_video_output_next */
   NULL, /* get_metrics */
   NULL, /* translate_aspect */
   NULL, /* update_window_title */
   luna_ctx_check_window,
   NULL, /* set_resize */
   luna_ctx_has_focus,
   luna_ctx_suppress_screensaver,
   true, /* has_windowed */
   luna_ctx_swap_buffers,
   luna_ctx_input_driver,
   luna_ctx_get_proc_address,
   NULL, /* image_buffer_init */
   NULL, /* image_buffer_write */
   NULL, /* show_mouse */
   "luna",
   luna_ctx_get_flags,
   luna_ctx_set_flags,
   NULL, /* bind_hw_render */
   NULL, /* get_context_data */
   NULL  /* make_current */
};
