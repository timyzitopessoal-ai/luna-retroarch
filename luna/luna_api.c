/* LunaGames: API da biblioteca libluna_retroarch.
 *
 * Tudo aqui roda na mesma thread, com o contexto OpenGL do app ativo:
 * luna_ra_init uma vez, luna_ra_iterate a cada quadro, luna_ra_deinit no fim.
 * Para jogar de novo, o app descarrega e carrega a biblioteca outra vez
 * (o RetroArch guarda muito estado global). */
#include <string.h>

#include <queues/task_queue.h>

#include "../frontend/frontend.h"
#include "../runloop.h"
#include "../command.h"
#include "../gfx/video_driver.h"
#include "luna_host.h"

#define LUNA_EXPORT __attribute__((visibility("default")))

struct luna_host_state luna_host = { 0, 640, 480, 60.0f, 1, 0, NULL };

static int luna_running = 0;

LUNA_EXPORT int luna_ra_api_version(void) { return 1; }

/* argv como o da linha de comando do RetroArch (argv[0] qualquer).
 * Devolve 0 se o jogo abriu. */
LUNA_EXPORT int luna_ra_init(int argc, char **argv, luna_get_proc_fn get_proc,
      unsigned fbo, unsigned width, unsigned height)
{
   luna_host.get_proc       = get_proc;
   luna_host.fbo            = fbo;
   luna_host.width          = width;
   luna_host.height         = height;
   luna_host.quit_requested = 0;
   if (rarch_main(argc, argv, NULL) != 0)
      return 1;
   luna_running = 1;
   return 0;
}

/* Um quadro. Devolve 0 enquanto o jogo roda e -1 quando ele terminou. */
LUNA_EXPORT int luna_ra_iterate(unsigned fbo, unsigned width, unsigned height)
{
   int ret;
   if (!luna_running)
      return -1;
   luna_host.fbo    = fbo;
   luna_host.width  = width;
   luna_host.height = height;
   ret = runloop_iterate();
   task_queue_check();
   return ret == -1 ? -1 : 0;
}

LUNA_EXPORT void luna_ra_deinit(void)
{
   if (!luna_running)
      return;
   luna_running = 0;
   main_exit(NULL);
}

/* Pede para fechar o jogo; o próximo luna_ra_iterate devolve -1. */
LUNA_EXPORT void luna_ra_request_quit(void)
{
   luna_host.quit_requested = 1;
}

LUNA_EXPORT void luna_ra_set_focus(int focused) { luna_host.focused = focused; }
LUNA_EXPORT void luna_ra_set_refresh(float hz) { if (hz > 1.0f) luna_host.refresh = hz; }

LUNA_EXPORT void luna_ra_set_pad(unsigned port, uint16_t buttons)
{
   if (port < LUNA_MAX_PADS)
   {
      luna_host.pad[port] = buttons;
      if ((int)port >= luna_host.pads_connected)
         luna_host.pads_connected = port + 1;
   }
}

LUNA_EXPORT void luna_ra_set_axis(unsigned port, unsigned axis, int16_t value)
{
   if (port < LUNA_MAX_PADS && axis < 4)
      luna_host.axis[port][axis] = value;
}

/* Quadros por segundo do núcleo (60,09 no SNES). O app chama luna_ra_iterate
 * nesse ritmo, seja qual for a frequência do monitor. */
LUNA_EXPORT double luna_ra_get_fps(void)
{
   video_driver_state_t *video_st = video_state_get_ptr();
   if (!luna_running || video_st->av_info.timing.fps <= 0.0)
      return 60.0;
   return video_st->av_info.timing.fps;
}
