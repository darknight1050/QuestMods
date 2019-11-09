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
#include "../questui/questui.hpp"

using namespace std;

static IL2CPP_Helper* helper = nullptr;

static ConfigDocument& config_doc = Configuration::config;

static struct Config_t {
    bool Lights = true;
    bool Walls = true;
    bool Sabers = true;
    bool Trails = true;
    bool Notes = true;
    bool QSabers = false;
    double SaberASpeed = 1.0;
    double SaberBSpeed = 1.0;
    double SabersStartDiff = 180.0;
    double LightASpeed = 2.0;
    double LightBSpeed = 2.0;
    double LightsStartDiff = 180.0;
    double WallsSpeed = 2.0;
} Config;

typedef struct ColorScheme : public Il2CppObject {
    char colorSchemeId[8];
    char colorSchemeName[8];
    int isEditable;
    Color saberAColor;
    Color saberBColor;
    Color environmentColor0;
    Color environmentColor1;
    Color obstaclesColor;
} ColorScheme;

static Il2CppObject* assetBundle = nullptr;
static Il2CppObject* customUIObject = nullptr;

static Il2CppObject* textSave = nullptr;
static Il2CppObject* toggleLights = nullptr;
static Il2CppObject* toggleWalls = nullptr;
static Il2CppObject* toggleSabers = nullptr;
static Il2CppObject* toggleTrails = nullptr;
static Il2CppObject* toggleNotes = nullptr;
static Il2CppObject* toggleQSabers = nullptr;

static bool isInTutorial = false; 

static bool settingColorScheme = false;

static ColorScheme defaultColorScheme;
static ColorScheme colorScheme;

static float saberA = 0;
static float saberB = 0;
static float environmentColor0 = 0;
static float environmentColor1 = 0;
static float obstaclesColor = 0;

static map<Il2CppObject*, vector<Il2CppObject*>> sabersMaterials; 

Color ColorFromHSB(float hue, float saturation, float brightness){
    hue/=360.0f;
    int r = 0, g = 0, b = 0;
    if (saturation == 0)
    {
        r = g = b = (int)(brightness * 255.0f + 0.5f);
    }
    else
    {
        float h = (hue - (float)floor (hue)) * 6.0f;
        float f = h - (float)floor (h);
        float p = brightness * (1.0f - saturation);
        float q = brightness * (1.0f - saturation * f);
        float t = brightness * (1.0f - (saturation * (1.0f - f)));
        switch ((int)h)
        {
            case 0:
                r = (int)(brightness * 255.0f + 0.5f);
                g = (int)(t * 255.0f + 0.5f);
                b = (int)(p * 255.0f + 0.5f);
                break;
            case 1:
                r = (int)(q * 255.0f + 0.5f);
                g = (int)(brightness * 255.0f + 0.5f);
                b = (int)(p * 255.0f + 0.5f);
                break;
            case 2:
                r = (int)(p * 255.0f + 0.5f);
                g = (int)(brightness * 255.0f + 0.5f);
                b = (int)(t * 255.0f + 0.5f);
                break;
            case 3:
                r = (int)(p * 255.0f + 0.5f);
                g = (int)(q * 255.0f + 0.5f);
                b = (int)(brightness * 255.0f + 0.5f);
                break;
            case 4:
                r = (int)(t * 255.0f + 0.5f);
                g = (int)(p * 255.0f + 0.5f);
                b = (int)(brightness * 255.0f + 0.5f);
                break;
            case 5:
                r = (int)(brightness * 255.0f + 0.5f);
                g = (int)(p * 255.0f + 0.5f);
                b = (int)(q * 255.0f + 0.5f);
                break;
        }
    }
    Color color;
    color.r = r/255.0f;
    color.g = g/255.0f;
    color.b = b/255.0f;
    color.a = 1.0f;
    return color;
}

