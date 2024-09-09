#include "ScriptEngine.hpp"
#include <iostream>

using createJVMFuncPointer_t = jint(*)(JavaVM** p_vm, void** p_env, void* vm_arg);

void ScriptEngine::init()
{
	// Load the JVM DLL dynamically
	HINSTANCE hGetProcIDDLL = LoadLibrary(L"C:\\Users\\matan\\source\\repos\\java\\bin\\server\\jvm.dll");

	if (!hGetProcIDDLL) {
		std::cout << "Could not load the dynamic library" << std::endl;
		exit(1);
	}

	auto createJVM = (createJVMFuncPointer_t)GetProcAddress(hGetProcIDDLL, "JNI_CreateJavaVM");

	JavaVMInitArgs vm_args;

	// We don't set the classpath here, so no classpath is provided initially
	vm_args.version = JNI_VERSION_10;  // Java 10 or higher (adjust if needed)
	vm_args.nOptions = 0;  // No options like classpath
	vm_args.ignoreUnrecognized = false;

	// Create the JVM
	if (createJVM(&jvm, (void**)&env, (void*)&vm_args) != JNI_OK) {
		std::cerr << "Failed to create the JVM" << std::endl;
		exit(1);
	}

	// Display JVM version
	std::cout << "JVM load succeeded: Version ";
	jint ver = env->GetVersion();
	std::cout << ((ver >> 16) & 0x0f) << "." << (ver & 0x0f) << std::endl;
}

ScriptEngine::~ScriptEngine()
{
	if (jvm) {
		jvm->DestroyJavaVM();  // Destroy the JVM when done
	}
}

void ScriptEngine::checkForException()
{
	if (env->ExceptionCheck()) {
		env->ExceptionDescribe();  // Print the exception stack trace
		env->ExceptionClear();     // Clear the exception
	}
}

void ScriptEngine::loadJar(const std::string& jarLocation)
{
	// Find the URLClassLoader class
	jclass classLoaderClass = env->FindClass("java/net/URLClassLoader");
	if (classLoaderClass == nullptr) {
		std::cerr << "Error: Could not find java/net/URLClassLoader class!" << std::endl;
		checkForException();
		return;
	}

	// Get the constructor for URLClassLoader
	jmethodID classLoaderConstructor = env->GetMethodID(classLoaderClass, "<init>", "([Ljava/net/URL;)V");
	if (classLoaderConstructor == nullptr) {
		std::cerr << "Error: Could not find URLClassLoader constructor!" << std::endl;
		checkForException();
		return;
	}

	// Find the URL class
	jclass urlClass = env->FindClass("java/net/URL");
	if (urlClass == nullptr) {
		std::cerr << "Error: Could not find java/net/URL class!" << std::endl;
		checkForException();
		return;
	}

	// Get the constructor for URL
	jmethodID urlConstructor = env->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
	if (urlConstructor == nullptr) {
		std::cerr << "Error: Could not find URL constructor!" << std::endl;
		checkForException();
		return;
	}

	// Create a URL object for the JAR file
	std::string jarLoc = "file:" + jarLocation;
	jstring jarPath = env->NewStringUTF(jarLoc.c_str());
	jobject jarURL = env->NewObject(urlClass, urlConstructor, jarPath);
	if (jarURL == nullptr) {
		std::cerr << "Error: Could not create URL object for JAR!" << std::endl;
		checkForException();
		return;
	}

	// Create an array of URLs (just one in this case)
	jobjectArray urlArray = env->NewObjectArray(1, urlClass, jarURL);
	if (urlArray == nullptr) {
		std::cerr << "Error: Could not create URL array!" << std::endl;
		checkForException();
		return;
	}

	// Create a new instance of URLClassLoader
	jobject classLoader = env->NewObject(classLoaderClass, classLoaderConstructor, urlArray);
	if (classLoader == nullptr) {
		std::cerr << "Error: Could not create URLClassLoader instance!" << std::endl;
		checkForException();
		return;
	}

	// Store the URLClassLoader as a global reference
	globalJarLoaderRef = env->NewGlobalRef(classLoader);

	// Clean up local references
	env->DeleteLocalRef(jarPath);
	env->DeleteLocalRef(jarURL);
	env->DeleteLocalRef(urlArray);
}

void ScriptEngine::unloadJar()
{
	// Delete the global reference to the URLClassLoader
	if (globalJarLoaderRef != nullptr) {
		env->DeleteGlobalRef(globalJarLoaderRef);
		globalJarLoaderRef = nullptr;
	}

	// Optionally trigger garbage collection in Java
	jclass systemClass = env->FindClass("java/lang/System");
	if (systemClass == nullptr) {
		std::cerr << "Error: Could not find java/lang/System class!" << std::endl;
		checkForException();
		return;
	}

	jmethodID gcMethod = env->GetStaticMethodID(systemClass, "gc", "()V");
	if (gcMethod == nullptr) {
		std::cerr << "Error: Could not find System.gc() method!" << std::endl;
		checkForException();
		return;
	}

	env->CallStaticVoidMethod(systemClass, gcMethod);
	checkForException();
}
