#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

#include "helper.h"
#include "insertLoggingCode.h"
#include "modes.h"
#include "render.h"
#include "state.h"
#include <iostream>
#include <ncurses.h>

#define _COLOR_BLACK 16
#define _COLOR_GREY 242
#define _COLOR_RED 196
#define _COLOR_GREEN 46
#define _COLOR_YELLOW 226
#define _COLOR_BLUE 21
#define _COLOR_MAGENTA 201
#define _COLOR_CYAN 51
#define _COLOR_WHITE 231
#define _COLOR_ORANGE 214

#define BLACK 1
#define GREY 2
#define RED 3
#define GREEN 4
#define YELLOW 5
#define BLUE 6
#define MAGENTA 7
#define CYAN 8
#define WHITE 9
#define ORANGE 10

#define LINE_NUM_LENGTH 6
#define STATUS_BAR_HEIGHT 1

int invertColor(int color) { return color + 10; }

// is there a better way for arg passing?
void mvprintw_color(int r, int c, const char* formatString, const char* cstring1, int num, const char* cstring2, int color) {
    attron(COLOR_PAIR(color));
    mvprintw(r, c, formatString, cstring1, num, cstring2);
    attroff(COLOR_PAIR(color));
}

void mvprintw_color(int r, int c, const char* formatString, int num, int color) {
    attron(COLOR_PAIR(color));
    mvprintw(r, c, formatString, num);
    attroff(COLOR_PAIR(color));
}

void mvprintw_color(int r, int c, const char* formatString, const char* cstring, int color) {
    attron(COLOR_PAIR(color));
    mvprintw(r, c, formatString, cstring);
    attroff(COLOR_PAIR(color));
}

void mvaddch_color(int r, int c, char ch, int color) {
    attron(COLOR_PAIR(color));
    mvaddch(r, c, ch);
    attroff(COLOR_PAIR(color));
}

void initColors() {
    init_pair(BLACK, _COLOR_BLACK, _COLOR_BLACK);
    init_pair(GREY, _COLOR_GREY, _COLOR_BLACK);
    init_pair(RED, _COLOR_RED, _COLOR_BLACK);
    init_pair(GREEN, _COLOR_GREEN, _COLOR_BLACK);
    init_pair(YELLOW, _COLOR_YELLOW, _COLOR_BLACK);
    init_pair(BLUE, _COLOR_BLUE, _COLOR_BLACK);
    init_pair(MAGENTA, _COLOR_MAGENTA, _COLOR_BLACK);
    init_pair(CYAN, _COLOR_CYAN, _COLOR_BLACK);
    init_pair(WHITE, _COLOR_WHITE, _COLOR_BLACK);
    init_pair(ORANGE, _COLOR_ORANGE, _COLOR_BLACK);

    init_pair(invertColor(BLACK), _COLOR_BLACK, _COLOR_BLACK);
    init_pair(invertColor(GREY), _COLOR_BLACK, _COLOR_GREY);
    init_pair(invertColor(RED), _COLOR_BLACK, _COLOR_RED);
    init_pair(invertColor(GREEN), _COLOR_BLACK, _COLOR_GREEN);
    init_pair(invertColor(YELLOW), _COLOR_BLACK, _COLOR_YELLOW);
    init_pair(invertColor(BLUE), _COLOR_BLACK, _COLOR_BLUE);
    init_pair(invertColor(MAGENTA), _COLOR_BLACK, _COLOR_MAGENTA);
    init_pair(invertColor(CYAN), _COLOR_BLACK, _COLOR_CYAN);
    init_pair(invertColor(WHITE), _COLOR_BLACK, _COLOR_WHITE);
    init_pair(invertColor(ORANGE), _COLOR_BLACK, _COLOR_ORANGE);
}

void renderNumMatches(int offset, int selection, int total) {
    int index = offset;
    mvprintw_color(0, index, "%d", selection, WHITE);
    index += std::to_string(selection).length();
    mvprintw_color(0, index, " of ", "", WHITE);
    index += 4;
    mvprintw_color(0, index, "%d", total, WHITE);
    index += std::to_string(total).length();
}

