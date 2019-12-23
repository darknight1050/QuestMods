#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <iomanip>

#define RAPIDJSON_HAS_STDSTRING 1

#include "../beatsaber-hook/shared/utils/utils.h"

#define WAIT_TIME 175

using namespace std;

static bool isInMenu = false;

static long long LastThumbstickVertical = 0;

Il2CppObject* GetFirstObjectOfType(Il2CppClass* klass){
    Array<Il2CppObject*>* objects;
    il2cpp_utils::RunMethod(&objects, il2cpp_utils::GetClassFromName("UnityEngine", "Resources"), "FindObjectsOfTypeAll", il2cpp_functions::type_get_object(il2cpp_functions::class_get_type(klass)));
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
    bool value = SceneManager_SetActiveScene(scene);
    Il2CppString* nameObject;
    il2cpp_utils::RunMethod(&nameObject, il2cpp_utils::GetClassFromName("UnityEngine.SceneManagement", "Scene"), "GetNameInternal", &scene);
    const char* name = to_utf8(csstrtostr(nameObject)).c_str();
    log(INFO, "Scene: %s", name);
    isInMenu = strcmp(name, "MenuViewControllers") == 0;
    return value;
}

void UpdateThread()
{   
    while(true){
        usleep(10*1000);
        if(isInMenu){
            long long currentTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();;
            float thumbstickVertical = 0.0f;
            const MethodInfo* getAxisMethod = il2cpp_utils::GetMethod("UnityEngine", "Input", "GetAxis", 1);
            il2cpp_utils::RunMethod(&thumbstickVertical, nullptr, getAxisMethod, il2cpp_utils::createcsstr("VerticalRightHand"));
            if(thumbstickVertical == 0.0f){
                il2cpp_utils::RunMethod(&thumbstickVertical, nullptr, getAxisMethod, il2cpp_utils::createcsstr("VerticalLeftHand"));
            }
            Il2CppObject* levelCollectionViewController = GetFirstObjectOfType(il2cpp_utils::GetClassFromName("", "LevelCollectionViewController"));    
            if(levelCollectionViewController != nullptr){        
                Il2CppObject* levelCollectionTableView = il2cpp_utils::GetFieldValue(levelCollectionViewController, "_levelCollectionTableView");
                if(levelCollectionTableView != nullptr){
                    Il2CppObject* tableView = il2cpp_utils::GetFieldValue(levelCollectionTableView, "_tableView");
                    if(tableView != nullptr){
                        if(thumbstickVertical < -0.5f && currentTime-LastThumbstickVertical > WAIT_TIME){
                            Il2CppObject* pageUpButton = il2cpp_utils::GetFieldValue(tableView, "_pageUpButton");
                            if(pageUpButton != nullptr){
                                Il2CppObject* onClick;
                                il2cpp_utils::RunMethod(&onClick, pageUpButton, "get_onClick");
                                il2cpp_utils::RunMethod(onClick, "Invoke");
                            }
                            LastThumbstickVertical = currentTime;
                        }else
                        if(thumbstickVertical > 0.5f && currentTime-LastThumbstickVertical > WAIT_TIME){
                            Il2CppObject* pageDownButton = il2cpp_utils::GetFieldValue(tableView, "_pageDownButton");
                            if(pageDownButton != nullptr){
                                Il2CppObject* onClick;
                                il2cpp_utils::RunMethod(&onClick, pageDownButton, "get_onClick");
                                il2cpp_utils::RunMethod(onClick, "Invoke");
                            }
                            LastThumbstickVertical = currentTime;
                        }else if(thumbstickVertical == 0.0f){
                            LastThumbstickVertical = 0;
                        }
                    }
                }
            }
        }
    }
}

extern "C" void load()
{
    #ifdef __aarch64__
    log(INFO, "Is 64 bit!");
    #endif
    log(INFO, "Starting SongHelper installation...");
    il2cpp_functions::Init();
    INSTALL_HOOK_OFFSETLESS(SceneManager_SetActiveScene, il2cpp_utils::GetMethod("UnityEngine.SceneManagement", "SceneManager", "SetActiveScene", 1));
    thread updateThread(UpdateThread);
    updateThread.detach();
    log(INFO, "Successfully installed SongHelper!");
}