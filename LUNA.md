# luna-retroarch

Fork do RetroArch usado pelo LunaGames. O ramo de trabalho é `luna`, criado a partir da v1.22.2.

## RetroArch como biblioteca

O LunaGames carrega `libluna_retroarch.so` e o jogo é desenhado dentro da janela dele, com a interface do Luna por cima. Compilar:

    luna/build.sh                 # otimizado
    DEBUG_SYMS=1 luna/build.sh    # com símbolos para o gdb

O script roda `./configure --disable-qt --disable-ffmpeg` e `make LUNA_LIB=1`. Sem Qt porque, com ele, o `rarch_main` roda o laço do jogo sozinho e nunca volta ao app. Sem FFmpeg porque a gravação de vídeo da v1.22.2 não compila com o FFmpeg 8 (`AVCodec` não tem mais `sample_fmts`).

O que o fork muda (tudo atrás de `HAVE_LUNA`):

- `luna/luna_api.c`: a API (`luna_ra_init`, `luna_ra_iterate`, `luna_ra_deinit`, `luna_ra_request_quit`, `luna_ra_set_pad`, `luna_ra_set_axis`, `luna_ra_set_focus`, `luna_ra_set_refresh`). Tudo na mesma thread, com o contexto OpenGL do app ativo. Para outra partida, o app descarrega e carrega a biblioteca de novo (o RetroArch guarda muito estado global).
- `gfx/drivers_context/luna_ctx.c`: contexto `luna` para o driver `glcore`. Não cria janela nem troca buffers; tamanho, foco e funções do OpenGL vêm do app.
- `gl3.c` e `shader_gl3.cpp`: a imagem final sai no framebuffer do app (`LUNA_DEFAULT_FBO`), e não no 0. No GtkGLArea o framebuffer é outro.
- `input/drivers_joypad/luna_joypad.c`: controle `luna`. O app entrega cada jogador já no desenho do RetroPad (bit n = `RETRO_DEVICE_ID_JOYPAD` n), com os botões trocados aplicados.
- `platform_unix.c`: não instala os tratadores de SIGINT/SIGTERM (os sinais são do app).

Configuração mínima do `retroarch.cfg` para esse modo:

    video_driver = "glcore"
    video_context_driver = "luna"
    input_driver = "null"
    input_joypad_driver = "luna"
    input_autodetect_enable = "false"
    input_player1_b_btn = "0"   # e assim por diante: botão n = RetroPad n

## Atualizar a partir do RetroArch original

    git fetch upstream --tags
