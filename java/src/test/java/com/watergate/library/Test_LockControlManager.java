package com.watergate.library;

import org.junit.Test;

import static org.junit.Assert.*;

/**
 * Created by subho on 17/10/16.
 */
public class Test_LockControlManager {
	private static final String CONFIG_FILE =
			"../test/data/test-sem-conf.json";
	private static final String CONTROL_CONFIG_PATH = "/configuration/control";

	@Test
	public void create() throws Exception {
		LockEnv.createEnv(CONFIG_FILE, "ADHOC");

		LockControlManager manager = new LockControlManager();
		manager.create(CONTROL_CONFIG_PATH);
		manager.clearLocks();

		manager.dispose();
		LockEnv.shutdown();
	}

}