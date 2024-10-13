#pragma once

#include "state.h"
#include <string>
#include <vector>

void pasteFileFromClipboard(State* state, const std::string& destFolder);
void copyFileToClipboard(State* state, const std::string& filePath);
void copyToClipboard(const std::string& originalString);
std::string getFromClipboard();
void pasteFromClipboardVisual(State* state);
void pasteFromClipboard(State* state);
void pasteFromClipboardAfter(State* state);
