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
#include <math.h>

struct BoneInfluence
{
    uint16_t indices[4];
    float weights[4];
};

FbxVector4 rotate(FbxVector4 v, float angle, int axis = 0)
{
    FbxVector4 r = v;
    auto x = (axis + 1) % 3;
    auto y = (axis + 2) % 3;
    if (x > y) { auto t = x; x = y; y = t; }
    
    auto radian = M_PI * angle / 180;
    r.mData[x] = std::cos(radian) * v.mData[x] - std::sin(radian) * v.mData[y];
    r.mData[y] = std::sin(radian) * v.mData[x] + std::cos(radian) * v.mData[y];
    return r;
}

void rotate(double *v, float angle, int axis = 0)
{
    auto x = (axis + 1) % 3;
    auto y = (axis + 2) % 3;
    if (x > y) { auto t = x; x = y; y = t; }
    
    auto radian = M_PI * angle / 180;
    auto tx = std::cos(radian) * v[x] - std::sin(radian) * v[y];
    auto ty = std::sin(radian) * v[x] + std::cos(radian) * v[y];
    v[x] = tx;
    v[y] = ty;
}

void convert(double *v)
{
    rotate(v, -90, 0);
    v[0] = -v[0];
}

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
    
    auto skeleton = fs.read<Skeleton>();
    
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
    
    auto meshNode = FbxNode::Create(scene, name.c_str());
    scene->GetRootNode()->AddChild(meshNode);
    scene->ConnectSrcObject(meshNode);
    
    auto mesh = FbxMesh::Create(scene, name.c_str());
    meshNode->SetNodeAttribute(mesh);
    
    if (skeleton.bones.size())
    {
        std::vector<std::map<uint16_t, float>> weights;
        weights.resize(poses.size());
        
        auto vertex = 0;
        for (auto iter = influences.begin(); iter != influences.end(); iter++)
        {
            for (auto i = 0; i < 4; i++)
            {
                auto index = iter->indices[i];
                auto &cluster = weights[index];
                cluster[vertex] = iter->weights[i];
            }
            vertex++;
        }
        
        std::vector<FbxNode*> bones;
        for (auto i = 0; i < skeleton.poses.size(); i++)
        {
            auto &name = skeleton.names[i];
            auto node = FbxNode::Create(scene, name.c_str());
            auto bone = FbxSkeleton::Create(scene, name.c_str());
            bone->SetSkeletonType( skeleton.nodes[i] == -1? FbxSkeleton::eRoot : FbxSkeleton::eLimb);
            node->SetNodeAttribute(bone);
            bones.push_back(node);
        }
        
        for (auto iter = tangents.begin(); iter != tangents.end(); iter++) { convert((double *)&*iter); }
        for (auto iter = vertices.begin(); iter != vertices.end(); iter++) { convert((double *)&*iter); }
        for (auto iter = normals.begin(); iter != normals.end(); iter++) { convert((double *)&*iter); }
        
        for (auto i = 0; i < bones.size(); i++)
        {
            auto node = bones[i];
            auto pindex = skeleton.nodes[i];
            auto parent = pindex == -1? scene->GetRootNode() : bones[pindex];
            parent->AddChild(node);
            auto &pose = skeleton.poses[i];
            FbxAMatrix mat(pose.position, pose.rotation, pose.scale);
            node->LclTranslation.Set(mat.GetT());
            node->LclRotation.Set(mat.GetR());
            node->LclScaling.Set(mat.GetS());
        }
        
        auto skin = FbxSkin::Create(scene, "Skin");
        for (auto i = 0; i < skeleton.bones.size(); i++)
        {
            if (i >= weights.size()) {continue;}
            
            auto index = skeleton.bones[i];
            auto &name = skeleton.names[index];
            auto &bone = bones[index];
            
            auto cluster = FbxCluster::Create(scene, name.c_str());
            cluster->SetLink(bone);
            cluster->SetLinkMode(FbxCluster::eNormalize);
            cluster->SetTransformLinkMatrix(bone->EvaluateGlobalTransform());
            
            auto &cluster_weights = weights[i];
            cluster->SetControlPointIWCount(static_cast<int>(cluster_weights.size()));
            for (auto iter = cluster_weights.begin(); iter != cluster_weights.end(); iter++)
            {
                cluster->AddControlPointIndex(iter->first, iter->second);
            }
            
            skin->AddCluster(cluster);
        }
        
        mesh->AddDeformer(skin);
    }
    
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
    meshNode->AddMaterial(material);
    
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
