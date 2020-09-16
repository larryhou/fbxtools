//
//  main.cpp
//  fbxgen
//
//  Created by LARRYHOU on 2020/9/15.
//  Copyright © 2020 LARRYHOU. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <serialize.h>
#include <vector>

void generate_meshfbx(FileStream &fs)
{
    FbxManager* manager = FbxManager::Create();
    manager->SetIOSettings(FbxIOSettings::Create(manager, IOSROOT));
    
    auto name = fs.read<std::string>();
    
    float aabb[6];
    fs.read(&aabb[0], 6);
    std::vector<FbxMatrix> poese = fs.read_vector<FbxMatrix>();
    std::vector<uint64_t> weights = fs.read_vector<uint64_t>();
    
    auto vertices = fs.read_vector<FbxVector3>();
    auto triangles = fs.read_vector<uint32_t>();
    
    auto tangents = fs.read_vector<FbxVector4>();
    auto normals = fs.read_vector<FbxVector3>();
    auto uvs = fs.read_vector<FbxVector2>();
    
    auto scene = FbxScene::Create(manager, "Scene");
    {
        const FbxSystemUnit::ConversionOptions options = {
            true, /* mConvertRrsNodes */
            true, /* mConvertLimits */
            true, /* mConvertClusters */
            true, /* mConvertLightIntensity */
            true, /* mConvertPhotometricLProperties */
            true  /* mConvertCameraClipPlanes */
        };
        FbxSystemUnit::m.ConvertScene(scene, options);
    }
    
    auto node = FbxNode::Create(manager, name.c_str());
    scene->GetRootNode()->AddChild(node);
    scene->ConnectSrcObject(node);
    
    auto mesh = FbxMesh::Create(manager, name.c_str());
    node->SetNodeAttribute(mesh);
    
    auto layer = mesh->GetLayer(0);
    if (!layer)
    {
        mesh->CreateLayer();
        layer = mesh->GetLayer(0);
    }
    
    auto material = FbxSurfaceLambert::Create(manager, "Lambert");
    material->ShadingModel.Set("Lambert");
    material->Emissive.Set(FbxDouble3(0, 0, 0));
    material->Ambient.Set(FbxDouble3(1, 1, 1));
    material->Diffuse.Set(FbxDouble3(1, 1, 1));
    material->TransparencyFactor.Set(0);
    node->AddMaterial(material);
    
    // vertices
    mesh->InitControlPoints(static_cast<int>(vertices.size()));
    for (auto i = 0; i < vertices.size(); i++) { mesh->SetControlPointAt(vertices[i], i); }
    for (auto iter = triangles.begin(); iter != triangles.end();)
    {
        mesh->BeginPolygon();
        mesh->AddPolygon(*iter++);
        mesh->AddPolygon(*iter++);
        mesh->AddPolygon(*iter++);
        mesh->EndPolygon();
    }
    
    { // normals
        auto element = FbxLayerElementNormal::Create(mesh, "Normals");
        element->SetMappingMode(fbxsdk::FbxLayerElement::EMappingMode::eByControlPoint);
        element->SetReferenceMode(fbxsdk::FbxLayerElement::EReferenceMode::eDirect);
        for (auto iter = normals.begin(); iter != normals.end(); iter++)
        {
            element->GetDirectArray().Add(*iter);
        }
        layer->SetNormals(element);
    }
    
    { // uvs
        auto element = FbxLayerElementUV::Create(mesh, "UVs");
        element->SetMappingMode(fbxsdk::FbxLayerElement::EMappingMode::eByControlPoint);
        element->SetReferenceMode(fbxsdk::FbxLayerElement::EReferenceMode::eDirect);
        for (auto iter = uvs.begin(); iter != uvs.end(); iter++)
        {
            element->GetDirectArray().Add(*iter);
        }
        layer->SetUVs(element);
    }
    
    { // tangents
        auto element = FbxLayerElementTangent::Create(mesh, "Tangents");
        element->SetMappingMode(fbxsdk::FbxLayerElement::EMappingMode::eByControlPoint);
        element->SetReferenceMode(fbxsdk::FbxLayerElement::EReferenceMode::eDirect);
        for (auto iter = tangents.begin(); iter != tangents.end(); iter++)
        {
            element->GetDirectArray().Add(*iter);
        }
        layer->SetTangents(element);
    }
    
    std::string error;
    std::string savename(name + ".fbx");
    auto exporter = FbxExporter::Create(manager, "");
    if (!exporter->Initialize(savename.c_str(), -1, manager->GetIOSettings()))
    {
        error = exporter->GetStatus().GetErrorString();
        manager->Destroy();
        return;
    }
    if (!exporter->Export(scene))
    {
        error = exporter->GetStatus().GetErrorString();
        manager->Destroy();
        return;
    }
    printf(">> %s\n", savename.c_str());
    manager->Destroy();
}

void load_mesh_database(const char* filename)
{
    FileStream fs(filename, std::ios_base::in);
    if (!fs.good()) {return;}
    
    fs.read<uint32_t>();
    fs.read<std::string>();
    fs.read<std::string>();
    
    auto count = fs.read<uint32_t>();
    for (auto i = 0; i < count; i++)
    {
        fs.read<std::string>();
        generate_meshfbx(fs);
    }
}

int main(int argc, const char * argv[])
{
    for (auto i = 1; i < argc; i++)
    {
        printf("[%d] %s\n", i, argv[i]);
        load_mesh_database(argv[i]);
    }
    return 0;
}
