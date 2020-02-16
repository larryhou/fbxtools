//
//  main.cpp
//  fbxqs
//
//  Created by LARRYHOU on 2020/2/15.
//  Copyright © 2020 LARRYHOU. All rights reserved.
//

#include <iostream>
#include <fbxsdk.h>
#include <fbxsdk/fileio/fbxiosettings.h>
#include <fbxsdk/core/fbxdatatypes.h>
#include <fbxsdk/scene/geometry/fbxlayer.h>
#include <string>
#include <sstream>
#include <fstream>
#include <map>

#include "serialize.h"

struct MeshStatistics
{
    int vertices;
    int polygons;
    int triangles;
    int wastes;
    
    MeshStatistics(int v, int p, int t, int w): vertices(v), polygons(p), triangles(t), wastes(w) {}
    MeshStatistics(): MeshStatistics(0, 0, 0, 0) {}
    
    MeshStatistics operator+(MeshStatistics v)
    {
        return MeshStatistics(vertices + v.vertices, polygons + v.polygons, triangles + v.triangles, wastes + v.wastes);
    }
    
    MeshStatistics& operator+=(MeshStatistics v)
    {
        vertices += v.vertices;
        polygons += v.polygons;
        triangles += v.triangles;
        wastes += v.wastes;
        return *this;
    }
};

enum TerminalFilter
{
    none = 0, error, info, check, debug
};

struct FileOptions
{
    std::string filename;
    std::map<std::string, std::string> data;
    TerminalFilter filter = debug;
    bool mesh;
    bool texture;
    bool check;
    
    FileOptions(std::string file)
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
        
        mesh = get("mesh");
        texture = get("texture");
        if (get("debug")) { filter = ::debug; }
        if (get("error")) { filter = ::error; }
        check = get("check");
        if (check) { filter = ::check; }
    }
    
    bool get(std::string key)
    {
        auto iter = data.find(key);
        if (iter == data.end()) { return false; }
        auto &v = iter->second;
        if (v.empty()) { return true; }
        return atoi(v.c_str()) != 0;
    }
    
    bool get(std::string key, std::string &value)
    {
        auto iter = data.find(key);
        if (iter == data.end()) { return false; }
        value = iter->second;
        return true;
    }
    
    void print(TerminalFilter filter, std::function<void()> closure)
    {
        if (filter == this->filter) { closure(); }
    }
};



template<typename T>
void encode(FbxLayerElementTemplate<T> *element, MeshFile &fs)
{
    FbxLayerElementArrayTemplate<T> &data = element->GetDirectArray();
    fs.write<char>('d');
    for (auto i = 0; i < data.GetCount(); i++)
    {
        fs.write<T>(data.GetAt(i));
    }
    
    auto flag = element->GetReferenceMode() == fbxsdk::FbxLayerElement::eIndexToDirect;
    fs.write<bool>(flag); // index mapping
    if (flag)
    {
        FbxLayerElementArrayTemplate<int> &indice = element->GetIndexArray();
        fs.write('i');
        for (auto i = 0; i < indice.GetCount(); i++)
        {
            fs.write<int>(indice.GetAt(i));
        }
    }
}

#include <sys/stat.h>

void exportMesh(FbxMesh *mesh, FileOptions &fo)
{
    auto pos = fo.filename.rfind('.');
    std::string workspace = fo.filename.substr(0, pos) + ".fbm";
    mkdir(workspace.c_str(), 0777);
    
    std::string filename = workspace + "/" + mesh->GetName() + ".mesh";
    MeshFile fs(filename.c_str());
    fs.write('M');
    fs.write('E');
    fs.write('S');
    fs.write('H');
    // vertices
    fs.write('V');
    auto numVertices = mesh->GetControlPointsCount();
    fs.write<int>(numVertices);
    fs.write<FbxVector4>(mesh->GetControlPoints(), numVertices);
    
    // triangles
    fs.write('T');
    auto offset = fs.tell();
    fs.write<int>(0); // triangle count
    auto numTriangles = 0;
    for (auto i = 0; i < mesh->GetPolygonCount(); i++)
    {
        auto size = mesh->GetPolygonSize(i);
        for (auto t = 0; t < size - 2; t++) // auto split polygons with more than 3 vertices
        {
            ++numTriangles;
            for (auto n = 0; n < 3; n++) // write single triangle
            {
                auto index = mesh->GetPolygonVertex(i, t + n);
                fs.write<int>(index);
            }
        }
    }
    
    // write real triangle count
    auto position = fs.tell();
    fs.seek(offset, std::fstream::beg);
    fs.write<int>(numTriangles);
    fs.seek(position, std::fstream::beg); // restore cursor
    
    // encode normals
    auto layer = mesh->GetLayer(0);
    auto normals = layer->GetNormals();
    if (normals != NULL)
    {
        fs.write<char>('N');
        encode<FbxVector4>(normals, fs);
    }
    
    // encode uvmapping
    auto uvs = layer->GetUVs();
    if (uvs != NULL)
    {
        fs.write<char>('U');
        encode<FbxVector2>(uvs, fs);
    }
}

