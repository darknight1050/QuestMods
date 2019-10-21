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

#include "../beatsaber-hook/shared/inline-hook/inlineHook.h"

#define LOG_LEVEL CRITICAL | ERROR | WARNING | INFO | DEBUG

#define MOD_ID "RainbowSabers"
#define VERSION "0.0.2"

#include "../beatsaber-hook/shared/utils/utils.h"
#include "../beatsaber-hook/shared/utils/typedefs.h"

using namespace il2cpp_utils;
using namespace il2cpp_functions;

static auto& config_doc = Configuration::config;

static struct Config_t {
	bool SabersActive = true;
	bool LightsActive = true;
	bool WallsActive = true;
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


static bool hookInit = false;

ColorScheme colorScheme;

static float saberA = 0;
static float saberB = 0;
static float environmentColor0 = 0;
static float environmentColor1 = 0;
static float obstaclesColor = 0;
static bool InTutorial = false; 

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

Color getColorFromManager(Il2CppObject* colorManager, const char* fieldName){
	Color color;
	GetFieldValue(&color, GetFieldObjectValue(colorManager, fieldName), "_color");
	return color; 
}

MAKE_HOOK_OFFSETLESS(TutorialController_Awake, void, Il2CppObject* self){
	TutorialController_Awake(self);
	InTutorial = true;
}

MAKE_HOOK_OFFSETLESS(TutorialController_OnDestroy, void, Il2CppObject* self){
	TutorialController_OnDestroy(self);
	InTutorial = false;
}

MAKE_HOOK_OFFSETLESS(SaberBurnMarkSparkles_LateUpdate, void, Il2CppObject* self, void *type){
	Init();
	Il2CppObject* colorManager = GetFieldObjectValue(self, "_colorManager");
	if(InTutorial){
		colorScheme.saberAColor = getColorFromManager(colorManager, "_saberAColor");
		colorScheme.saberBColor = getColorFromManager(colorManager, "_saberBColor");
		colorScheme.environmentColor0 = getColorFromManager(colorManager, "_environmentColor0");
		colorScheme.environmentColor1 = getColorFromManager(colorManager, "_environmentColor1");
		colorScheme.obstaclesColor = getColorFromManager(colorManager, "_obstaclesColor");
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
			colorScheme.saberAColor = getColorFromManager(colorManager, "_saberAColor");
			colorScheme.saberBColor = getColorFromManager(colorManager, "_saberBColor");
		}
		if(Config.LightsActive){
			colorScheme.environmentColor0 = ColorFromHSB(environmentColor0, 1.0, 1.0);
			colorScheme.environmentColor1 = ColorFromHSB(environmentColor1, 1.0, 1.0);
		}else{
			colorScheme.environmentColor0 = getColorFromManager(colorManager, "_environmentColor0");
			colorScheme.environmentColor1 = getColorFromManager(colorManager, "_environmentColor1");
		}
		
		if(Config.WallsActive){
			colorScheme.obstaclesColor = ColorFromHSB(obstaclesColor, 1.0, 1.0);
		}else{
			colorScheme.obstaclesColor = getColorFromManager(colorManager, "_obstaclesColor");
		}
	}
	RunMethod(colorManager, "SetColorScheme", &colorScheme);
	SaberBurnMarkSparkles_LateUpdate(self, type);
}

