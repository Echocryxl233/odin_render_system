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

//template<typename ... TArgs>
//void Log(wstring format, TArgs... args) {
//
//  wstring arr[] = { (wstring(args))... };
//  int index = 0;
//
//  for (auto s : arr) {
//    wstring rep(L"%");
//    rep += std::to_wstring(index);
//
//    size_t pos = format.find(rep);
//    if (wstring::npos == pos)
//      break;
//
//    format += std::to_wstring(__LINE__);
//
//    format = format.replace(pos, rep.length(), s);
//    ++index;
//  }
//  //  format += L"\n";
//  format += Location();
//  OutputDebugString(format.c_str());
//}

void Log(wstring format) {
  //wstring output;
  //output += format;
  format += Location();

  OutputDebugString(format.c_str());
}

};