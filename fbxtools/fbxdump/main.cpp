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
#include <vector>
#include <map>

#include "../common/arguments.h"
#include "../common/serialize.h"

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

struct FileOptions: public ArgumentOptions
{
    bool mesh;
    bool texture;
    bool check;
    bool obj;
    
    FileOptions(std::string file): ArgumentOptions(file)
    {
        obj = get("obj");
        mesh = get("mesh");
        texture = get("texture");
        if (get("debug")) { filter = ::debug; }
        if (get("error")) { filter = ::error; }
        check = get("check");
        if (check) { filter = ::check; }
    }
};


template<typename T>
void encode(FbxLayerElementTemplate<T> *element, MeshStream &fs)
{
    FbxLayerElementArrayTemplate<T> &data = element->GetDirectArray();
    fs.write<char>('d');
    fs.write<char>(element->GetMappingMode());
    fs.write<int>(data.GetCount());
    fs.algin();
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
        fs.write<int>(indice.GetCount());
        fs.algin();
        for (auto i = 0; i < indice.GetCount(); i++)
        {
            fs.write<int>(indice.GetAt(i));
        }
    }
}

#include <sys/stat.h>

void exportOBJ(FbxMesh *mesh, FileOptions &fo)
{
    auto pos = fo.filename.rfind('.');
    std::string workspace = fo.filename.substr(0, pos) + ".fbm";
    mkdir(workspace.c_str(), 0777);
    
    std::string name = mesh->GetName();
    if (name.empty())
    {
        name = mesh->GetNode()->GetName();
    }
    
    auto unit = mesh->GetScene()->GetGlobalSettings().GetSystemUnit();
    
    std::string filename = workspace + "/" + name + ".obj";
    MeshStream fs(filename.c_str());
    
    char line[1024];
    for (auto i = 0; i < mesh->GetControlPointsCount(); i++)
    {
        auto point = mesh->GetControlPointAt(i) * (unit.GetScaleFactor() / 100);
        auto size = sprintf(line, "v %.6f %.6f %.6f \n", point.mData[0], point.mData[1], point.mData[2]);
        fs.write(line, size);
    }
    
    std::function<int32_t(int32_t, int32_t)> normalMapping = nullptr;
    auto normals = mesh->GetElementNormal();
    if (normals != nullptr)
    {
        auto &indices = normals->GetIndexArray();
        auto &directs = normals->GetDirectArray();
        if (normals->GetReferenceMode() == fbxsdk::FbxLayerElement::eDirect)
        {
            normalMapping = [&](int32_t pi, int32_t ci)->int32_t
            {
                return ci;
            };
        }
        else
        {
            normalMapping = [&](int32_t pi, int32_t ci)->int32_t
            {
                return indices[pi];
            };
        }
        
        for (auto i = 0; i < directs.GetCount(); i++)
        {
            auto n = directs.GetAt(i);
            auto size = sprintf(line, "vn %.6f %.6f %.6f\n", n.mData[0], n.mData[1], n.mData[2]);
            fs.write(line, size);
        }
    }
    
    std::function<int32_t(int32_t, int32_t)> uvMapping = nullptr;
    auto uvs = mesh->GetElementUV();
    if (uvs != nullptr)
    {
        auto &indices = uvs->GetIndexArray();
        auto &directs = uvs->GetDirectArray();
        if (uvs->GetReferenceMode() == fbxsdk::FbxLayerElement::eDirect)
        {
            uvMapping = [&](int32_t pi, int32_t ci)->int32_t
            {
                return ci;
            };
        }
        else
        {
            uvMapping = [&](int32_t pi,  int32_t ci)->int32_t
            {
                return indices[pi];
            };
        }
        
        for (auto i = 0; i < directs.GetCount(); i++)
        {
            auto n = directs.GetAt(i);
            auto size = sprintf(line, "vt %.6f %.6f\n", n.mData[0], 1 - n.mData[1]);
            fs.write(line, size);
        }
    }
    
    char *ptr = nullptr;
    auto polygonVertexIndex = 0;
    auto controlVertexIndex = 0;
    for (auto i = 0; i < mesh->GetPolygonCount(); i++)
    {
        ptr = line;
        ptr += sprintf(ptr, "f");
        for (auto n = 0; n < mesh->GetPolygonSize(i); n++)
        {
            controlVertexIndex = mesh->GetPolygonVertex(i, n);
            ptr += sprintf(ptr, " %d", controlVertexIndex + 1);
            if (uvMapping != nullptr)
            {
                ptr += sprintf(ptr, "/%d", uvMapping(polygonVertexIndex, controlVertexIndex) + 1);
            }
            
            if (normals != nullptr)
            {
                if (uvMapping == nullptr) { ptr += sprintf(ptr, "/"); }
                ptr += sprintf(ptr, "/%d", normalMapping(polygonVertexIndex, controlVertexIndex) + 1);
            }
            
            polygonVertexIndex++;
        }
        ptr += sprintf(ptr, "\n");
        fs.write(line, ptr - line);
    }
}

