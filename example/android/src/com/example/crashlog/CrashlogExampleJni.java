package com.example.crashlog;

class CrashlogExampleJni {
	static {
		System.loadLibrary("crashlog");
	}

	public native void setLogPath(String path);

	public native void setAppVersion(String ver);

	public native void forceCrash();
}