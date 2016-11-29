#ifndef WIRE_UTIL_H
#define WIRE_UTIL_H
void WireWriteByte(int, uint8_t);
void WireWriteRegister(int address, uint8_t reg, uint8_t value);
void WireRequestArray(int address, uint32_t* buffer, uint8_t amount);
#endif
