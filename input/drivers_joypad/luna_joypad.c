/* LunaGames: controle entregue pelo app.
 *
 * O LunaGames lê os controles, aplica os botões trocados pelo jogador e
 * entrega cada jogador já no desenho do RetroPad: o botão n deste driver é
 * o RETRO_DEVICE_ID_JOYPAD n. O retroarch.cfg gerado pelo app liga cada
 * botão do RetroPad ao botão de mesmo número. */
#include <stdint.h>

#include "../input_driver.h"
#include "../../luna/luna_host.h"

static void *luna_joypad_init(void *data) { return (void*)-1; }
static void luna_joypad_destroy(void) { }

static bool luna_joypad_query_pad(unsigned pad)
{
   return pad < LUNA_MAX_PADS && (int)pad < luna_host.pads_connected;
}

static int32_t luna_joypad_button(unsigned port, uint16_t joykey)
{
   if (port >= LUNA_MAX_PADS || joykey >= 16)
      return 0;
   return (luna_host.pad[port] >> joykey) & 1;
}

static int16_t luna_joypad_axis_state(unsigned port, uint32_t joyaxis)
{
   int16_t val = 0;
   unsigned axis;
   if (port >= LUNA_MAX_PADS || joyaxis == AXIS_NONE)
      return 0;
   if (AXIS_NEG_GET(joyaxis) < 4)
   {
      axis = AXIS_NEG_GET(joyaxis);
      val  = luna_host.axis[port][axis];
      return val < 0 ? val : 0;
   }
   if (AXIS_POS_GET(joyaxis) < 4)
   {
      axis = AXIS_POS_GET(joyaxis);
      val  = luna_host.axis[port][axis];
      return val > 0 ? val : 0;
   }
   return 0;
}

static int16_t luna_joypad_axis(unsigned port, uint32_t joyaxis)
{
   return luna_joypad_axis_state(port, joyaxis);
}

static int16_t luna_joypad_state(rarch_joypad_info_t *joypad_info,
      const struct retro_keybind *binds, unsigned port)
{
   int i;
   int16_t ret       = 0;
   uint16_t port_idx = joypad_info->joy_idx;
   if (port_idx >= LUNA_MAX_PADS)
      return 0;
   for (i = 0; i < RARCH_FIRST_CUSTOM_BIND; i++)
   {
      const uint64_t joykey = (binds[i].joykey != NO_BTN)
         ? binds[i].joykey : joypad_info->auto_binds[i].joykey;
      if ((uint16_t)joykey != NO_BTN && luna_joypad_button(port_idx, (uint16_t)joykey))
         ret |= (1 << i);
   }
   return ret;
}

static void luna_joypad_poll(void) { }

static const char *luna_joypad_name(unsigned pad) { return "LunaGames"; }

input_device_driver_t luna_joypad = {
   luna_joypad_init,
   luna_joypad_query_pad,
   luna_joypad_destroy,
   luna_joypad_button,
   luna_joypad_state,
   NULL, /* get_buttons */
   luna_joypad_axis,
   luna_joypad_poll,
   NULL, /* set_rumble */
   NULL, /* set_rumble_gain */
   NULL, /* set_sensor_state */
   NULL, /* get_sensor_input */
   luna_joypad_name,
   "luna",
};
