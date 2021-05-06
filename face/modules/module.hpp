#ifndef FACE_MODULE_HPP_
#define FACE_MODULE_HPP_

#include<string>
#include<iostream>
#include<unordered_map>
#include<functional>


#include "../kernel/include/common.hpp"


class Module
{
public:
    virtual bool init() = 0;
    virtual bool process() = 0;
    virtual bool provideData() = 0;
};

class ModuleFactory
{
public:
    static ModuleFactory *getInstance()
    {
        if (factory_ != nullptr)
        {
            factory_ = new (std::nothrow) ModuleFactory();
        }
        return (factory_);
    }
    virtual ~ModuleFactory() {}

    bool register(std::string typeName, std::function<Module *<const std::string &>> func)
    {
        if (func == nullptr)
        {
            return false;
        }
        bool ret = map_.insert(std::make_pair(typeName, func)).second;
        return ret;
    }

    Module *create(const std::string &typeName, const std::string name)
    {
        auto iter = map_.find(typeName);
        if (iter == map_.end())
        {
            return nullptr;
        }
        return iter->second(name);
    }

    std::vector<std::string> getRegisted()
    {
        std::vector<std::string> res;
        for (auto &iter : map_)
        {
            res.push_back(iter.first);
        }
        return res;
    }

private:
    ModuleFactory() {};
    static ModuleFactory *factory_;
    static std::unordered_map<std::string, std::function<Module *(const std::string &)>> map_;
};

template <typename T>
class ModuleCreator
{
    struct Register
    {
        Register()
        {
            char *class_name = nullptr;
            std::string typeName;
#ifdef __GNUC__
            szDemangleName = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#else
            // in this format?:     szDemangleName =  typeid(T).name();
            szDemangleName = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#endif
            if (nullptr != class_name)
            {
                typeName = class_name;
                free(class_name);
            }
            ModuleFactory::getInstance()->
        }
    }
};

#endif // FACE_MODULE_HPP_