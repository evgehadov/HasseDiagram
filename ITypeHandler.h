#ifndef AUTOLABA_I_TYPE_HANDLER_H
#define AUTOLABA_I_TYPE_HANDLER_H

#include <string>

struct ITypeHandler {
    virtual ~ITypeHandler() = default;
    virtual std::string Name() const = 0;

    virtual void RunWithSource(int src) = 0;

    virtual void Run() { RunWithSource(/*src*/1); }
};

#endif
