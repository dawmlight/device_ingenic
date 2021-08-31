#ifndef TYPE_H_
#define TYPE_H_

#include <stdio.h>
static const int CHANNEL_CMD = 0x00;
static const int CHANNEL_SPEICAL = 0x01;
static const int CHANNEL_DATA = 0x2;
static const int CHANNEL_FILE = 0x3;
//static const string CHINESE_CODE = "UTF-8";

//static const int8_t NULL = -1;
static const int8_t BOOL = 0x0;
static const char BYTE = 0x1;
static const int8_t SHORT = 0x2;
static const int8_t INT = 0x3;
static const int8_t LONG = 0x4;
static const int8_t FLOAT = 0x5;
static const int8_t DOUBLE = 0x6;
//	static const int8_t CHAR = 0x7;
static const int8_t STRING = 0x8;
static const int8_t BOOLEAN_ARY = 0x9;
static const int8_t BYTE_ARY = 0xa;
static const int8_t SHORT_ARY = 0xb;
static const int8_t INT_ARY = 0xc;
static const int8_t LONG_ARY = 0xd;
static const int8_t FLOAT_ARY = 0xe;
static const int8_t DOUBLE_ARY = 0xf;
static const int8_t CHAR_ARY = 0x10;
static const int8_t STRING_ARY = 0x11;
static const int8_t SYNCDATA_ARY = 0x12;

static const int INT_LENGTH = 4;
  // static const int CHAR_LENGTH=2;
static const int BOOLEAN_LENGTH = 1;
static const int LONG_LENGTH = 8;
static const int SHORT_LENGTH = 2;
static const int FLOAT_LENGTH = 4;
static const int DOUBLE_LENGTH = 8;
static const int BYTE_LENGTH = 1;

#endif  // TYPE_H_
