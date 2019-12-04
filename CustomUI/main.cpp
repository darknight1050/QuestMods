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

static Il2CppObject* assetBundle = nullptr;
static Il2CppObject* customUIObject = nullptr;

static int counter = 0;

void UpdateTextCounter(){
    char text[64];
    sprintf(text, "%d", counter);
    Il2CppObject* textObject = UnityHelper::GetComponentInChildren(customUIObject, il2cpp_utils::GetClassFromName("TMPro", "TextMeshProUGUI"), "TextCounter");
    il2cpp_utils::RunMethod(textObject, "set_text", il2cpp_utils::createcsstr(text));
}


void ButtonCounterClick(Il2CppObject* button){
    log(INFO, "ButtonCounterClick!");
    counter++;
    UpdateTextCounter();
}

void OnLoadAssetComplete(Il2CppObject* asset){
    il2cpp_utils::RunMethod(&customUIObject, il2cpp_utils::GetClassFromName("UnityEngine", "Object"), "Instantiate", asset);
    UnityHelper::SetParent(customUIObject, QuestUI::GetQuestUIModInfo().Panel);

    UnityHelper::AddButtonOnClick(QuestUI::GetQuestUIInfo()->ButtonBinder, customUIObject, "ButtonCounter", (UnityHelper::ButtonOnClickFunction*)ButtonCounterClick);

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

extern "C" void load()
{
    il2cpp_functions::Init();
    QuestUI::Initialize("CustomUI", QuestUIOnInitialized);

    log(INFO, "Successfully installed CustomUI!");
}