void exportMesh(FbxMesh *mesh, FileOptions &fo)
{
    auto pos = fo.filename.rfind('.');
    std::string workspace = fo.filename.substr(0, pos) + ".fbm";
    mkdir(workspace.c_str(), 0777);
    
    std::string name = mesh->GetName();
    if (name.empty())
    {
        name = mesh->GetNode()->GetName();
    }
    
    auto unit = mesh->GetScene()->GetGlobalSettings().GetSystemUnit();
    
    std::string filename = workspace + "/" + name + ".mesh";
    MeshStream fs(filename.c_str());
    fs.write('M');
    fs.write('E');
    fs.write('S');
    fs.write('H');
    // vertices
    fs.write('V');
    auto numControlVertices = mesh->GetControlPointsCount();
    fs.write<int>(numControlVertices);
    fs.algin();
    for (auto i = 0; i < numControlVertices; i++)
    {
        fs.write<FbxVector4>(mesh->GetControlPointAt(i) * (unit.GetScaleFactor() / 100));
    }
    
    // triangles
    fs.write('T');
    auto offset = fs.tell();
    fs.write<int>(0); // triangle count
    auto numTriangles = 0;
    std::vector<int> polygonVertices;
    fs.algin();
    for (auto i = 0; i < mesh->GetPolygonCount(); i++)
    {
        auto size = mesh->GetPolygonSize(i);
        auto anchor = polygonVertices.size();
        for (auto t = 0; t < size; t++) // auto split polygons with more than 3 vertices
        {
            auto vertex = mesh->GetPolygonVertex(i, t);
            if (t > 0 && t < size - 1)
            {
                fs.write<int>((int)anchor);
                fs.write<int>((int)polygonVertices.size());
                fs.write<int>((int)polygonVertices.size() + 1);
                numTriangles += 1;
            }
            polygonVertices.push_back(vertex);
        }
    }
    
    // write real triangle count
    auto position = fs.tell();
    fs.seek(offset, std::fstream::beg);
    fs.write<int>(numTriangles);
    fs.seek(position, std::fstream::beg); // restore cursor
    
    // encode polygon vertices
    fs.write<char>('P');
    fs.write<int>((int)polygonVertices.size());
    fs.algin();
    fs.write<int>(&polygonVertices.front(), (int)polygonVertices.size());
    fs.write<char>('Z');
    
    // encode normals
    auto layer = mesh->GetLayer(0);
    auto normals = layer->GetNormals();
    if (normals != NULL)
    {
        fs.write<char>('n');
        encode<FbxVector4>(normals, fs);
    }
    
    // encode tangents
    auto tangents = layer->GetTangents();
    if (tangents != NULL)
    {
        fs.write<char>('t');
        encode<FbxVector4>(tangents, fs);
    }
    
    // encode vertex colors
    auto colors = layer->GetVertexColors();
    if (colors != NULL)
    {
        fs.write<char>('c');
        encode<FbxColor>(colors, fs);
    }
    
    // encode uvmapping
    auto uvs = layer->GetUVs();
    if (uvs != NULL)
    {
        fs.write<char>('u');
        encode<FbxVector2>(uvs, fs);
    }
}

namespace buffers
{
    char text[1024];
}

template<typename T>
std::string describe(FbxLayerElementTemplate<T> *data)
{
    char *ptr = buffers::text;
    auto mode = data->GetReferenceMode();
    FbxLayerElementArrayTemplate<T> &directs = data->GetDirectArray();
    auto dsize = directs.GetCount() * sizeof(T);
    auto isize = 0;
    ptr += sprintf(ptr, "d(%d,#%d)", (int)dsize, directs.GetCount());
    if (mode != fbxsdk::FbxLayerElement::eDirect)
    {
        auto &indices = data->GetIndexArray();
        isize = indices.GetCount() * sizeof(int);
        ptr += sprintf(ptr, " + i(%d,#%d)", isize, indices.GetCount());
    }
    ptr += sprintf(ptr, " = %d", (int)dsize + isize);
    return buffers::text;
}

