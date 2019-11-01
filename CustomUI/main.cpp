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

static Il2CppObject* assetBundle;
static Il2CppObject* customUIObject;

static int counter = 0;

void UpdateTextCounter(){
    char text[64];
    sprintf(text, "%d", counter);
    Il2CppObject* textObject = UnityHelper::GetComponentInChildren(helper, customUIObject, helper->GetClassFromName("TMPro", "TextMeshProUGUI"), "TextCounter");
    helper->RunMethod(textObject, "set_text", helper->createcsstr(text));
}


void ButtonCounterClick(Il2CppObject* button){
    log(INFO, "ButtonCounterClick!");
    counter++;
    UpdateTextCounter();
}

void OnLoadAssetComplete(Il2CppObject* asset){
    helper->RunMethod(&customUIObject, nullptr, helper->class_get_method_from_name(helper->GetClassFromName("UnityEngine", "Object"), "Instantiate", 1), asset);
    UnityHelper::SetParent(helper, customUIObject, QuestUI::GetQuestUIModInfo().Panel);

    UnityHelper::AddButtonOnClick(helper, QuestUI::GetQuestUIInfo()->ButtonBinder, customUIObject, "ButtonCounter", (UnityHelper::ButtonOnClickFunction*)ButtonCounterClick);

    UpdateTextCounter();
}

void OnLoadAssetBundleComplete(Il2CppObject* assetBundleArg){
    assetBundle = assetBundleArg;
    UnityAssetLoader::LoadAssetFromAssetBundleAsync(assetBundle, (UnityAssetLoader_OnLoadAssetCompleteFunction*)OnLoadAssetComplete);
}

void QuestUIOnInitialized(){
    if(assetBundle == nullptr){
        UnityAssetLoader::LoadAssetBundleFromFileAsync("/sdcard/Android/data/com.beatgames.beatsaber/files/uis/customUI.qui", (UnityAssetLoader_OnLoadAssetBundleCompleteFunction*)OnLoadAssetBundleComplete);
    }else{
        UnityAssetLoader::LoadAssetFromAssetBundleAsync(assetBundle, (UnityAssetLoader_OnLoadAssetCompleteFunction*)OnLoadAssetComplete);
    }
}
void InitHooks(){
    sleep(1);
    helper = new IL2CPP_Helper();
    helper->Initialize();
    
    QuestUI::Initialize("CustomUI", QuestUIOnInitialized);
    /*QuestUI::Initialize("TestMod 6", QuestUIOnInitializedE);
    QuestUI::Initialize("TestMod 7", QuestUIOnInitializedE);
    QuestUI::Initialize("TestMod 8", QuestUIOnInitializedE);
    QuestUI::Initialize("TestMod 9", QuestUIOnInitializedE);
    QuestUI::Initialize("TestMod 10", QuestUIOnInitializedE);*/
    
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
