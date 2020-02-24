//
//  main.cpp
//  fbxtrim
//
//  Created by LARRYHOU on 2020/2/16.
//  Copyright © 2020 LARRYHOU. All rights reserved.
//

#include <iostream>
#include <fbxsdk.h>
#include <fbxsdk/core/fbxdatatypes.h>
#include <string>
#include <vector>

void trim(FbxMesh *mesh)
{
    auto count = 0;
    if ((count = mesh->GetElementMaterialCount()))
    {
        for (auto i = 0; i < count; i++)
        {
            mesh->RemoveElementMaterial(mesh->GetElementMaterial(0));
        }
        
        mesh->GetNode()->RemoveAllMaterials();
    }
    
    if ((count = mesh->GetElementVertexColorCount()))
    {
        for (auto i = 0; i < count; i++)
        {
            mesh->RemoveElementVertexColor(mesh->GetElementVertexColor(0));
        }
    }
    
    if ((count = mesh->GetElementUVCount()))
    {
        for (auto i = 1; i < count; i++)
        {
            mesh->RemoveElementUV(mesh->GetElementUV(1));
        }
    }
}

void trim(FbxScene *scene)
{
    for (auto i = 0; i < scene->GetSrcObjectCount(); i++)
    {
        auto obj = scene->GetSrcObject(i);
        if (obj->Is<FbxNode>())
        {
            auto node = static_cast<FbxNode *>(obj);
            auto attribute = node->GetNodeAttribute();
            if (!attribute) {continue;}
            
            auto type = attribute->GetAttributeType();
            switch (type)
            {
                case FbxNodeAttribute::eMesh:
                    trim(static_cast<FbxMesh *>(attribute));
                    break;
                default:break;
            }
        }
        else if (obj->Is<FbxTexture>())
        {
            obj->DisconnectAllDstObject();
        }
    }
}

void process(std::string filename, FbxManager *pManager)
{
    auto importer = FbxImporter::Create(pManager, "");
    if (!importer->Initialize(filename.c_str(), -1, pManager->GetIOSettings()))
    {
        return;
    }
    
    auto scene = FbxScene::Create(pManager, "Scene");
    if (!importer->Import(scene))
    {
        return;
    }
    
    importer->Destroy();
    
    trim(scene);
    
    auto pos = filename.rfind('.');
    std::string savename = filename.substr(0, pos) + "_t" + filename.substr(pos);
    
    auto exporter = FbxExporter::Create(pManager, "");
    if (!exporter->Initialize(savename.c_str(), -1, pManager->GetIOSettings()))
    {
        return;
    }
    
    if (!exporter->Export(scene))
    {
        return;
    }
    
    printf(">>> %s\n", savename.c_str());
    
    scene->Destroy();
}

int main(int argc, const char * argv[])
{
    if (argc <= 1) {return 1;}
    
    FbxManager* pManager = FbxManager::Create();
    pManager->SetIOSettings(FbxIOSettings::Create(pManager, IOSROOT));
    
    for (auto i = 1; i < argc; i++)
    {
        printf("[%d/%d] %s\n", i, argc - 1, argv[i]);
        process(argv[i], pManager);
    }
    
    pManager->Destroy();
    return 0;
}
