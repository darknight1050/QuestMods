#include <vector>
#include <thread>
#include <chrono>

#define RAPIDJSON_HAS_STDSTRING 1

#include "../beatsaber-hook/shared/utils/utils.h"
#include "../questui/questui.hpp"
#include "../TwitchIRC/TwitchIRCClient.hpp"

using namespace std;

static bool boolTrue = true;
static bool boolFalse = false;

static struct Config_t {
    char* Nick = "";
    char* OAuth = "";
    char* Channel = "";
    Vector3 Position = {0.0f, 3.0f, 4.0f};
    Vector3 Rotation = {-36.0f, 0.0f, 0.0f};
    Vector3 Scale = {0.0025f, 0.0025f, 0.0025f};
} Config;

struct ChatObject {
    string Text;
    Il2CppObject* GameObject;
};

static rapidjson::Document& config_doc = Configuration::config;

static IL2CPP_Helper* helper = nullptr;
static TwitchIRCClient* client = nullptr;
static Il2CppObject* assetBundle = nullptr;
static Il2CppObject* customUIObject = nullptr;
static Il2CppObject* chatObject_Template = nullptr;
static vector<ChatObject*> chatObjects;
static vector<ChatObject*> chatObjectsToAdd;
static const int maxVisibleObjects = 24;

static long long lastUpdate = 0;
static bool needUpdate = false;
static bool threadStarted = false;

void UpdateList(){
    if(!chatObjectsToAdd.empty())
        needUpdate = true;
    long long currentTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();;
    if(chatObject_Template != nullptr && needUpdate && currentTime-lastUpdate > 100){
        needUpdate = false;
        lastUpdate = currentTime;
        if(!chatObjectsToAdd.empty()){
            chatObjects.push_back(chatObjectsToAdd[0]);
            chatObjectsToAdd.erase(chatObjectsToAdd.begin());
        }
        for(int i = 0;i<chatObjects.size();i++){
                ChatObject* chatObject = chatObjects[i];
                if(chatObject->GameObject == nullptr){
                    ChatObject* lastChatObject = nullptr;
                    if(i > 0)
                        lastChatObject = chatObjects[i-1];
                    Vector3 lastChatObjectPosition;
                    Vector3 lastChatObjectSize;
                    if(lastChatObject != nullptr && lastChatObject->GameObject != nullptr){
                        helper->RunMethod(&lastChatObjectPosition, lastChatObject->GameObject, "get_localPosition");
                        helper->RunMethod(&lastChatObjectSize, lastChatObject->GameObject, "get_sizeDelta");
                    }
                    helper->RunMethod(&chatObject->GameObject, nullptr, helper->class_get_method_from_name(helper->GetClassFromName("UnityEngine", "Object"), "Instantiate", 1), chatObject_Template);
                    helper->RunMethod(chatObject->GameObject, "set_name", helper->createcsstr("ChatObject"));
                    UnityHelper::SetSameParent(helper, chatObject->GameObject, chatObject_Template);
                    UnityHelper::SetGameObjectActive(helper, chatObject->GameObject, true);
                    UnityHelper::SetButtonText(helper, chatObject->GameObject, chatObject->Text);
                }
            }
        while(maxVisibleObjects<chatObjects.size()){
            ChatObject* chatObject = chatObjects[0];
            if(chatObject->GameObject != nullptr)
                helper->RunStaticMethod(helper->GetClassFromName("UnityEngine", "Object"), "Destroy", UnityHelper::GetGameObject(helper, chatObject->GameObject));
            delete chatObject;
            chatObjects.erase(chatObjects.begin());
        }
    }
}

void AddChatObject(string text){
    ChatObject* chatObject = new ChatObject();
    chatObject->Text.assign(text.c_str(), text.size());
    chatObjectsToAdd.push_back(chatObject);   
}

void OnChatMessage(IRCMessage message, TwitchIRCClient* client)
{    
    string text = message.prefix.nick + ": " + message.parameters.at(message.parameters.size() - 1);
    
    log(INFO, "Twitch Chat: %s", text.c_str());
    AddChatObject(text);
}

void ConnectTwitch(){
    client = new TwitchIRCClient();
    if (client->InitSocket())
    {
        if (client->Connect())
        {
            if (client->Login(Config.Nick, Config.OAuth))
            {
                log(INFO, "Twitch Chat: Logged In as %s!", Config.Nick);
                client->HookIRCCommand("PRIVMSG", OnChatMessage);
                if (client->JoinChannel(Config.Channel)){
                    log(INFO, "Twitch Chat: Joined Channel %s!", Config.Channel);
                    while (client->Connected())
                        client->ReceiveData();
                }
            }
            log(INFO, "Twitch Chat: Disconnected!");
        }
    }
}

