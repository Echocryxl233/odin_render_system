#include "file_utility.h"

using namespace std;
using namespace FileUtility;

namespace FileUtility {
ByteArray NullBytes = make_shared<vector<byte>>(vector<byte>());
};

ByteArray FileUtility::LoadFile(const wstring& file_name) {
  ifstream file(file_name, ios::in | ios::binary );
  if (!file) {
    DebugUtility::Log(L"Can not open file : %0", file_name);
    return NullBytes;
  }
  
  size_t byte_size = file.seekg(0, ios::end).tellg();
  ByteArray byte_stream= make_shared<vector<byte>>(byte_size);

  file.seekg(0, ios::beg).read((char*)byte_stream->data(), byte_stream->size());
  
  file.close();
  
  //  assert(byte_stream->size() == byte_size);
  
  return byte_stream;
}