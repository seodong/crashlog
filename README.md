#crashlog

Android app crash logs to a file.

안드로이드 앱에서 Crash 로그를 파일로 기록하는 코드입니다.
기존 Android에서는 개발자 모드에서만 crash 로그를 기록합니다.
crashlog는 앱에 libcrashlog.so를 동적로딩하면, 앱에서 지정한 디렉토리로 crash log를 파일로 생성합니다.

![ViewPagerIndicator Sample Screenshots][1]

##Complie
```
$cd example/android
$make
```

하면 컴파일을 합니다. 

```
ls ./bin/crashlog-release-unsigned.apk 
```

apk 파일이 생성됩다.

##Test

앱에서 force crash 버튼을 누르면 {ExternalStorageDirectory}/crashlog/crash.txt 파일이 생성됩니다.

```
cat {ExternalStorageDirectory}/crashlog/crash.txt
Time : 2014-07-29 15:11:08 KST
Build fingerprint: 'samsung/c1skt/c1skt:4.3/JSS15J/E210SKSUGND1:user/release-keys'
pid: 31788, tid: 31788, name: xample.crashlog  >>> com.example.crashlog <<<
signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 00000000
    r0 41e25b00  r1 a5b0001d  r2 00000001  r3 00000000
    r4 57e7ead0  r5 41e247c0  r6 00000000  r7 400a9dd4
    r8 bef702a0  r9 400a9dcc  sl 41e247d0  fp bef702b4
    ip 5c5f8409  sp bef702a0  lr 40b3c4d0  pc 5c5f840c

backtrace: 17
  #00  pc 0000240c  /data/app-lib/com.example.crashlog-1/libcrashlog.so (Java_com_example_crashlog_CrashlogExampleJni_forceCrash+3)
  #01  pc 0001e4cc  /system/lib/libdvm.so (dvmPlatformInvoke+112)
  #02  pc 0004e99b  /system/lib/libdvm.so (dvmCallJNIMethod(unsigned int const*, JValue*, Method const*, Thread*)+398)
  #03  pc 00050651  /system/lib/libdvm.so (dvmResolveNativeMethod(unsigned int const*, JValue*, Method const*, Thread*)+256)
  #04  pc 000278e0  /system/lib/libdvm.so
  #05  pc 0002be80  /system/lib/libdvm.so (dvmInterpret(Thread*, Method const*, JValue*)+184)
  #06  pc 00060dbb  /system/lib/libdvm.so (dvmInvokeMethod(Object*, Method const*, ArrayObject*, ArrayObject*, ClassObject*, bool)+350)
  #07  pc 00068a7f  /system/lib/libdvm.so
  #08  pc 000278e0  /system/lib/libdvm.so
  #09  pc 0002be80  /system/lib/libdvm.so (dvmInterpret(Thread*, Method const*, JValue*)+184)
  #10  pc 00060afd  /system/lib/libdvm.so (dvmCallMethodV(Thread*, Method const*, Object*, bool, JValue*, std::__va_list)+292)
  #11  pc 0004a57b  /system/lib/libdvm.so
  #12  pc 00054e63  /system/lib/libandroid_runtime.so
  #13  pc 0005638b  /system/lib/libandroid_runtime.so (android::AndroidRuntime::start(char const*, char const*)+378)
  #14  pc 0000105b  /system/bin/app_process
  #15  pc 0000dc4f  /system/lib/libc.so (__libc_init+50)
  #16  pc 00000d7c  /system/bin/app_process

stack:
         bef70260  40180000  /system/lib/libc.so (__libc_malloc_dispatch+4294672384)
         bef70264  00000000  
         bef70268  5c5f8409  /data/app-lib/com.example.crashlog-1/libcrashlog.so (Java_com_example_crashlog_CrashlogExampleJni_forceCrash)
         bef7026c  6030c428  
         bef70270  00000000  
         bef70274  40145ac1  /system/lib/libc.so (free+12)
         bef70278  400e6494  
         bef7027c  40b6e51b  /system/lib/libdvm.so
         bef70280  00000000  
         bef70284  4006cb1b  /system/lib/libbinder.so (android::Parcel::ipcSetDataReference(unsigned char const*, unsigned int, unsigned int const*, unsigned int, void (*)(android::Parcel*, unsigned char const*, unsigned int, unsigned int const*, unsigned int, void*), void*)+26)
         bef70288  428ebfd0  /dev/ashmem/dalvik-heap (deleted)
         bef7028c  41e247c0  [heap]
         bef70290  40bcfc6c  /system/lib/libdvm.so
         bef70294  00000001  
         bef70298  df0027ad  
         bef7029c  00000000  
    #00  bef702a0  400a9dc8  
         ........  ........
    #01  bef702a0  400a9dc8  
         bef702a4  00000001  
         bef702a8  00000000  
         bef702ac  428ebfd0  /dev/ashmem/dalvik-heap (deleted)
         bef702b0  401831f4   (__stack_chk_guard+4294664192)
         bef702b4  40b6c99f  /system/lib/libdvm.so (dvmCallJNIMethod(unsigned int const*, JValue*, Method const*, Thread*)+402)
    #02  bef702b8  400a9dc8  
         bef702bc  5f152f84  /data/dalvik-cache/data@app@com.example.crashlog-1.apk@classes.dex
         bef702c0  5c5f8409  /data/app-lib/com.example.crashlog-1/libcrashlog.so (Java_com_example_crashlog_CrashlogExampleJni_forceCrash)
         bef702c4  41e247d0  [heap]
         bef702c8  00000000  
         bef702cc  40b73f1d  /system/lib/libdvm.so (dvmAddTrackedAlloc+24)
         bef702d0  00000000  
         bef702d4  00000000  
         bef702d8  428f7ba8  /dev/ashmem/dalvik-heap (deleted)
         bef702dc  401831f4   (__stack_chk_guard+4294664192)
         bef702e0  00000012  
         bef702e4  40b48704  /system/lib/libdvm.so (dvmMalloc(unsigned int, int)+96)
         bef702e8  00000008  
         bef702ec  41e247c0  [heap]
         bef702f0  428f7b88  /dev/ashmem/dalvik-heap (deleted)
         bef702f4  40bcfc6c  /system/lib/libdvm.so
         ........  ........
    #03  bef703d0  57afe4d0  /dev/ashmem/dalvik-LinearAlloc (deleted)
         bef703d4  40b3c4d0  /system/lib/libdvm.so (dvmPlatformInvoke+116)
         bef703d8  400a9bac  
         bef703dc  00000001  
         bef703e0  00000000  
         bef703e4  57afe4d0  /dev/ashmem/dalvik-LinearAlloc (deleted)
         bef703e8  41e247c0  [heap]
         bef703ec  40b6c9a7  /system/lib/libdvm.so (dvmCallJNIMethod(unsigned int const*, JValue*, Method const*, Thread*)+410)
         bef703f0  400a9bac  
         bef703f4  595e3e26  /system/framework/framework.odex
         bef703f8  402a5ae7  /system/lib/libandroid_runtime.so
         bef703fc  41e247d0  [heap]
         bef70400  00000000  
         bef70404  401831f4   (__stack_chk_guard+4294664192)
         bef70408  a5a0001d  
         bef7040c  00000000  
         ........  ........
    #04  bef70508  2f617661  
         bef7050c  fffffe58  
         bef70510  40b4c79c  /system/lib/libdvm.so (dvmMterpStd(Thread*))
         bef70514  00000000  
         bef70518  bef705d0  [stack]
         bef7051c  41e247c0  [heap]
         bef70520  bef7053c  [stack]
         bef70524  57bb76b8  /dev/ashmem/dalvik-LinearAlloc (deleted)
         bef70528  428b0d24  /dev/ashmem/dalvik-heap (deleted)
         bef7052c  40b49e84  /system/lib/libdvm.so (dvmInterpret(Thread*, Method const*, JValue*)+188)
```


[1]: https://raw.githubusercontent.com/seodong/crashlog/master/screens.png
