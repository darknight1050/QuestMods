#include <map>
#include <vector>
#include <thread>

#include "../beatsaber-hook/shared/utils/utils.h"

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

std::string HitSoundFilePath = "/sdcard/Android/data/com.beatgames.beatsaber/files/sounds/HitSound.ogg";
std::string BadHitSoundFilePath = "/sdcard/Android/data/com.beatgames.beatsaber/files/sounds/BadHitSound.ogg";

static Il2CppArray* HitSoundAudioClips = nullptr;
static Il2CppObject* HitSoundWebRequest = nullptr;
static Il2CppObject* HitSoundAsyncOp = nullptr;

static Il2CppArray* BadHitSoundAudioClips = nullptr;
static Il2CppObject* BadHitSoundWebRequest = nullptr;
static Il2CppObject* BadHitSoundAsyncOp = nullptr;

void HitSoundAudioClipCreateRequestComplete()
{
	log(DEBUG, "Attempting to get Content!");
	Il2CppObject* hitSoundAudioClip;
	if (il2cpp_utils::RunMethod(&hitSoundAudioClip, il2cpp_utils::GetClassFromName("UnityEngine.Networking", "DownloadHandlerAudioClip"), "GetContent", HitSoundWebRequest))
	{
		Il2CppObject* noteCutSoundEffectManager = GetFirstObjectOfType(il2cpp_utils::GetClassFromName("", "NoteCutSoundEffectManager"));
		if(noteCutSoundEffectManager != nullptr){
			HitSoundAudioClips = il2cpp_functions::array_new(il2cpp_utils::GetClassFromName("UnityEngine", "AudioClip"), 1);
			if (HitSoundAudioClips == nullptr)
			{
				log(ERROR, "Failed to make HitSoundAudioClips Array");
			}
			il2cpp_array_set(HitSoundAudioClips, Il2CppObject*, 0, hitSoundAudioClip);
			log(DEBUG, "Setting _randomLongCutSoundPicker and _randomShortCutSoundPicker objects...");
			if (!il2cpp_utils::SetFieldValue(il2cpp_utils::GetFieldValue(noteCutSoundEffectManager, "_randomLongCutSoundPicker"), "_objects", HitSoundAudioClips))
			{
				log(ERROR, "Couldn't set _randomLongCutSoundPicker");
			}
			if (!il2cpp_utils::SetFieldValue(il2cpp_utils::GetFieldValue(noteCutSoundEffectManager, "_randomShortCutSoundPicker"), "_objects", HitSoundAudioClips))
			{
				log(ERROR, "Couldn't set _randomShortCutSoundPicker");
			}			
		}
	}else{
		log(DEBUG, "Failed to execute getContent");
	}
}

void loadHitSoundAudioClip()
{
	Il2CppString* filePath = il2cpp_utils::createcsstr(HitSoundFilePath);
	Il2CppString* requestPath = il2cpp_utils::createcsstr("file:///" + HitSoundFilePath);
	if (il2cpp_utils::RunMethod(il2cpp_utils::GetClassFromName("System.IO", "File"), "Exists", &filePath))
	{
		int audioType = 14; //OGGVORBIS
		log(DEBUG, "File exist");
		log(DEBUG, "Attempting to get AudioClip...");
		if (!il2cpp_utils::RunMethod(&HitSoundWebRequest, il2cpp_utils::GetClassFromName("UnityEngine.Networking", "UnityWebRequestMultimedia"), "GetAudioClip", requestPath, &audioType))
		{
			log(DEBUG, "Failed to get AudioClip!");
		};

		log(DEBUG, "Attempting to send WebRequest");
		if (!il2cpp_utils::RunMethod(&HitSoundAsyncOp, HitSoundWebRequest, "SendWebRequest"))
		{
			log(DEBUG, "Failed to send WebRequest");
		}
		auto action = il2cpp_utils::MakeAction(nullptr, HitSoundAudioClipCreateRequestComplete, il2cpp_functions::class_get_type(il2cpp_utils::GetClassFromName("System", "Action")));
		if (action == nullptr)
		{
			log(ERROR, "Couldn't make HitSoundAudioClipCreateRequestComplete Action");
		}
		if (!il2cpp_utils::SetFieldValue(HitSoundAsyncOp, "m_completeCallback", action))
		{
			log(ERROR, "Couldn't set HitSoundAudioClipCreateRequestComplete Action");
		}
	}
	else
	{
		log(DEBUG, "File doesn't exist!");
	}
}


