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

#include "../beatsaber-hook/shared/inline-hook/inlineHook.h"

#define LOG_LEVEL CRITICAL | ERROR | WARNING | INFO | DEBUG

#include "../beatsaber-hook/shared/utils/utils.h"
#include "../beatsaber-hook/shared/utils/typedefs.h"

using namespace il2cpp_utils;
using namespace il2cpp_functions;
using namespace std;

static auto& config_doc = Configuration::config;

static struct Config_t {
	bool LightsActive = true;
	bool WallsActive = true;
	bool SabersActive = true;
	bool TrailsActive = true;
	bool DisableNotes = false;
	bool QSabersActive = false;
	double SaberASpeed = 1.0;
	double SaberBSpeed = 1.0;
	double SabersStartDiff = 180.0;
	double LightASpeed = 2.0;
	double LightBSpeed = 2.0;
	double LightsStartDiff = 180.0;
	double WallsSpeed = 2.0;
} Config;

typedef struct {
	char pad[0x10];
	char colorSchemeId[8];
	char colorSchemeName[8];
	int isEditable;
	Color saberAColor;
	Color saberBColor;
	Color environmentColor0;
	Color environmentColor1;
	Color obstaclesColor;
} ColorScheme;

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
	GetFieldValue(&color, GetFieldObjectValue(colorManager, fieldName), "_color");
	return color; 
}

