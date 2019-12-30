#include "game_setting.h"



namespace GameCore {

namespace GameSetting {

using namespace std;

template<typename T>
class KeyValuePair {
public:
  void AddPair(const string& name, T value) {
    if (pairs_.find(name) == pairs_.end()) {
      pairs_[name] = move(value);
    }
  }
  bool GetValue(const string& name, T& value) {
    if (pairs_.find(name) != pairs_.end()) {
      value = pairs_[name];
      return true;
    }
    return false;
  }
private:
  map<string, T> pairs_;
};

KeyValuePair<bool> boolean_pairs_;
KeyValuePair<float> float_pairs_;
KeyValuePair<int> int_pairs_;
KeyValuePair<string> string_pairs_;

bool GetBoolValue(const string& name) {
  bool value = false;
  boolean_pairs_.GetValue(name, value);
  return value;
}

float GetFloatValue(const string& name) {
  float value = 0.0f;
  float_pairs_.GetValue(name, value);
  return value;
}

int GetIntValue(const string& name) {
  int value = 0;
  int_pairs_.GetValue(name, value);
  return value;
}

string GetStringValue(const string& name) {
  string value;
  string_pairs_.GetValue(name, value);
  return value;
}

void LoadConfig(const string& filename) {

  ifstream fin(filename);
  if (!fin)
  {
    wstring box_msg = DebugUtility::AnsiToWString(filename);
    box_msg += L" not found.";
    MessageBox(0, box_msg.c_str(), 0, 0);
    return;
  }

  string value_line;
  istringstream iss_line;
  string name;
  string type;
  int value_int;
  float value_float;
  string value_string;
  while (std::getline(fin, value_line)) {
    iss_line.str(value_line);
    iss_line >> type >> name;

    if (type == "Bool") {
      iss_line >> value_int;
      boolean_pairs_.AddPair(name, value_int);
    }
    else if (type == "Float") {

      iss_line >> value_float;
      float_pairs_.AddPair(name, value_float);
    }
    else if (type == "Int") {
      iss_line >> value_int;
      int_pairs_.AddPair(name, value_int);
    }
    else if (type == "String") {
      iss_line >> value_string;
      string_pairs_.AddPair(name, value_string);
    }

    type.clear();
    name.clear();
    iss_line.clear();
  }
  fin.close();
}

};

};