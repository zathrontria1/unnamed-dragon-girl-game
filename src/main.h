#ifndef MAIN_H
#define MAIN_H

int main(void);
void * Main_GetFunctionPointer(uint16_t routine);
void Main_Reset();

void __write();  // Disable the screen printing functions

#endif // MAIN_H
