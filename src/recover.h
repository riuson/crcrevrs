#ifndef RECOVER_H
#define RECOVER_H

#include <stdint.h>
#include "logger.h"

#define AllOnes 0xffffffff
#define DefaultPolynomial 0xEDB88320
#define TableSize 256

class Recover
{
private:
    uint32_t *mTable;
public:
    Recover(void);
    ~Recover(void);
    void patchFile(const char *inputFileName, const char *outputFileName, uint32_t address, uint32_t crc, logger *log);
    void patch(uint8_t *buffer, uint32_t size, uint32_t address, uint32_t crc, logger *log);
private:
    void findInTable(uint8_t sourceValue,  uint32_t *tableValue, uint32_t *tableIndex);
    void setByte(uint32_t *dword, uint8_t index, uint8_t byte);
    uint8_t getByte(uint32_t dword, uint8_t index);
    uint32_t getHashNext(uint32_t previousValue, uint8_t nextByte);
    uint32_t getHashPrev(uint32_t nextValue, uint8_t nextByte);
    uint32_t calculateStub(uint32_t a, uint32_t f);
};

#endif // RECOVER_H
