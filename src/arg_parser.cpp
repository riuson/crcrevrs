#include "arg_parser.h"
#include "3dparty/simpleopt.h"
//#include <wchar.h>
//#include <char.h>
#include <stdlib.h>
#include <iostream>
#include <cstdio>
using namespace std;

ArgumentsParser::ArgumentsParser(int argc, char *argv[])
{
    enum {
        OPT_HELP    = (0x01 << 0),
        OPT_ADDRESS = (0x01 << 1),
        OPT_CRC     = (0x01 << 2),
        OPT_CRC_AT  = (0x01 << 3),
        OPT_INPUT   = (0x01 << 4),
        OPT_OUTPUT  = (0x01 << 5),
        OPT_VERBOSE = (0x01 << 6)
    };

    CSimpleOpt::SOption g_rgOptions[] = {
        {OPT_ADDRESS, "--address", SO_REQ_SEP},
        {OPT_CRC,     "--crc", SO_REQ_SEP},
        {OPT_CRC_AT,  "--crc-at", SO_REQ_SEP},
        {OPT_INPUT,   "--input", SO_REQ_SEP},
        {OPT_OUTPUT,  "--output", SO_REQ_SEP},
        {OPT_VERBOSE, "--verbose", SO_NONE},
        {OPT_HELP,    "--help", SO_NONE},
        {OPT_HELP,    "-?", SO_NONE},
        SO_END_OF_OPTIONS
    };

    this->mCrcSource = CrcFromInput;
    this->mVerbose = false;
    uint32_t argumentFlags = 0;

    CSimpleOpt args(argc, argv, g_rgOptions);

    while (args.Next()) {
        if (args.LastError() == SO_SUCCESS) {
            // handle option, using OptionId(), OptionText() and OptionArg()
            switch (args.OptionId()) {
                case OPT_ADDRESS: {
                    uint32_t a;
                    sscanf(args.OptionArg(), "%x", &a);
                    this->mAddress = a;
                    argumentFlags |= OPT_ADDRESS;
                    break;
                }

                case OPT_CRC: {
                    uint32_t a;
                    sscanf(args.OptionArg(), "%x", &a);
                    this->mCrc = a;
                    this->mCrcSource = CrcFromInput;
                    argumentFlags |= OPT_CRC;
                    break;
                }

                case OPT_CRC_AT: {
                    uint32_t a;
                    sscanf(args.OptionArg(), "%x", &a);
                    this->mCrc = a;
                    this->mCrcSource = CrcFromAddress;
                    argumentFlags |= OPT_CRC_AT;
                    break;
                }

                case OPT_INPUT: {
                    snprintf(this->mInputFileName, sizeof(this->mInputFileName), "%s", args.OptionArg());
                    argumentFlags |= OPT_INPUT;
                    break;
                }

                case OPT_OUTPUT: {
                    snprintf(this->mOutputFileName, sizeof(this->mOutputFileName), "%s", args.OptionArg());
                    argumentFlags |= OPT_OUTPUT;
                    break;
                }

                case OPT_VERBOSE: {
                    this->mVerbose = true;
                    argumentFlags |= OPT_VERBOSE;
                    break;
                }

                default: {
                    argumentFlags = 0;
                    break;
                }
            }

            /*_sntprintf(strBuffer, sizeof(strBuffer),
                _T("Option, ID: %d, Text: '%s', Argument: '%s'\n"),
                args.OptionId(), args.OptionText(),
                (args.OptionArg() != NULL ? args.OptionArg() : _T("")));
            Log(strBuffer);*/
        } else {
            // handle error, one of: SO_OPT_INVALID, SO_OPT_MULTIPLE,
            // SO_ARG_INVALID, SO_ARG_INVALID_TYPE, SO_ARG_MISSING
            argumentFlags = 0;
        }
    }

    // check for options presented
    uint32_t flags1 = OPT_ADDRESS | OPT_INPUT | OPT_OUTPUT | OPT_CRC;
    uint32_t flags2 = OPT_ADDRESS | OPT_INPUT | OPT_OUTPUT | OPT_CRC_AT;

    if ((argumentFlags & flags1) == flags1) {
        this->mValid = true;
    } else if ((argumentFlags & flags2) == flags2) {
        this->mValid = true;
    } else {
        this->mValid = false;
    }
}

ArgumentsParser::~ArgumentsParser(void)
{
}

bool ArgumentsParser::valid(void)
{
    return (this->mValid);
}

uint32_t ArgumentsParser::address(void)
{
    return (this->mAddress);
}

uint32_t ArgumentsParser::crc(void)
{
    return (this->mCrc);
}

CrcFrom ArgumentsParser::crcSource(void)
{
    return (this->mCrcSource);
}

char *ArgumentsParser::inputFileName(void)
{
    return (this->mInputFileName);
}

char *ArgumentsParser::outputFileName(void)
{
    return (this->mOutputFileName);
}

bool ArgumentsParser::verbose(void)
{
    return (this->mVerbose);
}
