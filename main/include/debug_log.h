#pragma once

#ifndef DEBUGLOG_H
#define DEBUGLOG_H

#include <string>
#include <Windows.h>

using namespace std;

namespace DebugUtility {

std::wstring AnsiToWString(const std::string& str);

std::wstring Location();
//std::wstring Location() {
//  wstring output;
//  output += L"\t\t(From \t" + AnsiToWString(__FILE__);
//  output += L" , line ";
//  output += to_wstring(__LINE__);
//  output += L")\n";
//  return output;
//}

template<typename ... TArgs>
string Format(string format, TArgs... args) {
  string arr[] = { (string(args))... };

  int index = 0;

  for (auto s : arr) {
    string rep("%");
    rep += std::to_string(index);

    size_t pos = format.find(rep);
    if (string::npos == pos)
      break;

    format = format.replace(pos, rep.length(), s);
    ++index;
  }
  return format;
}

template <typename T, typename... Args>
void FormatInternal(std::string& format, int index, T first, Args... args) {
  std::string rep("%");
  rep += std::to_string(index);
  size_t pos = format.find(rep);

  std::string format_arg;

  if constexpr (std::is_same_v<std::string, T>) {
    format_arg = first;
  }
  else {
    format_arg = std::to_string(first);
  }
  if (std::string::npos != pos) {
    format = format.replace(pos, rep.length(), format_arg);
  }

  if constexpr ((sizeof...(args)) > 0) {
    FormatInternal(format, index + 1, std::forward<Args>(args)...);
  }
}

template<typename... Args>
string FormatT(std::string format, Args... args) {

  FormatInternal(format, 0, std::forward<Args>(args)...);
  return format;
}


template<typename ... TArgs>
void Log(wstring format, TArgs... args) {

  wstring arr[] = { (wstring(args))... };
  //wstring arr[] = { (to_wstring(args))... };
  int index = 0;

  for (auto s : arr) {
    wstring rep(L"%");
    rep += std::to_wstring(index);

    size_t pos = format.find(rep);
    if (wstring::npos == pos)
      break;

// format += std::to_wstring(__LINE__);

    format = format.replace(pos, rep.length(), s);
    ++index;
  }
  //  format += L"\n";
  format += Location();
  OutputDebugString(format.c_str());
}
    
extern void Log(wstring format);
};


class DebugLog {
public:

  //template<typename ... TArgs>
  //static void Print(string format, TArgs... args);

  template<typename ... TArgs>
  static void Print(wstring format, TArgs... args);

  static void Print(wstring format);

  static std::wstring AnsiToWString(const std::string& str)
  {
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
  }

private:

};

//template<typename ... TArgs>
//inline void DebugLog::Print(string format, TArgs... args) {
//
//  string arr[] = {(string(args))...};  
//  int index = 0;
//
//  for (auto s : arr) {
//    string rep("%");
//    rep += std::to_string(index);  
//
//    size_t pos = format.find(rep);
//    if (string::npos == pos)
//      break;
//
//    format = format.replace(pos, rep.length(), s);
//    ++index;
//  }
//
//  OutputDebugString(format.c_str());
//}

template<typename ... TArgs>
inline void DebugLog::Print(wstring format, TArgs... args) {
  
  wstring arr[] = {(wstring(args))...};  
  int index = 0;

  for (auto s : arr) {
    wstring rep(L"%");
    rep += std::to_wstring(index);  

    size_t pos = format.find(rep);
    if (wstring::npos == pos)
      break;

    format += std::to_wstring(__LINE__);

    format = format.replace(pos, rep.length(), s);
    ++index;
  }
  format += L"\n";

  OutputDebugString(format.c_str());
}

inline void DebugLog::Print(wstring format) {
  wstring output = AnsiToWString(__FILE__);
  output += to_wstring(__LINE__);
  output += format;
  output += L"\n";

  OutputDebugString(output.c_str());
}

#endif // !DEBUGLOG_H