#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <wchar.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/limits.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <map>
#include <vector>
#include <thread>
#include <unistd.h>

#include "../beatsaber-hook/shared/utils/utils.h"

using namespace std;

static IL2CPP_Helper helper;

Il2CppObject* assetBundleCreateRequest;
Il2CppObject* assetBundleRequest;

Il2CppObject* customUI;

Il2CppClass* objectClass;
const MethodInfo* object_Instantiate;

Il2CppObject* menuTransformParent;
Il2CppObject* buttonBinder;

Array<Il2CppObject*>* menuButtons;
Il2CppObject* modsButton;

static bool boolTrue = true;
static bool boolFalse = false;

void SetUIActive(bool active){
    if(active){
        helper.RunMethod(customUI, "SetActive", &boolTrue);
        for(int i = 0;i<menuButtons->Length();i++)
            helper.RunMethod(menuButtons->values[i], "set_enabled", &boolFalse);
    }else{
        helper.RunMethod(customUI, "SetActive", &boolFalse);
        for(int i = 0;i<menuButtons->Length();i++)
            helper.RunMethod(menuButtons->values[i], "set_enabled", &boolTrue);
    }
}

void ModsButtonOnClick(){
    log(INFO, "ModsButtonOnClick!");
    int state = 0;
    helper.RunMethod(modsButton, "DoStateTransition", &state, &boolTrue);
    SetUIActive(true);
}

void BackButtonOnClick(){
    log(INFO, "BackButtonOnClick!");
    SetUIActive(false);
}

void AssetBundleComplete(){
    log(INFO, "AssetBundleComplete!");
    Il2CppObject* customUIAsset;
    helper.RunMethod(&customUIAsset, assetBundleRequest, "get_asset");
    helper.RunMethod(&customUI, nullptr, object_Instantiate, customUIAsset);
    
    helper.RunMethod(customUI, "SetActive", &boolFalse);
    Il2CppObject* customUITransform;
    helper.RunMethod(&customUITransform, customUI, "get_transform");
    
    helper.RunMethod(customUITransform, "SetParent", menuTransformParent, &boolFalse);

    Array<Il2CppObject*>* customUIButtons;
    helper.RunMethod(&customUIButtons, customUI, "GetComponentsInChildren", helper.type_get_object(helper.class_get_type(helper.GetClassFromName("UnityEngine.UI", "Button"))), &boolTrue);
    for(int i = 0;i<customUIButtons->Length();i++){
        Il2CppObject* customUIButton = customUIButtons->values[i];
        Il2CppString* customUIButtonNameObject;
        helper.RunMethod(&customUIButtonNameObject, customUIButton, "get_name");
        const char* customUIButtonName = to_utf8(csstrtostr(customUIButtonNameObject)).c_str();
        void* onClickMethod = nullptr;
        if (strcmp(customUIButtonName, "ButtonBack") == 0)
        {
            onClickMethod = (void*)BackButtonOnClick;
        }
        if(onClickMethod != nullptr){
            auto action = helper.MakeAction(nullptr, onClickMethod, helper.class_get_type(helper.GetClassFromName("System", "Action")));
            helper.RunMethod(buttonBinder, "AddBinding", customUIButton, action);
        }
    }
    
}

void AssetBundleCreateRequestComplete(){
    log(INFO, "AssetBundleCreateRequestComplete!");
    Il2CppObject* customUIAssetBundle;
    helper.RunMethod(&customUIAssetBundle, assetBundleCreateRequest, "get_assetBundle");
    helper.RunMethod(&assetBundleRequest, customUIAssetBundle, "LoadAssetAsync", helper.createcsstr("_customui"), helper.type_get_object(helper.class_get_type(helper.GetClassFromName("UnityEngine", "GameObject"))));
    auto action = helper.MakeAction(nullptr, AssetBundleComplete, helper.class_get_type(helper.GetClassFromName("System", "Action")));
    helper.SetFieldValue(assetBundleRequest, "m_completeCallback", action);
}

