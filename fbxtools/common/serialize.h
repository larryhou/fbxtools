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

struct Transform
{
    FbxVector3 position;
    FbxQuaternion rotation;
    FbxVector3 scale;
};

struct Skeleton
{
    std::vector<int32_t> nodes;
    std::vector<std::string> names;
    std::vector<Transform> poses;
    std::vector<int32_t> bones;
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
    
    template<typename T> T read() { T v; read(v); return v; }
    template<typename T> void read(T &v);
    template<typename T> void write(const T &v);

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
    
    template<typename T>
    void read_vector(std::vector<T> &v)
    {
        auto count = read<uint32_t>();
        
        v.resize(count);
        for (auto i = 0; i < count; i++) { v[i] = read<T>(); }
    }
    
    template<typename T>
    void write_vector(const std::vector<T> &v)
    {
        write(static_cast<uint32_t>(v.size()));
        for (auto iter = v.begin(); iter != v.end(); iter++)
        {
            write(*iter);
        }
    }
    
    ~FileStream() { __fs.close();  }
};

template<typename T>
void FileStream::write(const T &v)
{
    __fs.write((const char *)&v, sizeof(T));
}

template<typename T>
void FileStream::read(T &v)
{
    __fs.read((char *)&v, sizeof(T));
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
void FileStream::write(const std::string &v)
{
    write(static_cast<uint32_t>(v.size()));
    __fs.write(v.c_str(), v.size());
}

template<>
void FileStream::read(std::string &s)
{
    auto size = read<uint32_t>();
    s.resize(size);
    __fs.read(const_cast<char*>(s.data()), size);
}

template<>
void FileStream::write(const FbxDouble4 &v)
{
    write<float>(static_cast<float>(v.mData[0]));
    write<float>(static_cast<float>(v.mData[1]));
    write<float>(static_cast<float>(v.mData[2]));
    write<float>(static_cast<float>(v.mData[3]));
}

template<>
void FileStream::read(FbxDouble4 &v)
{
    v.mData[0] = read<float>();
    v.mData[1] = read<float>();
    v.mData[2] = read<float>();
    v.mData[3] = read<float>();
}

template<>
void FileStream::write(const FbxVector4 &v)
{
    write<float>(static_cast<float>(v.mData[0]));
    write<float>(static_cast<float>(v.mData[1]));
    write<float>(static_cast<float>(v.mData[2]));
    write<float>(static_cast<float>(v.mData[3]));
}

template<>
void FileStream::read(FbxVector4 &v)
{
    v.mData[0] = read<float>();
    v.mData[1] = read<float>();
    v.mData[2] = read<float>();
    v.mData[3] = read<float>();
}

template<>
void FileStream::write(const FbxQuaternion &v)
{
    write<float>(static_cast<float>(v.mData[0]));
    write<float>(static_cast<float>(v.mData[1]));
    write<float>(static_cast<float>(v.mData[2]));
    write<float>(static_cast<float>(v.mData[3]));
}

template<>
void FileStream::read(FbxQuaternion &v)
{
    v.mData[0] = read<float>();
    v.mData[1] = read<float>();
    v.mData[2] = read<float>();
    v.mData[3] = read<float>();
}

template<>
void FileStream::write(const FbxDouble3 &v)
{
    write<float>(static_cast<float>(v.mData[0]));
    write<float>(static_cast<float>(v.mData[1]));
    write<float>(static_cast<float>(v.mData[2]));
}

template<>
void FileStream::read(FbxDouble3 &v)
{
    v.mData[0] = read<float>();
    v.mData[1] = read<float>();
    v.mData[2] = read<float>();
}

template<>
void FileStream::write(const FbxVector3 &v)
{
    write<float>(static_cast<float>(v.mData[0]));
    write<float>(static_cast<float>(v.mData[1]));
    write<float>(static_cast<float>(v.mData[2]));
}

template<>
void FileStream::read(FbxVector3 &v)
{
    v.mData[0] = read<float>();
    v.mData[1] = read<float>();
    v.mData[2] = read<float>();
}

template<>
void FileStream::write(const FbxDouble2 &v)
{
    write<float>(static_cast<float>(v.mData[0]));
    write<float>(static_cast<float>(v.mData[1]));
}

template<>
void FileStream::read(FbxDouble2 &v)
{
    v.mData[0] = read<float>();
    v.mData[1] = read<float>();
}

template<>
void FileStream::write(const FbxVector2 &v)
{
    write<float>(static_cast<float>(v.mData[0]));
    write<float>(static_cast<float>(v.mData[1]));
}

template<>
void FileStream::read(FbxVector2 &v)
{
    v.mData[0] = read<float>();
    v.mData[1] = read<float>();
}

template<>
void FileStream::write(const Transform &v)
{
    write(v.position);
    write(v.rotation);
    write(v.scale);
}

template<>
void FileStream::read(Transform &v)
{
    read(v.position);
    read(v.rotation);
    read(v.scale);
}

template<>
void FileStream::write(const Skeleton &v)
{
    write_vector(v.nodes);
    write_vector(v.names);
    write_vector(v.poses);
    write_vector(v.bones);
}

template<>
void FileStream::read(Skeleton &v)
{
    read_vector(v.nodes);
    read_vector(v.names);
    read_vector(v.poses);
    read_vector(v.bones);
}

template<>
void FileStream::write(const FbxMatrix &m)
{
    auto ptr = (FbxDouble *)&m;
    for (auto i = 0; i < 16; i++) { write(static_cast<float>(*ptr++)); }
}

template<>
void FileStream::read(FbxMatrix &m)
{
    auto ptr = (FbxDouble *)&m;
    for (auto i = 0; i < 16; i++) { *ptr++ = read<float>(); }
}

template<>
void FileStream::write(const FbxAMatrix &m)
{
    auto ptr = (FbxDouble *)&m;
    for (auto i = 0; i < 16; i++)
    {
        if (i == 1 || i == 2 || i == 4 || i == 8 || i == 12)
        {
            write(static_cast<float>(-*ptr++));
        }
        else
        {
            write(static_cast<float>(*ptr++));
        }
    }
}

template<>
void FileStream::read(FbxAMatrix &m)
{
    auto ptr = (FbxDouble *)&m;
    for (auto i = 0; i < 16; i++)
    {
        if (i == 1 || i == 2 || i == 4 || i == 8 || i == 12)
        {
            *ptr++ = -read<float>();
        }
        else
        {
            *ptr++ = read<float>();
        }
    }
}

template<>
void FileStream::write(const FbxColor &v)
{
    write<float>(static_cast<float>(v.mRed));
    write<float>(static_cast<float>(v.mGreen));
    write<float>(static_cast<float>(v.mBlue));
    write<float>(static_cast<float>(v.mAlpha));
}

template<>
void FileStream::read(FbxColor &v)
{
    v.mRed = read<float>();
    v.mGreen = read<float>();
    v.mBlue = read<float>();
    v.mAlpha = read<float>();
}

#endif /* serialize_h */
