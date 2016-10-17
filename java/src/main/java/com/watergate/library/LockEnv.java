package com.watergate.library;


import com.watergate.library.ObjectState.EObjectState;
import com.watergate.library.ObjectState.StateException;

/**
 * Created by subho on 16/10/16.
 */
public class LockEnv {
	static {
		System.loadLibrary("watergate");
	}

	private ObjectState state = new ObjectState();

	private native void create(String filename, String appname) throws LockControlException;

	private native void dispose() throws LockControlException;

	private static final LockEnv instance = new LockEnv();

	public static final void createEnv(String configf, String appname) throws
			LockControlException {
		try {
			instance.create(configf, appname);

			instance.state.setState(EObjectState.Available);
		} catch (Throwable t) {
			instance.state.setError(t);
			throw new LockControlException("Error initializing Lock " +
					"environment.", t);
		}
	}

	public static final void shutdown() throws LockControlException, StateException {
		ObjectState.CHECK_STATE(instance.state, EObjectState.Available);
		instance.state.setState(EObjectState.Disposed);
		instance.dispose();
	}

	public static final LockEnv get() throws StateException {
		ObjectState.CHECK_STATE(instance.state, EObjectState.Available);
		return instance;
	}
}
