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
#define VERSION "0.0.1"

#include "../beatsaber-hook/shared/utils/utils.h"
#include "../beatsaber-hook/shared/utils/typedefs.h"

#define SaberBurnMarkSparkles_LateUpdate_Offset 0xA0A5BC

#define GameNoteController_Update_Offset 0xA535F8

static auto& config_doc = Configuration::config;

static struct Config_t {
	bool SabersActive = true;
	bool LightsActive = true;
	double SaberASpeed = 1.0;
	double SaberBSpeed = 1.0;
	double SabersStartDiff = 180.0;
	double LightASpeed = 2.0;
	double LightBSpeed = 2.0;
	double LightsStartDiff = 180.0;
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

static float saberA = 0;
static float saberB = 0;
static float environmentColor0 = 0;
static float environmentColor1 = 0;
static float obstaclesColor = 0;

Color ColorFormHSV(float H, float S, float V) {
	float C = S * V;
	float X = C * (1 - abs(fmod(H / 60.0f, 2) - 1));
	float m = V - C;
	float Rs, Gs, Bs;

	if(H >= 0 && H < 60) {
		Rs = C;
		Gs = X;
		Bs = 0;	
	}
	else if(H >= 60 && H < 120) {	
		Rs = X;
		Gs = C;
		Bs = 0;	
	}
	else if(H >= 120 && H < 180) {
		Rs = 0;
		Gs = C;
		Bs = X;	
	}
	else if(H >= 180 && H < 240) {
		Rs = 0;
		Gs = X;
		Bs = C;	
	}
	else if(H >= 240 && H < 300) {
		Rs = X;
		Gs = 0;
		Bs = C;	
	}
	else {
		Rs = C;
		Gs = 0;
		Bs = X;	
	}
	Color color;
	color.r = (Rs + m);
	color.g = (Gs + m);
	color.b = (Bs + m);
	color.a = 1.0f;
	return color;
}

Color getColorFromManager(Il2CppObject* color){
	auto klass = il2cpp_functions::object_get_class(color);
	auto field = il2cpp_functions::class_get_field_from_name(klass, "_color");
	Color saberColor;
	il2cpp_functions::field_get_value(color, field, &saberColor);
	return saberColor;
}

Color getColorFromManager(Il2CppObject* colorManager, const char* fieldName){
	auto klass = il2cpp_functions::object_get_class(colorManager);
	auto fieldColorManager = il2cpp_functions::class_get_field_from_name(klass, fieldName);
	Il2CppObject* color = il2cpp_functions::field_get_value_object(fieldColorManager, colorManager);
	return getColorFromManager(color);
}

MAKE_HOOK(SaberBurnMarkSparkles_LateUpdate, SaberBurnMarkSparkles_LateUpdate_Offset, void, Il2CppObject* self, void *type){
	il2cpp_functions::Init();
	SaberBurnMarkSparkles_LateUpdate(self, type);
	auto klass = il2cpp_functions::object_get_class(self);
	auto fieldColorManager = il2cpp_functions::class_get_field_from_name(klass, "_colorManager");
	Il2CppObject* colorManager = il2cpp_functions::field_get_value_object(fieldColorManager, self);
	
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
	obstaclesColor+=1;
	if(obstaclesColor > 360){
		obstaclesColor = 0;
	}
	ColorScheme colorScheme;
	if(Config.SabersActive){
		colorScheme.saberAColor = ColorFormHSV(saberA, 1.0, 1.0);
		colorScheme.saberBColor = ColorFormHSV(saberB, 1.0, 1.0);
	}else{
		colorScheme.saberAColor = getColorFromManager(colorManager, "_saberAColor");
		colorScheme.saberBColor = getColorFromManager(colorManager, "_saberBColor");
	}
	if(Config.LightsActive){
		colorScheme.environmentColor0 = ColorFormHSV(environmentColor0, 1.0, 1.0);
		colorScheme.environmentColor1 = ColorFormHSV(environmentColor1, 1.0, 1.0);
	}else{
		colorScheme.environmentColor0 = getColorFromManager(colorManager, "_environmentColor0");
		colorScheme.environmentColor1 = getColorFromManager(colorManager, "_environmentColor1");
	}
	colorScheme.obstaclesColor = getColorFromManager(colorManager, "_obstaclesColor");
	il2cpp_utils::RunMethod(colorManager, "SetColorScheme", &colorScheme);
  
}

MAKE_HOOK(GameNoteController_Update, GameNoteController_Update_Offset, void, Il2CppObject* self){
	auto klass = il2cpp_functions::object_get_class(self);
	auto fielDisappearingArrowController = il2cpp_functions::class_get_field_from_name(klass, "_disappearingArrowController");
	Il2CppObject* disappearingArrowController = il2cpp_functions::field_get_value_object(fielDisappearingArrowController, self);
	auto disappearingArrowControllerClass = il2cpp_functions::object_get_class(disappearingArrowController);
	auto fielColorNoteVisuals = il2cpp_functions::class_get_field_from_name(disappearingArrowControllerClass, "_colorNoteVisuals");
	Il2CppObject* colorNoteVisuals = il2cpp_functions::field_get_value_object(fielColorNoteVisuals, disappearingArrowController);
	auto colorNoteVisualsClass = il2cpp_functions::object_get_class(colorNoteVisuals);
	auto fieldNoteController = il2cpp_functions::class_get_field_from_name(colorNoteVisualsClass, "_noteController");
	Il2CppObject* noteController = il2cpp_functions::field_get_value_object(fieldNoteController, colorNoteVisuals);
	il2cpp_utils::RunMethod(colorNoteVisuals, "HandleNoteControllerDidInitEvent", noteController);
	GameNoteController_Update(self);
}

void createDefaultConfig() {
	log(INFO, "Creating Configuration...");
	config_doc.AddMember("SabersActive", Config.SabersActive, config_doc.GetAllocator());
	config_doc.AddMember("LightsActive", Config.LightsActive, config_doc.GetAllocator());
	config_doc.AddMember("SaberASpeed", Config.SaberASpeed, config_doc.GetAllocator());
	config_doc.AddMember("SaberBSpeed", Config.SaberBSpeed, config_doc.GetAllocator());
	config_doc.AddMember("SabersStartDiff", Config.SabersStartDiff, config_doc.GetAllocator());
	config_doc.AddMember("LightASpeed", Config.LightASpeed, config_doc.GetAllocator());
	config_doc.AddMember("LightBSpeed", Config.LightBSpeed, config_doc.GetAllocator());
	config_doc.AddMember("LightsStartDiff", Config.LightsStartDiff, config_doc.GetAllocator());
    Configuration::Write();
	log(INFO, "Created Configuration!");
}

bool loadConfig() { 
	log(INFO, "Loading Configuration...");
	Configuration::Load();
	if(config_doc.HasMember("SabersActive") && config_doc["SabersActive"].IsBool() &&
		config_doc.HasMember("LightsActive") && config_doc["LightsActive"].IsBool() &&
		config_doc.HasMember("SaberASpeed") && config_doc["SaberASpeed"].IsDouble() &&
		config_doc.HasMember("SaberBSpeed") && config_doc["SaberBSpeed"].IsDouble() && 
		config_doc.HasMember("SabersStartDiff") && config_doc["SabersStartDiff"].IsDouble() && 
		config_doc.HasMember("LightASpeed") && config_doc["LightASpeed"].IsDouble() &&
		config_doc.HasMember("LightBSpeed") && config_doc["LightBSpeed"].IsDouble() &&
		config_doc.HasMember("LightsStartDiff") && config_doc["LightsStartDiff"].IsDouble()){
		Config.SabersActive = config_doc["SabersActive"].GetBool();
		Config.LightsActive = config_doc["LightsActive"].GetBool();
		Config.SaberASpeed = config_doc["SaberASpeed"].GetDouble();
		Config.SaberBSpeed = config_doc["SaberBSpeed"].GetDouble();
		Config.SabersStartDiff = config_doc["SabersStartDiff"].GetDouble();
		Config.LightASpeed = config_doc["LightASpeed"].GetDouble();
		Config.LightBSpeed = config_doc["LightBSpeed"].GetDouble();
		Config.LightsStartDiff = config_doc["LightsStartDiff"].GetDouble();
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
    INSTALL_HOOK(SaberBurnMarkSparkles_LateUpdate);
	
	if(!loadConfig())
		 createDefaultConfig();
	saberB = Config.SabersStartDiff;
	environmentColor1 = Config.LightsStartDiff;
	if(Config.SabersActive)
		INSTALL_HOOK(GameNoteController_Update);
    log(DEBUG, "Successfully installed RainbowSabers!");
}
