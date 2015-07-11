#ifndef ARG_PARSER_H
#define ARG_PARSER_H

#include <stdint.h>
#include "recover.h"

class ArgumentsParser
{
private:
    bool mValid;
    uint32_t mAddress;
    uint32_t mCrc;
    char mInputFileName[2048];
    char mOutputFileName[2048];
    CrcFrom mCrcSource;
    bool mVerbose;

public:
    ArgumentsParser(int argc, char *argv[]);
    ~ArgumentsParser(void);
    bool valid(void);
    uint32_t address(void);
    uint32_t crc(void);
    CrcFrom crcSource(void);
    char *inputFileName(void);
    char *outputFileName(void);
    bool verbose();
};


#endif // ARG_PARSER_H