int renderStatusBar(State* state) {
    int offset = 0;
    if (state->showFileStack == true) {
        for (int i = (int)(state->fileStack.size() - 1); i >= 0; i--) {
            mvprintw_color(
                state->fileStack.size() - i - 1,
                state->maxX - state->fileStack[i].length() - 2,
                "\"%s\"",
                state->fileStack[i].c_str(),
                i == (int)state->fileStackIndex ? RED : WHITE
            );
        }
    } else {
        mvprintw_color(0, state->maxX - (state->filename.length() + 2), "\"%s\"", state->filename.c_str(), WHITE);
    }
    if (state->status.length() > 0) {
        mvprintw_color(0, offset, "%s ", state->status.c_str(), RED);
        offset += state->status.length() + 1;
    }
    if (state->mode == NAMING) {
        mvprintw_color(0, offset, "name: %s", state->name.query.c_str(), WHITE);
        offset += state->name.cursor + 6;
        return offset;
    } else if (state->mode == COMMANDLINE) {
        mvprintw_color(0, offset, ":%s", state->commandLine.query.c_str(), WHITE);
        offset += state->commandLine.cursor + 1;
        return offset;
    } else if (state->mode == GREP) {
        mvprintw_color(0, offset, "> %s", state->grep.query.c_str(), GREEN);
        offset += state->grep.cursor + 2;
        renderNumMatches(state->grep.query.length() + 4, state->grep.selection + 1, state->grepOutput.size());
        return offset;
    } else if (state->mode == FINDFILE) {
        mvprintw_color(0, offset, "> ", "", YELLOW);
        mvprintw_color(0, offset + 2, "%s", state->findFile.query.c_str(), state->selectAll ? invertColor(YELLOW) : YELLOW);
        offset += state->findFile.cursor + 2;
        renderNumMatches(state->findFile.query.length() + 4, state->findFile.selection + 1, state->findFileOutput.size());
        return offset;
    } else {
        std::string displayQuery = state->search.query;
        for (size_t i = 0; i < displayQuery.length(); ++i) {
            if (displayQuery[i] == '\n') {
                displayQuery.replace(i, 1, "\\n");
                i++;
            }
        }
        if (displayQuery.length() > 60) {
            displayQuery = safeSubstring(displayQuery, 0, 60);
            displayQuery += "...";
        }
        mvprintw_color(0, offset, "/%s", displayQuery.c_str(), state->searchFail ? RED : GREEN);
        offset += displayQuery.length() + 1;
        if (state->replacing) {
            mvprintw_color(0, offset, "/%s", state->replace.query.c_str(), MAGENTA);
            offset += state->replace.query.length() + 1;
        }
        for (unsigned int i = 0; i < state->harpoonFiles.size(); i++) {
            auto min_name = minimize_filename(state->harpoonFiles[i]);
            mvprintw_color(0, offset, " %s", min_name.c_str(), state->harpoonFiles[i] == state->filename ? YELLOW : GREY);
            offset += min_name.length() + 1;
        }
        if (state->replacing) {
            return state->search.query.length() + state->replace.cursor + 2;
        } else if (state->mode == SEARCH) {
            return state->search.cursor + 1;
        }
    }
    return -1;
}

std::string minimize_filename(const std::string& filename) {
    std::vector<std::string> parts;
    std::stringstream ss(filename);
    std::string part;
    std::string minimized;
    while (std::getline(ss, part, '/')) {
        parts.push_back(part);
    }
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        if (!parts[i].empty()) {
            minimized += parts[i][0];
            minimized += '/';
        }
    }
    minimized += parts.back();
    return minimized;
}

int getColorFromChar(char c) {
    if (c == '[' || c == ']') {
        return GREEN;
    } else if (c == '(' || c == ')') {
        return YELLOW;
    } else if (c == '{' || c == '}') {
        return MAGENTA;
    } else if (c == '\'' || c == '"' || c == '`') {
        return CYAN;
    } else {
        return WHITE;
    }
}

int getSearchColor(State* state, int row, unsigned int startOfSearch) {
    if (state->row == (unsigned int)row && startOfSearch + state->search.query.length() >= state->col && startOfSearch <= state->col) {
        return invertColor(MAGENTA);
    } else {
        return invertColor(CYAN);
    }
}

bool isMergeConflict(const std::string& str) {
    const std::vector<std::string> markers = {"<<<<<<<", "=======", ">>>>>>>", "|||||||"};
    for (const auto& marker : markers) {
        if (str.length() >= marker.length()) {
            if (str.substr(0, marker.length()) == marker) {
                return true;
            }
        }
    }
    return false;
}

