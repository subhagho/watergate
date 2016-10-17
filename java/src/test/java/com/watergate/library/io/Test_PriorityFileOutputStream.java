package com.watergate.library.io;

import com.watergate.library.LockClientEnv;
import com.watergate.library.LockControlManager;
import com.watergate.library.LockEnv;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.FileInputStream;

import static org.junit.Assert.*;

/**
 * Created by subho on 17/10/16.
 */
public class Test_PriorityFileOutputStream {
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
	public void write() throws Exception {
		StringBuffer data = new StringBuffer();
		FileInputStream fis = new FileInputStream(DATA_FILE);
		while (true) {
			byte[] buff = new byte[1024];
			int r = fis.read(buff);
			if (r > 0) {
				String s = new String(buff, 0, r, "UTF-8");
				data.append(s);
				if (r < 1024) {
					break;
				}
			} else {
				break;
			}
		}
		fis.close();
		if (data.length() > 0) {
			PriorityFileOutputStream fos = new PriorityFileOutputStream
					(OUTPUT_FILE, false, (short) 2);
			fos.write(data.toString().getBytes("UTF-8"));
			fos.close();
		}
		LockClientEnv.getLockClient().test_assert();
	}

}