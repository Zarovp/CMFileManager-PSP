#pragma once

#include <psprtc.h>

/// Checks whether a result code indicates success.
#define R_SUCCEEDED(res)   ((res)>=0)
/// Checks whether a result code indicates failure.
#define R_FAILED(res)      ((res)<0)

void Utils_EndDrawing(void);
void Utils_SetMax(int *set, int value, int max);
void Utils_SetMin(int *set, int value, int min);
char *Utils_Basename(const char *filename);
void Utils_GetSizeString(char *string, u64 size);
int Utils_GetDateString(char string[24], pspTime time);
void Utils_AppendArr(char subject[], const char insert[], int pos);
int Utils_Alphasort(const void *p1, const void *p2);
