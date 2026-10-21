# tts-phonemizer-espeak

Text to IPA phonemes with [espeak-ng](https://github.com/espeak-ng/espeak-ng), as a separate process for llama.cpp TTS (Piper voices).
llama.cpp is MIT and does not link espeak-ng (GPL-3.0-or-later); it starts this program and talks to it over stdin/stdout.

License: GPL-3.0-or-later, see `COPYING`.

## Build

espeak-ng is pinned to commit `724808c` (it has `espeak_TextToPhonemesWithTerminator`, which release 1.52.0 does not).

```sh
git clone https://github.com/espeak-ng/espeak-ng.git ../espeak-ng
cd ../espeak-ng && git checkout 724808c
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DUSE_ASYNC=OFF \
      -DUSE_MBROLA=OFF -DUSE_LIBSONIC=OFF -DUSE_LIBPCAUDIO=OFF -DUSE_KLATT=OFF -DUSE_SPEECHPLAYER=OFF \
      -DENABLE_TESTS=OFF -DESPEAK_COMPAT=OFF
cmake --build build
cd ../tts-phonemizer-espeak
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The compiled voice data is in `../espeak-ng/build/espeak-ng-data`; pass it with `--data-dir`.

## Protocol

UTF-8, line based; one process serves many requests.

```
request : <voice>\t<text>\n                              "\n", "\t", "\\" in text are escaped as \n, \t, \\
response: <terminator>\t<sentence_end>\t<phonemes>\n     one line per clause
          END\n
on error: ERR\t<message>\n then END\n
```

`terminator` is the espeak clause terminator masked with `0xFFFFF`, `sentence_end` is 1 if the clause ends a sentence. `phonemes` is the raw espeak IPA output; normalization, language-switch markers and punctuation are handled by the caller.
