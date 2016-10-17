package com.watergate.library.io;

import com.watergate.library.LockClientEnv;
import com.watergate.library.LockControlManager;
import com.watergate.library.LockEnv;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;

import static org.junit.Assert.*;

/**
 * Created by subho on 17/10/16.
 */
public class Test_PriorityFileInputStream {
	private static final String CONFIG_FILE =
			"../test/data/test-sem-conf.json";
	private static final String CONTROL_DEF_CONFIG_PATH =
			"/configuration/control/def";
	private static final String CONTROL_CONFIG_PATH = "/configuration/control";
	private static final String DATA_FILE = "../test/data/fs-data.txt";
	private static final String OUTPUT_FILE = "/tmp/output.txt";

	private LockControlManager manager = null;

	@Before
	public void setUp() throws Exception {
		LockEnv.createEnv(CONFIG_FILE, "Test_PriorityFileInputStream");

		manager = new LockControlManager();
		manager.create(CONTROL_CONFIG_PATH);

		LockClientEnv.createEnv(CONTROL_DEF_CONFIG_PATH);

	}

	@After
	public void tearDown() throws Exception {
		if (manager != null)
			manager.dispose();
		LockClientEnv.shutdown();
	}

	@Test
	public void read() throws Exception {
		Files.copy(Paths.get(DATA_FILE), Paths.get(OUTPUT_FILE),
				StandardCopyOption.REPLACE_EXISTING);
		PriorityFileInputStream pis = new PriorityFileInputStream
				(OUTPUT_FILE, (short) 1);
		StringBuffer read = new StringBuffer();
		while (true) {
			byte[] buff = new byte[10024];
			int r = pis.read(buff);
			if (r > 0) {
				String s = new String(buff, 0, r, "UTF-8");
				read.append(s);
				if (r < 1024) {
					break;
				}
			} else {
				break;
			}
		}
		pis.close();
	}

}