void printChar(State* state, int row, int col, char c, int color) {
    if (' ' <= c && c <= '~') {
        mvaddch_color(row - state->windowPosition.row + 1, col + getLineNumberOffset(state), c, color);
    } else if (c == '\t') {
        mvaddch_color(row - state->windowPosition.row + 1, col + getLineNumberOffset(state), ' ', invertColor(RED));
    } else if (' ' <= unctrl(c) && unctrl(c) <= '~') {
        mvaddch_color(row - state->windowPosition.row + 1, col + getLineNumberOffset(state), unctrl(c), invertColor(MAGENTA));
    } else {
        mvaddch_color(row - state->windowPosition.row + 1, col + getLineNumberOffset(state), ' ', invertColor(MAGENTA));
    }
}

void printLineNumber(State* state, int i) {
    int r = i - state->windowPosition.row + 1;
    bool isCurrentRow = i == (int)state->row;
    int border = state->fileExplorerOpen ? state->fileExplorerSize : 0;
    if (isCurrentRow == true) {
        mvprintw_color(r, border, "%5d", i + 1, WHITE);
    } else if (state->recording) {
        mvprintw_color(r, border, "%5d", i + 1, RED);
    } else {
        mvprintw_color(r, border, "%5d", i + 1, GREY);
    }
    bool isLogging = getLoggingRegex(state) != "" && std::regex_search(state->data[i], std::regex(getLoggingRegex(state)));
    bool endsWithSpace = state->data[i].back() == ' ';
    bool isOnMark = (int)state->mark.mark == i && state->mark.filename == state->filename;
    int color = BLACK;
    if (endsWithSpace && state->mode != TYPING) {
        color = RED;
    } else if (isLogging) {
        color = YELLOW;
    } else if (isOnMark) {
        color = CYAN;
    }
    mvprintw_color(r, border + 5, "%c", '|', color);
    if (state->mode == BLAME && state->blame.size() >= state->data.size() && i < (int)state->data.size()) {
        mvprintw_color(r, border + 6, "%-65s", state->blame[i].substr(0, 65).c_str(), i == (int)state->row ? invertColor(WHITE) : WHITE);
    }
}

bool isRowColInVisual(State* state, unsigned int i, unsigned int j) {
    if (state->mode == VISUAL) {
        unsigned int minR;
        unsigned int minC;
        unsigned int maxR;
        unsigned int maxC;
        if (state->row < state->visual.row) {
            minR = state->row;
            minC = state->col;
            maxR = state->visual.row;
            maxC = state->visual.col;
        } else {
            minR = state->visual.row;
            minC = state->visual.col;
            maxR = state->row;
            maxC = state->col;
        }
        if (state->visualType == LINE) {
            if (minR <= i && i <= maxR) {
                return true;
            }
        } else if (state->visualType == BLOCK) {
            if (minR <= i && i <= maxR) {
                return (minC <= j && j <= maxC) || (maxC <= j && j <= minC);
            }
        } else if (state->visualType == NORMAL) {
            if (minR < i && i < maxR) {
                return true;
            } else if (minR == i && maxR == i) {
                return (minC <= j && j <= maxC) || (maxC <= j && j <= minC);
            } else if (minR == i) {
                return minC <= j;
            } else if (maxR == i) {
                return maxC >= j;
            }
        }
    }
    return false;
}

bool isInSearchQuery(State* state, unsigned int row, unsigned int col) {
    if (state->search.query != "" && col + state->search.query.length() <= state->data[row].length()) {
        if (state->search.query == state->data[row].substr(col, state->search.query.length())) {
            return true;
        }
    }
    return false;
}