MAKE_HOOK_OFFSETLESS(GameNoteController_Update, void, Il2CppObject* self){
	Init();
	Il2CppObject* disappearingArrowController = GetFieldObjectValue(self, "_disappearingArrowController");
	Il2CppObject* colorNoteVisuals = GetFieldObjectValue(disappearingArrowController, "_colorNoteVisuals");
	Il2CppObject* noteController = GetFieldObjectValue(colorNoteVisuals, "_noteController");
	
	Il2CppObject* noteData = GetFieldObjectValue(noteController, "_noteData");

	int noteType;
	RunMethod(&noteType, noteData, "get_noteType"); 
	Color noteColor = noteType == 1 ? colorScheme.saberBColor : colorScheme.saberAColor;
	SetFieldValue(colorNoteVisuals, "_noteColor", &noteColor); 
	
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

MAKE_HOOK_OFFSETLESS(InitHooks, void*, void* arg1, void* arg2, void* arg3){
	if(!hookInit){
		Il2CppClass* saberBurnMarkSparklesClass = GetClassFromName("", "SaberBurnMarkSparkles");
		const MethodInfo* saberBurnMarkSparkles_LateUpdateMethod = class_get_method_from_name(saberBurnMarkSparklesClass, "LateUpdate", 0);
		INSTALL_HOOK_OFFSETLESS(SaberBurnMarkSparkles_LateUpdate, saberBurnMarkSparkles_LateUpdateMethod);
		
		Il2CppClass* tutorialControllerClass = GetClassFromName("", "TutorialController");
		const MethodInfo* tutorialController_AwakeMethod = class_get_method_from_name(tutorialControllerClass, "Awake", 0);
		const MethodInfo* tutorialController_OnDestroyMethod = class_get_method_from_name(tutorialControllerClass, "OnDestroy", 0);
		INSTALL_HOOK_OFFSETLESS(TutorialController_Awake, tutorialController_AwakeMethod);
		INSTALL_HOOK_OFFSETLESS(TutorialController_OnDestroy, tutorialController_OnDestroyMethod);

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
		hookInit = true;
	}
	return InitHooks(arg1, arg2, arg3);
}

void createDefaultConfig() {
	log(INFO, "Creating Configuration...");
	config_doc.RemoveMember("SabersActive");
	config_doc.RemoveMember("LightsActive");
	config_doc.RemoveMember("WallsActive");
	config_doc.RemoveMember("SaberASpeed");
	config_doc.RemoveMember("SaberBSpeed");
	config_doc.RemoveMember("SabersStartDiff");
	config_doc.RemoveMember("LightASpeed");
	config_doc.RemoveMember("LightBSpeed");
	config_doc.RemoveMember("LightsStartDiff");
	config_doc.RemoveMember("WallsSpeed");
	
	config_doc.AddMember("SabersActive", Config.SabersActive, config_doc.GetAllocator());
	config_doc.AddMember("LightsActive", Config.LightsActive, config_doc.GetAllocator());
	config_doc.AddMember("WallsActive", Config.WallsActive, config_doc.GetAllocator());
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
	if(config_doc.HasMember("SabersActive") && config_doc["SabersActive"].IsBool() &&
		config_doc.HasMember("LightsActive") && config_doc["LightsActive"].IsBool() &&
		config_doc.HasMember("WallsActive") && config_doc["WallsActive"].IsBool() &&
		config_doc.HasMember("SaberASpeed") && config_doc["SaberASpeed"].IsDouble() &&
		config_doc.HasMember("SaberBSpeed") && config_doc["SaberBSpeed"].IsDouble() && 
		config_doc.HasMember("SabersStartDiff") && config_doc["SabersStartDiff"].IsDouble() && 
		config_doc.HasMember("LightASpeed") && config_doc["LightASpeed"].IsDouble() &&
		config_doc.HasMember("LightBSpeed") && config_doc["LightBSpeed"].IsDouble() &&
		config_doc.HasMember("LightsStartDiff") && config_doc["LightsStartDiff"].IsDouble() &&
		config_doc.HasMember("WallsSpeed") && config_doc["WallsSpeed"].IsDouble()){
		Config.SabersActive = config_doc["SabersActive"].GetBool();
		Config.LightsActive = config_doc["LightsActive"].GetBool();
		Config.WallsActive = config_doc["WallsActive"].GetBool();
		Config.SaberASpeed = config_doc["SaberASpeed"].GetDouble();
		Config.SaberBSpeed = config_doc["SaberBSpeed"].GetDouble();
		Config.SabersStartDiff = config_doc["SabersStartDiff"].GetDouble();
		Config.LightASpeed = config_doc["LightASpeed"].GetDouble();
		Config.LightBSpeed = config_doc["LightBSpeed"].GetDouble();
		Config.LightsStartDiff = config_doc["LightsStartDiff"].GetDouble();
		Config.WallsSpeed = config_doc["WallsSpeed"].GetDouble();
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

	uintptr_t base = baseAddr("/data/app/com.beatgames.beatsaber-1/lib/arm64/libunity.so");
	uintptr_t hookAddr = FindPattern(base, 0x1000000, "ff 83 01 d1 f8 13 00 f9 f7 5b 03 a9 f5 53 04 a9 f3 7b 05 a9 f4 03 02 aa f5 03 01 aa f6 03 00 aa");
	if(hookAddr){
		INSTALL_HOOK_DIRECT(InitHooks, hookAddr);
		log(INFO, "Successfully installed RainbowSabers!");
	}else{
  		log(ERROR, "Couldn't find Method in libunity.so!");
	}
  
}
