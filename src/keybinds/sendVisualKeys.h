#pragma once

#include "../util/bounds.h"
#include "../util/state.h"

bool isValidMoveableChunk(State* state, Bounds bounds);
bool visualBlockValid(State* state);
Bounds getBounds(State* state);
Bounds getBoundsNoVisualCheck(State* state);
void setStateFromWordPosition(State* state, WordPosition pos);
bool sendVisualKeys(State* state, char c);
std::string getInVisual(State* state);
Position changeInVisual(State* state);
Position copyInVisual(State* state);
Position deleteInVisual(State* state);
