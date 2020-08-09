#include "debug_log.h"

namespace DebugUtility {

std::wstring Location() {
  wstring output;
  output += L"\t\t(From \t" + AnsiToWString(__FILE__);
  output += L" , line ";
  //  output += to_wstring(__LINE__);
  output += L")\n";
  return output;
}

std::wstring AnsiToWString(const std::string& str)
{
  WCHAR buffer[512];
  MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
  return std::wstring(buffer);
}





//template <typename... Args>
//inline std::string FormatT(std::string format, Args... args) {
//  FormatInternal(format, std::forward<Args>(args)...);
//  return format;
//}

void Log(wstring format) {
  //wstring output;
  //output += format;
  format += Location();

  OutputDebugString(format.c_str());
}

};