Color GetColorFromManager(Il2CppObject* colorManager, const char* fieldName){
    Color color;
    helper->GetFieldValue(&color, helper->GetFieldObjectValue(colorManager, fieldName), "_color");
    return color; 
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

void CacheSaber(Il2CppObject* saber){
    if(!isInTutorial){
        if(Config.QSabers){
            vector<Il2CppObject*> materials; 
            bool getInactive = false;
            Il2CppString* glowString = helper->createcsstr("_Glow");
            Il2CppString* bloomString = helper->createcsstr("_Bloom");
            Il2CppClass* shaderClass = helper->GetClassFromName("UnityEngine", "Shader");
            const MethodInfo* shader_PropertyToIDMethod = helper->class_get_method_from_name(shaderClass, "PropertyToID", 1);
            int glowID, bloomID;
            helper->RunMethod(&glowID, nullptr, shader_PropertyToIDMethod, glowString);
            helper->RunMethod(&bloomID, nullptr, shader_PropertyToIDMethod, bloomString);
            Array<Il2CppObject*>* childTransforms;
            helper->RunMethod(&childTransforms, saber, "GetComponentsInChildren", helper->type_get_object(helper->class_get_type(helper->GetClassFromName("UnityEngine", "Transform"))), &getInactive);
            for (int i= 0; i< childTransforms->Length(); i++)
            {
                Array<Il2CppObject*>* renderers;
                helper->RunMethod(&renderers, childTransforms->values[i], "GetComponentsInChildren", helper->type_get_object(helper->class_get_type(helper->GetClassFromName("UnityEngine", "Renderer"))), &getInactive);
                for (int j = 0; j < renderers->Length(); j++)
                {
                    Array<Il2CppObject*>* sharedMaterials;
                    helper->RunMethod(&sharedMaterials, renderers->values[j], "get_sharedMaterials");
                    for (int h = 0; h < sharedMaterials->Length(); h++)
                    {
                        Il2CppObject* material = sharedMaterials->values[h];
                        bool setColor = false;
                        bool hasGlow;
                        helper->RunMethod(&hasGlow, material, "HasProperty", &glowID);
                        if (hasGlow)
                        {
                            float glowFloat;
                            helper->RunMethod(&glowFloat, material, "GetFloat", &glowID);
                            if (glowFloat > 0)
                                setColor = true;
                        }
                        if (!setColor)
                        {
                            bool hasBloom;
                            helper->RunMethod(&hasBloom, material, "HasProperty", &bloomID);
                            if (hasBloom)
                            {
                                float bloomFloat;
                                helper->RunMethod(&bloomFloat, material, "GetFloat", &bloomID);
                                if (bloomFloat > 0)
                                    setColor = true;
                            }
                        }
                        if (setColor)
                        {
                            materials.push_back(material); 
                        }
                    }
                }
            }
            if(materials.size() > 0)
                sabersMaterials[saber] = materials;
        }
    }
    
}

void SetSaberColor(Il2CppObject* saber, Color color){
    Il2CppString* colorString = helper->createcsstr("_Color");
    map<Il2CppObject*, vector<Il2CppObject*>>::iterator it = sabersMaterials.find(saber);
    if(it == sabersMaterials.end())
    {
        CacheSaber(saber);
    }
    for (Il2CppObject* material : sabersMaterials[saber]) 
    {
        helper->RunMethod(material, "SetColor", colorString, &color);
    }
}

MAKE_HOOK_OFFSETLESS(TutorialController_Awake, void, Il2CppObject* self){
    TutorialController_Awake(self);
    isInTutorial = true;
}

MAKE_HOOK_OFFSETLESS(TutorialController_OnDestroy, void, Il2CppObject* self){
    TutorialController_OnDestroy(self);
    isInTutorial = false;
}
MAKE_HOOK_OFFSETLESS(ColorManager__SetColorScheme, void, Il2CppObject* self, ColorScheme scheme){
    if(!settingColorScheme){
        defaultColorScheme = scheme;
    }
    settingColorScheme = false;
    ColorManager__SetColorScheme(self, scheme);
}

MAKE_HOOK_OFFSETLESS(SaberManager_Update, void, Il2CppObject* self){
    Il2CppObject* colorManager = GetFirstObjectOfType(helper->GetClassFromName("", "ColorManager"));
    if (colorManager != nullptr)
    {
        if(!isInTutorial && Config.QSabers){
            SetSaberColor(helper->GetFieldObjectValue(self, "_leftSaber"), colorScheme.saberAColor);
            SetSaberColor(helper->GetFieldObjectValue(self, "_rightSaber"), colorScheme.saberBColor);
        }
    }
    SaberManager_Update(self);
}

MAKE_HOOK_OFFSETLESS(SaberBurnMarkSparkles_LateUpdate, void, Il2CppObject* self, void *type){
    Il2CppObject* colorManager = helper->GetFieldObjectValue(self, "_colorManager");
    if(colorManager != nullptr){
        if(isInTutorial){
            colorScheme.saberAColor = GetColorFromManager(colorManager, "_saberAColor");
            colorScheme.saberBColor = GetColorFromManager(colorManager, "_saberBColor");
            colorScheme.environmentColor0 = GetColorFromManager(colorManager, "_environmentColor0");
            colorScheme.environmentColor1 = GetColorFromManager(colorManager, "_environmentColor1");
            colorScheme.obstaclesColor = GetColorFromManager(colorManager, "_obstaclesColor");
        }else{
            saberA = fmod(saberA+Config.SaberASpeed, 360);
            saberB = fmod(saberB+Config.SaberBSpeed, 360);
            environmentColor0 = fmod(environmentColor0+Config.LightASpeed, 360);
            environmentColor1 = fmod(environmentColor1+Config.LightBSpeed, 360);
            obstaclesColor = fmod(obstaclesColor+Config.WallsSpeed, 360);
            if(Config.Sabers){
                colorScheme.saberAColor = ColorFromHSB(saberA, 1.0, 1.0);
                colorScheme.saberBColor = ColorFromHSB(saberB, 1.0, 1.0);
            }else{
                colorScheme.saberAColor = GetColorFromManager(colorManager, "_saberAColor");
                colorScheme.saberBColor = GetColorFromManager(colorManager, "_saberBColor");
            }
            if(Config.Lights){
                colorScheme.environmentColor0 = ColorFromHSB(environmentColor0, 1.0, 1.0);
                colorScheme.environmentColor1 = ColorFromHSB(environmentColor1, 1.0, 1.0);
            }else{
                colorScheme.environmentColor0 = GetColorFromManager(colorManager, "_environmentColor0");
                colorScheme.environmentColor1 = GetColorFromManager(colorManager, "_environmentColor1");
            }
            if(Config.Walls){
                colorScheme.obstaclesColor = ColorFromHSB(obstaclesColor, 1.0, 1.0);
            }else{
                colorScheme.obstaclesColor = GetColorFromManager(colorManager, "_obstaclesColor");
            }
        }
        settingColorScheme = true;
        helper->RunMethod(colorManager, "SetColorScheme", &colorScheme);
    }
    SaberBurnMarkSparkles_LateUpdate(self, type);
    
}

MAKE_HOOK_OFFSETLESS(SaberWeaponTrail_get_color, Color, Il2CppObject* self){
    if(!Config.Sabers && Config.Trails){
        Il2CppObject* saberTypeObject = helper->GetFieldObjectValue(self, "_saberTypeObject");
        int saberType;
        helper->RunMethod(&saberType, saberTypeObject, "get_saberType"); 
        Color multiplierSaberColor;
        helper->GetFieldValue(&multiplierSaberColor, self, "_multiplierSaberColor");
        Color saberColor = saberType == 1 ? ColorFromHSB(saberB, 1.0, 1.0) : ColorFromHSB(saberA, 1.0, 1.0);

        saberColor.r = powf(saberColor.r * multiplierSaberColor.r, 2.2f);
        saberColor.g = powf(saberColor.g * multiplierSaberColor.g, 2.2f);
        saberColor.b = powf(saberColor.b * multiplierSaberColor.b, 2.2f);
        saberColor.a = saberColor.a * multiplierSaberColor.a;

        return saberColor;
    }
    return SaberWeaponTrail_get_color(self);
}

MAKE_HOOK_OFFSETLESS(GameNoteController_Update, void, Il2CppObject* self){
    if(Config.Sabers){
        Il2CppObject* disappearingArrowController = helper->GetFieldObjectValue(self, "_disappearingArrowController");
        Il2CppObject* colorNoteVisuals = helper->GetFieldObjectValue(disappearingArrowController, "_colorNoteVisuals");
        Il2CppObject* noteController = helper->GetFieldObjectValue(colorNoteVisuals, "_noteController");
        
        Il2CppObject* noteData = helper->GetFieldObjectValue(noteController, "_noteData");
        int noteType;
        helper->RunMethod(&noteType, noteData, "get_noteType"); 

        Color noteColor;
        
        if(Config.Notes){
            noteColor = noteType == 1 ? colorScheme.saberBColor : colorScheme.saberAColor;
            helper->SetFieldValue(colorNoteVisuals, "_noteColor", &noteColor); 
        }else{
            noteColor = noteType == 1 ? defaultColorScheme.saberBColor : defaultColorScheme.saberAColor;
        } 
        
        float arrowGlowIntensity;
        helper->GetFieldValue(&arrowGlowIntensity, colorNoteVisuals, "_arrowGlowIntensity");
        Color arrowGlowSpriteRendererColor = noteColor;
        arrowGlowSpriteRendererColor.a = arrowGlowIntensity;
        Il2CppObject* arrowGlowSpriteRenderer = helper->GetFieldObjectValue(colorNoteVisuals, "_arrowGlowSpriteRenderer");
        helper->RunMethod(arrowGlowSpriteRenderer, "set_color", &arrowGlowSpriteRendererColor); 
        Il2CppObject* circleGlowSpriteRenderer = helper->GetFieldObjectValue(colorNoteVisuals, "_circleGlowSpriteRenderer");
        helper->RunMethod(circleGlowSpriteRenderer, "set_color", &noteColor); 
        Array<Il2CppObject*>* materialPropertyBlockControllers = reinterpret_cast<Array<Il2CppObject*>*>(helper->GetFieldObjectValue(colorNoteVisuals, "_materialPropertyBlockControllers"));
        
        for(int i = 0;i<materialPropertyBlockControllers->Length();i++){
            Il2CppObject* materialPropertyBlockController = materialPropertyBlockControllers->values[i];
            Il2CppObject* materialPropertyBlock;
            helper->RunMethod(&materialPropertyBlock, materialPropertyBlockController, "get_materialPropertyBlock"); 
            Color materialPropertyBlockColor = noteColor;
            materialPropertyBlockColor.a = 1.0f;
            helper->RunMethod(materialPropertyBlock, "SetColor", helper->createcsstr("_Color"), &materialPropertyBlockColor);
            helper->RunMethod(materialPropertyBlockController, "ApplyChanges");
        }
    }
    GameNoteController_Update(self);
}

MAKE_HOOK_OFFSETLESS(ObstacleController_Update, void, Il2CppObject* self){
    if(Config.Walls){
        Il2CppObject* stretchableObstacle = helper->GetFieldObjectValue(self, "_stretchableObstacle");
        float addColorMultiplier;
        helper->GetFieldValue(&addColorMultiplier, stretchableObstacle, "_addColorMultiplier");

        Color color = colorScheme.obstaclesColor;
        Color color2 = color;
        color2.r *= addColorMultiplier;
        color2.g *= addColorMultiplier;
        color2.b *= addColorMultiplier;
        color2.a = 0.0f;

        Il2CppObject* obstacleFrame = helper->GetFieldObjectValue(stretchableObstacle, "_obstacleFrame");
        helper->SetFieldValue(obstacleFrame, "color", &color); 
        helper->RunMethod(obstacleFrame, "Refresh");
        Il2CppObject* obstacleFakeGlow = helper->GetFieldObjectValue(stretchableObstacle, "_obstacleFakeGlow");
        helper->SetFieldValue(obstacleFakeGlow, "color", &color); 
        helper->RunMethod(obstacleFakeGlow, "Refresh");

        Array<Il2CppObject*>* addColorSetters = reinterpret_cast<Array<Il2CppObject*>*>(helper->GetFieldObjectValue(stretchableObstacle, "_addColorSetters"));
        for(int i = 0;i<addColorSetters->Length();i++){
            helper->RunMethod(addColorSetters->values[i], "SetColor", &color2);
        }

        Array<Il2CppObject*>* tintColorSetters = reinterpret_cast<Array<Il2CppObject*>*>(helper->GetFieldObjectValue(stretchableObstacle, "_tintColorSetters"));
        for(int i = 0;i<tintColorSetters->Length();i++){
            helper->RunMethod(tintColorSetters->values[i], "SetColor", &color);
        }
    }
    ObstacleController_Update(self);
}


void SaveConfig();

void TextSaveClear(){
    sleep(1);
    helper->RunMethod(textSave, "set_text", helper->createcsstr(""));
}

void ButtonSaveOnClick(Il2CppObject* button){
    Config.Lights = UnityHelper::GetToggleIsOn(helper, toggleLights);
    Config.Walls = UnityHelper::GetToggleIsOn(helper, toggleWalls);
    Config.Sabers = UnityHelper::GetToggleIsOn(helper, toggleSabers);
    Config.Trails = UnityHelper::GetToggleIsOn(helper, toggleTrails);
    Config.Notes = UnityHelper::GetToggleIsOn(helper, toggleNotes);
    Config.QSabers = UnityHelper::GetToggleIsOn(helper, toggleQSabers);
    SaveConfig();
    helper->RunMethod(textSave, "set_text", helper->createcsstr("Saved Configuration!"));
    std::thread textSaveClearThread(TextSaveClear);
    textSaveClearThread.detach();
}

void OnLoadAssetComplete(Il2CppObject* asset){
    helper->RunMethod(&customUIObject, nullptr, helper->class_get_method_from_name(helper->GetClassFromName("UnityEngine", "Object"), "Instantiate", 1), asset);
    UnityHelper::SetParent(helper, customUIObject, QuestUI::GetQuestUIModInfo().Panel);

    UnityHelper::AddButtonOnClick(helper, QuestUI::GetQuestUIInfo()->ButtonBinder, customUIObject, "ButtonSave", (UnityHelper::ButtonOnClickFunction*)ButtonSaveOnClick);

    textSave = UnityHelper::GetComponentInChildren(helper, customUIObject, helper->GetClassFromName("TMPro", "TextMeshProUGUI"), "TextSave");
    toggleLights = UnityHelper::GetComponentInChildren(helper, customUIObject, helper->GetClassFromName("UnityEngine.UI", "Toggle"), "ToggleLights");
    toggleWalls = UnityHelper::GetComponentInChildren(helper, customUIObject, helper->GetClassFromName("UnityEngine.UI", "Toggle"), "ToggleWalls");
    toggleSabers = UnityHelper::GetComponentInChildren(helper, customUIObject, helper->GetClassFromName("UnityEngine.UI", "Toggle"), "ToggleSabers");
    toggleTrails = UnityHelper::GetComponentInChildren(helper, customUIObject, helper->GetClassFromName("UnityEngine.UI", "Toggle"), "ToggleTrails");
    toggleNotes = UnityHelper::GetComponentInChildren(helper, customUIObject, helper->GetClassFromName("UnityEngine.UI", "Toggle"), "ToggleNotes");
    toggleQSabers = UnityHelper::GetComponentInChildren(helper, customUIObject, helper->GetClassFromName("UnityEngine.UI", "Toggle"), "ToggleQSabers");

    UnityHelper::SetToggleIsOn(helper, toggleLights, Config.Lights);
    UnityHelper::SetToggleIsOn(helper, toggleWalls, Config.Walls);
    UnityHelper::SetToggleIsOn(helper, toggleSabers, Config.Sabers);
    UnityHelper::SetToggleIsOn(helper, toggleTrails, Config.Trails);
    UnityHelper::SetToggleIsOn(helper, toggleNotes, Config.Notes);
    UnityHelper::SetToggleIsOn(helper, toggleQSabers, Config.QSabers);
}

void OnLoadAssetBundleComplete(Il2CppObject* assetBundleArg){
    assetBundle = assetBundleArg;
    UnityAssetLoader::LoadAssetFromAssetBundleAsync(assetBundle, (UnityAssetLoader_OnLoadAssetCompleteFunction*)OnLoadAssetComplete);
}

void QuestUIOnInitialized(){
    if(assetBundle == nullptr){
        UnityAssetLoader::LoadAssetBundleFromFileAsync("/sdcard/Android/data/com.beatgames.beatsaber/files/uis/rainbowmodUI.qui", (UnityAssetLoader_OnLoadAssetBundleCompleteFunction*)OnLoadAssetBundleComplete);
    }else{
        UnityAssetLoader::LoadAssetFromAssetBundleAsync(assetBundle, (UnityAssetLoader_OnLoadAssetCompleteFunction*)OnLoadAssetComplete);
    }
}

void InitHooks(){
    sleep(1);
    helper = new IL2CPP_Helper();
    helper->Initialize();

    QuestUI::Initialize("Rainbow Mod", QuestUIOnInitialized);
    
    Il2CppClass* tutorialControllerClass = helper->GetClassFromName("", "TutorialController");
    INSTALL_HOOK_OFFSETLESS(TutorialController_Awake, helper->class_get_method_from_name(tutorialControllerClass, "Awake", 0));
    INSTALL_HOOK_OFFSETLESS(TutorialController_OnDestroy, helper->class_get_method_from_name(tutorialControllerClass, "OnDestroy", 0));

    INSTALL_HOOK_OFFSETLESS(ColorManager__SetColorScheme, helper->class_get_method_from_name(helper->GetClassFromName("", "ColorManager"), "SetColorScheme", 1));

    INSTALL_HOOK_OFFSETLESS(SaberManager_Update, helper->class_get_method_from_name(helper->GetClassFromName("", "SaberManager"), "Update", 0));

    INSTALL_HOOK_OFFSETLESS(SaberBurnMarkSparkles_LateUpdate, helper->class_get_method_from_name(helper->GetClassFromName("", "SaberBurnMarkSparkles"), "LateUpdate", 0));
        
    INSTALL_HOOK_OFFSETLESS(SaberWeaponTrail_get_color, helper->class_get_method_from_name(helper->GetClassFromName("", "SaberWeaponTrail"), "get_color", 0));
    INSTALL_HOOK_OFFSETLESS(GameNoteController_Update, helper->class_get_method_from_name(helper->GetClassFromName("", "GameNoteController"), "Update", 0));
    INSTALL_HOOK_OFFSETLESS(ObstacleController_Update, helper->class_get_method_from_name(helper->GetClassFromName("", "ObstacleController"), "Update", 0));
    log(INFO, "Successfully installed Hooks!");
}

void SaveConfig() {
    log(INFO, "Saving Configuration...");
    config_doc.RemoveAllMembers();
    config_doc.SetObject();
    rapidjson::CrtAllocator& allocator = config_doc.GetAllocator();
    config_doc.AddMember("Lights", Config.Lights, allocator);
    config_doc.AddMember("Walls", Config.Walls, allocator);
    config_doc.AddMember("Sabers", Config.Sabers, allocator);
    config_doc.AddMember("Trails", Config.Trails, allocator);
    config_doc.AddMember("Notes", Config.Notes, allocator);
    config_doc.AddMember("QSabers", Config.QSabers, allocator);
    config_doc.AddMember("SaberASpeed", Config.SaberASpeed, allocator);
    config_doc.AddMember("SaberBSpeed", Config.SaberBSpeed, allocator);
    config_doc.AddMember("SabersStartDiff", Config.SabersStartDiff, allocator);
    config_doc.AddMember("WallsSpeed", Config.WallsSpeed, allocator);
    Configuration::Write();
    log(INFO, "Saved Configuration!");
}

bool LoadConfig() { 
    log(INFO, "Loading Configuration...");
    Configuration::Load();
    bool foundEverything = true;
    if(config_doc.HasMember("Lights") && config_doc["Lights"].IsBool()){
        Config.Lights = config_doc["Lights"].GetBool();    
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("Walls") && config_doc["Walls"].IsBool()){
        Config.Walls = config_doc["Walls"].GetBool();    
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("Sabers") && config_doc["Sabers"].IsBool()){
        Config.Sabers = config_doc["Sabers"].GetBool();    
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("Trails") && config_doc["Trails"].IsBool()){
        Config.Trails = config_doc["Trails"].GetBool();    
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("Notes") && config_doc["Notes"].IsBool()){
        Config.Notes = config_doc["Notes"].GetBool();    
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("QSabers") && config_doc["QSabers"].IsBool()){
        Config.QSabers = config_doc["QSabers"].GetBool();    
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("SaberASpeed") && config_doc["SaberASpeed"].IsDouble()){
        Config.SaberASpeed = config_doc["SaberASpeed"].GetDouble();    
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("SaberBSpeed") && config_doc["SaberBSpeed"].IsDouble()){
        Config.SaberBSpeed = config_doc["SaberBSpeed"].GetDouble();    
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("SabersStartDiff") && config_doc["SabersStartDiff"].IsDouble()){
        Config.SabersStartDiff = config_doc["SabersStartDiff"].GetDouble();    
    }else{
        foundEverything = false;
    }
    if(config_doc.HasMember("WallsSpeed") && config_doc["WallsSpeed"].IsDouble()){
        Config.WallsSpeed = config_doc["WallsSpeed"].GetDouble();    
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

    saberB = Config.SabersStartDiff;
    environmentColor1 = Config.LightsStartDiff;

    std::thread initHooksThread(InitHooks);
    initHooksThread.detach();
    log(INFO, "Successfully installed RainbowMod!");
}
