// нужно, для умного вывода всех типов в консоли
#ifndef AUTOLABA_TYPEREGISTRY_H
#define AUTOLABA_TYPEREGISTRY_H
#include <functional>
#include <memory>
#include <string>

struct ITypeHandler;

struct TypeEntry {
    std::string name;
    std::function<std::unique_ptr<ITypeHandler>()> factory;
};
#endif //AUTOLABA_TYPEREGISTRY_H
