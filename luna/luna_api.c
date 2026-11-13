/* LunaGames: API da biblioteca libluna_retroarch.
 *
 * Tudo aqui roda na mesma thread, com o contexto OpenGL do app ativo:
 * luna_ra_init uma vez, luna_ra_iterate a cada quadro, luna_ra_deinit no fim.
 * Para jogar de novo, o app descarrega e carrega a biblioteca outra vez
 * (o RetroArch guarda muito estado global). */
#include <stdlib.h>
#include <string.h>

#include <queues/task_queue.h>

#include "../frontend/frontend.h"
#include "../runloop.h"
#include "../command.h"
#include "../gfx/video_driver.h"
#include "../audio/audio_driver.h"
#include "../configuration.h"
#include "../cheat_manager.h"
#include "../core.h"
#include "../driver.h"
#include "../input/input_driver.h"
#include "luna_host.h"

#define LUNA_EXPORT __attribute__((visibility("default")))

struct luna_host_state luna_host = { 0, 640, 480, 60.0f, 1, 0, NULL };

static int luna_running = 0;

LUNA_EXPORT int luna_ra_api_version(void) { return 2; }

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

/* Muda uma opção com o jogo aberto (API 2). Devolve 0 se a opção existe.
 *   "scale_integer" "0"/"1"   pixels todos do mesmo tamanho (com bordas)
 *   "aspect"        número    posição na lista de formatos do RetroArch (22 = do núcleo, 1 = 16:9)
 *   "volume"        dB        -80 a 12 (0 = volume normal)
 *   "fast_forward"  "0"/"1"   acelerando: o RetroArch não espera o som (e o silencia, com
 *                             audio_fastforward_mute); o ritmo de luna_ra_iterate é o app quem dá */
LUNA_EXPORT int luna_ra_set(const char *key, const char *value)
{
   settings_t *settings = config_get_ptr();
   if (!luna_running || !key || !value || !settings)
      return 1;
   if (!strcmp(key, "scale_integer"))
   {
      configuration_set_bool(settings, settings->bools.video_scale_integer, atoi(value) != 0);
      command_event(CMD_EVENT_VIDEO_SET_ASPECT_RATIO, NULL);
      return 0;
   }
   if (!strcmp(key, "aspect"))
   {
      int idx = atoi(value);
      if (idx < 0 || idx >= ASPECT_RATIO_END)
         return 1;
      configuration_set_uint(settings, settings->uints.video_aspect_ratio_idx, (unsigned)idx);
      command_event(CMD_EVENT_VIDEO_SET_ASPECT_RATIO, NULL);
      return 0;
   }
   if (!strcmp(key, "volume"))
   {
      float db = (float)atof(value);
      if (db < -80.0f) db = -80.0f;
      if (db > 12.0f)  db = 12.0f;
      configuration_set_float(settings, settings->floats.audio_volume, db);
      audio_set_float(AUDIO_ACTION_VOLUME_GAIN, db);
      return 0;
   }
   if (!strcmp(key, "fast_forward"))
   {
      /* O mesmo que a tecla de acelerar do RetroArch (runloop.c). */
      runloop_state_t      *runloop_st = runloop_state_get_ptr();
      input_driver_state_t *input_st   = input_state_get_ptr();
      audio_driver_state_t *audio_st   = audio_state_get_ptr();
      if (atoi(value) != 0)
      {
         input_st->flags   |= INP_FLAG_NONBLOCKING;
         runloop_st->flags |= RUNLOOP_FLAG_FASTMOTION;
         if (settings->bools.audio_fastforward_mute)
            audio_st->flags |= AUDIO_FLAG_MUTED;
      }
      else
      {
         input_st->flags   &= ~INP_FLAG_NONBLOCKING;
         runloop_st->flags &= ~RUNLOOP_FLAG_FASTMOTION;
         runloop_st->fastforward_after_frames = 1;
         audio_st->flags   &= ~AUDIO_FLAG_MUTED;
      }
      driver_set_nonblock_state();
      return 0;
   }
   return 1;
}

/* Troca as trapaças do jogo pelas de um arquivo .cht (formato do RetroArch; valem as com
 * cheatN_enable = true). Caminho vazio ou NULL tira todas. Devolve 0 se deu certo.
 * No hardcore, o próprio RetroArch pausa o hardcore ao aplicar uma trapaça. */
LUNA_EXPORT int luna_ra_cheats(const char *path)
{
   if (!luna_running)
      return 1;
   cheat_manager_state_free();
   core_reset_cheat();
   if (!path || !*path)
      return 0;
   if (!cheat_manager_load(path, false))
      return 1;
   cheat_manager_apply_cheats(false);
   return 0;
}
