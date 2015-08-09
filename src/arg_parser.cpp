#include "arg_parser.h"
#include "3dparty/simpleopt.h"
#include <stdlib.h>
#include <iostream>
#include <cstdio>
#include <string.h>
#include <fstream>
using namespace std;

ArgumentsParser::ArgumentsParser(int argc, char *argv[])
{
    CSimpleOpt::SOption g_rgOptions[] = {
        {OPT_ADDRESS, "--address", SO_REQ_SEP},
        {OPT_CRC,     "--crc", SO_REQ_SEP},
        {OPT_CRC_AT,  "--crc-at", SO_REQ_SEP},
        {OPT_INPUT,   "--input", SO_REQ_SEP},
        {OPT_OUTPUT,  "--output", SO_REQ_SEP},
        {OPT_VERBOSE, "--verbose", SO_NONE},
        {OPT_VERSION, "--version", SO_NONE},
        {OPT_HELP,    "--help", SO_NONE},
        {OPT_HELP,    "-?", SO_NONE},
        SO_END_OF_OPTIONS
    };

    this->mCrcSource = CrcFromNone;
    this->mVerbose = false;
    this->mShowVersion = false;
    this->mCollectedOpts = OPT_NONE;
    this->mValid = false;
    this->mFileSize = 0;

    CSimpleOpt args(argc, argv, g_rgOptions);

    while (args.Next()) {
        if (args.LastError() == SO_SUCCESS) {
            // handle option, using OptionId(), OptionText() and OptionArg()
            switch (args.OptionId()) {
                case OPT_ADDRESS: {
                    uint32_t a;

                    if (this->strtoul(args.OptionArg(), &a)) {
                        this->mCrcWriteAddress = a;
                        this->mCollectedOpts = (Opts)(this->mCollectedOpts | args.OptionId());
                    }

                    break;
                }

                case OPT_CRC: {
                    uint32_t a;

                    if (this->strtoul(args.OptionArg(), &a)) {
                        this->mCrcResult = a;
                        this->mCrcSource = CrcFromInput;
                        this->mCollectedOpts = (Opts)(this->mCollectedOpts | args.OptionId());
                    }

                    break;
                }

                case OPT_CRC_AT: {
                    uint32_t a;

                    if (this->strtoul(args.OptionArg(), &a)) {
                        this->mCrcReadAddress = a;
                        this->mCrcSource = CrcFromAddress;
                        this->mCollectedOpts = (Opts)(this->mCollectedOpts | args.OptionId());
                    }

                    break;
                }

                case OPT_INPUT: {
                    snprintf(this->mInputFileName, sizeof(this->mInputFileName), "%s", args.OptionArg());
                    this->mCollectedOpts = (Opts)(this->mCollectedOpts | args.OptionId());
                    break;
                }

                case OPT_OUTPUT: {
                    snprintf(this->mOutputFileName, sizeof(this->mOutputFileName), "%s", args.OptionArg());
                    this->mCollectedOpts = (Opts)(this->mCollectedOpts | args.OptionId());
                    break;
                }

                case OPT_VERBOSE: {
                    this->mVerbose = true;
                    this->mCollectedOpts = (Opts)(this->mCollectedOpts | args.OptionId());
                    break;
                }

                case OPT_VERSION: {
                    this->mShowVersion = true;
                    this->mCollectedOpts = (Opts)(this->mCollectedOpts | args.OptionId());
                    break;
                }

                default: {
                    this->mCollectedOpts = OPT_NONE;
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
            this->mCollectedOpts = OPT_NONE;
        }
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
    return (this->mCrcWriteAddress);
}

uint32_t ArgumentsParser::crc(void)
{
    return (this->mCrcResult);
}

const char *ArgumentsParser::inputFileName(void) const
{
    return (this->mInputFileName);
}

const char *ArgumentsParser::outputFileName(void) const
{
    return (this->mOutputFileName);
}

bool ArgumentsParser::verbose(void)
{
    return (this->mVerbose);
}

bool ArgumentsParser::showVersion()
{
    return this->mShowVersion;
}

bool ArgumentsParser::validate(logger *log)
{
    bool result = true;
    char strBuffer[2048];

    if ((this->mCollectedOpts & OPT_INPUT) == OPT_NONE) {
        log("Input file not specified");
        result = false;
    }

    if ((this->mCollectedOpts & OPT_OUTPUT) == OPT_NONE) {
        log("Output file not specified");
        result = false;
    }

    if ((this->mCollectedOpts & (OPT_CRC | OPT_CRC_AT)) == OPT_NONE) {
        log("Desired CRC not specified");
        result = false;
    }

    if (result) {
        // try open source file
        ifstream fileIn(this->mInputFileName, ios::in | ios::binary | ios::ate);

        if (fileIn.is_open()) {
            // get length
            streampos filesize = fileIn.tellg();

            // if file not empty
            if (filesize > 0) {
                // save length
                this->mFileSize = (uint32_t)filesize;

                // check file size and address
                if (result) {
                    /* [                     file content                    ]
                     * 0x0 0x1 0x2 0x3 0x4 0x5 0x6 0x7 ... n-4 n-3 n-2 n-1 n±0 n+1 n+2 n+3 n+4 ...
                     * valid, checksum in file
                     * [    CRC32    ]
                     *     [    CRC32    ]
                     *                                     [    CRC32    ]
                     *                                         [    CRC32    ]
                     * valid, checksum extends file
                     *                                             [    CRC32    ]
                     *                                                 [    CRC32    ]
                     *                                                     [    CRC32    ]
                     *                                                         [    CRC32    ]
                     * invalid, address out of file size
                     *                                                             [    CRC32    ]
                     * invalid, address limited by 32bit
                     * CRC32 ]                                                                            [ CRC32 ...
                     */

                    uint32_t address_plus_4 = this->mCrcWriteAddress + 4;

                    // check for address limited by 32 bit
                    if (address_plus_4 < 4) {
                        snprintf(strBuffer, sizeof(strBuffer), "Invalid address for CRC: from 0x%08x to 0x%08x", this->mCrcWriteAddress, address_plus_4);
                        log(strBuffer);
                        result = false;
                    }

                    // check for out of file size
                    if (result && this->mCrcWriteAddress > this->mFileSize) {
                        snprintf(strBuffer, sizeof(strBuffer), "Invalid address for CRC: 0x%08x, but last address in file is 0x%08x and max possible address is 0x%08x", this->mCrcWriteAddress, this->mFileSize - 1, this->mFileSize);
                        log(strBuffer);
                        result = false;
                    }
                }

                // if specified, read crc from position in file
                if (result && this->mCrcSource == CrcFromAddress) {
                    fileIn.seekg(this->mCrcReadAddress, ios::beg);
                    uint32_t readedCrc = 0;

                    if (fileIn.read((char *)&readedCrc, 4).gcount() == 4) {
                        this->mCrcResult = readedCrc;
                    } else {
                        snprintf(strBuffer, sizeof(strBuffer), "Cannot read input file: %s", this->mInputFileName);
                        log(strBuffer);
                        result = false;
                    }
                }

                if (result && this->mCrcSource != CrcFromAddress) {
                    fileIn.seekg(0, ios::beg);
                    char b;

                    if (fileIn.read(&b, 1).gcount() != 1) {
                        snprintf(strBuffer, sizeof(strBuffer), "Cannot read input file: %s", this->mInputFileName);
                        log(strBuffer);
                        result = false;
                    }
                }
            }

            fileIn.close();
        } else {
            snprintf(strBuffer, sizeof(strBuffer), "Cannot open input file: %s", this->mInputFileName);
            log(strBuffer);
            result = false;
        }
    }

    if (result) {
        // try open output file
        ofstream fileOut(this->mOutputFileName, ios::out | ios::binary);

        if (fileOut.is_open()) {
            fileOut.close();
        } else {
            snprintf(strBuffer, sizeof(strBuffer), "Cannot open output file: %s", this->mOutputFileName);
            log(strBuffer);
            result = false;
        }
    }

    return result;
}

bool ArgumentsParser::strtoul(const char *str, uint32_t *value) const
{
    bool result = false;
    uint32_t a;
    *value = 0;

    if ((strstr(str, "x") != NULL) || (strstr(str, "X") != NULL)) {
        if (sscanf(str, "%x", &a) == 1) {
            *value = a;
            result = true;
        }
    } else {
        if (sscanf(str, "%u", &a) == 1) {
            *value = a;
            result = true;
        }
    }

    return result;
}
