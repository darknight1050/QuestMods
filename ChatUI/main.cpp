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
    Vector3 PositionMenu = {0.0f, 3.0f, 4.0f};
    Vector3 RotationMenu = {-36.0f, 0.0f, 0.0f};
    Vector3 ScaleMenu = {1.0f, 1.0f, 1.0f};
    Vector3 PositionGame = {0.0f, 3.0f, 4.0f};
    Vector3 RotationGame = {-36.0f, 0.0f, 0.0f};
    Vector3 ScaleGame = {1.0f, 1.0f, 1.0f};
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
static bool isInMenu = false;

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
    Vector3 scale;
    if(isInMenu){
        helper->RunMethod(objectTransform, "set_position", &Config.PositionMenu);
        helper->RunMethod(objectTransform, "set_eulerAngles", &Config.RotationMenu);
        scale = Config.ScaleMenu;
    }else{
        helper->RunMethod(objectTransform, "set_position", &Config.PositionGame);
        helper->RunMethod(objectTransform, "set_eulerAngles", &Config.RotationGame);
        scale = Config.ScaleGame;
    }
    scale.x *= 0.0025f;
    scale.y *= 0.0025f;
    scale.z *= 0.0025f;
    helper->RunMethod(objectTransform, "set_localScale", &scale);

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
    isInMenu = strcmp(name, "MenuCore") == 0;
    if(isInMenu || strcmp(name, "GameCore") == 0) {
        if(assetBundle == nullptr){
            UnityAssetLoader::LoadAssetBundleFromFileAsync("/sdcard/Android/data/com.beatgames.beatsaber/files/uis/chatUI.qui", (UnityAssetLoader_OnLoadAssetBundleCompleteFunction*)OnLoadAssetBundleComplete);
        }else{
            UnityAssetLoader::LoadAssetFromAssetBundleAsync(assetBundle, (UnityAssetLoader_OnLoadAssetCompleteFunction*)OnLoadAssetComplete);
        }
    }
    return SceneManager_SetActiveScene(scene);
}

void QuestUIOnInitialized(){}

