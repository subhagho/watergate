package com.watergate.library;

import org.junit.Test;

import static org.junit.Assert.*;

/**
 * Created by subho on 17/10/16.
 */
public class Test_LockClientEnv {
	private static final String CONFIG_FILE =
			"../test/data/test-sem-conf.json";
	private static final String CONTROL_DEF_CONFIG_PATH =
			"/configuration/control/def";
	private static final String CONTROL_CONFIG_PATH = "/configuration/control";

	@Test
	public void createEnv() throws Exception {
		LockEnv.createEnv(CONFIG_FILE, "Test_LockClientEnv");

		LockControlManager manager = new LockControlManager();
		manager.create(CONTROL_CONFIG_PATH);

		LockClientEnv.createEnv(CONTROL_DEF_CONFIG_PATH);

		manager.dispose();
		LockClientEnv.shutdown();
	}

}