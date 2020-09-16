//
//  serialize.hpp
//  fbxdump
//
//  Created by LARRYHOU on 2020/2/16.
//  Copyright © 2020 LARRYHOU. All rights reserved.
//

#ifndef serialize_h
#define serialize_h

#include <fstream>
#include <fbxsdk.h>
#include <fbxsdk/core/fbxdatatypes.h>
#include <vector>

using seek_dir = std::ios_base::seek_dir;

struct FBXSDK_DLL FbxVector3 : public FbxDouble3
{
    operator FbxVector4() const
    {
        FbxVector4 v;
        memcpy(v.mData, mData, sizeof(mData));
        return v;
    }
};

class FileStream
{
    std::fstream __fs;
public:
    FileStream(const char *filename): FileStream(filename, std::fstream::out) {}
    FileStream(const char *filename, std::ios_base::openmode mode)
    {
        __fs.open(filename, mode);
    }
    
    bool good() const { return __fs.good(); }
    
    std::fstream::pos_type tellg() { return __fs.tellg(); }
    void seek(std::fstream::pos_type pos, seek_dir whence)
    {
        __fs.seekg(pos, whence);
    }
    
    void alginp(int size = 8)
    {
        auto mode = __fs.tellg() % size;
        if (mode != 0)
        {
            for (auto i = 0; i < size - mode; i++){__fs.put(0);}
        }
    }
    
    template<typename T> void write(const T v);
    template<typename T> T read();

    template<typename T> void write(const T *v, size_t count);
    template<typename T> void read(T *v, size_t count);
    
    template<typename T>
    std::vector<T> read_vector()
    {
        auto count = read<uint32_t>();
        std::vector<T> data;
        data.reserve(count);
        for (auto i = 0; i < count; i++) { data.push_back(read<T>()); }
        return data;
    }
    
    ~FileStream() { __fs.close();  }
};

template<typename T>
void FileStream::write(const T v)
{
    __fs.write((const char *)&v, sizeof(T));
}

template<typename T>
T FileStream::read()
{
    T v;
    __fs.read((char *)&v, sizeof(T));
    return v;
}

template<typename T>
void FileStream::write(const T *v, size_t count)
{
    __fs.write((const char *)v, sizeof(T) * count);
}

template<typename T>
void FileStream::read(T *v, size_t count)
{
    __fs.read((char *)v, sizeof(T) * count);
}

template<>
void FileStream::write(const char *v, size_t count)
{
    __fs.write(v, count);
}

template<>
void FileStream::read(char *v, size_t count)
{
    __fs.read(v, count);
}

template<>
void FileStream::write(const std::string v)
{
    write(static_cast<uint32_t>(v.size()));
    __fs.write(v.c_str(), v.size());
}

template<>
std::string FileStream::read()
{
    auto size = read<uint32_t>();
    std::string s(size, ' ');
    __fs.read(const_cast<char*>(s.data()), size);
    return s;
}

template<>
void FileStream::write(const FbxVector4 v)
{
    write<float>(static_cast<float>(v.mData[0]));
    write<float>(static_cast<float>(v.mData[1]));
    write<float>(static_cast<float>(v.mData[2]));
    write<float>(static_cast<float>(v.mData[3]));
}

template<>
FbxVector4 FileStream::read()
{
    FbxVector4 v;
    v.mData[0] = read<float>();
    v.mData[1] = read<float>();
    v.mData[2] = read<float>();
    v.mData[3] = read<float>();
    return v;
}

template<>
void FileStream::write(const FbxVector3 v)
{
    write<float>(static_cast<float>(v.mData[0]));
    write<float>(static_cast<float>(v.mData[1]));
    write<float>(static_cast<float>(v.mData[2]));
}

template<>
FbxVector3 FileStream::read()
{
    FbxVector3 v;
    v.mData[0] = read<float>();
    v.mData[1] = read<float>();
    v.mData[2] = read<float>();
    return v;
}

template<>
void FileStream::write(const FbxVector2 v)
{
    write<float>(static_cast<float>(v.mData[0]));
    write<float>(static_cast<float>(v.mData[1]));
}

template<>
FbxVector2 FileStream::read()
{
    FbxVector2 v;
    v.mData[0] = read<float>();
    v.mData[1] = read<float>();
    return v;
}

template<>
void FileStream::write(const FbxMatrix v)
{
    write(v.GetRow(0));
    write(v.GetRow(1));
    write(v.GetRow(2));
    write(v.GetRow(3));
}

template<>
FbxMatrix FileStream::read()
{
    FbxMatrix v;
    v.SetRow(0, read<FbxVector4>());
    v.SetRow(1, read<FbxVector4>());
    v.SetRow(2, read<FbxVector4>());
    v.SetRow(3, read<FbxVector4>());
    return v;
}

template<>
void FileStream::write(const FbxColor v)
{
    write<float>(static_cast<float>(v.mRed));
    write<float>(static_cast<float>(v.mGreen));
    write<float>(static_cast<float>(v.mBlue));
    write<float>(static_cast<float>(v.mAlpha));
}

template<>
FbxColor FileStream::read()
{
    FbxColor v;
    v.mRed = read<float>();
    v.mGreen = read<float>();
    v.mBlue = read<float>();
    v.mAlpha = read<float>();
    return v;
}

#endif /* serialize_h */
