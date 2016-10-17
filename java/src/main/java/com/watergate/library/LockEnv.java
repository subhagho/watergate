package com.watergate.library;

/**
 * Created by subho on 16/10/16.
 */
public class LockEnv {
	static {
		System.loadLibrary("watergate");
	}

	public native void create(String filename, String appname);

	public native void dispose();
}