Il2CppObject* textObject;
MAKE_HOOK_OFFSETLESS(MainMenuViewController_DidActivate, void, Il2CppObject* self, bool firstActivation, int type){
    MainMenuViewController_DidActivate(self, firstActivation, type);    
    if(firstActivation){
        helper.RunMethod(&buttonBinder, self, "get__buttonBinder");
       
        Il2CppObject* settingsButton = helper.GetFieldObjectValue(self, "_settingsButton");
        Il2CppObject* settingsButtonTransform;
        helper.RunMethod(&settingsButtonTransform, settingsButton, "get_transform");
        Il2CppObject* settingsButtonTransformParent;
        helper.RunMethod(&settingsButtonTransformParent, settingsButtonTransform, "GetParent");
        helper.RunMethod(&menuTransformParent, settingsButtonTransformParent, "GetParent");
        helper.RunMethod(&menuTransformParent, menuTransformParent, "GetParent");
        helper.RunMethod(&menuTransformParent, menuTransformParent, "GetParent");
        //helper.RunMethod(&menuTransformParent, menuTransformParent, "GetParent");
        //helper.RunMethod(&menuTransformParent, menuTransformParent, "GetParent");
        
        
        helper.RunMethod(&modsButton, nullptr, object_Instantiate, settingsButton);
        Il2CppObject* modsButtonTransform;
        helper.RunMethod(&modsButtonTransform, modsButton, "get_transform");
        helper.RunMethod(modsButtonTransform, "SetParent", settingsButtonTransformParent, &boolFalse);

        helper.RunMethod(&textObject, modsButton, "GetComponentInChildren", helper.type_get_object(helper.class_get_type(helper.GetClassFromName("TMPro", "TextMeshProUGUI"))), &boolFalse);
        
        auto action = helper.MakeAction(nullptr, ModsButtonOnClick, helper.class_get_type(helper.GetClassFromName("System", "Action")));
        helper.RunMethod(buttonBinder, "AddBinding", modsButton, action);
    
        helper.RunMethod(&menuButtons, menuTransformParent, "GetComponentsInChildren", helper.type_get_object(helper.class_get_type(helper.GetClassFromName("UnityEngine.UI", "Button"))), &boolFalse); 
        log(INFO, "%d", menuButtons->Length());
    
        Il2CppClass* assetBundleClass = helper.GetClassFromName("UnityEngine", "AssetBundle");
        const MethodInfo* assetBundle_LoadFromFileAsyncMethod = helper.class_get_method_from_name(assetBundleClass, "LoadFromFileAsync", 1);
          
        helper.RunMethod(&assetBundleCreateRequest, nullptr, assetBundle_LoadFromFileAsyncMethod, helper.createcsstr("/sdcard/Android/data/com.beatgames.beatsaber/files/uis/testUI.qui"));
        action = helper.MakeAction(nullptr, AssetBundleCreateRequestComplete, helper.class_get_type(helper.GetClassFromName("System", "Action")));
        helper.SetFieldValue(assetBundleCreateRequest, "m_completeCallback", action);
    }
    Il2CppString* textString = helper.createcsstr("Mods");
    helper.RunMethod(textObject, "set_text", textString);
}

void InitHooks(){
    sleep(1);

    helper.Initialize();

    objectClass = helper.GetClassFromName("UnityEngine", "Object");
    object_Instantiate = helper.class_get_method_from_name(objectClass, "Instantiate", 1);
     
    Il2CppClass* mainMenuViewControllerClass = helper.GetClassFromName("", "MainMenuViewController");
    const MethodInfo* mainMenuViewController_DidActivate = helper.class_get_method_from_name(mainMenuViewControllerClass, "DidActivate", 2);
    INSTALL_HOOK_OFFSETLESS(MainMenuViewController_DidActivate, mainMenuViewController_DidActivate);

    log(INFO, "Successfully installed Hooks!");
}

__attribute__((constructor)) void lib_main()
{
    #ifdef __aarch64__
    log(INFO, "Is 64 bit!");
    #endif

    std::thread initHooksThread(InitHooks);
    initHooksThread.detach();
    log(INFO, "Successfully installed CustomUI!");
}
