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

#define SaberBurnMarkSparkles_LateUpdate_Offset 0xA0A5BC
#define GameNoteController_Update_Offset 0xA535F8
#define ObstacleController_Update_Offset 0xCE044C
#define TutorialController_Awake_Offset 0xB2F4C8
#define TutorialController_OnDestroy_Offset 0xB2F944

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
	char colorSchemeId[0x18];
  	char colorSchemeName[8];
    int isEditable;
    Color saberAColor;
    Color saberBColor;
    Color environmentColor0;
    Color environmentColor1;
	Color obstaclesColor;
} ColorScheme;

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

MAKE_HOOK(SaberBurnMarkSparkles_LateUpdate, SaberBurnMarkSparkles_LateUpdate_Offset, void, Il2CppObject* self, void *type){
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

MAKE_HOOK(GameNoteController_Update, GameNoteController_Update_Offset, void, Il2CppObject* self){
	Init();
	Il2CppObject* disappearingArrowController = GetFieldObjectValue(self, "_disappearingArrowController");
	Il2CppObject* colorNoteVisuals = GetFieldObjectValue(disappearingArrowController, "_colorNoteVisuals");
	Il2CppObject* noteController = GetFieldObjectValue(colorNoteVisuals, "_noteController");
	RunMethod(colorNoteVisuals, "HandleNoteControllerDidInitEvent", noteController);
	GameNoteController_Update(self);
}

MAKE_HOOK(ObstacleController_Update, ObstacleController_Update_Offset, void, Il2CppObject* self){
	Init();
	Il2CppObject* stretchableObstacle = GetFieldObjectValue(self, "_stretchableObstacle");
	Il2CppObject* obstacleFrame = GetFieldObjectValue(stretchableObstacle, "_obstacleFrame");
	
	float width, height, length;
	GetFieldValue(&width, obstacleFrame, "width");
	GetFieldValue(&height, obstacleFrame, "height");
	GetFieldValue(&length, obstacleFrame, "length");
	RunMethod(stretchableObstacle, "SetSizeAndColor", &width, &height, &length, &colorScheme.obstaclesColor);
	ObstacleController_Update(self);
}

MAKE_HOOK(TutorialController_Awake, TutorialController_Awake_Offset, void, Il2CppObject* self){
	TutorialController_Awake(self);
	InTutorial = true;
}

MAKE_HOOK(TutorialController_OnDestroy, TutorialController_OnDestroy_Offset, void, Il2CppObject* self){
	TutorialController_OnDestroy(self);
	InTutorial = false;
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
    #ifdef __aarch64__
    log(INFO, "Is 64 bit!");
    #endif
	
	INSTALL_HOOK(TutorialController_Awake);
	INSTALL_HOOK(TutorialController_OnDestroy);
	INSTALL_HOOK(SaberBurnMarkSparkles_LateUpdate);
	if(!loadConfig())
		 createDefaultConfig();
	saberB = Config.SabersStartDiff;
	environmentColor1 = Config.LightsStartDiff;
	if(Config.SabersActive)
		INSTALL_HOOK(GameNoteController_Update);
	if(Config.WallsActive)
		INSTALL_HOOK(ObstacleController_Update);
    log(DEBUG, "Successfully installed RainbowSabers!");
}
