call adb shell am force-stop com.beatgames.beatsaber

call C:/Development/Tools/android-ndk-r20/ndk-build.cmd NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./Android.mk NDK_APPLICATION_MK=./Application.mk

call adb push libs/arm64-v8a/librainbowmod.so /sdcard/Android/data/com.beatgames.beatsaber/files/mods/librainbowmod.so

call adb shell am start com.beatgames.beatsaber/com.unity3d.player.UnityPlayerActivity

