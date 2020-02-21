//
//  main.cpp
//  fbxconvert
//
//  Created by LARRYHOU on 2020/2/21.
//  Copyright © 2020 LARRYHOU. All rights reserved.
//

#include <iostream>
#include <fbxsdk.h>
#include <fbxsdk/fileio/fbxiosettings.h>
#include <fbxsdk/core/fbxdatatypes.h>

#include "../common/arguments.h"

struct FileOptions: public ArgumentOptions
{
    std::string extension;
    bool unit;
    
    FileOptions(std::string file): ArgumentOptions(file)
    {
        if (!get("type", extension))
        {
            extension = "obj";
        }
        
        unit = get("unit");
    }
};

#include <sys/stat.h>

bool process(FileOptions &fo, FbxManager *manager, std::string &error)
{
    auto filepath = fo.filename;
    auto dot = filepath.rfind('.');
    std::string workspace = filepath.substr(0, dot) + ".fbm";
    mkdir(workspace.c_str(), 0777);
    
    std::string name;
    auto sep = filepath.rfind('/');
    if (sep == std::string::npos)
    {
        name = filepath.substr(0, dot);
    }
    else
    {
        name = filepath.substr(sep + 1, dot - sep - 1);
    }
    
    std::string savename = workspace + "/" + name + "." + fo.extension;
    
    // read
    auto importer = FbxImporter::Create(manager, "");
    if (!importer->Initialize(filepath.c_str(), -1, manager->GetIOSettings()))
    {
        error = importer->GetStatus().GetErrorString();
        return false;
    }
    
    auto scene = FbxScene::Create(manager, "Scene");
    if (!importer->Import(scene))
    {
        error = importer->GetStatus().GetErrorString();
        return false;
    }
    
    importer->Destroy();
    
    // export
    manager->GetIOSettings()->SetBoolProp(EXP_FBX_EMBEDDED, true);
    
    auto exporter = FbxExporter::Create(manager, "");
    if (!exporter->Initialize(savename.c_str(), -1, manager->GetIOSettings()))
    {
        error = exporter->GetStatus().GetErrorString();
        return false;
    }
    
    if (fo.unit)
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
    
    if (!exporter->Export(scene))
    {
        error = exporter->GetStatus().GetErrorString();
        return false;
    }
    
    printf(">>> %s\n", savename.c_str());
    return true;
}

int main(int argc, const char * argv[])
{
    if (argc <= 1) {return 1;}
    
    FbxManager* manager = FbxManager::Create();
    manager->SetIOSettings(FbxIOSettings::Create(manager, IOSROOT));
    
    for (auto i = 1; i < argc; i++)
    {
        FileOptions fo(argv[i]);
        printf("[%d/%d] %s\n", i, argc - 1, argv[i]);
        std::string error;
        if (!process(fo, manager, error))
        {
            printf("[E] %s\n", error.c_str());
        }
        printf("\n");
    }
    
    manager->Destroy();
    return 0;
}
