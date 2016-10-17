package com.watergate.library;


/**
 * Created by subho on 16/10/16.
 */
public class LockControlManager {
	static {
		System.loadLibrary("watergate");
	}

	public native void create(String configpath) throws LockControlException;

	public native void clearLocks() throws LockControlException;

	public native void dispose() throws LockControlException;
}
