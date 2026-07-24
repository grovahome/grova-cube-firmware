#pragma once

void outputs_begin();

bool outputs_setAux12v(bool on);
bool outputs_setAux5v(bool on);
bool outputs_setByName(const char* output, bool on);

bool outputs_isAux12vOn();
bool outputs_isAux5vOn();
bool outputs_hasAux12v();
bool outputs_hasAux5v();