void BadHitSoundAudioClipCreateRequestComplete()
{
	log(DEBUG, "Attempting to get Content!");
	Il2CppObject* badHitSoundAudioClip;
	if (il2cpp_utils::RunMethod(&badHitSoundAudioClip, il2cpp_utils::GetClassFromName("UnityEngine.Networking", "DownloadHandlerAudioClip"), "GetContent", BadHitSoundWebRequest))
	{
		BadHitSoundAudioClips = il2cpp_functions::array_new(il2cpp_utils::GetClassFromName("UnityEngine", "AudioClip"), 1);
		if (BadHitSoundAudioClips == nullptr)
		{
			log(ERROR, "Failed to make BadHitSoundAudioClips Array");
		}
		il2cpp_array_set(BadHitSoundAudioClips, Il2CppObject*, 0, badHitSoundAudioClip);
		Array<Il2CppObject*>* objects;
		il2cpp_utils::RunMethod(&objects, il2cpp_utils::GetClassFromName("UnityEngine", "Resources"), "FindObjectsOfTypeAll", il2cpp_functions::type_get_object(il2cpp_functions::class_get_type(il2cpp_utils::GetClassFromName("", "NoteCutSoundEffect"))));
		if (objects != nullptr)
		{
			for(int i = 0;i<objects->Length();i++){
				log(DEBUG, "Setting BadHitSoundAudioClips! (BadHitSoundAudioClipCreateRequestComplete)");
				if (!il2cpp_utils::SetFieldValue(objects->values[i], "_badCutSoundEffectAudioClips", BadHitSoundAudioClips))
				{
					log(ERROR, "Couldn't set _badCutSoundEffectAudioClips");
				}
				Il2CppObject* badCutRandomSoundPicker = il2cpp_utils::GetFieldValue(objects->values[i], "_badCutRandomSoundPicker");
				if(badCutRandomSoundPicker != nullptr){
					if (!il2cpp_utils::SetFieldValue(badCutRandomSoundPicker, "_objects", BadHitSoundAudioClips))
					{
						log(ERROR, "Couldn't set _badCutRandomSoundPicker");
					}
				}
			}
		}
	}else{
		log(DEBUG, "Failed to execute getContent");
	}
}

void loadBadHitSoundAudioClip()
{
	Il2CppString* filePath = il2cpp_utils::createcsstr(BadHitSoundFilePath);
	Il2CppString* requestPath = il2cpp_utils::createcsstr("file:///" + BadHitSoundFilePath);
	if (il2cpp_utils::RunMethod(il2cpp_utils::GetClassFromName("System.IO", "File"), "Exists", &filePath))
	{
		int audioType = 14; //OGGVORBIS
		log(DEBUG, "File exist");
		log(DEBUG, "Attempting to get AudioClip...");
		if (!il2cpp_utils::RunMethod(&BadHitSoundWebRequest, il2cpp_utils::GetClassFromName("UnityEngine.Networking", "UnityWebRequestMultimedia"), "GetAudioClip", requestPath, &audioType))
		{
			log(DEBUG, "Failed to get AudioClip!");
		};

		log(DEBUG, "Attempting to send WebRequest");
		if (!il2cpp_utils::RunMethod(&BadHitSoundAsyncOp, BadHitSoundWebRequest, "SendWebRequest"))
		{
			log(DEBUG, "Failed to send WebRequest");
		}
		auto action = il2cpp_utils::MakeAction(nullptr, BadHitSoundAudioClipCreateRequestComplete, il2cpp_functions::class_get_type(il2cpp_utils::GetClassFromName("System", "Action")));
		if (action == nullptr)
		{
			log(ERROR, "Couldn't make BadHitSoundAudioClipCreateRequestComplete Action");
		}
		if (!il2cpp_utils::SetFieldValue(BadHitSoundAsyncOp, "m_completeCallback", action))
		{
			log(ERROR, "Couldn't set BadHitSoundAudioClipCreateRequestComplete Action");
		}
	}
	else
	{
		log(DEBUG, "File doesn't exist!");
	}
}

MAKE_HOOK_OFFSETLESS(NoteCutSoundEffectManager_Start, void, Il2CppObject *self)
{
	if(HitSoundAudioClips == nullptr)
		loadHitSoundAudioClip();
	if(BadHitSoundAudioClips == nullptr)
		loadBadHitSoundAudioClip();
	NoteCutSoundEffectManager_Start(self);
}

MAKE_HOOK_OFFSETLESS(NoteCutSoundEffect_Awake, void, Il2CppObject *self)
{
	NoteCutSoundEffect_Awake(self);
	if(BadHitSoundAudioClips != nullptr){
		log(DEBUG, "Setting BadHitSoundAudioClips! (NoteCutSoundEffect_Awake)");
		if (!il2cpp_utils::SetFieldValue(self, "_badCutSoundEffectAudioClips", BadHitSoundAudioClips))
		{
			log(ERROR, "Couldn't set _badCutSoundEffectAudioClips");
		}
		Il2CppObject* badCutRandomSoundPicker = il2cpp_utils::GetFieldValue(self, "_badCutRandomSoundPicker");
		if(badCutRandomSoundPicker != nullptr){
			if (!il2cpp_utils::SetFieldValue(badCutRandomSoundPicker, "_objects", BadHitSoundAudioClips))
			{
				log(ERROR, "Couldn't set _badCutRandomSoundPicker");
			}
		}
	}
}

MAKE_HOOK_OFFSETLESS(SceneManager_SetActiveScene, bool, int scene)
{
    HitSoundAudioClips = nullptr;
    BadHitSoundAudioClips = nullptr;
    return SceneManager_SetActiveScene(scene);
}

extern "C" void load()
{
	il2cpp_functions::Init();
	log(INFO, "Installing HitSoundChanger");
	INSTALL_HOOK_OFFSETLESS(NoteCutSoundEffect_Awake, il2cpp_utils::GetMethod("", "NoteCutSoundEffect", "Awake", 0));
	INSTALL_HOOK_OFFSETLESS(NoteCutSoundEffectManager_Start, il2cpp_utils::GetMethod("", "NoteCutSoundEffectManager", "Start", 0));
	INSTALL_HOOK_OFFSETLESS(SceneManager_SetActiveScene, il2cpp_utils::GetMethod("UnityEngine.SceneManagement", "SceneManager", "SetActiveScene", 1));
   	log(INFO, "Installed HitSoundChanger!");
}
