package com.watergate.library;


import com.watergate.library.ObjectState.EObjectState;
import com.watergate.library.utils.LogUtils;

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
				case 3:
					return Expired;
				case 9:
					return Retry;
				case 6:
					return Timeout;
				case 1:
					return QuotaReached;
				case 7:
					return QuotaAvailable;
				case 5:
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
		LogUtils.debug(getClass(), String.format("[name=%s][priority=%d] Lock" +
				" response %d", name, priority, r));
		return ELockResult.parse(r);
	}

	public ELockResult getLock(String name, int priority, double quota, long
			timeout) throws LockControlException {
		int r = lock(name, priority, quota, timeout);
		LogUtils.debug(getClass(), String.format("[name=%s][priority=%d] Lock" +
				" response %d", name, priority, r));
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
