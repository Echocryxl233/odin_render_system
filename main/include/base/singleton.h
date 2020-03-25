#pragma once

#ifndef SINGLETON_H
#define SINGLETON_H

//namespace Utility {

template<typename T>
class Singleton {

template<typename T>
friend class Singleton;

public:
  static T& Instance() {
    static T instance;
    return instance;
  }
};

//};



#endif // !SINGLETON_H

