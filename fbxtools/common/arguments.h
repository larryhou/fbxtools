//
//  arguments.h
//  fbxtools
//
//  Created by LARRYHOU on 2020/2/21.
//  Copyright © 2020 LARRYHOU. All rights reserved.
//

#ifndef arguments_h
#define arguments_h

#include <sstream>
#include <string>
#include <map>

enum TerminalFilter
{
    none = 0, error, info, check, debug
};

struct ArgumentOptions
{
    std::string filename;
    std::map<std::string, std::string> data;
    TerminalFilter filter = debug;
    
    ArgumentOptions(std::string file)
    {
        filename = file;
        auto iter = file.begin();
        while (iter != file.end())
        {
            if (*iter == '?')
            {
                filename = std::string(file.begin(), iter);
                
                filter = ::info;
                std::string parameters(iter + 1, file.end());
                
                std::string option;
                std::stringstream stream(parameters);
                while (getline(stream, option, '&'))
                {
                    auto pos = option.find('=');
                    if (pos == std::string::npos)
                    {
                        data.insert(std::make_pair(option, std::string()));
                    }
                    else
                    {
                        auto label = option.substr(0, pos);
                        auto value = option.substr(pos + 1);
                        data.insert(std::make_pair(label, value));
                    }
                }
                break;
            }
            ++iter;
        }
    }
    
    virtual bool get(std::string key)
    {
        auto iter = data.find(key);
        if (iter == data.end()) { return false; }
        auto &v = iter->second;
        if (v.empty()) { return true; }
        return atoi(v.c_str()) != 0;
    }
    
    virtual bool get(std::string key, std::string &value)
    {
        auto iter = data.find(key);
        if (iter == data.end()) { return false; }
        value = iter->second;
        return true;
    }
    
    virtual void print(TerminalFilter filter, std::function<void()> closure)
    {
        if (filter == this->filter) { closure(); }
    }
};

#endif /* arguments_h */
