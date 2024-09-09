#pragma once
#include <windows.h>
#include <jni.h>
#include <string>

class ScriptEngine
{
public:
	ScriptEngine() = default;

	void init();

	~ScriptEngine();

private:
	JavaVM* jvm;
	JNIEnv* env;
	jobject globalJarLoaderRef = nullptr;

	void checkForException();
	void loadJar(const std::string& jarLocation);
	void unloadJar();
};