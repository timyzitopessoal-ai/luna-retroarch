#!/bin/sh
# Gera libluna_retroarch.so, o RetroArch como biblioteca do LunaGames.
#   luna/build.sh          compila otimizado
#   DEBUG_SYMS=1 luna/build.sh   compila com símbolos para o gdb
set -e
cd "$(dirname "$0")/.."
# Sem Qt: com ele o rarch_main roda o laço do jogo sozinho e nunca volta ao app.
# Sem FFmpeg: a gravação da 1.22.2 não compila com o FFmpeg 8.
./configure --disable-qt --disable-ffmpeg "$@" > /dev/null
if [ -n "$DEBUG_SYMS" ]; then
   export CFLAGS="-O1 -g" CXXFLAGS="-O1 -g"
fi
nice -n 19 make LUNA_LIB=1 -j"$(nproc)"
