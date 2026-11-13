# luna-retroarch

Fork do RetroArch usado pelo LunaGames. O ramo de trabalho é `luna`, criado a partir da v1.22.2.

## Compilar (Linux)

    ./configure --disable-ffmpeg
    nice -n 19 make -j8

O `--disable-ffmpeg` é necessário porque a gravação de vídeo da v1.22.2 não compila com o FFmpeg 8 (`AVCodec` não tem mais `sample_fmts`). O LunaGames não usa essa gravação.

## Atualizar a partir do RetroArch original

    git fetch upstream --tags
