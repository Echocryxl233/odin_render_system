#pragma once

#ifndef FILEUTILITY_H
#define FILEUTILITY_H


#include <assert.h>
#include <fstream>
#include <memory>
#include <vector>

#include "debug_log.h"
//  #include <ppl.h>

namespace FileUtility {

using namespace std;

typedef unsigned char byte;


typedef shared_ptr<vector<byte>> ByteArray;

extern ByteArray NullBytes;

ByteArray LoadFile(const wstring& file_name);

};

#endif // !FILEUTILITY_H

