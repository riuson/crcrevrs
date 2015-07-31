#ifndef ARG_PARSER_H
#define ARG_PARSER_H

#include <stdint.h>
#include "recover.h"
#include "logger.h"

class ArgumentsParser
{
public:
    ArgumentsParser(int argc, char *argv[]);
    ~ArgumentsParser(void);
    bool valid(void);
    uint32_t address(void);
    uint32_t crc(void);
    const char *inputFileName(void) const;
    const char *outputFileName(void) const;
    bool verbose();
    bool validate(logger *log);

private:
    enum Opts {
        OPT_NONE    = 0,
        OPT_HELP    = (0x01 << 0),
        OPT_ADDRESS = (0x01 << 1),
        OPT_CRC     = (0x01 << 2),
        OPT_CRC_AT  = (0x01 << 3),
        OPT_INPUT   = (0x01 << 4),
        OPT_OUTPUT  = (0x01 << 5),
        OPT_VERBOSE = (0x01 << 6)
    };

    enum CrcFrom {
        CrcFromNone,
        CrcFromInput,
        CrcFromAddress
    };

    char mInputFileName[2048];
    char mOutputFileName[2048];
    uint32_t mCrcResult;
    uint32_t mCrcWriteAddress;
    uint32_t mCrcReadAddress;
    CrcFrom mCrcSource;

    Opts mCollectedOpts;
    bool mValid;
    bool mVerbose;
    uint32_t mFileSize;
};


#endif // ARG_PARSER_H
