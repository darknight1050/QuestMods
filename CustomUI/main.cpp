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

#include "../beatsaber-hook/shared/utils/utils.h"

using namespace std;

static IL2CPP_Helper helper;

void (*ModsButtonCallback)();

void ModsButtonOnPressed(){
    log(INFO, "Button Pressed!");
}

Il2CppObject* textObject;
MAKE_HOOK_OFFSETLESS(MainMenuViewController_DidActivate, void, Il2CppObject* self, bool firstActivation, int type){
    MainMenuViewController_DidActivate(self, firstActivation, type);    
    if(firstActivation){
       
        Il2CppClass* objectClass = helper.GetClassFromName("UnityEngine", "Object");
        const MethodInfo* object_Instantiate = helper.class_get_method_from_name(objectClass, "Instantiate", 1);
     
        bool boolFalse = false;
        bool boolTrue = true;
        Il2CppObject* buttonBinder;
        helper.RunMethod(&buttonBinder, self, "get__buttonBinder");
        auto action = helper.MakeAction(nullptr, ModsButtonCallback, helper.class_get_type(helper.GetClassFromName("System", "Action")));

        Il2CppObject* settingsButton = helper.GetFieldObjectValue(self, "_settingsButton");
        Il2CppObject* settingsButtonTransform;
        helper.RunMethod(&settingsButtonTransform, settingsButton, "get_transform");
        Il2CppObject* settingsButtonTransformParent;
        helper.RunMethod(&settingsButtonTransformParent, settingsButtonTransform, "GetParent");
    
        Il2CppObject* modsButton;
        helper.RunMethod(&modsButton, nullptr, object_Instantiate, settingsButton);
        Il2CppObject* modsButtonTransform;
        helper.RunMethod(&modsButtonTransform, modsButton, "get_transform");
        helper.RunMethod(modsButtonTransform, "SetParent", settingsButtonTransformParent, &boolFalse);

        helper.RunMethod(&textObject, modsButton, "GetComponentInChildren", helper.type_get_object(helper.class_get_type(helper.GetClassFromName("TMPro", "TextMeshProUGUI"))), &boolFalse);
        
        helper.RunMethod(buttonBinder, "AddBinding", modsButton, action);
    }
    Il2CppString* textString = helper.createcsstr("Mods");
    helper.RunMethod(textObject, "set_text", textString);
}

void InitHooks(){
    sleep(1);

    *(void**)(&ModsButtonCallback) = (void*)ModsButtonOnPressed;
    
    helper.Initialize();
  
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
