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
    
    void algin(int size = 8)
    {
        auto mode = __fs.tellg() % size;
        if (mode != 0)
        {
            for (auto i = 0; i < size - mode; i++){__fs.put(0);}
        }
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
    __fs.write((const char *)v, sizeof(T) * count);
}

template<>
void MeshFile::write(const FbxVector4 v)
{
    write<float>(static_cast<float>(v.mData[0]));
    write<float>(static_cast<float>(v.mData[1]));
    write<float>(static_cast<float>(v.mData[2]));
    write<float>(static_cast<float>(v.mData[3]));
}

template<>
void MeshFile::write(const FbxVector2 v)
{
    write<float>(static_cast<float>(v.mData[0]));
    write<float>(static_cast<float>(v.mData[1]));
}

template<>
void MeshFile::write(const FbxColor v)
{
    write<float>(static_cast<float>(v.mRed));
    write<float>(static_cast<float>(v.mGreen));
    write<float>(static_cast<float>(v.mBlue));
    write<float>(static_cast<float>(v.mAlpha));
}

#endif /* serialize_h */
