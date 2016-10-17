package com.watergate.library;


import com.watergate.library.ObjectState.EObjectState;

/**
 * Created by subho on 16/10/16.
 */
public class LockControlClient {
	static {
		System.loadLibrary("watergate");
	}

	public static enum ELockResult {
		Locked, Expired, Retry, Timeout, QuotaReached, QuotaAvailable, Error,
		None;

		public static ELockResult parse(int state) {
			switch (state) {
				case 0:
					return Locked;
				case 1:
					return Expired;
				case 2:
					return Retry;
				case 3:
					return Timeout;
				case 4:
					return QuotaReached;
				case 5:
					return QuotaAvailable;
				case 6:
					return Error;
				default:
					return None;
			}
		}
	}

	public static enum EResourceType {
		UNKNOWN, IO, NET, FS;

		public static EResourceType parse(int value) {
			switch (value) {
				case 0:
					return UNKNOWN;
				case 1:
					return IO;
				case 2:
					return NET;
				case 3:
					return FS;
				default:
					return UNKNOWN;
			}
		}

		public static int getValue(EResourceType type) {
			switch (type) {
				case UNKNOWN:
					return 0;
				case IO:
					return 1;
				case NET:
					return 2;
				case FS:
					return 3;
			}
			return 0;
		}
	}

	public native void create(String configpath) throws LockControlException;

	public native String findLockByName(String name, int type) throws LockControlException;

	public native double getQuota(String name) throws LockControlException;

	private native int lock(String name, int priority, double quota) throws LockControlException;

	private native int lock(String name, int priority, double quota, long
			timeout) throws LockControlException;

	public native void register_thread(String lockname) throws LockControlException;

	public native boolean release(String name, int priority) throws LockControlException;

	private native int getControlState() throws LockControlException;

	private native String getControlError() throws LockControlException;

	public native void dispose() throws LockControlException;

	public native void test_assert() throws LockControlException;

	private ObjectState state = new ObjectState();

	public ELockResult getLock(String name, int priority, double quota) throws LockControlException {
		int r = lock(name, priority, quota);
		return ELockResult.parse(r);
	}

	public ELockResult getLock(String name, int priority, double quota, long
			timeout) throws LockControlException {
		int r = lock(name, priority, quota, timeout);
		return ELockResult.parse(r);
	}

	public ObjectState getState() throws LockControlException {
		int s = getControlState();
		state.setState(EObjectState.parse(s));
		if (state.hasError()) {
			String mesg = getControlError();
			state.setError(new Exception(mesg));
		}

		return state;
	}
}
