package com.watergate.library;

/**
 * Created by subho on 16/10/16.
 */
public class LockControlManager {
	static {
		System.loadLibrary("watergate");
	}

	public native void create(String configpath);

	public native void clearLocks();

	public native void dispose();
}