void printLine(State* state, int row) {
    if (isRowColInVisual(state, row, 0) == true && state->data[row].length() == 0) {
        printChar(state, row, 0, ' ', invertColor(WHITE));
    } else {
        bool isInString = false;
        bool skipNext = false;
        unsigned int searchCounter = 0;
        int renderCol = 0;
        unsigned int startOfSearch = 0;
        bool isComment = false;
        char stringType;
        unsigned int col = 0;
        while (col < state->data[row].length() && col < state->windowPosition.col + state->maxX - getLineNumberOffset(state)) {
            renderCol = renderAutoComplete(state, row, col, renderCol);
            if (searchCounter == 0 && isInSearchQuery(state, row, col)) {
                searchCounter = state->search.query.length();
                startOfSearch = col;
            }
            if (skipNext == false) {
                char current = state->data[row][col];
                if (isInString && current == '\\') {
                    skipNext = true;
                } else {
                    if (isInString == false && (current == '"' || current == '`' || current == '\'')) {
                        isInString = true;
                        stringType = current;
                    } else if (isInString == true && current == stringType) {
                        isInString = false;
                    }
                }
            } else {
                skipNext = false;
            }
            if (!isInString && state->data[row].substr(col, state->commentSymbol.length()) == state->commentSymbol && state->commentSymbol != "") {
                isComment = true;
            }
            if (col >= state->windowPosition.col) {
                if (state->replacing && searchCounter != 0) {
                    for (unsigned int i = 0; i < state->replace.query.length(); i++) {
                        printChar(state, row, renderCol, state->replace.query[i], getSearchColor(state, row, startOfSearch));
                        renderCol++;
                    }
                    col += state->search.query.length();
                    searchCounter = 0;
                } else {
                    int color;
                    if (state->matching.row == (unsigned int)row && state->matching.col == col && (state->matching.row != state->row || state->matching.col != state->col)) {
                        color = invertColor(GREY);
                    } else {
                        if (searchCounter != 0 && state->searching == true) {
                            color = getSearchColor(state, row, startOfSearch);
                        } else if (isMergeConflict(state->data[row])) {
                            color = RED;
                        } else if (isComment) {
                            color = GREEN;
                        } else if (isInString == true && getExtension(state->filename) != "md" && getExtension(state->filename) != "txt") {
                            color = CYAN;
                        } else {
                            color = getColorFromChar(state->data[row][col]);
                        }
                        if (isRowColInVisual(state, row, col)) {
                            color = invertColor(color);
                        }
                    }
                    printChar(state, row, renderCol, state->data[row][col], color);
                    renderCol++;
                    col++;
                }
            } else {
                col++;
            }
            if (searchCounter != 0) {
                searchCounter -= 1;
            }
        }
        renderCol = renderAutoComplete(state, row, col, renderCol);
    }
}

bool isRowInVisual(State* state, int row) { return ((int)state->visual.row <= row && row <= (int)state->row) || ((int)state->row <= row && row <= (int)state->visual.row); }

unsigned int renderAutoComplete(State* state, int row, unsigned int col, unsigned int renderCol) {
    if ((state->mode == TYPING || state->mode == MULTICURSOR) && row == (int)state->row && col == state->col && isRowInVisual(state, row)) {
        std::string completion = autocomplete(state, getCurrentWord(state));
        if (state->data[row].substr(col, completion.length()) != completion) {
            for (unsigned int i = 0; i < completion.length(); i++) {
                printChar(state, row, col + i, completion[i], GREY);
            }
            return renderCol + completion.length();
        }
    }
    return renderCol;
}

void renderGrepOutput(State* state) {
    unsigned int index;
    if ((int)state->grep.selection - ((int)state->maxY / 2) > 0) {
        index = state->grep.selection - state->maxY / 2;
    } else {
        index = 0;
    }
    unsigned int renderIndex = 1;
    int color;
    for (unsigned int i = index; i < state->grepOutput.size() && i < index + state->maxY; i++) {
        color = isTestFile(state->grepOutput[i].path.string()) ? ORANGE : WHITE;
        mvprintw_color(
            renderIndex,
            0,
            "%s:%d %s\n",
            state->grepOutput[i].path.c_str(),
            state->grepOutput[i].lineNum,
            state->grepOutput[i].line.c_str(),
            i == state->grep.selection ? invertColor(color) : color
        );
        renderIndex++;
    }
}

void renderFindFileOutput(State* state) {
    unsigned int index;
    if ((int)state->findFile.selection - ((int)state->maxY / 2) > 0) {
        index = state->findFile.selection - state->maxY / 2;
    } else {
        index = 0;
    }
    unsigned int renderIndex = 1;
    int color;
    for (unsigned int i = index; i < state->findFileOutput.size() && i < index + state->maxY; i++) {
        color = isTestFile(state->findFileOutput[i]) ? ORANGE : WHITE;
        mvprintw_color(
            renderIndex,
            0,
            "%s\n",
            state->findFileOutput[i].c_str(),
            i == state->findFile.selection ? invertColor(color) : color
        );
        renderIndex++;
    }
}

void renderVisibleLines(State* state) {
    for (int i = state->windowPosition.row; i < (int)state->data.size() && i < (int)(state->maxY + state->windowPosition.row) - 1; i++) {
        printLineNumber(state, i);
        printLine(state, i);
    }
}

