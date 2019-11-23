#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <iomanip>

#define RAPIDJSON_HAS_STDSTRING 1

#include "../beatsaber-hook/shared/utils/utils.h"

#define WAIT_TIME 500

using namespace std;

static IL2CPP_Helper* helper = nullptr;

static bool isInMenu = false;

static long long LastPrimaryThumbstickUp = 0;
static long long LastPrimaryThumbstickDown = 0;
static long long LastPrimaryThumbstickLeft = 0;
static long long LastPrimaryThumbstickRight = 0;
static long long LastSecondaryThumbstickUp = 0;
static long long LastSecondaryThumbstickDown = 0;
static long long LastSecondaryThumbstickLeft = 0;
static long long LastSecondaryThumbstickRight = 0;

namespace Buttons
{
    static int None = 0;
    static int One = 1;
    static int Two = 2;
    static int Three = 4;
    static int Four = 8;
    static int Start = 256;
    static int Back = 512;
    static int PrimaryShoulder = 4096;
    static int PrimaryIndexTrigger = 8192;
    static int PrimaryHandTrigger = 16384;
    static int PrimaryThumbstick = 32768;
    static int PrimaryThumbstickUp = 65536;
    static int PrimaryThumbstickDown = 131072;
    static int PrimaryThumbstickLeft = 262144;
    static int PrimaryThumbstickRight = 524288;
    static int PrimaryTouchpad = 1024;
    static int SecondaryShoulder = 1048576;
    static int SecondaryIndexTrigger = 2097152;
    static int SecondaryHandTrigger = 4194304;
    static int SecondaryThumbstick = 8388608;
    static int SecondaryThumbstickUp = 16777216;
    static int SecondaryThumbstickDown = 33554432;
    static int SecondaryThumbstickLeft = 67108864;
    static int SecondaryThumbstickRight = 134217728;
    static int SecondaryTouchpad = 2048;
    static int DpadUp = 16;
    static int DpadDown = 32;
    static int DpadLeft = 64;
    static int DpadRight = 128;
    static int Up = 268435456;
    static int Down = 536870912;
    static int Left = 1073741824;
    static int Right = -2147483648;
    static int Any = -1;
}

Il2CppObject* GetFirstObjectOfType(Il2CppClass* klass){
    Il2CppClass* resourcesClass = helper->GetClassFromName("UnityEngine", "Resources");
    const MethodInfo* resources_FindObjectsOfTypeAllMethod = helper->class_get_method_from_name(resourcesClass, "FindObjectsOfTypeAll", 1);
    Array<Il2CppObject*>* objects;
    helper->RunMethod(&objects, nullptr, resources_FindObjectsOfTypeAllMethod, helper->type_get_object(helper->class_get_type(klass)));
    if (objects != nullptr)
    {
        return objects->values[0];
    }
    else
    {
        return nullptr;
    }
}

MAKE_HOOK_OFFSETLESS(SceneManager_SetActiveScene, bool, int scene)
{
    Il2CppString* nameObject;
    helper->RunStaticMethod(&nameObject, helper->GetClassFromName("UnityEngine.SceneManagement", "Scene"), "GetNameInternal", &scene);
    const char* name = to_utf8(csstrtostr(nameObject)).c_str();
    log(INFO, "Scene: %s", name);
    isInMenu = strcmp(name, "MenuCore") == 0;
    return SceneManager_SetActiveScene(scene);
}

