#include "sendCommandLineKeys.h"
#include "../util/clipboard.h"
#include "../util/helper.h"
#include "../util/modes.h"
#include "../util/query.h"
#include "../util/state.h"
#include <ncurses.h>
#include <sstream>
#include <string>
#include <vector>

void evaluateCommandLineQuery(State* state) {
    if (state->commandLine.query == "q") {
        endwin();
        exit(0);
    } else if (state->commandLine.query == "a") {
        state->autosave = !state->autosave;
        if (state->autosave) {
            state->status = "AUTOSAVE ON";
        } else {
            state->status = "AUTOSAVE OFF";
        }
    } else if (state->commandLine.query == "!sh") {
        for (unsigned int i = 0; i < state->data.size(); i++) {
            runCommand(state, state->data[i]);
        }
    } else if (state->commandLine.query == "w") {
        trimTrailingWhitespace(state);
        saveFile(state);
    } else if (state->commandLine.query == "wq" || state->commandLine.query == "x") {
        trimTrailingWhitespace(state);
        saveFile(state);
        endwin();
        exit(0);
    } else if (state->commandLine.query.substr(0, 1) == "s") {
        std::istringstream iss(state->commandLine.query);
        std::string s, first, second, g;
        if (std::getline(iss, s, '/') && std::getline(iss, first, '/') && std::getline(iss, second, '/')) {
            replaceCurrentLine(state, first, second);
        }
    } else if (state->commandLine.query.substr(0, 2) == "gs") {
        std::istringstream iss(state->commandLine.query);
        std::string s, first, second, g;
        if (std::getline(iss, s, '/') && std::getline(iss, first, '/') && std::getline(iss, second, '/')) {
            if (!state->dontSave) {
                replaceAllGlobally(state, first, second);
            }
        }
    } else if (state->commandLine.query.substr(0, 2) == "%s") {
        std::istringstream iss(state->commandLine.query);
        std::string s, first, second, g;
        do {
            if (std::getline(iss, s, '/') && std::getline(iss, first, '/') && std::getline(iss, second, '/')) {
                replaceAll(state, first, second);
            }
            std::getline(iss, s, ';');
        } while (!iss.eof());
    } else if (is_number(state->commandLine.query)) {
        unsigned int number = stoul(state->commandLine.query);
        if (number > 0) {
            state->row = number - 1;
        } else {
            state->row = 0;
        }
    }
}

void sendCommandLineKeys(State* state, int c) {
    if (c == 27) { // ESC
        backspaceAll(&state->commandLine);
        state->mode = SHORTCUTS;
    } else if (' ' <= c && c <= '~') {
        add(&state->commandLine, c);
    } else if (c == KEY_LEFT) {
        moveCursorLeft(&state->commandLine);
    } else if (c == KEY_RIGHT) {
        moveCursorRight(&state->commandLine);
    } else if (c == ctrl('g')) {
        if (state->commandLine.query.length() > 0) {
            if (state->commandLine.query[0] == 'g') {
                state->commandLine.query = state->commandLine.query.substr(1);
                if (state->commandLine.cursor > 0) {
                    state->commandLine.cursor -= 1;
                }
            } else {
                state->commandLine.query = 'g' + state->commandLine.query;
                state->commandLine.cursor += 1;
            }
        }
    } else if (c == KEY_BACKSPACE || c == 127) {
        backspace(&state->commandLine);
    } else if (c == ctrl('l')) {
        backspaceAll(&state->commandLine);
    } else if (c == ctrl('v')) {
        addFromClipboard(&state->commandLine);
    } else if (c == '\n') {
        evaluateCommandLineQuery(state);
        backspaceAll(&state->commandLine);
        state->mode = SHORTCUTS;
    }
}
