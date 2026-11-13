/* LunaGames: estado compartilhado entre o RetroArch e o app que o hospeda.
 *
 * Quando o RetroArch roda como biblioteca dentro do LunaGames (HAVE_LUNA),
 * a janela, o contexto OpenGL e o controle são do LunaGames. O RetroArch
 * desenha no framebuffer que o app indicar e lê os botões que o app entregar. */
#ifndef LUNA_HOST_H
#define LUNA_HOST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LUNA_MAX_PADS 16

typedef void *(*luna_get_proc_fn)(const char *name);

struct luna_host_state
{
   /* framebuffer onde a imagem final do jogo deve sair (no GtkGLArea não é o 0) */
   unsigned fbo;
   unsigned width;
   unsigned height;
   float    refresh;
   int      focused;
   int      quit_requested;
   luna_get_proc_fn get_proc;
   /* botões de cada jogador, já no desenho do RetroPad (bit n = RETRO_DEVICE_ID_JOYPAD n) */
   uint16_t pad[LUNA_MAX_PADS];
   /* eixos: 0 = esquerdo X, 1 = esquerdo Y, 2 = direito X, 3 = direito Y */
   int16_t  axis[LUNA_MAX_PADS][4];
   int      pads_connected;
};

extern struct luna_host_state luna_host;

#ifdef HAVE_LUNA
#define LUNA_DEFAULT_FBO (luna_host.fbo)
#else
#define LUNA_DEFAULT_FBO 0
#endif

#ifdef __cplusplus
}
#endif

#endif