void moveCursor(State* state, int cursorPosition) {
    if (cursorPosition != -1) {
        move(0, cursorPosition);
    } else if (state->mode == FILEEXPLORER) {
        move(state->fileExplorerIndex + 1 - state->fileExplorerWindowLine, 0);
    } else {
        unsigned int row = state->row + 1;
        if (row > state->windowPosition.row) {
            row -= state->windowPosition.row;
        } else {
            row = 1;
        }
        unsigned int col = state->col + getLineNumberOffset(state);
        if (state->col > state->data[state->row].length()) {
            col = state->data[state->row].length() + getLineNumberOffset(state);
            if (state->windowPosition.col > col) {
                col = getLineNumberOffset(state);
            } else {
                col -= state->windowPosition.col;
            }
        } else if (col > state->windowPosition.col) {
            col -= state->windowPosition.col;
        }
        move(row, col);
    }
}

int renderFileExplorerNode(FileExplorerNode* node, int r, int selectedRow, std::string startingSpaces, bool isFileExplorerOpen, int fileExplorerWindowLine, int fileExplorerSize) {
    int color;
    if (shouldIgnoreFile(node->path)) {
        color = GREY;
    } else if (node->isFolder) {
        color = CYAN;
    } else {
        color = WHITE;
    }
    int row = r + 1;
    int offset = 0;
    if (row - fileExplorerWindowLine > 0) {
        auto displayRow = row - fileExplorerWindowLine;
        mvprintw_color(displayRow, offset, "%s", startingSpaces.c_str(), WHITE);
        offset += startingSpaces.length();
        if (node->isFolder) {
            mvprintw_color(displayRow, offset, "%s", node->isOpen ? "v" : ">", GREY);
            offset += 1;
        }
        if (selectedRow == r) {
            mvprintw_color(displayRow, offset, "%s", "[", isFileExplorerOpen ? RED : YELLOW);
        }
        offset += 1;
        mvprintw_color(
            displayRow,
            offset,
            (std::string("%-") + std::to_string(fileExplorerSize - offset - 1) + std::string("s")).c_str(),
            (int)node->name.length() >= fileExplorerSize - offset - 1
                ? (node->name.substr(0, fileExplorerSize - 3 - offset - 1) + "...").c_str()
                : node->name.substr(0, fileExplorerSize - offset - 1).c_str(),
            color
        );
        offset += node->name.substr(0, fileExplorerSize - offset - 1).length();
        if (selectedRow == r) {
            mvprintw_color(displayRow, offset, "%s", "]", isFileExplorerOpen ? RED : YELLOW);
        }
    }
    if (node->isOpen) {
        for(unsigned int i = 0; i < node->children.size(); i++) {
            row = renderFileExplorerNode(&node->children[i], row, selectedRow, startingSpaces + "  ", isFileExplorerOpen, fileExplorerWindowLine, fileExplorerSize);
        }
    }
    return row;
}

void renderFileExplorer(State* state) {
    renderFileExplorerNode(state->fileExplorer, 0, state->fileExplorerIndex, std::string(""), state->mode == FILEEXPLORER, state->fileExplorerWindowLine, state->fileExplorerSize);
}

void fillPixels(State* state, std::vector<std::vector<Pixel>>& pixels) {
    for (unsigned int i = 0; i < state->maxY; i++) {
        for (unsigned int j = 0; j < state->maxX; j++) {
            pixels[i][j].character = ' ';
            pixels[i][j].color = WHITE;
        }
    }
}

unsigned int addToStatusBar(Query query, std::vector<std::vector<Pixel>>& pixels, Position& cursor, std::string prefix, int color, unsigned int c, bool setCursor) {
    if (setCursor) {
        cursor = {0, c + (unsigned int)prefix.length() + query.cursor};
    }
    std::string render = prefix + query.query;
    for (unsigned int i = 0; i < render.length() && c < pixels[0].size(); i++) {
        pixels[0][c].character = render[i];
        pixels[0][c].color = color;
        c++;
    }
    return c;
}

