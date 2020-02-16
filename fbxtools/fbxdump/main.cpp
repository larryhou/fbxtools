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
#include <string>

void dumpHierarchy(FbxNode* node, std::string indent = "")
{
    auto numChildren = node->GetChildCount();
    for (auto i = 0; i < numChildren; i++)
    {
        auto closed = numChildren == i + 1;
        auto child = node->GetChild(i);
        auto attribute = child->GetNodeAttribute();
        auto type = attribute->GetAttributeType();
        std::string name = "Unknown";
        auto isNull = false;
        switch (type)
        {
            case fbxsdk::FbxNodeAttribute::eNull:
                name = "Null";
                isNull = true;
                break;
            case fbxsdk::FbxNodeAttribute::eMarker:name = "Marker";break;
            case fbxsdk::FbxNodeAttribute::eSkeleton:name = "Skeleton";break;
            case fbxsdk::FbxNodeAttribute::eMesh:name = "Mesh";break;
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
        
        printf("%s%s─\e[4m%s\e[0m \e[%dm%s\e[0m\n", indent.c_str(), closed ? "└" : "├", name.c_str(), isNull ? 96:33,  child->GetName());
        dumpHierarchy(child, indent + (closed ? " " : "│") + "  ");
    }
}

bool process(const char *filepath, FbxManager *pManager)
{
    auto importer = FbxImporter::Create(pManager, "");
    if (!importer->Initialize(filepath, -1, pManager->GetIOSettings()))
    {
        printf("Call to FbxImporter::Intialize() failed.\n");
        printf("Error returned: %s \n", importer->GetStatus().GetErrorString());
        return false;
    }
    
    auto pScene = FbxScene::Create(pManager, "Scene");
    
    if (!importer->Import(pScene))
    {
        printf("%s\n", importer->GetStatus().GetErrorString());
        return false;
    }
    
    importer->Destroy();
    
    auto numStacks = pScene->GetSrcObjectCount<FbxAnimStack>();
    for (auto i = 0; i < numStacks; i++)
    {
        auto pAnimStack = pScene->GetSrcObject<FbxAnimStack>(i);
        printf("[Animation][%d/%d] %s\n", i + 1, numStacks, pAnimStack->GetName());
        //        auto numLayers = pAnimStack->GetSrcObjectCount<FbxAnimLayer>();
        //        for (auto l = 0; l < numLayers; l++)
        //        {
        //            auto pAnimLayer = pAnimStack->GetSrcObject<FbxAnimLayer>(l);
        //            auto numNodes = pAnimLayer->GetSrcObjectCount<FbxAnimCurveNode>();
        //            for (auto n = 0; n < numNodes; n++)
        //            {
        //                auto pCurveNode = pAnimLayer->GetSrcObject<FbxAnimCurveNode>(n);
        //                const auto &property = pCurveNode->GetDstProperty();
        //                auto pValue = property.Get<FbxDouble3>();
        //                auto pType = property.GetPropertyDataType().GetType();
        //
        //                const auto &parent = property.GetParent();
        //
        //                if (pCurveNode->GetCurveCount(0) > 0)
        //                {
        //                    printf("%s fbx=%s parent=%s\n", property.GetName().Buffer(), property.GetFbxObject()->GetName(), parent.GetName().Buffer());
        //                    for (auto c = 0; c < pCurveNode->GetCurveCount(0); c++)
        //                    {
        //                        auto pCurve = pCurveNode->GetCurve(0, c);
        //                        for( auto k = 0; k < pCurve->KeyGetCount(); k++)
        //                            {
        //                                auto pCurveKey = pCurve->KeyGet(k);
        //                                printf("");
        //                            }
        //                        printf("");
        //                    }
        //                }
        //                printf("%p\n", pCurveNode);
        //            }
        //
        //            printf("");
        //        }
    }
    
    dumpHierarchy(pScene->GetRootNode());
    
    pScene->Destroy();
    
    return true;
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
        printf("\n");
    }
    
    pManager->Destroy();
    
    return 0;
}