void SaveConfig() {
    log(INFO, "Saving Configuration...");
    config_doc.RemoveAllMembers();
    config_doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = config_doc.GetAllocator();
    config_doc.AddMember("Nick", string(Config.Nick), allocator);
    config_doc.AddMember("OAuth", string(Config.OAuth), allocator);
    config_doc.AddMember("Channel", string(Config.Channel), allocator);
    rapidjson::Value menuValue(rapidjson::kObjectType);
    {
        rapidjson::Value positionValue(rapidjson::kObjectType);
        positionValue.AddMember("X", Config.PositionMenu.x, allocator);
        positionValue.AddMember("Y", Config.PositionMenu.y, allocator);
        positionValue.AddMember("Z", Config.PositionMenu.z, allocator);
        menuValue.AddMember("Position", positionValue, allocator);
        rapidjson::Value rotationValue(rapidjson::kObjectType);
        rotationValue.AddMember("X", Config.RotationMenu.x, allocator);
        rotationValue.AddMember("Y", Config.RotationMenu.y, allocator);
        rotationValue.AddMember("Z", Config.RotationMenu.z, allocator);
        menuValue.AddMember("Rotation", rotationValue, allocator);
        rapidjson::Value scaleValue(rapidjson::kObjectType);
        scaleValue.AddMember("X", Config.ScaleMenu.x, allocator);
        scaleValue.AddMember("Y", Config.ScaleMenu.y, allocator);
        scaleValue.AddMember("Z", Config.ScaleMenu.z, allocator);
        menuValue.AddMember("Scale", scaleValue, allocator);
        config_doc.AddMember("Menu", menuValue, allocator);
    }
    rapidjson::Value gameValue(rapidjson::kObjectType);
    {
        rapidjson::Value positionValue(rapidjson::kObjectType);
        positionValue.AddMember("X", Config.PositionGame.x, allocator);
        positionValue.AddMember("Y", Config.PositionGame.y, allocator);
        positionValue.AddMember("Z", Config.PositionGame.z, allocator);
        gameValue.AddMember("Position", positionValue, allocator);
        rapidjson::Value rotationValue(rapidjson::kObjectType);
        rotationValue.AddMember("X", Config.RotationGame.x, allocator);
        rotationValue.AddMember("Y", Config.RotationGame.y, allocator);
        rotationValue.AddMember("Z", Config.RotationGame.z, allocator);
        gameValue.AddMember("Rotation", rotationValue, allocator);
        rapidjson::Value scaleValue(rapidjson::kObjectType);
        scaleValue.AddMember("X", Config.ScaleGame.x, allocator);
        scaleValue.AddMember("Y", Config.ScaleGame.y, allocator);
        scaleValue.AddMember("Z", Config.ScaleGame.z, allocator);
        gameValue.AddMember("Scale", scaleValue, allocator);
        config_doc.AddMember("Game", gameValue, allocator);
    }
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
    if(config_doc.HasMember("Menu") && config_doc["Menu"].IsObject()){
        rapidjson::Value menuValue = config_doc["Menu"].GetObject();
        if(menuValue.HasMember("Position") && menuValue["Position"].IsObject()){
            rapidjson::Value positionValue = menuValue["Position"].GetObject();
            if(positionValue.HasMember("X") && positionValue["X"].IsFloat() && positionValue.HasMember("Y") && positionValue["Y"].IsFloat() && positionValue.HasMember("Z") && positionValue["Z"].IsFloat()){
                Config.PositionMenu.x = positionValue["X"].GetFloat();
                Config.PositionMenu.y = positionValue["Y"].GetFloat();
                Config.PositionMenu.z = positionValue["Z"].GetFloat();
            }else{
                foundEverything = false;
            }
        }else{
            foundEverything = false;
        }
        if(menuValue.HasMember("Rotation") && menuValue["Rotation"].IsObject()){
            rapidjson::Value rotationValue = menuValue["Rotation"].GetObject();
            if(rotationValue.HasMember("X") && rotationValue["X"].IsFloat() && rotationValue.HasMember("Y") && rotationValue["Y"].IsFloat() && rotationValue.HasMember("Z") && rotationValue["Z"].IsFloat()){
                Config.RotationMenu.x = rotationValue["X"].GetFloat();
                Config.RotationMenu.y = rotationValue["Y"].GetFloat();
                Config.RotationMenu.z = rotationValue["Z"].GetFloat();
            }else{
                foundEverything = false;
            }
        }else{
            foundEverything = false;
        }
        if(menuValue.HasMember("Scale") && menuValue["Scale"].IsObject()){
            rapidjson::Value scaleValue = menuValue["Scale"].GetObject();
            if(scaleValue.HasMember("X") && scaleValue["X"].IsFloat() && scaleValue.HasMember("Y") && scaleValue["Y"].IsFloat() && scaleValue.HasMember("Z") && scaleValue["Z"].IsFloat()){
                Config.ScaleMenu.x = scaleValue["X"].GetFloat();
                Config.ScaleMenu.y = scaleValue["Y"].GetFloat();
                Config.ScaleMenu.z = scaleValue["Z"].GetFloat();
            }else{
                foundEverything = false;
            }
        }else{
            foundEverything = false;
        }
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("Game") && config_doc["Game"].IsObject()){
        rapidjson::Value gameValue = config_doc["Game"].GetObject();
        if(gameValue.HasMember("Position") && gameValue["Position"].IsObject()){
            rapidjson::Value positionValue = gameValue["Position"].GetObject();
            if(positionValue.HasMember("X") && positionValue["X"].IsFloat() && positionValue.HasMember("Y") && positionValue["Y"].IsFloat() && positionValue.HasMember("Z") && positionValue["Z"].IsFloat()){
                Config.PositionGame.x = positionValue["X"].GetFloat();
                Config.PositionGame.y = positionValue["Y"].GetFloat();
                Config.PositionGame.z = positionValue["Z"].GetFloat();
            }else{
                foundEverything = false;
            }
        }else{
            foundEverything = false;
        }
        if(gameValue.HasMember("Rotation") && gameValue["Rotation"].IsObject()){
            rapidjson::Value rotationValue = gameValue["Rotation"].GetObject();
            if(rotationValue.HasMember("X") && rotationValue["X"].IsFloat() && rotationValue.HasMember("Y") && rotationValue["Y"].IsFloat() && rotationValue.HasMember("Z") && rotationValue["Z"].IsFloat()){
                Config.RotationGame.x = rotationValue["X"].GetFloat();
                Config.RotationGame.y = rotationValue["Y"].GetFloat();
                Config.RotationGame.z = rotationValue["Z"].GetFloat();
            }else{
                foundEverything = false;
            }
        }else{
            foundEverything = false;
        }
        if(gameValue.HasMember("Scale") && gameValue["Scale"].IsObject()){
            rapidjson::Value scaleValue = gameValue["Scale"].GetObject();
            if(scaleValue.HasMember("X") && scaleValue["X"].IsFloat() && scaleValue.HasMember("Y") && scaleValue["Y"].IsFloat() && scaleValue.HasMember("Z") && scaleValue["Z"].IsFloat()){
                Config.ScaleGame.x = scaleValue["X"].GetFloat();
                Config.ScaleGame.y = scaleValue["Y"].GetFloat();
                Config.ScaleGame.z = scaleValue["Z"].GetFloat();
            }else{
                foundEverything = false;
            }
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

static void* libil2cpphandle;
MAKE_HOOK_OFFSETLESS(init_hook, void, const char* domain_name) {
    dlclose(libil2cpphandle);
    init_hook(domain_name);
    
    
    INSTALL_HOOK_OFFSETLESS(Camera_FireOnPostRender, helper->class_get_method_from_name(helper->GetClassFromName("UnityEngine", "Camera"), "FireOnPostRender", 1));
    INSTALL_HOOK_OFFSETLESS(SceneManager_SetActiveScene, helper->class_get_method_from_name(helper->GetClassFromName("UnityEngine.SceneManagement", "SceneManager"), "SetActiveScene", 1));
    
    log(INFO, "Successfully installed ChatUI!");
}

__attribute__((constructor)) void lib_main()
{
    #ifdef __aarch64__
    log(INFO, "Is 64 bit!");
    #endif
    if(!LoadConfig())
        SaveConfig();
    log(INFO, "Starting ChatUI installation...");

    libil2cpphandle = dlopen("/data/app/com.beatgames.beatsaber-1/lib/arm64/libil2cpp.so", RTLD_LOCAL | RTLD_LAZY);
    helper = new IL2CPP_Helper();
    helper->Initialize();
    INSTALL_HOOK_DIRECT(init_hook,  helper->init);
}
