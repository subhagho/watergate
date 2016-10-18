package com.watergate.library.io;

import com.watergate.library.LockControlManager;
import com.watergate.library.LockEnv;
import com.watergate.library.utils.LogUtils;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by subho on 17/10/16.
 */
public class Test_ProcessPriorityFileStream {
	private static final String CONFIG_FILE =
			"../test/data/test-sem-conf.json";
	private static final String CONTROL_CONFIG_PATH = "/configuration/control";
	private static final int PROCESS_COUNT = 20;
	private static final String LIBRARY_PATH =
			"../cmake/";
	private static final String TARGET_JAR =
			"./target/library-1.0-SNAPSHOT-jar-with-dependencies.jar:" +
					"./target/test-classes";
	private static final String JAVA =
			"java";

	private LockControlManager manager = null;

	@Before
	public void setUp() throws Exception {
		LockEnv.createEnv(CONFIG_FILE, "Test_ProcessPriorityFileStream");

		manager = new LockControlManager();
		manager.create(CONTROL_CONFIG_PATH);

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
	}

	@Test
	public void run() throws Exception {
		File d = new File(".");
		LogUtils.mesg(getClass(), "Current directory=" + d.getCanonicalPath());
		Process[] processes = new Process[PROCESS_COUNT];
		for (int ii = 0; ii < processes.length; ii++) {
			int priority = ii % 3;
			List<String> cmd = new ArrayList<>();
			cmd.add(JAVA);
			cmd.add("-ea");
			cmd.add("-Djava.library.path=" + LIBRARY_PATH);
			cmd.add("-cp");
			cmd.add(TARGET_JAR);
			cmd.add(PriorityTestRunner.class.getCanonicalName());
			cmd.add("--priority=" + priority);
			cmd.add("--index=" + ii);
			cmd.add("--cycles=20");

			LogUtils.mesg(getClass(), "Launching process [" + cmd.toString()
					+ "]...");
			ProcessBuilder pb = new ProcessBuilder(cmd);
			pb.directory(d);
			pb.inheritIO();

			processes[ii] = pb.start();
		}

		for (int ii = 0; ii < processes.length; ii++) {
			processes[ii].waitFor();
			LogUtils.mesg(getClass(), "Process [" + ii + "] existed with " +
					"status=" + processes[ii].exitValue());

		}
	}

}