void OnLoadAssetComplete(Il2CppObject* asset){
    helper->RunMethod(&customUIObject, nullptr, helper->class_get_method_from_name(helper->GetClassFromName("UnityEngine", "Object"), "Instantiate", 1), asset);

    Il2CppObject* objectTransform;
    helper->RunMethod(&objectTransform, customUIObject, "get_transform");

    helper->RunMethod(objectTransform, "set_position", &Config.Position);

    helper->RunMethod(objectTransform, "set_eulerAngles", &Config.Rotation);
   
    helper->RunMethod(objectTransform, "set_localScale", &Config.Scale);

    UnityHelper::SetGameObjectActive(helper, customUIObject, true);
    chatObject_Template = UnityHelper::GetComponentInChildren(helper, customUIObject, helper->GetClassFromName("UnityEngine", "RectTransform"), "ChatObject_Template");
    UnityHelper::SetGameObjectActive(helper, chatObject_Template, false);
    needUpdate = true;
    if(!threadStarted){
        threadStarted = true;
        thread testThread(ConnectTwitch);
        testThread.detach();
    }
}

void OnLoadAssetBundleComplete(Il2CppObject* assetBundleArg){
    assetBundle = assetBundleArg;
    UnityAssetLoader::LoadAssetFromAssetBundleAsync(assetBundle, (UnityAssetLoader_OnLoadAssetCompleteFunction*)OnLoadAssetComplete);
}

MAKE_HOOK_OFFSETLESS(Camera_FireOnPostRender, void, Il2CppObject* cam)
{   
    Camera_FireOnPostRender(cam);
    UpdateList();
}

MAKE_HOOK_OFFSETLESS(SceneManager_SetActiveScene, bool, int scene)
{
    if(customUIObject != nullptr){
        chatObject_Template = nullptr;
        for(int i = 0; i<chatObjects.size(); i++){
            chatObjects[i]->GameObject = nullptr;
        }
        helper->RunStaticMethod(customUIObject, "Destroy", customUIObject);
        customUIObject = nullptr;
        log(INFO, "Destroyed ChatUI!");
    }
    Il2CppString* nameObject;
    helper->RunStaticMethod(&nameObject, helper->GetClassFromName("UnityEngine.SceneManagement", "Scene"), "GetNameInternal", &scene);
    const char* name = to_utf8(csstrtostr(nameObject)).c_str();
    log(INFO, "Scene: %s", name);
    if(strcmp(name, "MenuCore") == 0 || strcmp(name, "GameCore") == 0) {
        if(assetBundle == nullptr){
            UnityAssetLoader::LoadAssetBundleFromFileAsync("/sdcard/Android/data/com.beatgames.beatsaber/files/uis/chatUI.qui", (UnityAssetLoader_OnLoadAssetBundleCompleteFunction*)OnLoadAssetBundleComplete);
        }else{
            UnityAssetLoader::LoadAssetFromAssetBundleAsync(assetBundle, (UnityAssetLoader_OnLoadAssetCompleteFunction*)OnLoadAssetComplete);
        }
    }
    return SceneManager_SetActiveScene(scene);
}

void QuestUIOnInitialized(){}

void InitHooks(){
    sleep(1);
    helper = new IL2CPP_Helper();
    helper->Initialize();
    
    INSTALL_HOOK_OFFSETLESS(Camera_FireOnPostRender, helper->class_get_method_from_name(helper->GetClassFromName("UnityEngine", "Camera"), "FireOnPostRender", 1));
    INSTALL_HOOK_OFFSETLESS(SceneManager_SetActiveScene, helper->class_get_method_from_name(helper->GetClassFromName("UnityEngine.SceneManagement", "SceneManager"), "SetActiveScene", 1));
    
    log(INFO, "Successfully installed ChatUI!");
}