Il2CppObject* GetFirstObjectOfType(Il2CppClass* klass){
	Il2CppClass* resourcesClass = GetClassFromName("UnityEngine", "Resources");
	const MethodInfo* resources_FindObjectsOfTypeAllMethod = class_get_method_from_name(resourcesClass, "FindObjectsOfTypeAll", 1);
	Array<Il2CppObject*>* objects;
	RunMethod(&objects, nullptr, resources_FindObjectsOfTypeAllMethod, type_get_object(class_get_type(klass)));
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
		if(Config.QSabersActive){
			vector<Il2CppObject*> materials; 
			bool getInactive = false;
			Il2CppString* glowString = createcsstr("_Glow");
			Il2CppString* bloomString = createcsstr("_Bloom");
			Il2CppClass* shaderClass = GetClassFromName("UnityEngine", "Shader");
			const MethodInfo* shader_PropertyToIDMethod = class_get_method_from_name(shaderClass, "PropertyToID", 1);
			int glowID, bloomID;
			RunMethod(&glowID, nullptr, shader_PropertyToIDMethod, glowString);
			RunMethod(&bloomID, nullptr, shader_PropertyToIDMethod, bloomString);
			Array<Il2CppObject*>* childTransforms;
			RunMethod(&childTransforms, saber, "GetComponentsInChildren", type_get_object(class_get_type(GetClassFromName("UnityEngine", "Transform"))), &getInactive);
			for (int i= 0; i< childTransforms->Length(); i++)
			{
				Array<Il2CppObject*>* renderers;
				RunMethod(&renderers, childTransforms->values[i], "GetComponentsInChildren", type_get_object(class_get_type(GetClassFromName("UnityEngine", "Renderer"))), &getInactive);
				for (int j = 0; j < renderers->Length(); j++)
				{
					Array<Il2CppObject*>* sharedMaterials;
					RunMethod(&sharedMaterials, renderers->values[j], "get_sharedMaterials");
					for (int h = 0; h < sharedMaterials->Length(); h++)
					{
						Il2CppObject* material = sharedMaterials->values[h];
						bool setColor = false;
						bool hasGlow;
						RunMethod(&hasGlow, material, "HasProperty", &glowID);
						if (hasGlow)
						{
							float glowFloat;
							RunMethod(&glowFloat, material, "GetFloat", &glowID);
							if (glowFloat > 0)
								setColor = true;
						}
						if (!setColor)
						{
							bool hasBloom;
							RunMethod(&hasBloom, material, "HasProperty", &bloomID);
							if (hasBloom)
							{
								float bloomFloat;
								RunMethod(&bloomFloat, material, "GetFloat", &bloomID);
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
	Il2CppString* colorString = createcsstr("_Color");
	map<Il2CppObject*, vector<Il2CppObject*>>::iterator it = sabersMaterials.find(saber);
	if(it == sabersMaterials.end())
	{
		CacheSaber(saber);
	}
	for (Il2CppObject* material : sabersMaterials[saber]) 
	{
		RunMethod(material, "SetColor", colorString, &color);
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
	Init();
	Il2CppObject* colorManager = GetFirstObjectOfType(GetClassFromName("", "ColorManager"));
	if (colorManager != nullptr)
	{
		if(!isInTutorial){
			if(Config.QSabersActive){
				SetSaberColor(GetFieldObjectValue(self, "_leftSaber"), colorScheme.saberAColor);
				SetSaberColor(GetFieldObjectValue(self, "_rightSaber"), colorScheme.saberBColor);
			}
		}
	}
	SaberManager_Update(self);
}

MAKE_HOOK_OFFSETLESS(SaberBurnMarkSparkles_LateUpdate, void, Il2CppObject* self, void *type){
	Init();
	Il2CppObject* colorManager = GetFieldObjectValue(self, "_colorManager");
	if(isInTutorial){
		colorScheme.saberAColor = GetColorFromManager(colorManager, "_saberAColor");
		colorScheme.saberBColor = GetColorFromManager(colorManager, "_saberBColor");
		colorScheme.environmentColor0 = GetColorFromManager(colorManager, "_environmentColor0");
		colorScheme.environmentColor1 = GetColorFromManager(colorManager, "_environmentColor1");
		colorScheme.obstaclesColor = GetColorFromManager(colorManager, "_obstaclesColor");
	}else{
		saberA+=Config.SaberASpeed;
		if(saberA > 360){
			saberA = 0;
		}
		saberB+=Config.SaberBSpeed;
		if(saberB > 360){
			saberB = 0;
		}
		environmentColor0+=Config.LightASpeed;
		if(environmentColor0 > 360){
			environmentColor0 = 0;
		}
		environmentColor1+=Config.LightBSpeed;
		if(environmentColor1 > 360){
			environmentColor1 = 0;
		}
		obstaclesColor+=Config.WallsSpeed;
		if(obstaclesColor > 360){
			obstaclesColor = 0;
		}
		if(Config.SabersActive){
			colorScheme.saberAColor = ColorFromHSB(saberA, 1.0, 1.0);
			colorScheme.saberBColor = ColorFromHSB(saberB, 1.0, 1.0);
		}else{
			colorScheme.saberAColor = GetColorFromManager(colorManager, "_saberAColor");
			colorScheme.saberBColor = GetColorFromManager(colorManager, "_saberBColor");
		}
		if(Config.LightsActive){
			colorScheme.environmentColor0 = ColorFromHSB(environmentColor0, 1.0, 1.0);
			colorScheme.environmentColor1 = ColorFromHSB(environmentColor1, 1.0, 1.0);
		}else{
			colorScheme.environmentColor0 = GetColorFromManager(colorManager, "_environmentColor0");
			colorScheme.environmentColor1 = GetColorFromManager(colorManager, "_environmentColor1");
		}
		if(Config.WallsActive){
			colorScheme.obstaclesColor = ColorFromHSB(obstaclesColor, 1.0, 1.0);
		}else{
			colorScheme.obstaclesColor = GetColorFromManager(colorManager, "_obstaclesColor");
		}
	}
	settingColorScheme = true;
	RunMethod(colorManager, "SetColorScheme", &colorScheme);
	SaberBurnMarkSparkles_LateUpdate(self, type);
}

MAKE_HOOK_OFFSETLESS(SaberWeaponTrail_get_color, Color, Il2CppObject* self){
	Init();
	Il2CppObject* saberTypeObject = GetFieldObjectValue(self, "_saberTypeObject");
	int saberType;
	RunMethod(&saberType, saberTypeObject, "get_saberType"); 
	Color multiplierSaberColor;
	GetFieldValue(&multiplierSaberColor, self, "_multiplierSaberColor");
	Color saberColor = saberType == 1 ? ColorFromHSB(saberB, 1.0, 1.0) : ColorFromHSB(saberA, 1.0, 1.0);

	saberColor.r = powf(saberColor.r * multiplierSaberColor.r, 2.2f);
	saberColor.g = powf(saberColor.g * multiplierSaberColor.g, 2.2f);
	saberColor.b = powf(saberColor.b * multiplierSaberColor.b, 2.2f);
	saberColor.a = saberColor.a * multiplierSaberColor.a;

	return saberColor;
}

MAKE_HOOK_OFFSETLESS(GameNoteController_Update, void, Il2CppObject* self){
	Init();
	Il2CppObject* disappearingArrowController = GetFieldObjectValue(self, "_disappearingArrowController");
	Il2CppObject* colorNoteVisuals = GetFieldObjectValue(disappearingArrowController, "_colorNoteVisuals");
	Il2CppObject* noteController = GetFieldObjectValue(colorNoteVisuals, "_noteController");
	
	Il2CppObject* noteData = GetFieldObjectValue(noteController, "_noteData");
	int noteType;
	RunMethod(&noteType, noteData, "get_noteType"); 

	Color noteColor;
	
	if(Config.DisableNotes){
		noteColor = noteType == 1 ? defaultColorScheme.saberBColor : defaultColorScheme.saberAColor;
		
	}else{
		noteColor = noteType == 1 ? colorScheme.saberBColor : colorScheme.saberAColor;
		SetFieldValue(colorNoteVisuals, "_noteColor", &noteColor); 
	} 
	
	float arrowGlowIntensity;
	GetFieldValue(&arrowGlowIntensity, colorNoteVisuals, "_arrowGlowIntensity");
	Color arrowGlowSpriteRendererColor = noteColor;
	arrowGlowSpriteRendererColor.a = arrowGlowIntensity;
	Il2CppObject* arrowGlowSpriteRenderer = GetFieldObjectValue(colorNoteVisuals, "_arrowGlowSpriteRenderer");
	RunMethod(arrowGlowSpriteRenderer, "set_color", &arrowGlowSpriteRendererColor); 
	Il2CppObject* circleGlowSpriteRenderer = GetFieldObjectValue(colorNoteVisuals, "_circleGlowSpriteRenderer");
	RunMethod(circleGlowSpriteRenderer, "set_color", &noteColor); 
	Array<Il2CppObject*>* materialPropertyBlockControllers = reinterpret_cast<Array<Il2CppObject*>*>(GetFieldObjectValue(colorNoteVisuals, "_materialPropertyBlockControllers"));
	
	for(int i = 0;i<materialPropertyBlockControllers->Length();i++){
		Il2CppObject* materialPropertyBlockController = materialPropertyBlockControllers->values[i];
		Il2CppObject* materialPropertyBlock;
		RunMethod(&materialPropertyBlock, materialPropertyBlockController, "get_materialPropertyBlock"); 
		Color materialPropertyBlockColor = noteColor;
		materialPropertyBlockColor.a = 1.0f;
		RunMethod(materialPropertyBlock, "SetColor", createcsstr("_Color"), &materialPropertyBlockColor);
		RunMethod(materialPropertyBlockController, "ApplyChanges");
	}
	GameNoteController_Update(self);
}

MAKE_HOOK_OFFSETLESS(ObstacleController_Update, void, Il2CppObject* self){
	Init();
	Il2CppObject* stretchableObstacle = GetFieldObjectValue(self, "_stretchableObstacle");
	float addColorMultiplier;
	GetFieldValue(&addColorMultiplier, stretchableObstacle, "_addColorMultiplier");

	Color color = colorScheme.obstaclesColor;
	Color color2 = color;
	color2.r *= addColorMultiplier;
	color2.g *= addColorMultiplier;
	color2.b *= addColorMultiplier;
	color2.a = 0.0f;

	Il2CppObject* obstacleFrame = GetFieldObjectValue(stretchableObstacle, "_obstacleFrame");
	SetFieldValue(obstacleFrame, "color", &color); 
	RunMethod(obstacleFrame, "Refresh");
	Il2CppObject* obstacleFakeGlow = GetFieldObjectValue(stretchableObstacle, "_obstacleFakeGlow");
	SetFieldValue(obstacleFakeGlow, "color", &color); 
	RunMethod(obstacleFakeGlow, "Refresh");

	Array<Il2CppObject*>* addColorSetters = reinterpret_cast<Array<Il2CppObject*>*>(GetFieldObjectValue(stretchableObstacle, "_addColorSetters"));
	for(int i = 0;i<addColorSetters->Length();i++){
		RunMethod(addColorSetters->values[i], "SetColor", &color2);
	}

	Array<Il2CppObject*>* tintColorSetters = reinterpret_cast<Array<Il2CppObject*>*>(GetFieldObjectValue(stretchableObstacle, "_tintColorSetters"));
	for(int i = 0;i<tintColorSetters->Length();i++){
		RunMethod(tintColorSetters->values[i], "SetColor", &color);
	}
	
	ObstacleController_Update(self);
}

void InitHooks(){

	sleep(2);
	Init();

	Il2CppClass* tutorialControllerClass = GetClassFromName("", "TutorialController");
	const MethodInfo* tutorialController_AwakeMethod = class_get_method_from_name(tutorialControllerClass, "Awake", 0);
	INSTALL_HOOK_OFFSETLESS(TutorialController_Awake, tutorialController_AwakeMethod);
	const MethodInfo* tutorialController_OnDestroyMethod = class_get_method_from_name(tutorialControllerClass, "OnDestroy", 0);
	INSTALL_HOOK_OFFSETLESS(TutorialController_OnDestroy, tutorialController_OnDestroyMethod);

	Il2CppClass* colorManagerClass = GetClassFromName("", "ColorManager");	
	const MethodInfo* colorManager_SetColorSchemeMethod = class_get_method_from_name(colorManagerClass, "SetColorScheme", 1);
	INSTALL_HOOK_OFFSETLESS(ColorManager__SetColorScheme, colorManager_SetColorSchemeMethod);

	Il2CppClass* saberManagerClass = GetClassFromName("", "SaberManager");
	const MethodInfo* saberManager_UpdateMethod = class_get_method_from_name(saberManagerClass, "Update", 0);
	INSTALL_HOOK_OFFSETLESS(SaberManager_Update, saberManager_UpdateMethod);

	Il2CppClass* saberBurnMarkSparklesClass = GetClassFromName("", "SaberBurnMarkSparkles");
	const MethodInfo* saberBurnMarkSparkles_LateUpdateMethod = class_get_method_from_name(saberBurnMarkSparklesClass, "LateUpdate", 0);
	INSTALL_HOOK_OFFSETLESS(SaberBurnMarkSparkles_LateUpdate, saberBurnMarkSparkles_LateUpdateMethod);
		
	if(!Config.SabersActive && Config.TrailsActive){
		Il2CppClass* saberWeaponTrailClass = GetClassFromName("", "SaberWeaponTrail");
		const MethodInfo* saberWeaponTrail_get_colorMethod = class_get_method_from_name(saberWeaponTrailClass, "get_color", 0);
		INSTALL_HOOK_OFFSETLESS(SaberWeaponTrail_get_color, saberWeaponTrail_get_colorMethod);
	}
	if(Config.SabersActive){
		Il2CppClass* gameNoteControllerClass = GetClassFromName("", "GameNoteController");
		const MethodInfo* gameNoteController_UpdateMethod = class_get_method_from_name(gameNoteControllerClass, "Update", 0);
		INSTALL_HOOK_OFFSETLESS(GameNoteController_Update, gameNoteController_UpdateMethod);
	}
	if(Config.WallsActive){
		Il2CppClass* obstacleControllerClass = GetClassFromName("", "ObstacleController");
		const MethodInfo* obstacleController_UpdateMethod = class_get_method_from_name(obstacleControllerClass, "Update", 0);
		INSTALL_HOOK_OFFSETLESS(ObstacleController_Update, obstacleController_UpdateMethod);
	}
	log(INFO, "Successfully installed Hooks!");
}

void createDefaultConfig() {
	log(INFO, "Creating Configuration...");
	config_doc.RemoveMember("LightsActive");
	config_doc.RemoveMember("WallsActive");
	config_doc.RemoveMember("SabersActive");
	config_doc.RemoveMember("TrailsActive");
	config_doc.RemoveMember("DisableNotes");
	config_doc.RemoveMember("QSabersActive");
	config_doc.RemoveMember("SaberASpeed");
	config_doc.RemoveMember("SaberBSpeed");
	config_doc.RemoveMember("SabersStartDiff");
	config_doc.RemoveMember("LightASpeed");
	config_doc.RemoveMember("LightBSpeed");
	config_doc.RemoveMember("LightsStartDiff");
	config_doc.RemoveMember("WallsSpeed");
	
	config_doc.AddMember("LightsActive", Config.LightsActive, config_doc.GetAllocator());
	config_doc.AddMember("WallsActive", Config.WallsActive, config_doc.GetAllocator());
	config_doc.AddMember("SabersActive", Config.SabersActive, config_doc.GetAllocator());
	config_doc.AddMember("TrailsActive", Config.TrailsActive, config_doc.GetAllocator());
	config_doc.AddMember("DisableNotes", Config.DisableNotes, config_doc.GetAllocator());
	config_doc.AddMember("QSabersActive", Config.QSabersActive, config_doc.GetAllocator());
	config_doc.AddMember("SaberASpeed", Config.SaberASpeed, config_doc.GetAllocator());
	config_doc.AddMember("SaberBSpeed", Config.SaberBSpeed, config_doc.GetAllocator());
	config_doc.AddMember("SabersStartDiff", Config.SabersStartDiff, config_doc.GetAllocator());
	config_doc.AddMember("LightASpeed", Config.LightASpeed, config_doc.GetAllocator());
	config_doc.AddMember("LightBSpeed", Config.LightBSpeed, config_doc.GetAllocator());
	config_doc.AddMember("LightsStartDiff", Config.LightsStartDiff, config_doc.GetAllocator());
	config_doc.AddMember("WallsSpeed", Config.WallsSpeed, config_doc.GetAllocator());
	Configuration::Write();
	log(INFO, "Created Configuration!");
}

bool loadConfig() { 
	log(INFO, "Loading Configuration...");
	Configuration::Load();
	bool foundEverything = true;
	if(config_doc.HasMember("LightsActive") && config_doc["LightsActive"].IsBool()){
		Config.LightsActive = config_doc["LightsActive"].GetBool();	
	}else{
		foundEverything = false;
	}
	if(config_doc.HasMember("WallsActive") && config_doc["WallsActive"].IsBool()){
		Config.WallsActive = config_doc["WallsActive"].GetBool();	
	}else{
		foundEverything = false;
	}
	if(config_doc.HasMember("SabersActive") && config_doc["SabersActive"].IsBool()){
		Config.SabersActive = config_doc["SabersActive"].GetBool();	
	}else{
		foundEverything = false;
	}
	if(config_doc.HasMember("TrailsActive") && config_doc["TrailsActive"].IsBool()){
		Config.TrailsActive = config_doc["TrailsActive"].GetBool();	
	}else{
		foundEverything = false;
	}
	if(config_doc.HasMember("DisableNotes") && config_doc["DisableNotes"].IsBool()){
		Config.DisableNotes = config_doc["DisableNotes"].GetBool();	
	}else{
		foundEverything = false;
	}
	if(config_doc.HasMember("QSabersActive") && config_doc["QSabersActive"].IsBool()){
		Config.QSabersActive = config_doc["QSabersActive"].GetBool();	
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
	if(config_doc.HasMember("LightASpeed") && config_doc["LightASpeed"].IsDouble()){
		Config.LightASpeed = config_doc["LightASpeed"].GetDouble();	
	}else{
		foundEverything = false;
	}
	if(config_doc.HasMember("LightBSpeed") && config_doc["LightBSpeed"].IsDouble()){
		Config.LightBSpeed = config_doc["LightBSpeed"].GetDouble();	
	}else{
		foundEverything = false;
	}
	if(config_doc.HasMember("LightsStartDiff") && config_doc["LightsStartDiff"].IsDouble()){
		Config.LightsStartDiff = config_doc["LightsStartDiff"].GetDouble();	
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
	Init();
	#ifdef __aarch64__
	log(INFO, "Is 64 bit!");
	#endif

	if(!loadConfig())
		createDefaultConfig();

	saberB = Config.SabersStartDiff;
	environmentColor1 = Config.LightsStartDiff;

	std::thread initHooksThread(InitHooks);
	initHooksThread.detach();
	log(INFO, "Successfully installed RainbowSabers!");
}
