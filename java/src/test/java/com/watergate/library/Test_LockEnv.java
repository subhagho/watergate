package com.watergate.library;

import org.junit.Test;

import static org.junit.Assert.*;

/**
 * Created by subho on 17/10/16.
 */
public class Test_LockEnv {
	private static final String CONFIG_FILE =
			"../test/data/test-sem-conf.json";

	@Test
	public void createEnv() throws Exception {
		LockEnv.createEnv(CONFIG_FILE, "Test_LockEnvv");
		LockEnv.get();
		LockEnv.shutdown();
	}

}