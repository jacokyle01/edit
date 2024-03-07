#include "clipboard.h"
#include "helper.h"
#include "state.h"
#include <array>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

std::string getFromClipboard() {
    std::string command;
#ifdef __APPLE__
    command = "pbpaste";
#elif defined(__linux__)
    command = "xclip -selection clipboard -o";
#else
#error "Platform not supported"
#endif

    std::string result;
    std::array<char, 256> buffer;
    FILE* pipe = popen(command.c_str(), "r");

    if (!pipe)
        throw std::runtime_error("popen() failed!");

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    int status = pclose(pipe);
    if (status != 0 || result.empty()) {
        return {""};
    }
    return result;
}

void pasteFromClipboard(State* state) {
    fixColOverMax(state);
    std::string result = getFromClipboard();
    std::vector<std::string> clip;
    std::stringstream ss(result);
    std::string line;
    while (std::getline(ss, line)) {
        clip.push_back(line);
    }
    if (state->data.size() == 0) {
        for (unsigned int i = 0; i < clip.size(); i++) {
            state->data.push_back(clip[i]);
        }
    } else if (!result.empty() && result.back() == '\n') {
        for (int i = 0; i < (int)clip.size(); i++) {
            state->data.insert(state->data.begin() + i + state->row, clip[i]);
        }
    } else if (clip.size() > 0) {
        std::string current = state->data[state->row];

        // break up current line
        state->data[state->row] = current.substr(0, state->col);

        // insert first line of clip
        state->data[state->row] += clip[0];
        int lastRow = state->row;

        // insert other lines of clip
        for (int i = 1; i < (int)clip.size(); i++) {
            int r = i + state->row;
            state->data.insert(state->data.begin() + r, clip[i]);
            lastRow = r;
        }

        // insert last part of broken line
        state->data[lastRow] += current.substr(state->col);
    }
}

void pasteFromClipboardAfter(State* state) {
    fixColOverMax(state);
    std::string result = getFromClipboard();
    std::vector<std::string> clip;
    std::stringstream ss(result);
    std::string line;
    while (std::getline(ss, line)) {
        clip.push_back(line);
    }
    if (!result.empty() && result.back() == '\n') {
        for (int i = 0; i < (int)clip.size(); i++) {
            state->data.insert(state->data.begin() + i + state->row + 1, clip[i]);
        }
    } else if (clip.size() > 0) {
        std::string current = state->data[state->row];

        // break up current line
        int breakCol = state->col;
        if (state->col + 1 <= state->data[state->row].length()) {
            breakCol = state->col + 1;
        }
        state->data[state->row] = current.substr(0, breakCol);

        // insert first line of clip
        state->data[state->row] += clip[0];
        int lastRow = state->row;

        // insert other lines of clip
        for (int i = 1; i < (int)clip.size(); i++) {
            int r = i + state->row;
            state->data.insert(state->data.begin() + r, clip[i]);
            lastRow = r;
        }

        // insert last part of broken line
        state->data[lastRow] += current.substr(breakCol);
    }
}

int copyToClipboard(const std::string& originalString) {
    std::string escapedString;
    for (char c : originalString) {
        switch (c) {
        case '\\':
            escapedString += "\\\\\\\\";
            break;
        case '\"':
            escapedString += "\\\"";
            break;
        case '`':
            escapedString += "\\`";
            break;
        case '$':
            escapedString += "\\$";
            break;
        default:
            escapedString += c;
            break;
        }
    }
#ifdef __APPLE__
    FILE* pipe = popen("pbcopy", "w");
#elif defined(__linux__)
    FILE* pipe = popen("xclip -selection clipboard", "w");
#else
#error "OS not supported"
#endif 
    if (pipe != nullptr) {
        fwrite(escapedString.c_str(), sizeof(char), escapedString.size(), pipe);
        pclose(pipe);
    }
    return 1;
}
