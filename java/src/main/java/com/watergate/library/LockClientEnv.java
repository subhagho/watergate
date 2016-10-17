package com.watergate.library;

import com.watergate.library.ObjectState.EObjectState;
import com.watergate.library.ObjectState.StateException;

/**
 * Created by subho on 17/10/16.
 */
public class LockClientEnv {
	private ObjectState state = new ObjectState();
	private LockControlClient client;

	private static final LockClientEnv clientEnv = new LockClientEnv();

	public static final void createEnv(String configf, String appname,
									   String clientConfigPath) throws
			LockControlException {
		try {
			LockEnv.createEnv(configf, appname);
			clientEnv.client = new LockControlClient();
			clientEnv.client.create(clientConfigPath);

			clientEnv.state.setState(EObjectState.Available);
		} catch (Throwable t) {
			clientEnv.state.setError(t);
			throw t;
		}
	}

	public static final void createEnv(String clientConfigPath) throws
			LockControlException {
		try {
			clientEnv.client = new LockControlClient();
			clientEnv.client.create(clientConfigPath);

			clientEnv.state.setState(EObjectState.Available);
		} catch (Throwable t) {
			clientEnv.state.setError(t);
			throw t;
		}
	}

	public static final void shutdown() throws LockControlException, StateException {
		ObjectState.CHECK_STATE(clientEnv.state, EObjectState.Available);

		clientEnv.state.setState(EObjectState.Disposed);
		clientEnv.client.dispose();

		LockEnv.shutdown();
	}

	public static final LockClientEnv get() throws StateException {
		ObjectState.CHECK_STATE(clientEnv.state, EObjectState.Available);

		return clientEnv;
	}

	public static final LockControlClient getLockClient() throws
			StateException {
		ObjectState.CHECK_STATE(clientEnv.state, EObjectState.Available);

		return clientEnv.client;
	}
}
