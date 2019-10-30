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
#include "../questui/questui.hpp"

using namespace std;

static IL2CPP_Helper* helper;

Il2CppObject* modPanel;
Il2CppObject* customUIObject;
bool pressed = false;

void ButtonTestOnClick(Il2CppObject* button){
    log(INFO, "ButtonTestOnClick!");
    Il2CppObject* textObject = UnityHelper::GetComponentInChildren(helper, customUIObject, helper->GetClassFromName("TMPro", "TextMeshProUGUI"), "TestText");
    pressed = !pressed;
    if(pressed){
         helper->RunMethod(textObject, "set_text", helper->createcsstr("Pressed"));
    }else{
        helper->RunMethod(textObject, "set_text", helper->createcsstr("Nope"));
    }
}

void AssetLoaderOnFinish(Il2CppObject* customUIAsset){
    helper->RunMethod(&customUIObject, nullptr, helper->class_get_method_from_name(helper->GetClassFromName("UnityEngine", "Object"), "Instantiate", 1), customUIAsset);
    UnityHelper::SetParent(helper, customUIObject, QuestUI::GetQuestUIModInfo().Panel);

    UnityHelper::AddButtonOnClick(helper, QuestUI::GetQuestUIInfo()->ButtonBinder, customUIObject, "ButtonTest", (void*)ButtonTestOnClick);
    log(INFO, "AssetLoaderOnFinish Called!");
}

void QuestUIOnInitialized(){
    UnityAssetLoader::LoadFromFileAsync("/sdcard/Android/data/com.beatgames.beatsaber/files/uis/customUI.qui", (UnityAssetLoader_OnFinishFunction*)AssetLoaderOnFinish);
}
void InitHooks(){
    sleep(1);
    helper = new IL2CPP_Helper();
    helper->Initialize();
    
    QuestUI::Initialize("CustomUI", (void*)QuestUIOnInitialized);
    
    log(INFO, "Successfully installed CustomUI!");
}

__attribute__((constructor)) void lib_main()
{
    #ifdef __aarch64__
    log(INFO, "Is 64 bit!");
    #endif

    std::thread initHooksThread(InitHooks);
    initHooksThread.detach();
}
