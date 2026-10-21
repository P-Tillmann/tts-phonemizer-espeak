// tts-phonemizer-espeak: text to IPA phonemes with espeak-ng, one clause per line.
// A separate process for llama.cpp TTS, so that llama.cpp itself does not link espeak-ng.
//
// Copyright (C) 2026 Peter Tillmann
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version. See COPYING.
//
// Protocol (UTF-8, line based, one process serves many requests):
//   request : <voice>\t<text>\n           in text, "\n", "\t" and "\\" are escaped as \n, \t, \\
//   response: <terminator>\t<sentence_end>\t<phonemes>\n    one line per clause
//             END\n
//   on error: ERR\t<message>\n then END\n
// terminator is the espeak clause terminator & 0xFFFFF, sentence_end is 1 if the clause ends a sentence.

#include <espeak-ng/speak_lib.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define VERSION "0.1.0"
#define CLAUSE_TYPE_SENTENCE 0x00080000

static void unescape(char * s) {
    char * w = s;
    for (char * r = s; *r; r++) {
        if (*r == '\\' && r[1]) {
            r++;
            *w++ = *r == 'n' ? '\n' : *r == 't' ? '\t' : *r;
        } else {
            *w++ = *r;
        }
    }
    *w = 0;
}

// default data dir: espeak-ng-data next to the executable, if it exists
static const char * exe_data_dir(void) {
    static char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 32);
    if (n <= 0) {
        return NULL;
    }
    buf[n] = 0;
    char * slash = strrchr(buf, '/');
    if (!slash) {
        return NULL;
    }
    strcpy(slash + 1, "espeak-ng-data");
    struct stat st;
    return stat(buf, &st) == 0 && S_ISDIR(st.st_mode) ? buf : NULL;
}

static void reply_error(const char * msg) {
    printf("ERR\t%s\nEND\n", msg);
    fflush(stdout);
}

int main(int argc, char ** argv) {
    const char * data_dir = NULL;
    int phoneme_mode = espeakPHONEMES_IPA;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--data-dir") && i + 1 < argc) {
            data_dir = argv[++i];
        } else if (!strcmp(argv[i], "--tie") && i + 1 < argc) {
            // tie character inside multi-letter phonemes, e.g. a^ɪ (ASCII only)
            phoneme_mode |= espeakPHONEMES_TIE | ((unsigned char) argv[++i][0] << 8);
        } else if (!strcmp(argv[i], "--version")) {
            printf("tts-phonemizer-espeak %s, espeak-ng %s\n", VERSION, espeak_Info(NULL));
            return 0;
        } else {
            fprintf(stderr, "usage: %s [--data-dir DIR] [--tie CHAR] [--version]\n", argv[0]);
            fprintf(stderr, "the default data dir is espeak-ng-data next to the executable\n");
            fprintf(stderr, "reads <voice>\\t<text> lines on stdin, see the protocol in main.c\n");
            return 1;
        }
    }
    if (!data_dir) {
        data_dir = exe_data_dir();
    }
    if (espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0, data_dir, 0) < 0) {
        fprintf(stderr, "failed to initialize espeak-ng (data dir: %s)\n", data_dir ? data_dir : "default");
        return 1;
    }

    char * line = NULL;
    size_t cap = 0;
    char voice[256] = "";
    while (getline(&line, &cap, stdin) >= 0) {
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) {
            line[--n] = 0;
        }
        char * text = strchr(line, '\t');
        if (!text) {
            reply_error("expected <voice>\\t<text>");
            continue;
        }
        *text++ = 0;
        unescape(text);
        if (strcmp(line, voice) != 0) {
            if (strlen(line) >= sizeof(voice) || espeak_SetVoiceByName(line) != EE_OK) {
                voice[0] = 0;
                reply_error("unknown voice");
                continue;
            }
            strcpy(voice, line);
        }
        const void * ptr = text;
        while (ptr != NULL) {
            int terminator = 0;
            const char * ph = espeak_TextToPhonemesWithTerminator(&ptr, espeakCHARS_AUTO, phoneme_mode, &terminator);
            if (!ph) {
                break;
            }
            printf("%d\t%d\t%s\n", terminator & 0x000FFFFF, (terminator & CLAUSE_TYPE_SENTENCE) ? 1 : 0, ph);
        }
        printf("END\n");
        fflush(stdout);
    }
    free(line);
    espeak_Terminate();
    return 0;
}