MAKE_HOOK_OFFSETLESS(OVRInput_Update, void)
{   
    OVRInput_Update();
    long long currentTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();;
    bool primaryThumbstickUp;
    bool primaryThumbstickDown;
    bool primaryThumbstickLeft;
    bool primaryThumbstickRight;
    bool secondaryThumbstickUp;
    bool secondaryThumbstickDown;
    bool secondaryThumbstickLeft;
    bool secondaryThumbstickRight;
    int none = 0, active = 0x80000000;
    helper->RunStaticMethod(&primaryThumbstickUp, helper->GetClassFromName("", "OVRInput"), "GetResolvedButton", &Buttons::PrimaryThumbstickUp, &none, &active);
    helper->RunStaticMethod(&primaryThumbstickDown, helper->GetClassFromName("", "OVRInput"), "GetResolvedButton", &Buttons::PrimaryThumbstickDown, &none, &active);
    helper->RunStaticMethod(&primaryThumbstickLeft, helper->GetClassFromName("", "OVRInput"), "GetResolvedButton", &Buttons::PrimaryThumbstickLeft, &none, &active);
    helper->RunStaticMethod(&primaryThumbstickRight, helper->GetClassFromName("", "OVRInput"), "GetResolvedButton", &Buttons::PrimaryThumbstickRight, &none, &active);
    helper->RunStaticMethod(&secondaryThumbstickUp, helper->GetClassFromName("", "OVRInput"), "GetResolvedButton", &Buttons::SecondaryThumbstickUp, &none, &active);
    helper->RunStaticMethod(&secondaryThumbstickDown, helper->GetClassFromName("", "OVRInput"), "GetResolvedButton", &Buttons::SecondaryThumbstickDown, &none, &active);
    helper->RunStaticMethod(&secondaryThumbstickLeft, helper->GetClassFromName("", "OVRInput"), "GetResolvedButton", &Buttons::SecondaryThumbstickLeft, &none, &active);
    helper->RunStaticMethod(&secondaryThumbstickRight, helper->GetClassFromName("", "OVRInput"), "GetResolvedButton", &Buttons::SecondaryThumbstickRight, &none, &active);
    if(isInMenu){
        Il2CppObject* levelPacksViewController = GetFirstObjectOfType(helper->GetClassFromName("", "LevelPacksViewController"));    
        if(levelPacksViewController != nullptr){        
            Il2CppObject* levelPacksTableView = helper->GetFieldObjectValue(levelPacksViewController, "_levelPacksTableView");
            if(levelPacksTableView != nullptr){
                Il2CppObject* tableView = helper->GetFieldObjectValue(levelPacksTableView, "_tableView");
                if(tableView != nullptr){
                    Il2CppObject* pageUpButton = helper->GetFieldObjectValue(tableView, "_pageUpButton");
                    Il2CppObject* pageDownButton = helper->GetFieldObjectValue(tableView, "_pageDownButton");
                    if((primaryThumbstickLeft && currentTime-LastPrimaryThumbstickLeft > WAIT_TIME) || (secondaryThumbstickLeft && currentTime-LastSecondaryThumbstickLeft > WAIT_TIME)){
                        Il2CppObject* onClick;
                        helper->RunMethod(&onClick, pageUpButton, "get_onClick");
                        helper->RunMethod(onClick, "Invoke");
                    }
                    if((primaryThumbstickRight && currentTime-LastPrimaryThumbstickRight > WAIT_TIME) || (secondaryThumbstickRight && currentTime-LastSecondaryThumbstickRight > WAIT_TIME)){
                        Il2CppObject* onClick;
                        helper->RunMethod(&onClick, pageDownButton, "get_onClick");
                        helper->RunMethod(onClick, "Invoke");
                    }
                }
            }
            
        }
        Il2CppObject* levelPackLevelsViewController = GetFirstObjectOfType(helper->GetClassFromName("", "LevelPackLevelsViewController"));    
        if(levelPackLevelsViewController != nullptr){        
            Il2CppObject* levelPackLevelsTableView = helper->GetFieldObjectValue(levelPackLevelsViewController, "_levelPackLevelsTableView");
            if(levelPackLevelsTableView != nullptr){
                Il2CppObject* tableView = helper->GetFieldObjectValue(levelPackLevelsTableView, "_tableView");
                if(tableView != nullptr){
                    Il2CppObject* pageUpButton = helper->GetFieldObjectValue(tableView, "_pageUpButton");
                    Il2CppObject* pageDownButton = helper->GetFieldObjectValue(tableView, "_pageDownButton");
                    if((primaryThumbstickUp && currentTime-LastPrimaryThumbstickUp > WAIT_TIME) || (secondaryThumbstickUp && currentTime-LastSecondaryThumbstickUp > WAIT_TIME)){
                        Il2CppObject* onClick;
                        helper->RunMethod(&onClick, pageUpButton, "get_onClick");
                        helper->RunMethod(onClick, "Invoke");
                    }
                    if((primaryThumbstickDown && currentTime-LastPrimaryThumbstickDown > WAIT_TIME) || (secondaryThumbstickDown && currentTime-LastSecondaryThumbstickDown > WAIT_TIME)){
                        Il2CppObject* onClick;
                        helper->RunMethod(&onClick, pageDownButton, "get_onClick");
                        helper->RunMethod(onClick, "Invoke");
                    }
                }
            }
        }
            
    }
    if(primaryThumbstickUp){
        if(LastPrimaryThumbstickUp == 0)
            LastPrimaryThumbstickUp = currentTime;
    }else{
        LastPrimaryThumbstickUp = 0;
    }
    if(primaryThumbstickDown){
        if(LastPrimaryThumbstickDown == 0)
            LastPrimaryThumbstickDown = currentTime;
    }else{
        LastPrimaryThumbstickDown = 0;
    }
    if(primaryThumbstickLeft){
        if(LastPrimaryThumbstickLeft == 0)
            LastPrimaryThumbstickLeft = currentTime;
    }else{
        LastPrimaryThumbstickLeft = 0;
    }
    if(primaryThumbstickRight){
        if(LastPrimaryThumbstickRight == 0)
            LastPrimaryThumbstickRight = currentTime;
    }else{
        LastPrimaryThumbstickRight = 0;
    }
    if(secondaryThumbstickUp){
        if(LastSecondaryThumbstickUp == 0)
            LastSecondaryThumbstickUp = currentTime;
    }else{
        LastSecondaryThumbstickUp = 0;
    }
    if(secondaryThumbstickDown){
        if(LastSecondaryThumbstickDown == 0)
            LastSecondaryThumbstickDown = currentTime;
    }else{
        LastSecondaryThumbstickDown = 0;
    }
    if(secondaryThumbstickLeft){
        if(LastSecondaryThumbstickLeft == 0)
            LastSecondaryThumbstickLeft = currentTime;
    }else{
        LastSecondaryThumbstickLeft = 0;
    }
    if(secondaryThumbstickRight){
        if(LastSecondaryThumbstickRight == 0)
            LastSecondaryThumbstickRight = currentTime;
    }else{
        LastSecondaryThumbstickRight = 0;
    }
}

static void* libil2cpphandle;
MAKE_HOOK_OFFSETLESS(init_hook, void, const char* domain_name) {
    dlclose(libil2cpphandle);
    init_hook(domain_name);
    
    INSTALL_HOOK_OFFSETLESS(OVRInput_Update, helper->class_get_method_from_name(helper->GetClassFromName("", "OVRInput"), "Update", 0));
    INSTALL_HOOK_OFFSETLESS(SceneManager_SetActiveScene, helper->class_get_method_from_name(helper->GetClassFromName("UnityEngine.SceneManagement", "SceneManager"), "SetActiveScene", 1));
    
    log(INFO, "Successfully installed SongHelper!");
}

__attribute__((constructor)) void lib_main()
{
    #ifdef __aarch64__
    log(INFO, "Is 64 bit!");
    #endif
    log(INFO, "Starting SongHelper installation...");

    libil2cpphandle = dlopen("/data/app/com.beatgames.beatsaber-1/lib/arm64/libil2cpp.so", RTLD_LOCAL | RTLD_LAZY);
    helper = new IL2CPP_Helper();
    helper->Initialize();
    INSTALL_HOOK_DIRECT(init_hook,  helper->init);
}
