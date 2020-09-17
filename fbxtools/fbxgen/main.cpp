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
#include <map>

struct BoneInfluence
{
    uint16_t indices[4];
    float weights[4];
};

void generate_meshfbx(FileStream &fs)
{
    FbxManager* manager = FbxManager::Create();
    manager->SetIOSettings(FbxIOSettings::Create(manager, IOSROOT));
    
    auto name = fs.read<std::string>();
    
    float aabb[6];
    fs.read(&aabb[0], 6);
    auto poses = fs.read_vector<FbxAMatrix>();
    auto influences = fs.read_vector<BoneInfluence>();
    
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
    
    auto node = FbxNode::Create(scene, name.c_str());
    scene->GetRootNode()->AddChild(node);
    scene->ConnectSrcObject(node);
    
    auto mesh = FbxMesh::Create(scene, name.c_str());
    node->SetNodeAttribute(mesh);
    
    std::vector<std::map<uint16_t, float>> bones;
    bones.resize(poses.size());
    
    auto pindex = 0;
    for (auto iter = influences.begin(); iter != influences.end(); iter++)
    {
        for (auto i = 0; i < 4; i++)
        {
            auto index = iter->indices[i];
            auto &weights = bones[index];
            weights[pindex] = iter->weights[i];
        }
        pindex++;
    }
    auto root = FbxNode::Create(scene, "Skeleton");
    {
        auto skeleton = FbxSkeleton::Create(scene, "");
        skeleton->SetSkeletonType(FbxSkeleton::eRoot);
        root->SetNodeAttribute(skeleton);
        scene->GetRootNode()->AddChild(root);
    }
    
    auto skin = FbxSkin::Create(scene, "Skin");
    for (auto i = 0; i < bones.size(); i++)
    {
        auto m = poses[i].Inverse();
        
        auto &weights = bones[i];
        auto bone = FbxNode::Create(scene, "");
        auto skeleton = FbxSkeleton::Create(scene, "");
        skeleton->SetSkeletonType(FbxSkeleton::eLimb);
        bone->SetNodeAttribute(skeleton);
        bone->LclTranslation.Set(m.GetT());
        bone->LclRotation.Set(m.GetR());
        bone->LclScaling.Set(m.GetS());
        root->AddChild(bone);
        
        auto cluster = FbxCluster::Create(scene, "");
        cluster->SetLink(bone);
        cluster->SetLinkMode(FbxCluster::eNormalize);
        cluster->SetTransformLinkMatrix(bone->EvaluateGlobalTransform());
        cluster->SetTransformMatrix(node->EvaluateGlobalTransform());
        
        cluster->SetControlPointIWCount(static_cast<int>(weights.size()));
        for (auto iter = weights.begin(); iter != weights.end(); iter++)
        {
            cluster->AddControlPointIndex(iter->first, iter->second);
        }
        
        skin->AddCluster(cluster);
    }
    
    mesh->AddDeformer(skin);
    
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
        element->SetMappingMode(FbxLayerElement::eByControlPoint);
        element->SetReferenceMode(FbxLayerElement::eDirect);
        for (auto iter = normals.begin(); iter != normals.end(); iter++)
        {
            element->GetDirectArray().Add(*iter);
        }
        layer->SetNormals(element);
    }
    
    { // uvs
        auto element = FbxLayerElementUV::Create(mesh, "UVs");
        element->SetMappingMode(FbxLayerElement::eByControlPoint);
        element->SetReferenceMode(FbxLayerElement::eDirect);
        for (auto iter = uvs.begin(); iter != uvs.end(); iter++)
        {
            element->GetDirectArray().Add(*iter);
        }
        layer->SetUVs(element);
    }
    
    { // tangents
        auto element = FbxLayerElementTangent::Create(mesh, "Tangents");
        element->SetMappingMode(FbxLayerElement::eByControlPoint);
        element->SetReferenceMode(FbxLayerElement::eDirect);
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