void SaveConfig() {
    log(INFO, "Saving Configuration...");
    config_doc.RemoveAllMembers();
    config_doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = config_doc.GetAllocator();
    config_doc.AddMember("Nick", string(Config.Nick), allocator);
    config_doc.AddMember("OAuth", string(Config.OAuth), allocator);
    config_doc.AddMember("Channel", string(Config.Channel), allocator);
    rapidjson::Value positionValue(rapidjson::kObjectType);
    positionValue.AddMember("X", Config.Position.x, allocator);
    positionValue.AddMember("Y", Config.Position.y, allocator);
    positionValue.AddMember("Z", Config.Position.z, allocator);
    config_doc.AddMember("Position", positionValue, allocator);
    rapidjson::Value rotationValue(rapidjson::kObjectType);
    rotationValue.AddMember("X", Config.Rotation.x, allocator);
    rotationValue.AddMember("Y", Config.Rotation.y, allocator);
    rotationValue.AddMember("Z", Config.Rotation.z, allocator);
    config_doc.AddMember("Rotation", rotationValue, allocator);
    rapidjson::Value scaleValue(rapidjson::kObjectType);
    scaleValue.AddMember("X", Config.Scale.x, allocator);
    scaleValue.AddMember("Y", Config.Scale.y, allocator);
    scaleValue.AddMember("Z", Config.Scale.z, allocator);
    config_doc.AddMember("Scale", scaleValue, allocator);
    Configuration::Write();
    log(INFO, "Saved Configuration!");
}

bool LoadConfig() { 
    log(INFO, "Loading Configuration...");
    Configuration::Load();
    bool foundEverything = true;
    if(config_doc.HasMember("Nick") && config_doc["Nick"].IsString()){
        char* buffer = (char*)malloc(config_doc["Nick"].GetStringLength());
        strcpy(buffer, config_doc["Nick"].GetString());
        Config.Nick = buffer; 
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("OAuth") && config_doc["OAuth"].IsString()){
        char* buffer = (char*)malloc(config_doc["OAuth"].GetStringLength());
        strcpy(buffer, config_doc["OAuth"].GetString());
        Config.OAuth = buffer;   
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("Channel") && config_doc["Channel"].IsString()){
        char* buffer = (char*)malloc(config_doc["Channel"].GetStringLength());
        strcpy(buffer, config_doc["Channel"].GetString());
        Config.Channel = buffer; 
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("Position") && config_doc["Position"].IsObject()){
        rapidjson::Value positionValue = config_doc["Position"].GetObject();
        if(positionValue.HasMember("X") && positionValue["X"].IsFloat() && positionValue.HasMember("Y") && positionValue["Y"].IsFloat() && positionValue.HasMember("Z") && positionValue["Z"].IsFloat()){
            Config.Position.x = positionValue["X"].GetFloat();
            Config.Position.y = positionValue["Y"].GetFloat();
            Config.Position.z = positionValue["Z"].GetFloat();
        }else{
            foundEverything = false;
        }
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("Rotation") && config_doc["Rotation"].IsObject()){
        rapidjson::Value rotationValue = config_doc["Rotation"].GetObject();
        if(rotationValue.HasMember("X") && rotationValue["X"].IsFloat() && rotationValue.HasMember("Y") && rotationValue["Y"].IsFloat() && rotationValue.HasMember("Z") && rotationValue["Z"].IsFloat()){
            Config.Rotation.x = rotationValue["X"].GetFloat();
            Config.Rotation.y = rotationValue["Y"].GetFloat();
            Config.Rotation.z = rotationValue["Z"].GetFloat();
        }else{
            foundEverything = false;
        }
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("Scale") && config_doc["Scale"].IsObject()){
        rapidjson::Value scaleValue = config_doc["Scale"].GetObject();
        if(scaleValue.HasMember("X") && scaleValue["X"].IsFloat() && scaleValue.HasMember("Y") && scaleValue["Y"].IsFloat() && scaleValue.HasMember("Z") && scaleValue["Z"].IsFloat()){
            Config.Scale.x = scaleValue["X"].GetFloat();
            Config.Scale.y = scaleValue["Y"].GetFloat();
            Config.Scale.z = scaleValue["Z"].GetFloat();
        }else{
            foundEverything = false;
        }
    }else{
        foundEverything = false;
    }
    if(foundEverything){
        log(INFO, "Loaded Configuration!");
        return true;
    }
    return false;
}

__attribute__((constructor)) void lib_main()
{
    #ifdef __aarch64__
    log(INFO, "Is 64 bit!");
    #endif
    if(!LoadConfig())
        SaveConfig();
    log(INFO, "Starting ChatUI installation...");
    thread initHooksThread(InitHooks);
    initHooksThread.detach();
}