void fillStatusBar(State* state, std::vector<std::vector<Pixel>>& pixels, Position& cursor) {
    unsigned int c = 0;
    if (state->status.length() > 0) {
        Query temp = {state->status, 0, 0};
        c = addToStatusBar(temp, pixels, cursor, "", RED, c, false);
    }
    if (state->mode == NAMING) {
        c = addToStatusBar(state->name, pixels, cursor, "name: ", WHITE, c, true);
    } else if (state->mode == COMMANDLINE) {
        c = addToStatusBar(state->commandLine, pixels, cursor, ":", WHITE, c, true);
    } else if (state->mode == GREP) {
        c = addToStatusBar(state->grep, pixels, cursor, "> ", GREEN, c, true);
    } else if (state->mode == FINDFILE) {
        c = addToStatusBar(state->findFile, pixels, cursor, "> ", YELLOW, c, true);
    } else {
        c = addToStatusBar(state->search, pixels, cursor, "/", state->searchFail ? RED : GREEN, c, state->mode == SEARCH);
        if (state->replacing) {
            c = addToStatusBar(state->replace, pixels, cursor, "/", MAGENTA, c, state->mode == SEARCH);
        }
        for (unsigned int i = 0; i < state->harpoonFiles.size(); i++) {
            auto min_name = minimize_filename(state->harpoonFiles[i]);
            Query temp = {min_name, 0, 0};
            c = addToStatusBar(temp, pixels, cursor, " ", state->harpoonFiles[i] == state->filename ? YELLOW : GREY, c, false);
        }
    }
    std::string displayFilename = std::string("\"") + state->filename + std::string("\"");
    for (unsigned int i = 0; i < displayFilename.length(); i++) {
        pixels[0].pop_back();
    }
    for (unsigned int i = 0; i < displayFilename.length(); i++) {
        pixels[0].push_back({displayFilename[i], WHITE});
    }
}

void fillLineNum(State* state, std::vector<std::vector<Pixel>>& pixels) {
    for (unsigned int i = 0; i + STATUS_BAR_HEIGHT < pixels.size() && i + state->windowPosition.row < state->data.size(); i++) {
        int r = i + state->windowPosition.row + 1;
        int border = state->fileExplorerOpen ? state->fileExplorerSize : 0;
        int color = GREY;
        if (i + state->windowPosition.row == state->row) {
            color = WHITE;
        } else if (state->recording) {
            color = RED;
        }
        std::string lineNum = std::to_string(r);
        if (lineNum.length() >= LINE_NUM_LENGTH) {
            lineNum = lineNum.substr(lineNum.length() - LINE_NUM_LENGTH + 1);
        } else {
            while (lineNum.length() + 1 < LINE_NUM_LENGTH) {
                lineNum = std::string(" ") + lineNum;
            }
        }
        for (unsigned int j = 0; j < lineNum.length(); j++) {
            pixels[i + STATUS_BAR_HEIGHT][j + border].character = lineNum[j];
            pixels[i + STATUS_BAR_HEIGHT][j + border].color = color;
        }
    }
    // TODO add extra Marks
}

void fillData(State* state, std::vector<std::vector<Pixel>>& pixels, Position& cursor) {
    unsigned int r = STATUS_BAR_HEIGHT;
    unsigned int c = LINE_NUM_LENGTH;
    for (unsigned int i = state->windowPosition.row; r < pixels.size() && i < state->data.size(); i++) {
        c = LINE_NUM_LENGTH;
        unsigned int j = 0;
        for (j = state->windowPosition.col; c < pixels[r].size() && j < state->data[i].size(); j++) {
            if (state->row == i && state->col == j) {
                cursor = {r, c};
            }
            pixels[r][c].character = state->data[i][j];
            pixels[r][c].color = WHITE;
            c++;
        }
        if (state->row == i && state->col >= j) {
            cursor = {r, c};
        }
        r++;
    }
}

void renderPixels(std::vector<std::vector<Pixel>> pixels) {
    erase();
    for (unsigned int i = 0; i < pixels.size(); i++) {
        for (unsigned int j = 0; j < pixels[i].size(); j++) {
            attron(COLOR_PAIR(pixels[i][j].color));
            mvaddch(i, j, pixels[i][j].character);
            attroff(COLOR_PAIR(pixels[i][j].color));
        }
    }
    wnoutrefresh(stdscr);
    doupdate();
}

void renderScreen(State* state) {
    std::vector<std::vector<Pixel>> pixels(state->maxY, std::vector<Pixel>(state->maxX));
    Position cursor;
    fillPixels(state, pixels);
    fillLineNum(state, pixels);
    // TODO grep, findfile, etc
    fillData(state, pixels, cursor);
    // TODO syntax highlighting
    fillStatusBar(state, pixels, cursor);
    // TODO renderFileStack
    renderPixels(pixels);
    move(cursor.row, cursor.col);
}

void initTerminal() {
    initscr();
    raw();
    keypad(stdscr, true);
    noecho();
    if (has_colors() == false) {
        endwin();
        std::cout << "Your terminal does not support color" << std::endl;
        exit(1);
    }
    start_color();
    if (COLORS < 256) {
        endwin();
        std::cout << "Your terminal does not support 256 colors" << std::endl;
        exit(1);
    }
    initColors();
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
}
