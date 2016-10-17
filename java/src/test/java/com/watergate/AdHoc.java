package com.watergate;

import com.watergate.library.LockClientEnv;
import com.watergate.library.LockControlManager;
import com.watergate.library.LockEnv;
import com.watergate.library.io.PriorityFileInputStream;
import com.watergate.library.io.PriorityFileOutputStream;

import java.io.File;
import java.io.FileInputStream;

/**
 * Created by subho on 17/10/16.
 */
public class AdHoc {
	private static final String CONFIG_FILE =
			"../test/data/test-sem-conf.json";
	private static final String CONTROL_DEF_CONFIG_PATH =
			"/configuration/control/def";
	private static final String CONTROL_CONFIG_PATH = "/configuration/control";
	private static final String DATA_FILE = "../test/data/fs-data.txt";
	private static final String OUTPUT_FILE = "/tmp/output.txt";

	public static void main(String[] args) {
		try {
			File cd = new File(".");
			System.out.println("Current directory [" + cd.getCanonicalPath() + "]");

			LockEnv.createEnv(CONFIG_FILE, "ADHOC");

			LockControlManager manager = new LockControlManager();
			manager.create(CONTROL_CONFIG_PATH);

			LockClientEnv.createEnv(CONTROL_DEF_CONFIG_PATH);
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

			manager.dispose();
			LockClientEnv.shutdown();

		} catch (Throwable t) {
			t.printStackTrace();
		}
	}
}
