//
//  serialize.hpp
//  fbxdump
//
//  Created by LARRYHOU on 2020/2/16.
//  Copyright © 2020 LARRYHOU. All rights reserved.
//

#ifndef serialize_h
#define serialize_h

#include <stdio.h>
#include <fstream>

class MeshFile
{
    std::fstream __fs;
public:
    MeshFile(const char *filename)
    {
        __fs.open(filename, std::fstream::out);
    }
    
    std::fstream::pos_type tell() { return __fs.tellg(); }
    void seek(std::fstream::pos_type pos, std::ios_base::seek_dir whence)
    {
        __fs.seekg(pos, whence);
    }
    
    template<typename T>
    void write(const T v);

    template<typename T>
    void write(const T *v, int count);
    
    ~MeshFile() { __fs.close();  }
};

template<typename T>
void MeshFile::write(const T v)
{
    __fs.write((const char *)&v, sizeof(T));
}

template<typename T>
void MeshFile::write(const T *v, int count)
{
    __fs.write((const char *)v, sizeof(v) * count);
}

#endif /* serialize_h */
