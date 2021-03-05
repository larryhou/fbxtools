//
//  main.cpp
//  fbxmerge
//
//  Created by LARRYHOU on 2021/3/5.
//  Copyright © 2021 LARRYHOU. All rights reserved.
//

#include <string>
#include <iostream>

#include <fbxsdk.h>
#include <fbxsdk/fileio/fbxiosettings.h>
#include <fbxsdk/core/fbxdatatypes.h>
#include "fbxptr.hpp"

void merge(std::string filename, FbxScene *parent)
{
    auto manager = parent->GetFbxManager();
    FbxPtr<FbxImporter> importer(FbxImporter::Create(manager, ""));
    if (!importer->Initialize(filename.c_str(), -1, manager->GetIOSettings())) { return; }
    
    auto scene = FbxScene::Create(manager, "Scene");
    if (!importer->Import(scene)) { return; }
    
    for (auto i = 0; i < scene->GetRootNode()->GetChildCount(); i++)
    {
        parent->GetRootNode()->AddChild(scene->GetRootNode()->GetChild(i));
    }
    
    scene->GetRootNode()->DisconnectAllSrcObject();
    
    for (auto i = 0; i < scene->GetSrcObjectCount(); i++)
    {
        auto obj = scene->GetSrcObject(i);
        if (obj == scene->GetRootNode() || *obj == scene->GetGlobalSettings()) { continue; }
        obj->ConnectDstObject(parent);
    }
    
    scene->DisconnectAllSrcObject();
}

int main(int argc, const char * argv[])
{
    if (argc <= 1) {return 1;}
    
    FbxPtr<FbxManager> manager;
    manager->SetIOSettings(FbxIOSettings::Create(manager, IOSROOT));
    
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
        
        // Convert the scene to meters using the defined options.
        FbxSystemUnit::m.ConvertScene(scene, options);
    }
    
    std::string filename(argv[1]);
    auto dot = filename.rfind('.');
    if (dot == std::string::npos) { return 2; }
    
    std::string savename = filename.substr(0, dot) + "_cat" + filename.substr(dot);
    
    FbxPtr<FbxExporter> exporter(FbxExporter::Create(manager, ""));
    if (!exporter->Initialize(savename.c_str(), -1, manager->GetIOSettings())) { return 3; }
    
    for (auto i = 1; i < argc; i++)
    {
        printf("[%d/%d] %s\n", i, argc - 1, argv[i]);
        merge(argv[i], scene);
    }

    if (!exporter->Export(scene)) { return 4; }
    
    std::cout << "[+] " << savename << std::endl;
    
    return 0;
}
