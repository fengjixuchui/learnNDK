#include <jni.h>
#include <string>
#include <stdlib.h>

#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "ceshi", __VA_ARGS__)

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_lahm_ctest_MainActivity_hello(JNIEnv *env, jobject instance) {

    // TODO
    std::string returnValue = "fcku";
//    他们所使用的方法名都是一样
//    只是c++中的所有函数不在需要传env的上下文了，这是因为c++中有this上下文关键字。
//    return (*env)->NewStringUTF(env, returnValue);
    return env->NewStringUTF(returnValue.c_str());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_lahm_ctest_MainActivity_getMetaValue(JNIEnv *env, jobject instance,
                                                      jstring name_) {
    const char *name = env->GetStringUTFChars(name_, 0);//好像自动生成的这句没用到

    //1 . 找到java代码的 class文件
    // jclass      (*FindClass)(JNIEnv*, const char*);
    jclass contextWrapper = env->FindClass("android/content/ContextWrapper");

    //2 寻找class里面的方法
    // jmethodID   (*GetMethodID)(JNIEnv*, jclass, const char*, const char*);
    jmethodID method_id_getPackageName = env->GetMethodID(contextWrapper, "getPackageName",
                                                          "()Ljava/lang/String;");

    //3 .调用这个方法拿packageName
    jstring packageName = (jstring) env->CallObjectMethod(instance, method_id_getPackageName);

    jmethodID method_id_getPackageManager = env->GetMethodID(contextWrapper, "getPackageManager",
                                                             "()Landroid/content/pm/PackageManager;");

    //拿packageManager实例
    jobject packageManagerObj = env->CallObjectMethod(instance, method_id_getPackageManager);

    //packageManager类
    jclass packageManagerClass = env->FindClass("android/content/pm/PackageManager");

    //在packageManager类里找方法
    jmethodID method_id_getApplicationInfo = env->GetMethodID(packageManagerClass,
                                                              "getApplicationInfo",
                                                              "(Ljava/lang/String;I)Landroid/content/pm/ApplicationInfo;");

    //拿到applicationInfo的实例
    jobject applicationInfoObj = env->CallObjectMethod(packageManagerObj,
                                                       method_id_getApplicationInfo,
                                                       packageName, 128);

    //metaData属于ApplicationInfo的类属性
    jclass applicationInfoClass = env->FindClass("android/content/pm/ApplicationInfo");
    //拿bundle变量id
    jfieldID bundleId = env->GetFieldID(applicationInfoClass, "metaData", "Landroid/os/Bundle;");
    //拿bundle变量实例
    jobject bundleObj = env->GetObjectField(applicationInfoObj, bundleId);

    //拿bundle的类
    jclass bundleClass = env->FindClass("android/os/Bundle");
    //拿bundle getString的id
    jmethodID method_id_getString = env->GetMethodID(bundleClass, "getString",
                                                     "(Ljava/lang/String;)Ljava/lang/String;");

    //调用getString方法，
    jstring result = (jstring) env->CallObjectMethod(bundleObj, method_id_getString,
                                                     name_);
    // 传个string那么纠结
//    jstring result = (jstring) env->CallObjectMethod(bundleObj, method_id_getString,
//                                                     env->NewStringUTF("shit"));
    env->DeleteLocalRef(packageManagerObj);
    env->DeleteLocalRef(applicationInfoObj);
    env->DeleteLocalRef(bundleObj);

    env->ReleaseStringUTFChars(name_, name);//好像自动生成的这句没用到

    return result;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_lahm_ctest_MainActivity_checkDebug(JNIEnv *env, jobject instance) {

    jclass contextWrapper = env->FindClass("android/content/ContextWrapper");

    jmethodID method_id_getPackageName = env->GetMethodID(contextWrapper, "getPackageName",
                                                          "()Ljava/lang/String;");
    jstring packageName = (jstring) env->CallObjectMethod(instance, method_id_getPackageName);

    jmethodID method_id_getPackageManager = env->GetMethodID(contextWrapper, "getPackageManager",
                                                             "()Landroid/content/pm/PackageManager;");

    jobject packageManagerObj = env->CallObjectMethod(instance, method_id_getPackageManager);

    jclass packageManagerClass = env->FindClass("android/content/pm/PackageManager");

    jmethodID method_id_getApplicationInfo = env->GetMethodID(packageManagerClass,
                                                              "getApplicationInfo",
                                                              "(Ljava/lang/String;I)Landroid/content/pm/ApplicationInfo;");

    jobject applicationInfoObj = env->CallObjectMethod(packageManagerObj,
                                                       method_id_getApplicationInfo,
                                                       packageName, 128);

    jclass applicationInfoClass = env->FindClass("android/content/pm/ApplicationInfo");

    jfieldID field_id_flags = env->GetFieldID(applicationInfoClass, "flags", "I");

    jint flags = env->GetIntField(applicationInfoObj, field_id_flags);

    env->DeleteLocalRef(packageManagerObj);
    env->DeleteLocalRef(applicationInfoObj);

    if ((flags & 2) != 0) {
//        pthread_exit(0);//这只是杀自己的c线程
        exit(0);//native kill form stdlib.h
    }

    return flags & 2;
}

extern "C"
JNIEXPORT void JNICALL
callMemLeak(JNIEnv *env, jobject obj, jint size) {
    //分配40M
    int *p = (int *) malloc(1024 * 1024 * size);

    //下一次分配之前，如果不释放，会造成40M的内存泄漏
//    free(p);
//    p = NULL;
    //分配10M
//    p = (int *) malloc(1024 * 1024 * 10);
//    free(p);
//    p = NULL;
}

//复写jni_onload完成动态注册
extern "C"
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved) {

    //声明变量
    jint result = JNI_ERR;
    JNIEnv *env;
    //获取JNI环境对象
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        LOGD("ERROR: GetEnv failed\n");
        return JNI_ERR;
    }

    JNINativeMethod methods[] = {
            {"callMemLeak", "(I)V", (void *) callMemLeak},
    };
    jclass clazz = env->FindClass("com/example/lahm/ctest/MainActivity");
    if (clazz == NULL) {
        LOGD("Native registration unable to find class");
        return JNI_ERR;
    }
    int methodsLength;
    //建立方法隐射关系
    //取得方法长度
    methodsLength = sizeof(methods) / sizeof(methods[0]);
    if (env->RegisterNatives(clazz, methods, methodsLength) != 0) {
        return JNI_ERR;
    }

    result = JNI_VERSION_1_6;
    return result;
}


//onUnLoad方法，在JNI组件被释放时调用
void JNI_OnUnload(JavaVM *vm, void *reserved) {
    LOGD("JNI unload...");
}