void printMeshAttributes(FbxMesh *mesh, std::string indent)
{
    auto count = 0;
    if ((count = mesh->GetElementNormalCount()))
    {
        printf("%s+Normal\n", indent.c_str());
        for (auto i = 0; i < count; i++)
        {
            auto data = mesh->GetElementNormal(i);
            printf("%s  -[%d] %s\n", indent.c_str(), i, describe(data).c_str());
        }
    }
    
    if ((count = mesh->GetElementUVCount()))
    {
        printf("%s+UV\n", indent.c_str());
        for (auto i = 0; i < count; i++)
        {
            auto data = mesh->GetElementUV(i);
            printf("%s  -[%d] %s\n", indent.c_str(), i, describe(data).c_str());
        }
    }
    
    if ((count = mesh->GetElementVertexColorCount()))
    {
        printf("%s+VertexColor\n", indent.c_str());
        for (auto i = 0; i < count; i++)
        {
            auto data = mesh->GetElementVertexColor(i);
            printf("%s  -[%d] %s\n", indent.c_str(), i, describe(data).c_str());
        }
    }
    
    if ((count = mesh->GetElementTangentCount()))
    {
        printf("%s+Tangent\n", indent.c_str());
        for (auto i = 0; i < count; i++)
        {
            auto data = mesh->GetElementTangent(i);
            printf("%s  -[%d] %s\n", indent.c_str(), i, describe(data).c_str());
        }
    }
    
    if ((count = mesh->GetElementBinormalCount()))
    {
        printf("%s+Binormal\n", indent.c_str());
        for (auto i = 0; i < count; i++)
        {
            auto data = mesh->GetElementBinormal(i);
            printf("%s  -[%d] %s\n", indent.c_str(), i, describe(data).c_str());
        }
    }
    
    if ((count = mesh->GetElementMaterialCount()))
    {
        printf("%s+Material\n", indent.c_str());
        for (auto i = 0; i < count; i++)
        {
            auto data = mesh->GetElementMaterial(i);
            printf("%s  -[%d] %s\n", indent.c_str(), i, describe(data).c_str());
        }
    }
    
    if ((count = mesh->GetElementPolygonGroupCount()))
    {
        printf("%s+PolygonGroup\n", indent.c_str());
        for (auto i = 0; i < count; i++)
        {
            auto data = mesh->GetElementPolygonGroup(i);
            printf("%s  -[%d] %s\n", indent.c_str(), i, describe(data).c_str());
        }
    }
    
    if ((count = mesh->GetElementUserDataCount()))
    {
        printf("%s+UserData\n", indent.c_str());
        for (auto i = 0; i < count; i++)
        {
            auto data = mesh->GetElementUserData(i);
            printf("%s  -[%d] %s\n", indent.c_str(), i, describe(data).c_str());
        }
    }
    
    if ((count = mesh->GetElementSmoothingCount()))
    {
        printf("%s+Smoothing\n", indent.c_str());
        for (auto i = 0; i < count; i++)
        {
            auto data = mesh->GetElementSmoothing(i);
            printf("%s  -[%d] %s\n", indent.c_str(), i, describe(data).c_str());
        }
    }
    
    if ((count = mesh->GetElementVertexCreaseCount()))
    {
        printf("%s+VertexCrease\n", indent.c_str());
        for (auto i = 0; i < count; i++)
        {
            auto data = mesh->GetElementVertexCrease(i);
            printf("%s  -[%d] %s\n", indent.c_str(), i, describe(data).c_str());
        }
    }
    
    if ((count = mesh->GetElementVisibilityCount()))
    {
        printf("%s+Visibility\n", indent.c_str());
        for (auto i = 0; i < count; i++)
        {
            auto data = mesh->GetElementVisibility(i);
            printf("%s  -[%d] %s\n", indent.c_str(), i, describe(data).c_str());
        }
    }
    
    if ((count = mesh->GetElementHoleCount()))
    {
        printf("%s+Hole\n", indent.c_str());
        for (auto i = 0; i < count; i++)
        {
            auto data = mesh->GetElementHole(i);
            printf("%s  -[%d] %s\n", indent.c_str(), i, describe(data).c_str());
        }
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
                printf(" vertices=%d polygons=%d triangles=%d", mesh->GetControlPointsCount(), polygonCount, triangleCount);
            });
            
            if (fo.mesh) { exportMesh(mesh, fo); }
            if (fo.obj) {exportOBJ(mesh, fo);}
        }
        fo.print(debug, [&]{printf("\n");});
        if (isMesh)
        {
            auto mesh = static_cast<FbxMesh *>(attribute);
            printMeshAttributes(mesh, (closed ? " " : "│") + indent + "  ");
        }
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
    auto unit = FbxSystemUnit::cm;
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
