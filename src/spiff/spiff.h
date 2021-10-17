#ifndef SPIFF_H
#define SPIFF_H
#include <FS.h>

String readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);

#endif