MeshStatistics dumpNodeHierarchy(FbxNode* node, FileOptions &fo, std::string indent = "")
{
    MeshStatistics stat;
    auto numChildren = node->GetChildCount();
    for (auto i = 0; i < numChildren; i++)
    {
        auto closed = numChildren == i + 1;
        auto child = node->GetChild(i);
        auto attribute = child->GetNodeAttribute();
        auto type = attribute->GetAttributeType();
        std::string name = "Unknown";
        auto isNull = false;
        auto isMesh = false;
        switch (type)
        {
            case fbxsdk::FbxNodeAttribute::eNull:
                name = "Null";
                isNull = true;
                break;
            case fbxsdk::FbxNodeAttribute::eMarker:name = "Marker";break;
            case fbxsdk::FbxNodeAttribute::eSkeleton:name = "Skeleton";break;
            case fbxsdk::FbxNodeAttribute::eMesh:
                name = "Mesh";
                isMesh = true;
                break;
            case fbxsdk::FbxNodeAttribute::eNurbs:name = "Nurbs";break;
            case fbxsdk::FbxNodeAttribute::ePatch:name = "Patch";break;
            case fbxsdk::FbxNodeAttribute::eCamera:name = "Camera";break;
            case fbxsdk::FbxNodeAttribute::eCameraStereo:name = "CameraStereo";break;
            case fbxsdk::FbxNodeAttribute::eCameraSwitcher:name = "CameraSwitcher";break;
            case fbxsdk::FbxNodeAttribute::eLight:name = "Light";break;
            case fbxsdk::FbxNodeAttribute::eOpticalReference:name = "OpticalReference";break;
            case fbxsdk::FbxNodeAttribute::eOpticalMarker:name = "OpticalMarker";break;
            case fbxsdk::FbxNodeAttribute::eNurbsCurve:name = "NurbsCurve";break;
            case fbxsdk::FbxNodeAttribute::eTrimNurbsSurface:name = "TrimNurbsSurface";break;
            case fbxsdk::FbxNodeAttribute::eBoundary:name = "Boundary";break;
            case fbxsdk::FbxNodeAttribute::eNurbsSurface:name = "NurbsSurface";break;
            case fbxsdk::FbxNodeAttribute::eShape:name = "Shape";break;
            case fbxsdk::FbxNodeAttribute::eLODGroup:name = "LODGroup";break;
            case fbxsdk::FbxNodeAttribute::eSubDiv:name = "SubDiv";break;
            case fbxsdk::FbxNodeAttribute::eCachedEffect:name = "CachedEffect";break;
            case fbxsdk::FbxNodeAttribute::eLine:name = "Line";break;
            default:break;
        }
        fo.print(debug, [&]{
            printf("%s%s─\e[4m%s\e[0m \e[%dm%s\e[0m", indent.c_str(), closed ? "└" : "├", name.c_str(), isNull ? 96:33,  child->GetName());
        });

        if (isMesh)
        {
            auto mesh = static_cast<FbxMesh *>(attribute);
            auto polygonCount = mesh->GetPolygonCount();
            auto triangleCount = 0;
            auto usedVertexCount = 0;
            for (auto p = 0; p < polygonCount; p++)
            {
                auto size = mesh->GetPolygonSize(p);
                triangleCount += size - 2;
                usedVertexCount += size;
            }
            auto wastes = mesh->GetPolygonVertexCount() - usedVertexCount;
            stat.vertices += mesh->GetControlPointsCount();
            stat.polygons += polygonCount;
            stat.triangles += triangleCount;
            stat.wastes += wastes;
            fo.print(debug, [&]{
                printf(" vertices=%d polygons=%d triangles=%d", mesh->GetPolygonVertexCount(), polygonCount, triangleCount);
            });
            
            if (fo.mesh) { exportMesh(mesh, fo); }
        }
        fo.print(debug, [&]{printf("\n");});
        stat += dumpNodeHierarchy(child, fo, indent + (closed ? " " : "│") + "  ");
    }
    
    return stat;
}

bool process(FileOptions &fo, FbxManager *pManager, MeshStatistics &statistics)
{
    auto importer = FbxImporter::Create(pManager, "");
    if (!importer->Initialize(fo.filename.c_str(), -1, pManager->GetIOSettings()))
    {
        fo.print(error, [&]{
            printf("Call to FbxImporter::Intialize() failed.\n");
            printf("Error returned: %s \n", importer->GetStatus().GetErrorString());
        });
        return false;
    }
    
    auto pScene = FbxScene::Create(pManager, "Scene");
    
    if (!importer->Import(pScene))
    {
        fo.print(error, [&]{
            printf("%s\n", importer->GetStatus().GetErrorString());
        });
        return false;
    }
    
    importer->Destroy();
    
    auto numStacks = pScene->GetSrcObjectCount<FbxAnimStack>();
    for (auto i = 0; i < numStacks; i++)
    {
        auto pAnimStack = pScene->GetSrcObject<FbxAnimStack>(i);
        fo.print(debug, [&]{
            printf("[Animation][%d/%d] %s\n", i + 1, numStacks, pAnimStack->GetName());
        });
    }
    
    auto stat = dumpNodeHierarchy(pScene->GetRootNode(), fo);
    fo.print(debug, [&]{
        printf("# vertices=%d polygons=%d triangles=%d\n", stat.vertices, stat.polygons, stat.triangles);
    });
    
    fo.print(check, [&]{
        printf("%s %d %d %d\n", fo.filename.c_str(), stat.vertices, stat.polygons, stat.triangles);
    });
    
    statistics += stat;
    
    pScene->Destroy();
    
    return true;
}

int main(int argc, const char * argv[])
{
    if (argc <= 1) {return 1;}
    
    FbxManager* pManager = FbxManager::Create();
    pManager->SetIOSettings(FbxIOSettings::Create(pManager, IOSROOT));
    
    MeshStatistics statistics;
    for (auto i = 1; i < argc; i++)
    {
        FileOptions fo(argv[i]);
        fo.print(debug, [&]{printf("[%d/%d] %s\n", i, argc - 1, argv[i]);});
        process(fo, pManager, statistics);
        fo.print(debug, [&]{printf("\n");});
    }
    
    if (argc > 2)
    {
        printf("[] vertices=%d polygons=%d triangles=%d\n", statistics.vertices, statistics.polygons, statistics.triangles);
    }
    
    pManager->Destroy();
    
    return 0;
}
