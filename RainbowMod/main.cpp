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

static rapidjson::Document& config_doc = Configuration::config;

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
    return il2cpp_utils::GetFieldValue<Color>(il2cpp_utils::GetFieldValue(colorManager, fieldName), "_color");; 
}

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

void CacheSaber(Il2CppObject* saber){
    if(!isInTutorial){
        if(Config.QSabers){
            vector<Il2CppObject*> materials; 
            bool getInactive = false;
            Il2CppString* glowString = il2cpp_utils::createcsstr("_Glow");
            Il2CppString* bloomString = il2cpp_utils::createcsstr("_Bloom");
            Il2CppClass* shaderClass = il2cpp_utils::GetClassFromName("UnityEngine", "Shader");
            const MethodInfo* shader_PropertyToIDMethod = il2cpp_functions::class_get_method_from_name(shaderClass, "PropertyToID", 1);
            int glowID, bloomID;
            il2cpp_utils::RunMethod(&glowID, nullptr, shader_PropertyToIDMethod, glowString);
            il2cpp_utils::RunMethod(&bloomID, nullptr, shader_PropertyToIDMethod, bloomString);
            Array<Il2CppObject*>* childTransforms;
            il2cpp_utils::RunMethod(&childTransforms, saber, "GetComponentsInChildren", il2cpp_functions::type_get_object(il2cpp_functions::class_get_type(il2cpp_utils::GetClassFromName("UnityEngine", "Transform"))), &getInactive);
            for (int i= 0; i< childTransforms->Length(); i++)
            {
                Array<Il2CppObject*>* renderers;
                il2cpp_utils::RunMethod(&renderers, childTransforms->values[i], "GetComponentsInChildren", il2cpp_functions::type_get_object(il2cpp_functions::class_get_type(il2cpp_utils::GetClassFromName("UnityEngine", "Renderer"))), &getInactive);
                for (int j = 0; j < renderers->Length(); j++)
                {
                    Array<Il2CppObject*>* sharedMaterials;
                    il2cpp_utils::RunMethod(&sharedMaterials, renderers->values[j], "get_sharedMaterials");
                    for (int h = 0; h < sharedMaterials->Length(); h++)
                    {
                        Il2CppObject* material = sharedMaterials->values[h];
                        bool setColor = false;
                        bool hasGlow;
                        il2cpp_utils::RunMethod(&hasGlow, material, "HasProperty", &glowID);
                        if (hasGlow)
                        {
                            float glowFloat;
                            il2cpp_utils::RunMethod(&glowFloat, material, "GetFloat", &glowID);
                            if (glowFloat > 0)
                                setColor = true;
                        }
                        if (!setColor)
                        {
                            bool hasBloom;
                            il2cpp_utils::RunMethod(&hasBloom, material, "HasProperty", &bloomID);
                            if (hasBloom)
                            {
                                float bloomFloat;
                                il2cpp_utils::RunMethod(&bloomFloat, material, "GetFloat", &bloomID);
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
    Il2CppString* colorString = il2cpp_utils::createcsstr("_Color");
    map<Il2CppObject*, vector<Il2CppObject*>>::iterator it = sabersMaterials.find(saber);
    if(it == sabersMaterials.end())
    {
        CacheSaber(saber);
    }
    for (Il2CppObject* material : sabersMaterials[saber]) 
    {
        il2cpp_utils::RunMethod(material, "SetColor", colorString, &color);
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
    Il2CppObject* colorManager = GetFirstObjectOfType(il2cpp_utils::GetClassFromName("", "ColorManager"));
    if (colorManager != nullptr)
    {
        if(!isInTutorial && Config.QSabers){
            SetSaberColor(il2cpp_utils::GetFieldValue(self, "_leftSaber"), colorScheme.saberAColor);
            SetSaberColor(il2cpp_utils::GetFieldValue(self, "_rightSaber"), colorScheme.saberBColor);
        }
    }
    SaberManager_Update(self);
}

MAKE_HOOK_OFFSETLESS(SaberBurnMarkSparkles_LateUpdate, void, Il2CppObject* self, void *type){
    Il2CppObject* colorManager = il2cpp_utils::GetFieldValue(self, "_colorManager");
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
        il2cpp_utils::RunMethod(colorManager, "SetColorScheme", &colorScheme);
    }
    SaberBurnMarkSparkles_LateUpdate(self, type);
    
}

MAKE_HOOK_OFFSETLESS(SaberWeaponTrail_get_color, Color, Il2CppObject* self){
    if(!Config.Sabers && Config.Trails){
        Il2CppObject* saberTypeObject = il2cpp_utils::GetFieldValue(self, "_saberTypeObject");
        int saberType;
        il2cpp_utils::RunMethod(&saberType, saberTypeObject, "get_saberType"); 
        Color multiplierSaberColor = il2cpp_utils::GetFieldValue<Color>(self, "_multiplierSaberColor");
        
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
        Il2CppObject* disappearingArrowController = il2cpp_utils::GetFieldValue(self, "_disappearingArrowController");
        Il2CppObject* colorNoteVisuals = il2cpp_utils::GetFieldValue(disappearingArrowController, "_colorNoteVisuals");
        Il2CppObject* noteController = il2cpp_utils::GetFieldValue(colorNoteVisuals, "_noteController");
        
        Il2CppObject* noteData = il2cpp_utils::GetFieldValue(noteController, "_noteData");
        int noteType;
        il2cpp_utils::RunMethod(&noteType, noteData, "get_noteType"); 

        Color noteColor;
        
        if(Config.Notes){
            noteColor = noteType == 1 ? colorScheme.saberBColor : colorScheme.saberAColor;
            il2cpp_utils::SetFieldValue(colorNoteVisuals, "_noteColor", &noteColor); 
        }else{
            noteColor = noteType == 1 ? defaultColorScheme.saberBColor : defaultColorScheme.saberAColor;
        } 
        
        float arrowGlowIntensity = il2cpp_utils::GetFieldValue<float>(colorNoteVisuals, "_arrowGlowIntensity");
        Color arrowGlowSpriteRendererColor = noteColor;
        arrowGlowSpriteRendererColor.a = arrowGlowIntensity;
        Il2CppObject* arrowGlowSpriteRenderer = il2cpp_utils::GetFieldValue(colorNoteVisuals, "_arrowGlowSpriteRenderer");
        il2cpp_utils::RunMethod(arrowGlowSpriteRenderer, "set_color", &arrowGlowSpriteRendererColor); 
        Il2CppObject* circleGlowSpriteRenderer = il2cpp_utils::GetFieldValue(colorNoteVisuals, "_circleGlowSpriteRenderer");
        il2cpp_utils::RunMethod(circleGlowSpriteRenderer, "set_color", &noteColor); 
        Array<Il2CppObject*>* materialPropertyBlockControllers = reinterpret_cast<Array<Il2CppObject*>*>(il2cpp_utils::GetFieldValue(colorNoteVisuals, "_materialPropertyBlockControllers"));
        
        for(int i = 0;i<materialPropertyBlockControllers->Length();i++){
            Il2CppObject* materialPropertyBlockController = materialPropertyBlockControllers->values[i];
            Il2CppObject* materialPropertyBlock;
            il2cpp_utils::RunMethod(&materialPropertyBlock, materialPropertyBlockController, "get_materialPropertyBlock"); 
            Color materialPropertyBlockColor = noteColor;
            materialPropertyBlockColor.a = 1.0f;
            il2cpp_utils::RunMethod(materialPropertyBlock, "SetColor", il2cpp_utils::createcsstr("_Color"), &materialPropertyBlockColor);
            il2cpp_utils::RunMethod(materialPropertyBlockController, "ApplyChanges");
        }
    }
    GameNoteController_Update(self);
}

MAKE_HOOK_OFFSETLESS(ObstacleController_Update, void, Il2CppObject* self){
    if(Config.Walls){
        Il2CppObject* stretchableObstacle = il2cpp_utils::GetFieldValue(self, "_stretchableObstacle");

        float addColorMultiplier = il2cpp_utils::GetFieldValue<float>(stretchableObstacle, "_addColorMultiplier");

        Color color = colorScheme.obstaclesColor;
        Color color2 = color;
        color2.r *= addColorMultiplier;
        color2.g *= addColorMultiplier;
        color2.b *= addColorMultiplier;
        color2.a = 0.0f;

        Il2CppObject* obstacleFrame = il2cpp_utils::GetFieldValue(stretchableObstacle, "_obstacleFrame");
        il2cpp_utils::SetFieldValue(obstacleFrame, "color", &color); 
        il2cpp_utils::RunMethod(obstacleFrame, "Refresh");
        Il2CppObject* obstacleFakeGlow = il2cpp_utils::GetFieldValue(stretchableObstacle, "_obstacleFakeGlow");
        il2cpp_utils::SetFieldValue(obstacleFakeGlow, "color", &color); 
        il2cpp_utils::RunMethod(obstacleFakeGlow, "Refresh");

        Array<Il2CppObject*>* addColorSetters = reinterpret_cast<Array<Il2CppObject*>*>(il2cpp_utils::GetFieldValue(stretchableObstacle, "_addColorSetters"));
        for(int i = 0;i<addColorSetters->Length();i++){
            il2cpp_utils::RunMethod(addColorSetters->values[i], "SetColor", &color2);
        }

        Array<Il2CppObject*>* tintColorSetters = reinterpret_cast<Array<Il2CppObject*>*>(il2cpp_utils::GetFieldValue(stretchableObstacle, "_tintColorSetters"));
        for(int i = 0;i<tintColorSetters->Length();i++){
            il2cpp_utils::RunMethod(tintColorSetters->values[i], "SetColor", &color);
        }
    }
    ObstacleController_Update(self);
}


void SaveConfig();

void TextSaveClear(){
    sleep(1);
    il2cpp_utils::RunMethod(textSave, "set_text", il2cpp_utils::createcsstr(""));
}

void ButtonSaveOnClick(Il2CppObject* button){
    Config.Lights = UnityHelper::GetToggleIsOn(toggleLights);
    Config.Walls = UnityHelper::GetToggleIsOn(toggleWalls);
    Config.Sabers = UnityHelper::GetToggleIsOn(toggleSabers);
    Config.Trails = UnityHelper::GetToggleIsOn(toggleTrails);
    Config.Notes = UnityHelper::GetToggleIsOn(toggleNotes);
    Config.QSabers = UnityHelper::GetToggleIsOn(toggleQSabers);
    SaveConfig();
    il2cpp_utils::RunMethod(textSave, "set_text", il2cpp_utils::createcsstr("Saved Configuration!"));
    std::thread textSaveClearThread(TextSaveClear);
    textSaveClearThread.detach();
}

void OnLoadAssetComplete(Il2CppObject* asset){
    il2cpp_utils::RunMethod(&customUIObject, nullptr, il2cpp_functions::class_get_method_from_name(il2cpp_utils::GetClassFromName("UnityEngine", "Object"), "Instantiate", 1), asset);
    UnityHelper::SetParent(customUIObject, QuestUI::GetQuestUIModInfo().Panel);

    UnityHelper::AddButtonOnClick(QuestUI::GetQuestUIInfo()->ButtonBinder, customUIObject, "ButtonSave", (UnityHelper::ButtonOnClickFunction*)ButtonSaveOnClick);

    textSave = UnityHelper::GetComponentInChildren(customUIObject, il2cpp_utils::GetClassFromName("TMPro", "TextMeshProUGUI"), "TextSave");
    toggleLights = UnityHelper::GetComponentInChildren(customUIObject, il2cpp_utils::GetClassFromName("UnityEngine.UI", "Toggle"), "ToggleLights");
    toggleWalls = UnityHelper::GetComponentInChildren(customUIObject, il2cpp_utils::GetClassFromName("UnityEngine.UI", "Toggle"), "ToggleWalls");
    toggleSabers = UnityHelper::GetComponentInChildren(customUIObject, il2cpp_utils::GetClassFromName("UnityEngine.UI", "Toggle"), "ToggleSabers");
    toggleTrails = UnityHelper::GetComponentInChildren(customUIObject, il2cpp_utils::GetClassFromName("UnityEngine.UI", "Toggle"), "ToggleTrails");
    toggleNotes = UnityHelper::GetComponentInChildren(customUIObject, il2cpp_utils::GetClassFromName("UnityEngine.UI", "Toggle"), "ToggleNotes");
    toggleQSabers = UnityHelper::GetComponentInChildren(customUIObject, il2cpp_utils::GetClassFromName("UnityEngine.UI", "Toggle"), "ToggleQSabers");

    UnityHelper::SetToggleIsOn(toggleLights, Config.Lights);
    UnityHelper::SetToggleIsOn(toggleWalls, Config.Walls);
    UnityHelper::SetToggleIsOn(toggleSabers, Config.Sabers);
    UnityHelper::SetToggleIsOn(toggleTrails, Config.Trails);
    UnityHelper::SetToggleIsOn(toggleNotes, Config.Notes);
    UnityHelper::SetToggleIsOn(toggleQSabers, Config.QSabers);
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

void SaveConfig() {
    log(INFO, "Saving Configuration...");
    config_doc.RemoveAllMembers();
    config_doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = config_doc.GetAllocator();
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
extern "C" void load()
{
    if(!LoadConfig())
        SaveConfig();

    saberB = Config.SabersStartDiff;
    environmentColor1 = Config.LightsStartDiff;

    log(INFO, "Starting RainbowMod installation...");
    il2cpp_functions::Init();
    //QuestUI::Initialize("Rainbow Mod", QuestUIOnInitialized);
    
    Il2CppClass* tutorialControllerClass = il2cpp_utils::GetClassFromName("", "TutorialController");
    INSTALL_HOOK_OFFSETLESS(TutorialController_Awake, il2cpp_functions::class_get_method_from_name(tutorialControllerClass, "Awake", 0));
    INSTALL_HOOK_OFFSETLESS(TutorialController_OnDestroy, il2cpp_functions::class_get_method_from_name(tutorialControllerClass, "OnDestroy", 0));

    INSTALL_HOOK_OFFSETLESS(ColorManager__SetColorScheme, il2cpp_functions::class_get_method_from_name(il2cpp_utils::GetClassFromName("", "ColorManager"), "SetColorScheme", 1));

    INSTALL_HOOK_OFFSETLESS(SaberManager_Update, il2cpp_functions::class_get_method_from_name(il2cpp_utils::GetClassFromName("", "SaberManager"), "Update", 0));

    INSTALL_HOOK_OFFSETLESS(SaberBurnMarkSparkles_LateUpdate, il2cpp_functions::class_get_method_from_name(il2cpp_utils::GetClassFromName("", "SaberBurnMarkSparkles"), "LateUpdate", 0));
        
    INSTALL_HOOK_OFFSETLESS(SaberWeaponTrail_get_color, il2cpp_functions::class_get_method_from_name(il2cpp_utils::GetClassFromName("", "SaberWeaponTrail"), "get_color", 0));
    INSTALL_HOOK_OFFSETLESS(GameNoteController_Update, il2cpp_functions::class_get_method_from_name(il2cpp_utils::GetClassFromName("", "GameNoteController"), "Update", 0));
    INSTALL_HOOK_OFFSETLESS(ObstacleController_Update, il2cpp_functions::class_get_method_from_name(il2cpp_utils::GetClassFromName("", "ObstacleController"), "Update", 0));

    log(INFO, "Successfully installed RainbowMod!");
}