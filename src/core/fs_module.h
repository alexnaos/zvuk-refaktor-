#pragma once
#include <Arduino.h>

void initFileSystem();
void removeFile(const char* path);
bool fileExists(const char* path);
