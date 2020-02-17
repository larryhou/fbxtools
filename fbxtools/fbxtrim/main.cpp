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

void process(std::string filename, FbxManager *pManager)
{
    auto importer = FbxImporter::Create(pManager, "");
    if (!importer->Initialize(filename.c_str(), -1, pManager->GetIOSettings()))
    {
        return;
    }
    
    auto fScene = FbxScene::Create(pManager, "Scene");
    if (!importer->Import(fScene))
    {
        return;
    }
    
    importer->Destroy();
    
    auto tScene = FbxScene::Create(pManager, "Scene");
    std::vector<FbxNode *> children;
    auto root = fScene->GetRootNode();
    for (auto i = 0; i < root->GetChildCount(); i++)
    {
        auto child = root->GetChild(i);
        children.push_back(child);
    }
    for (auto iter = children.begin(); iter != children.end(); iter++)
    {
        tScene->GetRootNode()->AddChild(*iter);
    }

    for (auto i = 0; i < fScene->GetSrcObjectCount(); i++)
    {
        auto obj = fScene->GetSrcObject(i);
        if (obj != root && !obj->Is<FbxGlobalSettings>())
        {
            obj->ConnectDstObject(tScene);
        }
        
        if (obj->Is<FbxPose>())
        {
            auto pose = static_cast<FbxPose *>(obj);
            for (auto n = 0; n < pose->GetCount(); n++)
            {
                auto node = pose->GetNode(n);
                node->GetName();
            }
        }
    }
    fScene->DisconnectAllSrcObject();
    
    auto pos = filename.rfind('.');
    std::string savename = filename.substr(0, pos) + "_t" + filename.substr(pos);
    pManager->GetIOSettings()->SetBoolProp(EXP_FBX_EMBEDDED, true);
    
    auto exporter = FbxExporter::Create(pManager, "");
    if (!exporter->Initialize(savename.c_str(), -1, pManager->GetIOSettings()))
    {
        return;
    }
    
    if (!exporter->Export(tScene))
    {
        return;
    }
    
    printf(">>> %s\n", savename.c_str());
    
    tScene->Destroy();
    fScene->Destroy();
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
