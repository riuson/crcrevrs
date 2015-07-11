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
        OPT_HELP,
        OPT_ADDRESS,
        OPT_CRC,
        OPT_CRC_AT,
        OPT_INPUT,
        OPT_OUTPUT,
        OPT_VERBOSE
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

    this->mValid = true;
    this->mCrcSource = CrcFromInput;
    this->mVerbose = false;
    int count = 0;

    CSimpleOpt args(argc, argv, g_rgOptions);

    while (args.Next()) {
        if (args.LastError() == SO_SUCCESS) {
            // handle option, using OptionId(), OptionText() and OptionArg()
            switch (args.OptionId()) {
                case OPT_ADDRESS: {
                    uint32_t a;
                    sscanf(args.OptionArg(), "%x", &a);
                    this->mAddress = a;
                    count++;
                    break;
                }

                case OPT_CRC: {
                    uint32_t a;
                    sscanf(args.OptionArg(), "%x", &a);
                    this->mCrc = a;
                    this->mCrcSource = CrcFromInput;
                    count++;
                    break;
                }

                case OPT_CRC_AT: {
                    uint32_t a;
                    sscanf(args.OptionArg(), "%x", &a);
                    this->mCrc = a;
                    this->mCrcSource = CrcFromAddress;
                    count++;
                    break;
                }

                case OPT_INPUT: {
                    snprintf(this->mInputFileName, sizeof(this->mInputFileName), "%s", args.OptionArg());
                    count++;
                    break;
                }

                case OPT_OUTPUT: {
                    snprintf(this->mOutputFileName, sizeof(this->mOutputFileName), "%s", args.OptionArg());
                    count++;
                    break;
                }

                case OPT_VERBOSE: {
                    this->mVerbose = true;
                    break;
                }

                default: {
                    this->mValid = false;
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
            this->mValid = false;
        }
    }

    if (count != 4) {
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
