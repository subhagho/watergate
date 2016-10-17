package com.watergate.library.io;

import com.watergate.library.LockClientEnv;
import com.watergate.library.LockControlManager;
import com.watergate.library.LockEnv;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;

/**
 * Created by subho on 17/10/16.
 */
public class Test_ThreadedPriorityFileStream {
	private static final String CONFIG_FILE =
			"../test/data/test-sem-conf.json";
	private static final String CONTROL_DEF_CONFIG_PATH =
			"/configuration/control/def";
	private static final String CONTROL_CONFIG_PATH = "/configuration/control";
	private static final String DATA_FILE = "../test/data/fs-data.txt";
	private static final String OUTPUT_FILE = "/tmp/output.txt";
	private static final int THREAD_COUNT = 20;

	private LockControlManager manager = null;

	@Before
	public void setUp() throws Exception {
		LockEnv.createEnv(CONFIG_FILE, "Test_PriorityFileInputStream");

		manager = new LockControlManager();
		manager.create(CONTROL_CONFIG_PATH);

		LockClientEnv.createEnv(CONTROL_DEF_CONFIG_PATH);

		String indir = String.format("/tmp/%s/input/", PriorityTestRunner
				.class.getSimpleName());
		File id = new File(indir);
		if (!id.exists()) {
			id.mkdirs();
		}
		String outdir = String.format("/tmp/%s/output/", PriorityTestRunner
				.class.getSimpleName());
		File od = new File(outdir);
		if (!od.exists()) {
			od.mkdirs();
		}
	}

	@After
	public void tearDown() throws Exception {
		if (manager != null)
			manager.dispose();
		LockClientEnv.shutdown();
	}

	@Test
	public void run() throws Exception {
		Thread[] threads = new Thread[THREAD_COUNT];
		for (int ii = 0; ii < THREAD_COUNT; ii++) {
			Thread t = new Thread(new PriorityTestRunner((short) (ii % 3), ii,
					20));
			t.start();
			threads[ii] = t;
		}
		for (int ii = 0; ii < THREAD_COUNT; ii++) {
			threads[ii].join();
		}
		LockClientEnv.getLockClient().test_assert();
	}

}