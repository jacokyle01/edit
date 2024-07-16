#include "sendGrepKeys.h"
#include "../util/clipboard.h"
#include "../util/helper.h"
#include "../util/modes.h"
#include "../util/query.h"
#include "../util/state.h"
#include <ncurses.h>
#include <string>
#include <vector>

void sendGrepKeys(State* state, int c) {
    if (c == 27) { // ESC
        state->mode = SHORTCUTS;
    } else if (' ' <= c && c <= '~') {
        add(&state->grep, c);
        state->grep.selection = 0;
    } else if (c == KEY_LEFT) {
        moveCursorLeft(&state->grep);
    } else if (c == KEY_RIGHT) {
        moveCursorRight(&state->grep);
    } else if (c == KEY_BACKSPACE || c == 127) {
        backspace(&state->grep);
        state->grep.selection = 0;
    } else if (c == ctrl('g')) {
        state->mode = FINDFILE;
    } else if (c == ctrl('l')) {
        backspaceAll(&state->grep);
    } else if (c == ctrl('d')) {
        for (unsigned int i = 0; i < state->maxY; i++) {
            if (state->grep.selection + 1 < state->grepOutput.size()) {
                state->grep.selection += 1;
            }
        }
    } else if (c == ctrl('u')) {
        for (unsigned int i = 0; i < state->maxY; i++) {
            if (state->grep.selection > 0) {
                state->grep.selection -= 1;
            }
        }
    } else if (c == KEY_DOWN || c == ctrl('n')) {
        if (state->grep.selection + 1 < state->grepOutput.size()) {
            state->grep.selection += 1;
        }
    } else if (c == KEY_UP || c == ctrl('p')) {
        if (state->grep.selection > 0) {
            state->grep.selection -= 1;
        }
    } else if (c == ctrl('r')) {
        if (state->grep.selection < state->grepOutput.size()) {
            std::filesystem::path selectedFile = state->grepOutput[state->grep.selection].path.string();
            std::filesystem::path currentDir = ((std::filesystem::path)state->filename).parent_path();
            std::filesystem::path relativePath = std::filesystem::relative(selectedFile, currentDir);
            copyToClipboard(relativePath.string());
            state->mode = SHORTCUTS;
        }
    } else if (c == ctrl('y')) {
        if (state->grep.selection < state->grepOutput.size()) {
            std::filesystem::path selectedFile = state->grepOutput[state->grep.selection].path.string();
            copyToClipboard(selectedFile);
            state->mode = SHORTCUTS;
        }
    } else if (c == ctrl('v')) {
        addFromClipboard(&state->grep);
    } else if (c == '\n') {
        if (state->grep.selection < state->grepOutput.size()) {
            std::filesystem::path selectedFile = state->grepOutput[state->grep.selection].path;
            int lineNum = state->grepOutput[state->grep.selection].lineNum;
            state->resetState(selectedFile);
            state->row = lineNum - 1;
            setSearchResultCurrentLine(state, state->grep.query);
        }
    }
    if (state->mode == GREP && c != ctrl('u') && c != ctrl('d') && c != ctrl('p') && c != ctrl('n')) {
        generateGrepOutput(state);
    }
}
